#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "msg.h"
#include "ph.h"

#if 0
should this just use network sockets?
or should i keep this local
#endif

struct shared_data{
    struct ph_msg_q* q;
    struct persistent_hash* ph;
    int peer_sock;
};

int prep_sock(char* ip){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sin = {0};

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

    sin.sin_port = PORT;
    sin.sin_family = AF_INET;
    /*sin.sin_addr*/
    inet_aton(ip, &sin.sin_addr);

    bind(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));
    listen(sock, 0);

    return sock;
}

/* this thread pops messages from queue and handles them one at a time
 * this design allows for all hash structures to be thread unsafe
 * because they'll never be called concurrently
 */
void* handler_thread(void* v_sd){
    struct shared_data* sd = v_sd;
    struct ph_msg* msg;

    while(1){
        msg = pop_ph_msg_q(sd->q);
        append_dump(sd->ph->dump_fn, msg);

        /*
         * peer sock must be passed along from read_request_thread!
         * peer sock is not the right value!!!
        */

        /* we can't use sd->peer_sock here */
        perform_msg_action_ph(sd->ph, msg, msg->peer_sock);
        /* TODO: we should free up memory alloc'd by recv_msg() here */
        printf("performed an action: %i, current state:\n", msg->act);
        print_maps(sd->ph);
    }

    return NULL;
}

/* this thread is spawned for each accepted connection - its sole purpose is
 * to read a complete struct ph_msg and add it to the queue
 *
 * we don't handle these msgs here because the operations on the map aren't
 * threadsafe
 */
void* read_request_thread(void* v_sd){
    struct shared_data* sd = v_sd;
    struct ph_msg* msg = malloc(sizeof(struct ph_msg));
    if(recv_msg(sd->peer_sock, msg)){
        msg->peer_sock = sd->peer_sock;
        insert_ph_msg_q(sd->q, msg);
    }
    else free(msg);
    free(sd);
    return NULL;
}

void spawn_read_request_thread(struct ph_msg_q* q, struct persistent_hash* ph, int sock){
    struct shared_data* sd = malloc(sizeof(struct shared_data));
    pthread_t pth;
    sd->peer_sock = sock;
    sd->q = q;
    sd->ph = ph;

    pthread_create(&pth, NULL, read_request_thread, sd);
    pthread_detach(pth);
}

void run_ph_server(char* ip){
    struct persistent_hash ph;
    struct ph_msg_q q;
    pthread_t handler_pth;
    struct shared_data handler_sd = {.q = &q, .ph = &ph, .peer_sock = -1};
    int sock = prep_sock(ip), peer_sock;

    init_ph(&ph, ".dumpfile");
    printf("ph initiated, %i operations restored\n", restore_ph(&ph));
    init_ph_msg_q(&q);

    pthread_create(&handler_pth, NULL, handler_thread, &handler_sd);
    /*pthread_detach();*/

    while(1){
        peer_sock = accept(sock, NULL, NULL);
        spawn_read_request_thread(&q, &ph, peer_sock);
    }
}

int main(){
    run_ph_server("192.168.86.21");
}

#if !1
struct ph_msg* gen_sequence(){
    struct ph_msg* msg = calloc(10, sizeof(struct ph_msg));
    msg[0].act = CREATE_MAP;
    msg[1].act = INSERT_PH_KEY_VALUE;
    msg[1].map_id = 0;
    msg[1].data_key_len = 6;
    msg[1].data_key = strdup("asher");
    msg[2].act = INSERT_PH_KEY_VALUE;
    msg[2].map_id = 0;
    msg[2].data_key_len = 6;
    msg[2].data_key = strdup("ASHER");
    msg[3].act = REMOVE_PH;
    msg[3].map_id = 0;
    msg[3].data_key_len = 6;
    msg[3].data_key = strdup("ASHER");

    return msg;
}

int main(){
    int sock = prep_sock("192.168.86.21");
    struct sockaddr_in peer_addr;
    socklen_t len;
    if(0)accept(sock, (struct sockaddr*)&peer_addr, &len);
    close(sock);

    if(0){
        /*
         * struct ph_msg pm;
         * char msg[] = "key";
         * char data[] = "DATA";
         * pm.map_id = 32;
         * pm.data_key = msg;
         * pm.data_key_len = sizeof(msg);
         * pm.int_value = 234;
         * pm.data_value = data;
         * pm.data_value_len = sizeof(data);
        */
        struct ph_msg* msgs = gen_sequence();
        for(int i = 0; i < 4; ++i){
            append_dump("dumper.dee", msgs+i);
        }
    }
    else{
        struct persistent_hash ph;
        init_ph(&ph, "dumper.dee");
        printf("ph initiated, %i operations restored\n", restore_ph(&ph));
        print_maps(&ph);
    }
}
#endif
/*
maybe the client will spawn a thread each time a message is sent to read responses
it could timeout after 5s
or i could just do a read with 5s timeout in the send function
see https://stackoverflow.com/questions/2917881/how-to-implement-a-timeout-in-read-function-call
all message related code will be in a separate file included by both server and client - they'll both need the writer code
ph.c will use it to write to disk, client to write to server


need to create a thread to accept connections, each new conn triggers spawning of a new thread to read from that conn
messages are added to a threadsafe queue 

all messages contain the same struct but fields are optional for some
they first recv an enum for what operation to perform - insert, lookup, create, remove
then and int for size of data_value

then the struct


these messages are popped from the main thread and handled in the order they arrive

each time a message is inserted into the queue, it's added to a file - this is safe to do here because it's one of the only threadsafe
regions of the code
there will be one function write_msg(int fd), which writes a struct msg to a fd
make sure to send in htonl() format
write(data_len)
write()

reader in server will:
    read(&int, 4) - data_key len
    ntohl(int)

    read(&int, 4) - data_value len
    ntohl(int)

    read(&int, 4) - int_value
    ntohl()

    read(n) - key
    read(n) - value
*/

