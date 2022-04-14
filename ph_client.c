#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "msg.h"

int establish_conn(char* ip){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peer_addr = {0};
    peer_addr.sin_port = PORT;
    peer_addr.sin_family = AF_INET;
    inet_aton(ip, &peer_addr.sin_addr);
    /*
     * struct sockaddr_in sin = {0};
     * sin.sin_port = PORT;
     * sin.sin_family = AF_INET;
     * inet_aton(ip, &sin.sin_addr);
     * bind(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));
    */
    if(connect(sock, (struct sockaddr*)&peer_addr, sizeof(struct sockaddr_in)))
        sock = -1;

    return sock;
}

int new_map(char* ip){
    struct ph_msg msg = {0};
    int peer_sock;
    msg.act = CREATE_MAP;

    peer_sock = establish_conn(ip);
    write_msg(peer_sock, &msg);
    msg.int_value = -1;
    if(!recv_msg(peer_sock, &msg))return -1;
    return msg.int_value;
}

_Bool insert_data(){
}

struct ph_msg* lookup_data(){
}

_Bool remove_data(){
}

int main(){
}
