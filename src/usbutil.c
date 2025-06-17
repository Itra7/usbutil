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
#include <assert.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>


#include "usbconf.h"
#include "utilities.h"
#include "usbutil.h"
#include "usbutilerrno.h"
#include "usbutilList.h"
#include "usbio.h"
#include "deviceinfo.h"


#define EXTRACT_IDPRODUCT (1U << 2)
#define EXTRACT_IDVENDOR  (1U << 3)
#define EXTRACT_DEVNUM    (1U << 4)
#define EXTRACT_BUSNUM    (1u << 5)
#define ATR(x) #x
#define STRINGIFY(x) ATR(x)
#define GET_SIZE_OF(x) (sizeof(x)/sizeof(x[0]))

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

void usbutil_close(struct usb_device* head, struct usb_device* target_device){
    if(target_device != NULL){
        free_usb_info(&target_device->dev, target_device->endpoint0);
    }
    if(head != NULL){
        free_usb_list(head);
    }

}
// TODO: add functionality for printing manufactured

int usbutil_list_devices(struct usb_device **ptr){
    struct usb_device *devs = malloc(sizeof(struct usb_device));
    if(devs == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, "%s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }

    list_init(&devs->list);    
    struct usb_device *copy_pointer = devs;
    
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

        char* last = strrchr(paths[i], '/');
        last++; // emit '/'
        if(last != NULL){
            snprintf(devs->sysfs_path, sizeof(devs->sysfs_path), "%s/%s", STRINGIFY(SYSPATH), last);
        }

        struct usb_device *_devs = malloc(sizeof(struct usb_device));
        if(_devs == NULL){
            usbutil_dbg(USBUTIL_MALLOC_FAIL, "%s %d", __FILE__, __LINE__);
            return USBUTIL_MALLOC_FAIL;    
        }
        list_add(&devs->list, &_devs->list);

        devs = extract_next_from_list(&(devs->list), struct usb_device, list);
        devs->list.next = NULL;
        closedir(dir);
    }
    *ptr = copy_pointer;
    return 0;
}

struct usb_device* find_proper_device(const char* idProduct, const char* idVendor, struct usb_device *devs){
    struct list_head *_list;
    struct list_head *senitel = &devs->list;
    list_for_each(_list, senitel){
        devs = extract_from_list(_list, struct usb_device, list);
        if(strcmp(idProduct, devs->idProduct) == 0 && strcmp(idVendor, devs->idVendor) == 0){
            return devs;
        }
    }
    return NULL; // not found
}


int usbutil_open(const char* idProduct, const char* idVendor, struct usb_device *devs){
    struct usb_device *find = find_proper_device(idProduct, idVendor, devs);
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
    
    find->fd = fd;
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
                usbutil_dbg(USBUTIL_EOPEN, "Unkown open file error %s %d", __FILE__, __LINE__);
                break;
        }
    }
    return fd;
}


int read_and_set_field(void *struct_ptr, const char *filepath, field_type type, size_t offset) {
    size_t len = strlen(filepath);
    char _filepath[len + 1]; 
    strcpy(_filepath, filepath);
    if (_filepath[len - 1] == '/') {
        _filepath[len - 1] = '\0';
    }

    FILE *fp = fopen(_filepath, "r");
    if (fp == NULL) {
        usbutil_dbg(USBUTIL_EOPEN, "Failed to open file %s at line %d", __FILE__, __LINE__);
        return USBUTIL_EOPEN;
    }

    void *field_addr = (char *)struct_ptr + offset;

    switch (type) {
        case TYPE_U8: {
            __u8 val = 0;
            fscanf(fp, "%hhu", &val);
            *(__u8 *)field_addr = val;
            break;
        }
        case TYPE_U32: {
            __u32 val = 0;
            /* using hexadecimal because only the wMaxPacketSize is type of __u32 */
            fscanf(fp, "%x", &val);
            *(__u32 *)field_addr = val;
            break;
        }
        case TYPE_STRING: {
            char *buf = malloc(128);
            if (!buf) {
                fclose(fp);
                return USBUTIL_MALLOC_FAIL;
            }
            if (fgets(buf, 128, fp)) {
                buf[strcspn(buf, "\n")] = '\0';
                *(char **)field_addr = buf;
            } else {
                free(buf);
            }
            break;
        }
    }

    fclose(fp);
    return 0;
}


