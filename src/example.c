#include <stdio.h>
#include <stdlib.h>

#include "usbconf.h"
#include "usbutilerrno.h"
#include "usbio.h"
#include "utilities.h"
#include "usbutil.h"


void print_list(struct list_of_devices* devs);
void free_list(struct list_of_devices **devs);

int main(){
    struct list_of_devices* devices = malloc(sizeof(struct list_of_devices));
    if(devices == NULL){
        return USBUTIL_MALLOC_FAIL;
    }
    struct list_of_devices* ptr = devices;
    int err = 0;
    if(usbutil_init() != 0){
        return -1;
    }

    err = usbutil_list_devices(&devices);
    if(err != 0){
        usbutil_dbg(err, "%s %d", __FILE__, __LINE__);
    }
    devices = ptr;
    err = usbutil_open("505b", "320f", devices);
    if(err != 0){
        usbutil_dbg(err, "%s %d", __FILE__, __LINE__);
    }


    print_list(ptr);
    free_list(&ptr);
    return 0;
}

void print_list(struct list_of_devices* devs){
    while(devs->next != NULL){
        printf("%s:%s %s:%s\n", devs->idProduct, devs->idVendor, devs->busnum, devs->devnum);
        devs = devs->next;
    }
}

void free_list(struct list_of_devices **devs){
    do{
        if((*devs)->next == NULL){
            free(*devs);
            break;
        }
        *devs = (*devs)->next;
        free((*devs)->prev);
    }while(*devs != NULL);
}