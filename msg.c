#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>
#include <pthread.h>

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
    timeout.tv_sec = 1;
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
         * it would make so mch more sense to just dump after each modification
         * actually maybe not because it woudl require >1 dump formats
        */

        msg->data_key = calloc(1, msg->data_key_len);
        msg->data_value = calloc(1, msg->data_value_len);

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
/*
ok, writing the request sending and handling code!

sending will be in a separate file - ph_client.c
there will be functions provided to send all enum action requests

handling will be a bit more complicated, bt not by much
*/

void init_ph_msg_q(struct ph_msg_q* mq){
    pthread_mutex_init(&mq->lock, NULL);
    pthread_cond_init(&mq->nonempty, NULL);
    mq->first = NULL;
}

void insert_ph_msg_q(struct ph_msg_q* mq, struct ph_msg* msg){
    struct ph_msg_q_entry* e = calloc(1, sizeof(struct ph_msg_q_entry));
    e->msg = msg;
    pthread_mutex_lock(&mq->lock);
    if(!mq->first)
        mq->first = mq->last = e;
    else{
        mq->last->next = e;
    }
    pthread_mutex_unlock(&mq->lock);
    pthread_cond_signal(&mq->nonempty);
}

struct ph_msg* pop_ph_msg_q(struct ph_msg_q* mq){
    struct ph_msg_q_entry* e = NULL;
    while(!e){
        pthread_mutex_lock(&mq->lock);

        e = mq->first;
        /* TODO: is this ok usage of mutex? */
        /* if q is empty, unlock and wait
         * pthread_cond_wait() then re-locks and we'll catch
         * the element in our next iteration
         */
        if(!e)pthread_cond_wait(&mq->nonempty, &mq->lock);
        /* last is only used for insertion so there's no need to update it here
         * when insert_ph_msg_q() sees that !first, last is updated
         */
        else mq->first = e->next;
        pthread_mutex_unlock(&mq->lock);
    }

    return e->msg;
}
