#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


#include "usbconf.h"
#include "usbutilerrno.h"
#include "usbio.h"
#include "utilities.h"
#include "usbutil.h"
#include "usbutilList.h"


void print_list(struct usb_device *devs);
void free_list(struct usb_device *devs);

int main(){
    struct usb_device* devices;
   
    int err = 0;
    if(usbutil_init() != 0){
        return -1;
    }

    err = usbutil_list_devices(&devices);
    if(err != 0){
        usbutil_dbg(err, "%s %d", __FILE__, __LINE__);
        return err;
    }
 
    // err = usbutil_open("505b", "320f", devices);
    // if(err != 0){
    //     usbutil_dbg(err, "failed open SUDO %s %d", __FILE__, __LINE__);
    //     return err;
    // }

    print_list(devices);

    devices->dev = read_usb_device("/sys/bus/usb/devices/1-1");

    printf("bLength = %d\n", devices->dev->bMaxPacketSize0);

    free_list(devices);

    

    return 0;
}

void print_list(struct usb_device* devs){
    struct list_head *_list;
    struct list_head *senitel = &devs->list;
    list_for_each(_list, senitel){
        devs = container_of(_list, struct usb_device, list);
        if(devs->list.next == NULL) break;
        printf("%s:%s %s:%s\n", devs->idProduct, devs->idVendor, devs->busnum, devs->devnum);
    }
}

void free_list(struct usb_device *devs){
    struct list_head *_list;
    struct list_head *senitel = &devs->list;
    int i = 0;
    list_for_each(_list, senitel){
        devs = container_of(_list, struct usb_device, list);
        free(devs);
        printf("%d", i);
        i++;
    }
}