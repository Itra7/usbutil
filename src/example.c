#include <stdio.h>
#include <stdlib.h>

#include "usbconf.h"
#include "usbutilerrno.h"
#include "usbio.h"
#include "utilities.h"

void print_list(struct list_of_devices* devs);
void free_list(struct list_of_devices **devs);

int main(){
    struct list_of_devices* devices = malloc(sizeof(struct list_of_devices));
    if(devices == NULL){
        return USBUTIL_MALLOC_FAIL;
    }
    struct list_of_devices* ptr = devices;
    if(usbutil_list_devices(&devices)){
        return -1;
    }

    print_list(ptr);
    free_list(&ptr);
    return 0;
}

void print_list(struct list_of_devices* devs){
    while(devs->next != NULL){
        printf("%s:%s\n", devs->idProduct, devs->idVendor);
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