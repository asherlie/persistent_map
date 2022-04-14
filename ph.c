#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "msg.h"
#include "ph.h"

#if 0
ph will have a list of hashmaps that will be kept in the memory of a running process

it can be interacted with using unix sockets
it will handle commands such as create_map() - returns an ID to a map
insert_map(ID, void* data, int len, hash_result/idx)
actually, i should try to write an abstract hashing function for void*

thread accpets conns, spawns read thread with fd provided, this thread reads command, adds command to command queue (threadsafe), 
this way we can keep the core implementation thread unsafe

for now it should be sum of bytes * constant prime % n_buckets

each command is recorded in a sys v queue by the process
on startup, this queue is read from and all queues are rebuilt

this is not resistant to machine restarts
#endif

void init_hm(struct hm* map, int n_buckets){
	map->n_buckets = n_buckets;
	map->buckets = calloc(sizeof(struct hm_entry*), n_buckets);
}

void init_ph(struct persistent_hash* ph, char* dump_fn){
    ph->dump_fn = dump_fn;
	ph->n_maps = 0;
	ph->cap = 100;
	ph->maps = malloc(sizeof(struct hm*)*ph->cap);
}

/* creates a new map and returns the assigned map ID */
int create_map(struct persistent_hash* ph){
	struct hm** tmp_maps;
	int id;
	if(ph->n_maps == ph->cap){
		ph->cap *= 2;
		tmp_maps = malloc(sizeof(struct hm*)*ph->cap);
		memcpy(tmp_maps, ph->maps, sizeof(struct hm*)*ph->n_maps);
		free(ph->maps);
		ph->maps = tmp_maps;
	}
	ph->maps[(id = ph->n_maps)] = malloc(sizeof(struct hm));
	init_hm(ph->maps[ph->n_maps++], 1000);

	return id;
}

int hash(void* data, int len, int max){
    uint64_t sum = 0;
    for(int i = 0; i < len; ++i)
        sum += ((char*)data)[i];
    sum *= ((sum%2) ? 7 : 11);
	return (sum+((char*)data)[len-1]) % max;
}

/*TODO: there should be a function to increment int value*/
_Bool insert_hm(struct hm* map, void* data_key, int data_key_len, void* data_value, int data_value_len, int int_value){
	struct hm_entry* e, * prev_e = NULL;
	int internal_idx = hash(data_key, data_key_len, map->n_buckets);
    _Bool alloc_e = 1;

    if(!data_key || !data_key_len)return 0;

	if(!(e = map->buckets[internal_idx]))
		e = (map->buckets[internal_idx] = calloc(sizeof(struct hm_entry), 1));
	else{
		for(; e->next; e = e->next){
            if(e->data_key_len == data_key_len && !memcmp(e->data_key, data_key, data_key_len)){
                /* we're modifying an existing entry */
                alloc_e = 0;
                break;
            }
        }
        prev_e = e;
		if(alloc_e)e = (e->next = malloc(sizeof(struct hm_entry)));
	}
    /* TODO: we just update data if provided in msg, is this the behavior we want?
     * doing this enables us to only update int_value if that's what we want to do
     */
    e->data_key = data_key;
    e->data_key_len = data_key_len;
	if(data_value){
        e->data_value = data_value;
        e->data_value_len = data_value_len;
    }
    /* int_value is always overwritten - bad behavior?
     * the user can always just lookup_entry and set it before requesting
     *
     * this also will be unnecessary once i implement int_value_add()
     */
	e->int_value = int_value;
    /* if we're overwriting an existing element, don't set prev or next */
    if(alloc_e){
        e->prev = prev_e;
        e->next = NULL;
    }
    return 1;
}

_Bool insert_ph_key_value(struct persistent_hash* ph, int id, void* data_key, int data_key_len, void* data_value, int data_value_len, int int_value){
	if(!ph->maps[id])return 0;
	return insert_hm(ph->maps[id], data_key, data_key_len, data_value, data_value_len, int_value);
}

_Bool insert_ph(struct persistent_hash* ph, int id, void* data, int len){
    return insert_ph_key_value(ph, id, data, len, NULL, -1, 0);
}

struct hm_entry* lookup_entry(struct persistent_hash* ph, int id, void* data_key, int len, int* hash_ret){
    int hashed_key = hash(data_key, len, ph->maps[id]->n_buckets);
    struct hm_entry* e;

    if(hash_ret)*hash_ret = hashed_key;
    e = ph->maps[id]->buckets[hashed_key];
    for(; e; e = e->next){
        if(!memcmp(e->data_key, data_key, len))break;
    }
    return e;
}

_Bool add_int_value(struct persistent_hash* ph, int map_id, void* data_key, int len, int add){
    struct hm_entry* e = lookup_entry(ph, map_id, data_key, len, NULL);
    if(!e)return 0;
    e->int_value += add;
    return 1;
}

