#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "usbconf.h"
#include "utilities.h"
#include "usbutil.h"
#include "usbutilerrno.h"


#define EXTRACT_IDPRODUCT (1<<2)
#define EXTRACT_IDVENDOR  (1<<3)
#define EXTRACT_DEVNUM    (1<<4)
#define EXTRACT_BUSNUM    (1<<5)
#define ATR(x) #x
#define STRINGIFY(x) ATR(x)

#define MAX_DEVICES 32
#define MAX_PATH 256*2


int usbutil_init(){
    struct utsname kernel_version;

    if(uname(&kernel_version) == -1){
        return USBUTIL_GETKERNEL;
    }
    if(strcmp(kernel_version.release, "2.6") < 0){
        return USBUTIL_KERNELVERSION;
    }

    return 0;
}

// TODO: add functionality for printing manufactured

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
            if(counter >= MAX_DEVICES){
                break;
            }
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
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &list_of_devices, EXTRACT_IDPRODUCT);
                    if(id == USBUTIL_EOPEN){ 
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "idVendor")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &list_of_devices, EXTRACT_IDVENDOR);
                    if(id == USBUTIL_EOPEN){
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "busnum")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &list_of_devices, EXTRACT_BUSNUM);
                    if(id == USBUTIL_EOPEN){
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "devnum")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &list_of_devices, EXTRACT_DEVNUM);
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

int usbutil_extract_from_file(char *paths, char *d_name, struct list_of_devices*** list_of_devices, int flag){
    char filepath[MAX_PATH];
    snprintf(filepath, MAX_PATH, "%s/%s", paths, d_name);
    int fd = open(filepath, O_RDONLY);
    if(fd == -1){
        return USBUTIL_EOPEN;
    }
    char buf[32];
    int cnt = read(fd, buf, sizeof(buf)-1);
    buf[cnt] = '\0';
    cnt--;
    if(flag == EXTRACT_IDPRODUCT){
        strncpy((**list_of_devices)->idProduct, buf, sizeof((**list_of_devices)->idProduct));
        (**list_of_devices)->idProduct[cnt] = '\0';
    }else if(flag == EXTRACT_IDVENDOR){
        strncpy((**list_of_devices)->idVendor, buf, sizeof((**list_of_devices)->idVendor));
        (**list_of_devices)->idVendor[cnt] = '\0';
    }else if(flag == EXTRACT_DEVNUM){
        strncpy((**list_of_devices)->devnum, buf, sizeof((**list_of_devices)->devnum));
        (**list_of_devices)->devnum[cnt] = '\0';
    }else if(flag == EXTRACT_BUSNUM){
        strncpy((**list_of_devices)->busnum, buf, sizeof((**list_of_devices)->busnum));
        (**list_of_devices)->busnum[cnt] = '\0';
    }
    close(fd);
    return 0;
}

static const struct list_of_devices* find_proper_device(const char* idProduct, const char* idVendor, const struct list_of_devices *devs){
    do{
        if(strcmp(idProduct, devs->idProduct) == 0 && strcmp(idVendor, devs->idVendor) == 0){
            return devs;
        }
        devs = devs->next;
    }while(devs != NULL);

    return NULL; // not found
}


int usbutil_open(const char* idProduct, const char* idVendor, const struct list_of_devices *devs){
    const struct list_of_devices *find = find_proper_device(idProduct, idVendor, devs);
    if(find == NULL){
        usbutil_dbg(USBUTIL_EOPEN, "%d %s", __LINE__, __FILE__);
        return USBUTIL_NOT_FOUND;
    }
    char BBB[6];
    char DDD[6];
    if(strcmp("010", find->busnum) >= 0){
        snprintf(BBB, sizeof(BBB), "0%s", find->busnum);
    }else{
        snprintf(BBB, sizeof(BBB), "00%s", find->busnum);
    }
    if(strcmp("010",  find->devnum) >= 0){
        snprintf(DDD, sizeof(DDD), "0%s", find->devnum);
    }else{
        snprintf(DDD, sizeof(DDD), "00%s", find->devnum);
    }
    char path[256];
    snprintf(path, sizeof(path), STRINGIFY(DEVICEIO_PATH) "/%s/%s", BBB, DDD);
    int fd = open(path, O_RDWR); // open only in sudo
    if(fd == -1){
        return USBUTIL_EOPEN; // check for open fail messanges
    }
    
    close(fd);
    return 0;

}