struct usb_endpoint_desc* read_usb_endpoint(const char* path){
    DIR* dir = opendir(path);
    if(dir == NULL){
        usbutil_dbg(USBUTIL_EOPEN, " Failed to open directory %s %d", __FILE__, __LINE__);
        return NULL;
    }

    struct dirent* entry;
    struct usb_endpoint_desc* usb_endpoint_desc = malloc(sizeof(struct usb_endpoint_desc));
    if(usb_endpoint_desc == NULL){
         usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to allocate struct %s %d", __FILE__, __LINE__);
         return NULL;
    }

    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == DT_REG){
            for(int i = 0; i < GET_SIZE_OF(_usb_endpoint_desc); i++){
                if(strcmp(entry->d_name, _usb_endpoint_desc[i].name) == 0){
                    char _path[256*2];
                    snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);
                    read_and_set_field(usb_endpoint_desc, _path, _usb_endpoint_desc[i].type, _usb_endpoint_desc[i].offset);
                }
            }
        }
    }
    closedir(dir);
    return usb_endpoint_desc;

}

struct usb_interface_desc* read_usb_interface(const char* path){
    DIR* dir = opendir(path);
    if(dir == NULL){
        usbutil_dbg(USBUTIL_EOPEN, " Failed to open directory %s %d", __FILE__, __LINE__);
        return NULL;
    }

    struct dirent* entry;
    struct usb_interface_desc* usb_interface_desc = malloc(sizeof(struct usb_interface_desc));
    if(usb_interface_desc == NULL){
         usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to allocate struct %s %d", __FILE__, __LINE__);
         return NULL;
    }

    __u8 endpoint_num = 0;
    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == DT_DIR){
            if(entry->d_name[0] == 'e' && entry->d_name[1] == 'p'){
                char _path[256*2];
                snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);
                
                struct usb_endpoint_desc* usb_endpoint_desc = read_usb_endpoint(_path);
                if(usb_endpoint_desc != NULL){
                    usb_interface_desc->usb_endpoint_desc[endpoint_num++] = usb_endpoint_desc;
                }else{
                    usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to retrieve info %s %d" __FILE__, __LINE__);
                    return NULL;
                }
            }
        }

        if(entry->d_type == DT_REG){
            for(int i = 0; i < GET_SIZE_OF(_usb_interface_desc); i++){
                if(strcmp(entry->d_name, _usb_interface_desc[i].name) == 0){
                    char _path[256*2];
                    snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);
                    read_and_set_field(usb_interface_desc, _path, _usb_interface_desc[i].type, _usb_interface_desc[i].offset);
                }
            }
        }
    }
    closedir(dir);
    return usb_interface_desc;
}


struct usb_desc* read_usb_device(const char* path, struct usb_endpoint_desc** endpoint0){
    char* last = strrchr(path, '/');
    last++;
    if(last[0] == 'u'){ // usb_root;
        return NULL;
    }
    
    DIR* dir = opendir(path);
    if(dir == NULL){
        usbutil_dbg(USBUTIL_EOPEN, " Failed to open directory %s %d", __FILE__, __LINE__);
        return NULL;
    }
    struct dirent* entry;

    struct usb_desc* usb_desc = malloc(sizeof(struct usb_desc));
    if(usb_desc == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to malloc usb_desc %s %d", __FILE__, __LINE__);
        return NULL;
    }
    usb_desc->usb_configuration_desc[0] = malloc(sizeof(struct usb_configuration_desc));
    if(usb_desc->usb_configuration_desc[0] == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to malloc usb_desc %s %d", __FILE__, __LINE__);
        return NULL;
    }


    

