int new_map_conditional(char* ip, _Bool conditional_creation);
int new_map(char* ip);
_Bool insert_data(char* ip, int map_id, void* key, int key_len, void* data, int data_len, int int_value);
_Bool upload_file(char* ip, int map_id, char* fn);
_Bool download_file(char* ip, int map_id, char* fn, char* new_fn);
struct ph_msg* lookup_data(char* ip, int map_id, void* key, int key_len);
_Bool add_int_value(char* ip, int map_id, void* key, int key_len, int num);
_Bool remove_data(char* ip, int map_id, void* key, int key_len);
