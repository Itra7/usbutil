#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "usbconf.h"
#include "utilities.h"
#include "usbutilerrno.h"


#define EXTRACT_IDPRODUCT (1<<2)
#define EXTRACT_IDVENDOR  (1<<3)
#define ATR(x) #x
#define STRINGIFY(x) ATR(x)
#define MAX_DEVICES 32
#define MAX_PATH 256*2


int usbutil_list_devices(struct list_of_devices** list_of_devices){
    
    INIT_DL_LIST(&list_of_devices);
    
    struct dirent *entry;
    DIR* dir = opendir(STRINGIFY(SYSPATH));
    if(dir == NULL){
        return USBUTIL_EOPEN;
    }

    char paths[MAX_DEVICES][MAX_PATH];
    int counter = 0;
    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == DT_LNK && strchr(entry->d_name, ':') == NULL){
            snprintf(paths[counter], MAX_PATH, "%s/%s", STRINGIFY(SYSPATH), entry->d_name);
            counter++;
        }
    }
    closedir(dir);

    for(int i = 0; i < counter; i++){
        dir = opendir(paths[i]);
        if(dir == NULL){
            return USBUTIL_EOPEN;
        }
        while((entry = readdir(dir)) != NULL){
            if(entry->d_type == DT_REG){
                if(!strcmp(entry->d_name, "idProduct")){
                    int id = usbutil_extract_id(paths[i], entry->d_name, &list_of_devices, EXTRACT_IDPRODUCT);
                    if(id == USBUTIL_EOPEN){ 
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "idVendor")){
                    int id = usbutil_extract_id(paths[i], entry->d_name, &list_of_devices, EXTRACT_IDVENDOR);
                    if(id == USBUTIL_EOPEN){
                        return USBUTIL_EOPEN;
                    }
                }      
            }
        }
        (*list_of_devices)->next = malloc(sizeof(struct list_of_devices));
        if(*list_of_devices == NULL){
            return USBUTIL_MALLOC_FAIL;
        }
        (*list_of_devices)->next->prev = *list_of_devices;
        *list_of_devices = (*list_of_devices)->next;
        (*list_of_devices)->next = NULL;
        closedir(dir);
    }


    return 0;
}

int usbutil_extract_id(char *paths, char *d_name, struct list_of_devices*** list_of_devices, int flag){
    char filepath[MAX_PATH];
    snprintf(filepath, MAX_PATH, "%s/%s", paths, d_name);
    int fd = open(filepath, O_RDONLY);
    if(fd == -1){
        return USBUTIL_EOPEN;
    }
    char buf[32];
    int cnt = read(fd, buf, sizeof(buf)-1);
    buf[cnt] = '\0';
    if(flag == EXTRACT_IDPRODUCT){
        strncpy((**list_of_devices)->idProduct, buf, 6);
        (**list_of_devices)->idProduct[4] = '\0';
    }else if(flag == EXTRACT_IDVENDOR){
        strncpy((**list_of_devices)->idVendor, buf, 6);
        (**list_of_devices)->idVendor[4] = '\0';
    }
    close(fd);
    return 0;
}