    /*  USB descriptor and USB configuration is stored in same file.
        Now when we find overlapping file name in both struct we will
        just copy paste in both structs
    */
    __u8 interface_num = 0;
    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == DT_DIR){
            /* If first character is number then it is interface */
            if(entry->d_name[0] - '0' >= 0 && entry->d_name[0] - '0' <= 9){
                char _path[256*2];
                snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);

                struct usb_interface_desc* usb_interface_desc = read_usb_interface(_path);

                if(usb_interface_desc != NULL){
                    usb_desc->usb_configuration_desc[0]->usb_interface_desc[interface_num++] = usb_interface_desc;
                }else{
                    usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to retrieve info %s %d" __FILE__, __LINE__);
                    return NULL;
                }
        
            }else if(entry->d_name[0] == 'e' && entry->d_name[1] == 'p'){
                char _path[256*2];
                snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);
                struct usb_endpoint_desc* usb_endpoint_desc = read_usb_endpoint(_path);

                if(usb_endpoint_desc != NULL){
                    *endpoint0 = usb_endpoint_desc;
                }else{
                    usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to retrieve info %s %d" __FILE__, __LINE__);
                    return NULL;
                }
            }
        }

        if(entry->d_type == DT_REG){
            for(int i = 0; i < GET_SIZE_OF(_usb_desc); i++){
                if(strcmp(entry->d_name, _usb_desc[i].name) == 0){
                    char _path[256*2];
                    snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);
                    read_and_set_field(usb_desc, _path, _usb_desc[i].type, _usb_desc[i].offset);
                }
            }
            for(int i = 0; i < GET_SIZE_OF(_usb_cofiguration_desc); i++){
                if(strcmp(entry->d_name, _usb_cofiguration_desc[i].name) == 0){
                    char _path[256*2];
                    snprintf(_path, sizeof(_path), "%s/%s/", path, entry->d_name);
                    read_and_set_field(usb_desc->usb_configuration_desc[0], _path, _usb_cofiguration_desc[i].type, _usb_cofiguration_desc[i].offset);                    
                }
            }
        }
    }
    closedir(dir);
    return usb_desc;
}

void free_usb_info(struct usb_desc** dev, struct usb_endpoint_desc* endpoint0){
    if(*dev == NULL){
        return;
    }
    if((*dev)->bcdDevice != NULL){
        free((*dev)->bcdDevice);
    }

    for(int j = 0; j < (*dev)->bNumConfigurations; j++){
        for(int i = 0; i < (*dev)->usb_configuration_desc[j]->bNumInterfaces; i++){
            for(int k = 0; k < (*dev)->usb_configuration_desc[j]->usb_interface_desc[i]->bNumEndpoints; k++){
                free((*dev)->usb_configuration_desc[j]->usb_interface_desc[i]->usb_endpoint_desc[k]->type);
                free((*dev)->usb_configuration_desc[j]->usb_interface_desc[i]->usb_endpoint_desc[k]);
            }   
            free((*dev)->usb_configuration_desc[j]->usb_interface_desc[i]);
        }
        free((*dev)->usb_configuration_desc[j]);
    }
    free(*dev);
    free(endpoint0->type);
    free(endpoint0);
}

void print_usb_list(struct usb_device* devs){
    struct list_head *_list;
    struct list_head *senitel = &devs->list;
    list_for_each(_list, senitel){
        devs = container_of(_list, struct usb_device, list);
        if(devs->list.next == NULL) break;
        printf("%s:%s %s:%s\n", devs->idProduct, devs->idVendor, devs->busnum, devs->devnum);
    }
}

void free_usb_list(struct usb_device *devs){
    struct list_head *_list;
    struct list_head *senitel = &devs->list;
    int i = 0;
    list_for_each(_list, senitel){
        devs = container_of(_list, struct usb_device, list);
        free(devs);
        printf("freed=%d", i);
        i++;
    }
}

int usbutil_extract_from_file(char *paths, char *d_name, struct usb_device** usb_device, int flag){
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
        strncpy((*usb_device)->idProduct, buf, sizeof((*usb_device)->idProduct));
        (*usb_device)->idProduct[cnt] = '\0';
    }else if(flag == EXTRACT_IDVENDOR){
        strncpy((*usb_device)->idVendor, buf, sizeof((*usb_device)->idVendor));
        (*usb_device)->idVendor[cnt] = '\0';
    }else if(flag == EXTRACT_DEVNUM){
        strncpy((*usb_device)->devnum, buf, sizeof((*usb_device)->devnum));
        (*usb_device)->devnum[cnt] = '\0';
    }else if(flag == EXTRACT_BUSNUM){
        strncpy((*usb_device)->busnum, buf, sizeof((*usb_device)->busnum));
        (*usb_device)->busnum[cnt] = '\0';
    }
    close(fd);
    return 0;
}


