struct hm_entry{
	/* key in key value pair - can also be used standalone as hashed storage */
	void* data_key;
	int len;

	/* this can be NULL - the value in key value pair */
	void* data_value;
	/* another optional field - an integer associated with the entry */
	int int_value;

	struct hm_entry* next, * prev;
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
