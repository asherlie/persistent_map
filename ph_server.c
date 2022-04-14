#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "msg.h"
#include "ph.h"

#define PORT 392

#if 0
should this just use network sockets?
or should i keep this local
#endif

int prep_sock(char* ip){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sin = {0};
    sin.sin_port = PORT;
    sin.sin_family = AF_INET;
    /*sin.sin_addr*/
    inet_aton(ip, &sin.sin_addr);

    bind(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));
    listen(sock, 0);

    return sock;
}

void run_ph_server(char* ip){
    struct persistent_hash ph;
    int sock = prep_sock(ip);
    init_ph(&ph, ".dumpfile");
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
        restore_ph(&ph);
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

