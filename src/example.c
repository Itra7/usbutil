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
    
    if(usbutil_init() != 0){
        return -1;
    }

    if(usbutil_list_devices(&devices) != 0){
        return -1;
    }
    devices = ptr;
    if(usbutil_open("505b", "320f", devices) != 0){
        printf("some error");
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