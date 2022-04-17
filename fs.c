#include <stdio.h>
#include <ctype.h>

#include "msg.h"
#include "ph_client.h"


/* ./fs <u|d|l> <address> <fn> [write_fn] */
int main(int a, char** b){
    char** lst, * ip = b[2];
    int len;
    switch(tolower(*b[1])){
        case 'u':
            if(a < 4)return 0;
            /* creating a map with index 0 if one doesn't exist */
            new_map_conditional(ip, 1);
            /* TODO: uploading very large files doesn't work - find out why */
            if(upload_file(ip, 0, b[3]))
                puts("succesfully uploaded file");
            else puts("failed to upload file");
            break;
        case 'd':
            if(a < 4)return 0;
            if(download_file(ip, 0, b[3], (a > 4) ? b[4] : b[3]))
                printf("downloaded file to \"%s\"\n", (a > 4) ? b[4] : b[3]);
            else puts("failed to download file");
            break;
        case 'l':
            if(a < 3)return 0;
            lst = list_str_keys(ip, 0, &len);
            if(!lst)break;
            for(int i = 0; i < len; ++i){
                printf("\"%s\"\n", lst[i]);
            }
            break;
    }

    return 1;
}
