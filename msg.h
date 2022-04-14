#include <pthread.h>

#define PORT 392

enum action {CREATE_MAP, INSERT_PH_KEY_VALUE, LOOKUP_ENTRY, REMOVE_PH};

/* this struct contains all necessary fields for any possible message
 * msg types include:
 *  create_map
 *  insert_ph_key_value
 *  lookup_entry
 *  remove_ph
 */
struct ph_msg{
    /* we need msg_id so that we can associate responses with the proper
     * requests once a response arrives
     *
     * NVM! we don't need this since connections never stay open past
     * an exchange!
     * all responses go to the socket that sent the query
     * so they can just read right after sending
     *
     */
    //int msg_id;
    /* TODO: this should really not be an enum - there's no guarantee
     * that an enum is 4 bytes wide
     */
    enum action act;
    int map_id;
    int data_key_len;
    int data_value_len;
    int int_value;
    void* data_key;
    void* data_value;
};

struct ph_msg_q_entry{
    struct ph_msg* msg;
    struct ph_msg_q_entry* next;
};

struct ph_msg_q{
    pthread_mutex_t lock;
    pthread_cond_t nonempty;
    struct ph_msg_q_entry* first, * last;
};

_Bool write_msg(int fd, struct ph_msg* msg);
_Bool recv_msg(int fd, struct ph_msg* msg);
_Bool append_dump(char* fn, struct ph_msg* msg);


void init_ph_msg_q(struct ph_msg_q* mq);
void insert_ph_msg_q(struct ph_msg_q* mq, struct ph_msg* msg);
struct ph_msg* pop_ph_msg_q(struct ph_msg_q* mq);