_Bool remove_ph(struct persistent_hash* ph, int id, void* data_key, int len){
    int hashed_key;
    struct hm_entry* e = lookup_entry(ph, id, data_key, len, &hashed_key);
    if(!e)return 0;
    /* removing first element */
    if(!e->prev){
        ph->maps[id]->buckets[hashed_key] = e->next;
        if(e->next)e->next->prev = NULL;
        free(e);
        return 1;
    }
    /* removing final element */
    e->prev->next = e->next;
    if(e->next)
        e->next->prev = e->prev;
    free(e);
    e = NULL;

    return 1;
}

/* TODO: we need to handle invalid map IDs */
_Bool perform_msg_action_ph(struct persistent_hash* ph, struct ph_msg* pm, int peer_sock){
/*enum action {CREATE_MAP, INSERT_PH_KEY_VALUE, LOOKUP_ENTRY, REMOVE_PH};*/
    struct ph_msg resp = {0};
    struct hm_entry* e;
    /*int resp = -1;*/
    switch(pm->act){
        case CREATE_MAP:
            resp.int_value = create_map(ph);
            break;
        case INSERT_PH_KEY_VALUE:
            resp.int_value = insert_ph_key_value(ph, pm->map_id, pm->data_key, pm->data_key_len, pm->data_value, pm->data_value_len, pm->int_value);
            break;
        case LOOKUP_ENTRY:
            if((e = lookup_entry(ph, pm->map_id, pm->data_key, pm->data_key_len, NULL))){
                resp.map_id = pm->map_id;
                resp.data_key_len = pm->data_key_len;
                resp.data_key = pm->data_key;
                resp.data_value_len = e->data_value_len;
                resp.data_value = e->data_value;
                resp.int_value = e->int_value;
            }
            // TODO: how do i return an entry
            // AH! with write_msg()
            // the receiver will just use a recv_msg() that i'll define it returns a populated msg
            break;
        case ADD_INT_VALUE:
            if(add_int_value(ph, pm->map_id, pm->data_key, pm->data_key_len, pm->int_value))
                resp.int_value = 1;
            break;
        case REMOVE_PH:
            if(remove_ph(ph, pm->map_id, pm->data_key, pm->data_key_len))
                resp.int_value = 1;
            break;
    }

    /* TODO: could call to htonl() make resp negative? */
    /* if a response is neccessary, send
     * peer will keep connection open until they recv a response
     */
    if(peer_sock != -1){
        /*if(resp_ptr)write_msg(peer_sock, resp_ptr);*/
        write_msg(peer_sock, &resp);
    }
    return resp.int_value >= 0;
}

/* to be called on startup to restore dumped values */
/*should this fn be here or in the thread that reads msgs*/
/*
 * it's good here actually, we'll just call it after we init or in init
 * we'll go through all entries in file, loading into temp msg
 * then call perform_msg_action_ph()
 * to rebuild or map
 * 
 * the server thread will call the very same perform_msg_action_ph() to handle messages as they arrive
 * hmm, should these messages actually just be added to the queue on startup and handled normally?
 * i think this should be the approach
 * so we never have any edge cases, just read from file, insert to queue, business as usual
 * might not be as safe though because messages might come in the way
 * 
 * unless we don't set up networkign until after this
 * easier actually to just have the code separated
*/

/*okay, we have a working restore(), now just need to append_dump() each time a message is popped from the queue in ph_server*/
int restore_ph(struct persistent_hash* ph){
    int entries_restored = 0;
    int fd  = open(ph->dump_fn, O_RDONLY);
    struct ph_msg msg;
    if(fd < 0)return -1;

    while(recv_msg(fd, &msg)){
        printf("performing restore action %i\n", msg.act);
        perform_msg_action_ph(ph, &msg, -1);
    }

    close(fd);
    return entries_restored;
}

void print_maps(struct persistent_hash* ph){
	for(int i = 0; i < ph->n_maps; ++i){
		printf("map: %i:\n", i);
		for(int j = 0; j < ph->maps[i]->n_buckets; ++j){
            if(!ph->maps[i]->buckets[j])continue;
			printf("  bucket %i:\n", j);
			for(struct hm_entry* e = ph->maps[i]->buckets[j]; e; e = e->next){
				printf("    key: \"%s\" %p:%i, val: %i\n", (char*)e->data_key, e->data_key, e->data_value_len, e->int_value);
			}
		}
	}
}

#if 0
int main(){
	struct persistent_hash ph;	
	int map_id = 0;
	char* data;
	init_ph(&ph);

	map_id = create_map(&ph);

    for(int i = 0; i < 20; ++i){
        data = strdup("asher");
        data[random() % 5] += random() % 9;
        insert_ph(&ph, map_id, data, 6);
    }


	print_maps(&ph);
}
#endif
