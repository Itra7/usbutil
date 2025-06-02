#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>

#include "usbconf.h"
#include "utilities.h"
#include "usbutil.h"
#include "usbutilerrno.h"
#include "usbutilList.h"
#include "usbio.h"


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

int usbutil_list_devices(struct list_of_devices **ptr){
    struct list_of_devices *devs = malloc(sizeof(struct list_of_devices));
    if(devs == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, "%s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }

    list_init(&devs->list);    
    struct list_of_devices *copy_pointer = devs;
    
    struct dirent *entry;
    DIR* dir = opendir(STRINGIFY(SYSPATH));
    if(dir == NULL){
        usbutil_dbg(USBUTIL_EOPEN, "Failed to open dir %s %d", __FILE__, __LINE__);
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
            usbutil_dbg(USBUTIL_EOPEN, "Failed to open dir %s %d", __FILE__, __LINE__);
            return USBUTIL_EOPEN;
        }
        while((entry = readdir(dir)) != NULL){
            if(entry->d_type == DT_REG){
                if(!strcmp(entry->d_name, "idProduct")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &devs, EXTRACT_IDPRODUCT);
                    if(id == USBUTIL_EOPEN){ 
                        usbutil_dbg(USBUTIL_EOPEN, "Failed to open file %s %d", __FILE__, __LINE__);
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "idVendor")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &devs, EXTRACT_IDVENDOR);
                    if(id == USBUTIL_EOPEN){
                        usbutil_dbg(USBUTIL_EOPEN, "Failed to open file %s %d", __FILE__, __LINE__);
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "busnum")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &devs, EXTRACT_BUSNUM);
                    if(id == USBUTIL_EOPEN){
                        usbutil_dbg(USBUTIL_EOPEN, "Failed to open file %s %d", __FILE__, __LINE__);
                        return USBUTIL_EOPEN;
                    }
                }else if(!strcmp(entry->d_name, "devnum")){
                    int id = usbutil_extract_from_file(paths[i], entry->d_name, &devs, EXTRACT_DEVNUM);
                    if(id == USBUTIL_EOPEN){
                        usbutil_dbg(USBUTIL_EOPEN, "Failed to open file %s %d", __FILE__, __LINE__);
                        return USBUTIL_EOPEN;
                    }
                }
            }
        }

        struct list_of_devices *_devs = malloc(sizeof(struct list_of_devices));
        if(_devs == NULL){
            usbutil_dbg(USBUTIL_MALLOC_FAIL, "%s %d", __FILE__, __LINE__);
            return USBUTIL_MALLOC_FAIL;    
        }
        list_add(&devs->list, &_devs->list);

        devs = extract_next_from_list(&(devs->list), struct list_of_devices, list);
        devs->list.next = NULL;
        closedir(dir);
    }

    *ptr = copy_pointer;

    return 0;
}

int usbutil_extract_from_file(char *paths, char *d_name, struct list_of_devices** list_of_devices, int flag){
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
        strncpy((*list_of_devices)->idProduct, buf, sizeof((*list_of_devices)->idProduct));
        (*list_of_devices)->idProduct[cnt] = '\0';
    }else if(flag == EXTRACT_IDVENDOR){
        strncpy((*list_of_devices)->idVendor, buf, sizeof((*list_of_devices)->idVendor));
        (*list_of_devices)->idVendor[cnt] = '\0';
    }else if(flag == EXTRACT_DEVNUM){
        strncpy((*list_of_devices)->devnum, buf, sizeof((*list_of_devices)->devnum));
        (*list_of_devices)->devnum[cnt] = '\0';
    }else if(flag == EXTRACT_BUSNUM){
        strncpy((*list_of_devices)->busnum, buf, sizeof((*list_of_devices)->busnum));
        (*list_of_devices)->busnum[cnt] = '\0';
    }
    close(fd);
    return 0;
}

static struct list_of_devices* find_proper_device(const char* idProduct, const char* idVendor, struct list_of_devices *devs){
    struct list_head *_list;
    struct list_head *senitel = &devs->list;
    list_for_each(_list, senitel){
        devs = extract_from_list(_list, struct list_of_devices, list);
        if(strcmp(idProduct, devs->idProduct) == 0 && strcmp(idVendor, devs->idVendor) == 0){
            return devs;
        }
    }

    return NULL; // not found
}


int usbutil_open(const char* idProduct, const char* idVendor, struct list_of_devices *devs){
    struct list_of_devices *find = find_proper_device(idProduct, idVendor, devs);
    if(find == NULL){
        usbutil_dbg(USBUTIL_NOT_FOUND, "%d %s", __LINE__, __FILE__);
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

// Implement check if flags are set
int sysfs_fd_open(const char* device, const char* file_name, const int flags){
    if(file_name == NULL || device == NULL){
        usbutil_dbg(USBUTIL_EOPEN, "Didn't passed file name or flags %s %d", __FILE__, __LINE__);
        return USBUTIL_EOPEN;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s/%s", STRINGIFY(SYSPATH), device, file_name);

    int fd = open(path, flags);
    if(fd == -1){
        switch(errno){
            case EACCES:
                usbutil_dbg(USBUTIL_EOPEN, " Access error %s %d", __FILE__, __LINE__);
                break;
            case EISDIR:
                usbutil_dbg(USBUTIL_EOPEN, " File is directory %s %d", __FILE__, __LINE__);
                break;
            case ENOENT:
                usbutil_dbg(USBUTIL_EOPEN, " File doesn't exist %s %d", __FILE__, __LINE__);
                break;
            default:
                usbutil_dbg(USBUTIL_EOPEN, "Unkown openfile error %s %d", __FILE__, __LINE__);
                break;
        }
    }
    return fd;
}

void get_device_speed(struct usb_device** usb_device){
    int fd = (*usb_device)->fd;
    __u8 speed = ioctl(fd, USBUTIL_USBDEVFS_GET_SPEED);
    (*usb_device)->device_speed = speed;
}


int get_capabilites(struct usb_device** usb_device){
    int fd = (*usb_device)->fd;
    __u32 capabilities;
    if(ioctl(fd, USBUTIL_USBDEVFS_GET_CAPABILITIES, &capabilities) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Failed to get capabilites %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }
    (*usb_device)->capabilities = capabilities;
    return 0;
}

int drop_privilegies(struct usb_device** usb_device, int capability){
    // if this capability was alreardy droped
    if((*usb_device)->capabilities & capability == 0){
        return 0;
    }
    int fd = (*usb_device)->fd;
    if(ioctl(fd, USBUTIL_USBDEVFS_DROP_PRIVILEGES, &capability) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Ioctl fail on drop privilegies %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }
    (*usb_device)->capabilities = (*usb_device)->capabilities & ~(capability);
    return 0;
}
