#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ph_client.h"
#include "msg.h"

/* TODO: all request functions should share code, they're so similar */

int establish_conn(char* ip){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peer_addr = {0};
    
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

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

/* TODO: socks need to be closed after recv_msg(j */
int new_map(char* ip){
    struct ph_msg msg = {0};
    int peer_sock;
    msg.act = CREATE_MAP;

    peer_sock = establish_conn(ip);
    if(peer_sock == -1)return -1;
    write_msg(peer_sock, &msg);
    msg.int_value = -1;
    if(!recv_msg(peer_sock, &msg))return -1;
    return msg.int_value;
}

_Bool insert_data(char* ip, int map_id, void* key, int key_len, void* data, int data_len, int int_value){
    struct ph_msg msg = {0};
    int peer_sock;
    msg.act = INSERT_PH_KEY_VALUE;
    msg.map_id = map_id;
    msg.data_key_len = key_len;
    msg.data_value_len = data_len;
    msg.int_value = int_value;
    msg.data_key = key;
    msg.data_value = data;
    
    peer_sock = establish_conn(ip);
    write_msg(peer_sock, &msg);
    if(!recv_msg(peer_sock, &msg))return -1;
    return msg.int_value;
}

_Bool upload_file(char* ip, int map_id, char* fn){
    FILE* fp = fopen(fn, "r");
    uint8_t* data;
    uint32_t len;
    _Bool ret;

    printf("%p\n", fp);
    if(!fp)return 0;

    fseek(fp, 0, SEEK_END);
    data = malloc((len = ftell(fp)));
    fseek(fp, 0, SEEK_SET);

    if(fread(data, 1, len, fp) != len){
        puts("UPSJk:");
        fclose(fp);
        return 0;
    }

    printf("uploading file of size %i\n", len);
    ret = insert_data(ip, map_id, fn, strlen(fn)+1, data, len, 0);

    free(data);

    fclose(fp);
    return ret;
}

struct ph_msg* lookup_data(char* ip, int map_id, void* key, int key_len){
    struct ph_msg* msg = calloc(1, sizeof(struct ph_msg));
    int peer_sock;

    msg->act = LOOKUP_ENTRY;
    msg->map_id = map_id;
    msg->data_key_len = key_len;
    msg->data_key = key;

    peer_sock = establish_conn(ip);
    write_msg(peer_sock, msg);
    if(!recv_msg(peer_sock, msg)){
        free(msg);
        return NULL;
    }
    return msg;
}

_Bool add_int_value(char* ip, int map_id, void* key, int key_len, int num){
    struct ph_msg msg = {0};
    int peer_sock;

    msg.act = ADD_INT_VALUE;
    msg.map_id = map_id;
    msg.data_key_len = key_len;
    msg.data_key = key;
    msg.int_value = num;

    peer_sock = establish_conn(ip);
    write_msg(peer_sock, &msg);
    if(!recv_msg(peer_sock, &msg))return -1;
    return msg.int_value;
}

_Bool remove_data(char* ip, int map_id, void* key, int key_len){
    struct ph_msg msg = {0};
    int peer_sock;

    msg.act = REMOVE_PH;
    msg.data_key_len = key_len;
    msg.data_key = key;
    msg.map_id = map_id;

    peer_sock = establish_conn(ip);
    write_msg(peer_sock, &msg);
    if(!recv_msg(peer_sock, &msg))return -1;
    return msg.int_value;
}

#if 0
/* TODO: test lookup_data() */
int main(){
    char ip[] = "192.168.86.21";
    char key[] = "/home/asher/somefile.extension";
    int map_id = 0;
    /*printf("new map created with id: %i\n", (map_id = new_map(ip)));*/
    printf("insert_data(): %i\n", insert_data(ip, map_id, key, sizeof(key), NULL, 0, 11));
}
#endif
