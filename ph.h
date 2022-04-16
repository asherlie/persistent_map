struct hm_entry{
	/* key in key value pair - can also be used standalone as hashed storage */
	void* data_key;
	int data_key_len;

	/* this can be NULL - the value in key value pair */
	void* data_value;
    int data_value_len;
	/* another optional field - an integer associated with the entry */
	int int_value;

	struct hm_entry* next, * prev;
};

struct hm{
	struct hm_entry** buckets;
	int n_buckets;
};

struct persistent_hash{
    char* dump_fn;
	struct hm** maps;
	int n_maps;
	int cap;
};

/* TODO: hash operations do not need to be exposed in this header */
void init_ph(struct persistent_hash* ph, char* dump_fn);
int create_map(struct persistent_hash* ph, _Bool conditional);
_Bool insert_ph(struct persistent_hash* ph, int id, void* data, int len);
_Bool insert_ph_key_value(struct persistent_hash* ph, int id, void* data_key, int data_key_len, void* data_value, int data_value_len, int int_value);
_Bool add_int_value(struct persistent_hash* ph, int map_id, void* data_key, int len, int add);
struct hm_entry* lookup_entry(struct persistent_hash* ph, int id, void* data_key, int len, int* hash_ret);
_Bool remove_ph(struct persistent_hash* ph, int id, void* data_key, int len);
_Bool perform_msg_action_ph(struct persistent_hash* ph, struct ph_msg* pm, int peer_sock);
int restore_ph(struct persistent_hash* ph);
void print_maps(struct persistent_hash* ph);
