int new_map(char* ip);
_Bool insert_data(char* ip, int map_id, void* key, int key_len, void* data, int data_len, int int_value);
struct ph_msg* lookup_data(char* ip, int map_id, void* key, int key_len);
_Bool add_int_value(char* ip, int map_id, void* key, int key_len, int num);
_Bool remove_data(char* ip, int map_id, void* key, int key_len);
