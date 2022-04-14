#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>

#include "msg.h"

_Bool write_net_order(int fd, int val){
    int32_t tmp_i = htonl(val);
    return write(fd, &tmp_i, 4) == 4;
}

_Bool write_msg(int fd, struct ph_msg* msg){
    _Bool ret = 1;
    ret &= write_net_order(fd, msg->act);
    ret &= write_net_order(fd, msg->map_id);
    ret &= write_net_order(fd, msg->data_key_len);
    ret &= write_net_order(fd, msg->data_value_len);
    ret &= write_net_order(fd, msg->int_value);

    ret &= write(fd, msg->data_key, msg->data_key_len) == msg->data_key_len;
    ret &= write(fd, msg->data_value, msg->data_value_len) == msg->data_value_len;

    return ret;
}

_Bool recv_msg(int fd, struct ph_msg* msg){
    fd_set set;
    struct timeval timeout;
    _Bool ret = 0;

    memset(msg, 0, sizeof(struct ph_msg));

    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if(select(fd+1, &set, NULL, NULL, &timeout) > 0){
        ret = 1;
        /* if there's data to read after 5s */
        ret &= read(fd, &msg->act, 4) == 4;
        ret &= read(fd, &msg->map_id, 4) == 4;
        ret &= read(fd, &msg->data_key_len, 4) == 4;
        ret &= read(fd, &msg->data_value_len, 4) == 4;
        ret &= read(fd, &msg->int_value, 4) == 4;

        msg->act = ntohl(msg->act);
        msg->map_id = ntohl(msg->map_id);
        msg->data_key_len = ntohl(msg->data_key_len);
        msg->data_value_len = ntohl(msg->data_value_len);
        msg->int_value = ntohl(msg->int_value);

        /*
         * make sure to FREE MEM
         * fuck it woudl make so mch more sense to just dump after each modification
         * actually maybe not because it woudl require >1 dump formats
        */

        msg->data_key = calloc(1, msg->data_key_len);
        msg->data_value = calloc(1, msg->data_key_len);

        ret &= read(fd, msg->data_key, msg->data_key_len) == msg->data_key_len;
        ret &= read(fd, msg->data_value, msg->data_value_len) == msg->data_value_len;
    }

    return ret;
}

_Bool append_dump(char* fn, struct ph_msg* msg){
    int fd = open(fn, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    _Bool ret = write_msg(fd, msg);
    close(fd);
    return ret;
}
