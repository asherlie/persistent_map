#include <stdio.h>
#include <ctype.h>

#include "msg.h"
#include "ph_client.h"

/* ./fs <u|d> file <address> [write_fn] */
int main(int a, char** b){
    if(a < 4)return 0;
    switch(tolower(*b[1])){
        case 'u':
            if(upload_file(b[3], 0, b[2]))
                puts("succesfully uploaded file");
            else puts("failed to upload file");
            break;
        case 'd':
            if(download_file(b[3], 0, b[2], (a > 4) ? b[4] : b[2]))
                printf("downloaded file to \"%s\"\n", (a > 4) ? b[4] : b[2]);
            else puts("failed to download file");
            break;
    }

    return 1;
}