/* IOCTL commands */

/* make it for isochrnous */
static void discard_urb(struct usbdevfs_urb* urbs, int first, int last, int fd){
    for(int i = last-1; i >= first; i--){
        if(ioctl(fd, USBUTIL_USBDEVFS_DISCARDURB, &urbs[i]) != 0){
            if(errno == ENODEV){
                usbutil_dbg(USBUTIL_NOT_FOUND, " Device not found %s %d", __FILE__, __LINE__);
            }else if(errno == EINVAL){
                usbutil_dbg(USBUTIL_NOT_FOUND, " URB not found %s %d", __FILE__, __LINE__);
            }
        }
    }
}

int clear_halt(struct usb_device* usb_device, int endpoint){
    int fd = usb_device->fd;
    if(ioctl(fd, USBUTIL_USBDEVFS_CLEAR_HALT, &endpoint) < 0){
        if(errno == ENODEV){
            usbutil_dbg(USBUTIL_OTHER, " no device %s %d", __FILE__, __LINE__);
            return USBUTIL_OTHER;
        }
        usbutil_dbg(USBUTIL_OTHER, " Unkwnow errno = %d %s %d", errno, __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    return 0;
}

static int send_iso_endpoint(struct usb_device* usb_device, int* sent){
    struct usbdevfs_urb* usbdevfs_urb = usb_device->usb_transfer->usbdevfs_urb;
    struct usb_endpoint_desc* endpoint = usb_device->usb_transfer->usb_endpoint_desc;
    struct usbdevfs_urb* urbs;
    int fd = usb_device->fd;

    int maxPacketSize = endpoint->wMaxPacketSize;

    int num_of_iso = usbdevfs_urb->buffer_length / maxPacketSize;
    if(usbdevfs_urb->buffer_length % maxPacketSize > 0){
        num_of_iso++;
    }

    struct usbdevfs_iso_packet_desc* iso = malloc(sizeof(struct usbdevfs_iso_packet_desc) * num_of_iso);
    if(iso == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, " Iso packets allocation fail %s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }
    
    urbs = calloc(1, sizeof(struct usbdevfs_urb));
    if(urbs == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, " URB allocation fail %s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }

    for(int i = 0; i < num_of_iso; i++){
        if(num_of_iso == 1){
            iso[i].length = usbdevfs_urb->buffer_length;
        }else{
            iso[i].length = maxPacketSize;
        }
        urbs->iso_frame_desc[i] = iso[i];
    }
 

    urbs->type = usbdevfs_urb->type;
    urbs->endpoint = usbdevfs_urb->endpoint;
    urbs->buffer = usbdevfs_urb->buffer;
    urbs->buffer_length = num_of_iso == 1 ? MIN(maxPacketSize, usbdevfs_urb->buffer_length) : num_of_iso * maxPacketSize;
    urbs->start_frame = 0;
    urbs->number_of_packets = num_of_iso;
    urbs->flags = 0;

    int ret = ioctl(fd, USBUTIL_USBDEVFS_SUBMITURB, urbs);
    if(ret < 0){
        free(iso);

        if(errno == ENODEV){
            usbutil_dbg(USBUTIL_NOT_FOUND, " device not found %s %d", __FILE__, __LINE__);
        }else if(errno == EINVAL){
            usbutil_dbg(USBUTIL_IOCTL_FAIL, " parameters failed %s %d", __FILE__, __LINE__);
        }else{
            usbutil_dbg(USBUTIL_OTHER, " unknown error errno = %d %s %d", errno, __FILE__, __LINE__);
        }
        discard_urb(urbs, 0, 1, fd);
        return USBUTIL_IOCTL_FAIL;
    }

    return 0;


}

static int send_control_endpoint(struct usb_device* usb_device, int* sent){
    struct usbdevfs_urb* usbdevfs_urb = usb_device->usb_transfer->usbdevfs_urb;
    struct usb_endpoint_desc* endpoint = usb_device->usb_transfer->usb_endpoint_desc;
    struct usbdevfs_urb* urbs;
    int fd = usb_device->fd;

    urbs = calloc(1, sizeof(struct usbdevfs_urb));
    if(urbs == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to allocate contorl urb %s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }

    int maxPacketSize = endpoint->wMaxPacketSize;

    if(usbdevfs_urb->buffer_length > maxPacketSize){
        usbutil_dbg(USBUTIL_OTHER, " Buffer too long %s %d", __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }

    urbs->type = usbdevfs_urb->type;
    urbs->endpoint = usbdevfs_urb->endpoint;
    urbs->buffer = usbdevfs_urb->buffer;
    urbs->buffer_length = usbdevfs_urb->buffer_length;

    if(ioctl(fd, USBUTIL_USBDEVFS_SUBMITURB, urbs) < 0){
        free(urbs);
        usbutil_dbg(USBUTIL_OTHER, " failed with errno=%d %s %d", errno, __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    *sent += urbs->buffer_length;
    return 0;

}

static int send_bulk_endpoint(struct usb_device* usb_device, int* sent){
    struct usbdevfs_urb* usbdevfs_urb = usb_device->usb_transfer->usbdevfs_urb;
    struct usb_endpoint_desc* endpoint = usb_device->usb_transfer->usb_endpoint_desc;
    struct usbdevfs_urb* urbs;
    int fd = usb_device->fd;

    int is_in = usb_endpoint_dir_in(endpoint);
    
    int maxPacketSize = endpoint->wMaxPacketSize;
    int number_of_urbs;

    if(usbdevfs_urb->buffer_length > maxPacketSize){
        number_of_urbs = usbdevfs_urb->buffer_length / maxPacketSize;

        if(usbdevfs_urb->buffer_length % maxPacketSize > 0){
            number_of_urbs++;
        }
    }else{
        number_of_urbs = 1;
    }
    urbs = calloc(number_of_urbs, sizeof(struct usbdevfs_urb));
    if(urbs == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, " Failed to allocate urbs %s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }

    for(int i = 0; i < number_of_urbs; i++){
        struct usbdevfs_urb* urb = &urbs[i];
        printf("num_of_urbs = %d, datasent = %d\n", number_of_urbs, *sent);

        urb->type = usbdevfs_urb->type;
        urb->endpoint = usbdevfs_urb->endpoint;
        urb->flags = 0;

        if(is_in && i < number_of_urbs-1){
            urb->flags = USBUTIL_USBDEVFS_URB_SHORT_NOT_OK;
        }

        if(i > 0){
            urb->flags |= USBUTIL_USBDEVFS_URB_BULK_CONTINUATION;
        }

        urb->buffer = usbdevfs_urb->buffer + (i * MIN(maxPacketSize, usbdevfs_urb->buffer_length));

        /* last one*/
        if(i == number_of_urbs-1){
            printf("AFTERnum_of_urbs = %d, datasent = %d\n", number_of_urbs, *sent);

            *sent += usbdevfs_urb->buffer_length % maxPacketSize == 0 ? MIN(maxPacketSize, usbdevfs_urb->buffer_length) : usbdevfs_urb->buffer_length % maxPacketSize;
            urb->buffer_length = usbdevfs_urb->buffer_length % maxPacketSize == 0 ? MIN(maxPacketSize, usbdevfs_urb->buffer_length) : usbdevfs_urb->buffer_length % maxPacketSize;
        }else{
            urb->buffer_length = MIN(maxPacketSize, usbdevfs_urb->buffer_length);
            *sent += MIN(maxPacketSize, usbdevfs_urb->buffer_length);
        }

        if(ioctl(fd, USBUTIL_USBDEVFS_SUBMITURB, urb) == 0){
            continue;
        }

        if(i == 0){
            usbutil_dbg(USBUTIL_OTHER, " Failed sending first urb %s %d errno=%d", __FILE__, __LINE__, errno);
            free(urb);
            return USBUTIL_OTHER;
        }


        if(errno == ENODEV){
            usbutil_dbg(USBUTIL_NOT_FOUND, " Device not found %s %d", __FILE__, __LINE__);
            return USBUTIL_NOT_FOUND;
        }else{
            usbutil_dbg(USBUTIL_OTHER, " Unknown error %s %d", __FILE__, __LINE__);
            return USBUTIL_OTHER;
        }

        /* transfer ended early because the device sent less data (e.g. a short packet). */
        if(errno == EREMOTEIO){
            return 0;
        }

        discard_urb(urbs, 0, i, fd);
    }
    /* SUBMITURB and DISCARDURB will automatically free other pointers */
    return 0;

}


int set_urb(struct usb_device* usb_device, unsigned char type, 
                                unsigned char endpoint, void* buffer, int length){
    struct usbdevfs_urb* usbdevfs_urb = malloc(sizeof(struct usbdevfs_urb));
    if(usbdevfs_urb == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, "Failed to allocate urb %s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }
    usb_device->usb_transfer = malloc(sizeof(struct usb_transfer));
    if(usb_device->usb_transfer == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, "Failed to allocate usb_transfer %s %d", __FILE__, __LINE__);
        return USBUTIL_MALLOC_FAIL;
    }
    usbdevfs_urb->type = type;
    usbdevfs_urb->endpoint = endpoint;
    usbdevfs_urb->buffer_length = length;
    usbdevfs_urb->buffer = buffer;
    usbdevfs_urb->flags = 0;

    if(type == USBUTIL_USBDEVFS_URB_TYPE_CONTROL){
        usb_device->usb_transfer->usb_endpoint_desc = usb_device->endpoint0;
        usb_device->usb_transfer->status = 0;
        usb_device->usb_transfer->usbdevfs_urb = usbdevfs_urb;
        return 0;    
    }

    struct usb_endpoint_desc* usb_endpoint_desc = get_endpoint_class(usb_device->dev->usb_configuration_desc[0], 
                                                                        endpoint);
    
    if(usb_endpoint_desc == NULL){
        usbutil_dbg(USBUTIL_MALLOC_FAIL, "Failed to allocate urb %s %d", __FILE__, __LINE__);
        free(usb_endpoint_desc);
        free(usbdevfs_urb);
        return USBUTIL_MALLOC_FAIL;
    }

    usb_device->usb_transfer->usb_endpoint_desc = usb_endpoint_desc;
    usb_device->usb_transfer->status = 0;
    usb_device->usb_transfer->usbdevfs_urb = usbdevfs_urb;
    return 0;
}

/* check return type */
int process_urb(struct usb_device* usb_device){
    struct usbdevfs_urb* usbdevfs_urb = usb_device->usb_transfer->usbdevfs_urb;
    int sent = 0;
    switch(usbdevfs_urb->type){
        case USBUTIL_USBDEVFS_URB_TYPE_ISO:{
            send_iso_endpoint(usb_device, &sent);
            break;
        }
        case USBUTIL_USBDEVFS_URB_TYPE_INTERRUPT:{
            send_bulk_endpoint(usb_device, &sent);
            break;
        }
        case USBUTIL_USBDEVFS_URB_TYPE_CONTROL:{
            send_control_endpoint(usb_device, &sent);
            break;
        }
        case USBUTIL_USBDEVFS_URB_TYPE_BULK:{
            send_bulk_endpoint(usb_device, &sent);
            break;
        }
        default:{
            usbutil_dbg(USBUTIL_OTHER, "unkown transfer? %s %d", __FILE__, __LINE__);
            return USBUTIL_OTHER;
        }
    }
    return sent;

}


void usbutil_get_device_speed(struct usb_device* usb_device){
    int fd = usb_device->fd;
    __u8 speed = ioctl(fd, USBUTIL_USBDEVFS_GET_SPEED);
    usb_device->device_speed = speed;
}


int get_capabilites(struct usb_device* usb_device){
    int fd = usb_device->fd;
    __u32 capabilities;
    if(ioctl(fd, USBUTIL_USBDEVFS_GET_CAPABILITIES, &capabilities) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Failed to get capabilites %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }
    usb_device->capabilities = capabilities;
    return 0;
}

int drop_privilegies(struct usb_device* usb_device, int capability){
    // if this capability was alreardy droped
    if((usb_device->capabilities & capability) == 0){
        return 0;
    }
    int fd = usb_device->fd;
    if(ioctl(fd, USBUTIL_USBDEVFS_DROP_PRIVILEGES, &capability) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Ioctl fail on drop privilegies %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }
    usb_device->capabilities = usb_device->capabilities & ~(capability);
    return 0;
}

/* Only for USB 3.0 or newest devices and wMaxStreams > 0 */
int alloc_stream(struct usb_device* usb_device, struct usbdevfs_streams* stream){
    int fd = usb_device->fd;
    /* add check for wMaxStreams */
    if(!(usb_device->device_speed == USB_SPEED_SUPER || usb_device->device_speed == USB_SPEED_SUPER_PLUS)){
        usbutil_dbg(USBUTIL_NOT_SUPPORTED, " Allocate stream%s %d", __FILE__, __LINE__);
        return USBUTIL_NOT_SUPPORTED;
    }
    if(ioctl(fd, USBUTIL_USBDEVFS_ALLOC_STREAMS, stream) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Ioctl fail on alloc stream %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }

    return 0;
}

int free_stream(struct usb_device* usb_device, struct usbdevfs_streams* stream){
    int fd = usb_device->fd;
    if(!(usb_device->device_speed == USB_SPEED_SUPER || usb_device->device_speed == USB_SPEED_SUPER_PLUS)){
        usbutil_dbg(USBUTIL_NOT_SUPPORTED, " Free stream%s %d", __FILE__, __LINE__);
        return USBUTIL_NOT_SUPPORTED;
    }
    if(ioctl(fd, USBUTIL_USBDEVFS_FREE_STREAMS, stream) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Ioctl fail on alloc stream %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }
    
    return 0;
}

int reap_urb(struct usb_device* usb_device, struct usbdevfs_urb *reap){
    int fd = usb_device->fd;
    if(ioctl(fd, USBUTIL_USBDEVFS_REAPURB, &reap) < 0){
        usbutil_dbg(USBUTIL_IOCTL_FAIL, " Ioctl fail on reap urb %s %d", __FILE__, __LINE__);
        return USBUTIL_IOCTL_FAIL;
    }
    return 0;
}

static int _claim_interface(struct usb_device* usb_device, int interface){
    int fd = usb_device->fd;
    if(ioctl(fd, USBUTIL_USBDEVFS_CLAIMINTERFACE, interface) < 0){
        if (errno == ENOENT){
            usbutil_dbg(USBUTIL_NOT_FOUND, "interface not founded %s %d", __FILE__, __LINE__);
			return USBUTIL_NOT_FOUND;
        }
		usbutil_dbg(USBUTIL_OTHER, "failed claiming interface %s %d" __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    return 0;
}

int detach_kernel_driver(struct usb_device* usb_device, int interface){
    int fd = usb_device->fd;
    struct usbdevfs_ioctl ioctl_pass;

    ioctl_pass.ifno = interface;
    ioctl_pass.ioctl_code = USBUTIL_USBDEVFS_DISCONNECT;
    ioctl_pass.data = NULL;

    struct usbdevfs_getdriver driver;
    driver.interface = interface;
    int ret = ioctl(fd, USBUTIL_USBDEVFS_GETDRIVER, &driver);
    if(ret == 0 && strcmp(driver.driver, "usbfs") == 0){
        /* driver is not found if it isn't connected to virtual bus */
        usbutil_dbg(USBUTIL_NOT_FOUND, "driver not found %s %d", __FILE__, __LINE__);
        return USBUTIL_NOT_FOUND;
    }

    ret = ioctl(fd, USBUTIL_USBDEVFS_DISCONNECT, &interface);
    if(ret < 0){
        if(errno == ENODATA){
            usbutil_dbg(USBUTIL_NOT_FOUND, " Not found %s %d", __FILE__, __LINE__);
            return USBUTIL_NOT_FOUND;
        }else if(errno == EINVAL){
            usbutil_dbg(USBUTIL_OTHER, " Invalid parameters %s %d", __FILE__, __LINE__);
            return USBUTIL_OTHER;
        }
        usbutil_dbg(USBUTIL_OTHER, "detach failed unknown error %s %d errno=%d", __FILE__, __LINE__, errno);
        return USBUTIL_OTHER;
    }
    usb_device->attached_kernel_driver = 1;
    return 0;
}


int attach_kernel_driver(struct usb_device* usb_device, int interface){
    struct usbdevfs_ioctl ioctl_pass;
    int fd = usb_device->fd;

    ioctl_pass.ifno = interface;
    ioctl_pass.ioctl_code = USBUTIL_USBDEVFS_CONNECT;
    ioctl_pass.data = NULL;

    int ret = ioctl(fd, USBUTIL_USBDEVFS_IOCTL, &ioctl_pass);
    if(ret == 0){
        usbutil_dbg(USBUTIL_NOT_FOUND, " Not found %s %d", __FILE__, __LINE__);
        return USBUTIL_NOT_FOUND;
    }else if(ret < 0){
        if(errno == ENODATA){
            usbutil_dbg(USBUTIL_NOT_FOUND, " Not found %s %d", __FILE__, __LINE__);
            return USBUTIL_NOT_FOUND;    
        }else if(errno == ENODEV){
            usbutil_dbg(USBUTIL_NOT_FOUND, " Not found driver %s %d", __FILE__, __LINE__);
            return USBUTIL_NOT_FOUND;    
        }else if(errno == EBUSY){
            usbutil_dbg(USBUTIL_NOT_FOUND, " device busy %s %d", __FILE__, __LINE__);
            return USBUTIL_NOT_FOUND;    
        }else{
            usbutil_dbg(USBUTIL_NOT_FOUND, " device busy %s %d", __FILE__, __LINE__);
            return USBUTIL_NOT_FOUND;
        }
    }
    
    usb_device->attached_kernel_driver = 0;
    return 0;
}

int detach_kernel_driver_and_claim(struct usb_device* usb_device, int interface){
    int fd = usb_device->fd;
    struct usbdevfs_disconnect_claim udc;
    
    udc.flags = USBUTIL_USBDEVFS_DISCONNECT_CLAIM_EXCEPT_DRIVER;
    udc.interface = interface;
    strcpy(udc.driver, "usbfs");

    if(ioctl(fd, USBUTIL_USBDEVFS_DISCONNECT_CLAIM, &udc) == 0){
        usb_device->attached_kernel_driver = 1;
        return 0;
    }
    if(errno == ENOTTY){
        int ret = detach_kernel_driver(usb_device, interface);
        if(ret != 0){
            return ret;
        }
        return _claim_interface(usb_device, interface);
    }else{
        usbutil_dbg(USBUTIL_OTHER, " failed to detach and claim interface errno = %d %s %d", errno, __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    
}


int claim_interface(struct usb_device* usb_device, int interface){
    if(interface < 0){
        usbutil_dbg(USBUTIL_OTHER, " Negative interface %s %d", __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    if(!usb_device->attached_kernel_driver){
        return detach_kernel_driver_and_claim(usb_device, interface);
    }
    return _claim_interface(usb_device, interface);
}

int release_interface(struct usb_device* usb_device, int interface){
    if(interface < 0){
        usbutil_dbg(USBUTIL_OTHER, " Negative interface %s %d", __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    int fd = usb_device->fd;

    if(ioctl(fd, USBUTIL_USBDEVFS_RELEASEINTERFACE, &interface) < 0){
        if (errno == ENODEV){
            usbutil_dbg(USBUTIL_NOT_FOUND, "interface not found %s %d", __FILE__, __LINE__);
			return USBUTIL_NOT_FOUND;
        }
        usbutil_dbg(USBUTIL_OTHER, " Error while releasing interface %s %d", __FILE__, __LINE__);
        return USBUTIL_OTHER;
    }
    if(!usb_device->attached_kernel_driver){
        int ret = attach_kernel_driver(usb_device, interface);
        if(ret != 0){
            return ret;
        }
    }
    return 0;
}



