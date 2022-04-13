#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

struct hm_entry{
	/* key in key value pair - can also be used standalone as hashed storage */
	void* data_key;
	int len;

	/* this can be NULL - the value in key value pair */
	void* data_value;
	/* another optional field - an integer associated with the entry */
	int int_value;

	struct hm_entry* next;
};

struct hm{
	struct hm_entry** buckets;
	int n_buckets;
};

struct persistent_hash{
	struct hm** maps;
	int n_maps;
	int cap;
};

void init_hm(struct hm* map, int n_buckets){
	map->n_buckets = n_buckets;
	map->buckets = calloc(sizeof(struct hm_entry*), n_buckets);
}

void init_ph(struct persistent_hash* ph){
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

int hash(void* data, int len){
	(void)data;
	(void)len;
	return 19;
}

void insert_hm(struct hm* map, void* data_key, int len, void* data_value, int int_value){
	struct hm_entry* e;
	int internal_idx = hash(data_key, len);
	if(!(e = map->buckets[internal_idx]))
		e = (map->buckets[internal_idx] = calloc(sizeof(struct hm_entry), 1));
	else{
		for(; e->next; e = e->next);
		e = (e->next = malloc(sizeof(struct hm_entry)));
	}
	e->data_key = data_key;
	e->len = len;
	e->data_value = data_value;
	e->int_value = int_value;
	e->next = NULL;
}

_Bool insert_ph(struct persistent_hash* ph, int id, void* data, int len){
	if(!ph->maps[id])return 0;
	insert_hm(ph->maps[id], data, len, NULL, -1);
	return 1;
}

void remove_ph(){
}

void print_maps(struct persistent_hash* ph){
	for(int i = 0; i < ph->n_maps; ++i){
		printf("map: %i:\n", i);
		for(int j = 0; j < ph->maps[i]->n_buckets; ++j){
			if(!ph->maps[i]->buckets[j])continue;
			printf("  bucket %i:\n", j);
			for(struct hm_entry* e = ph->maps[i]->buckets[j]; e; e = e->next){
				printf("    %p:%i\n", e->data_key, e->len);
			}
		}
	}
}

int main(){
	struct persistent_hash ph;	
	int map_id = 0;
	char data[6] = "asher";
	init_ph(&ph);

	map_id = create_map(&ph);

	insert_ph(&ph, map_id, data, 6);

	print_maps(&ph);
}
