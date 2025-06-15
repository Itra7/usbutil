#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>


#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <string.h>


#include "usbconf.h"
#include "usbutilerrno.h"
#include "usbio.h"
#include "utilities.h"
#include "usbutil.h"
#include "usbutilList.h"

#define MINORBITS        20
#define MINORMASK        ((1U << MINORBITS) - 1)

#define MAJOR(dev)        ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)        ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi)      (((ma) << MINORBITS) | (mi))


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
 
    
    err = usbutil_open("0129", "0bda", devices);
    if(err != 0){
        usbutil_dbg(err, "failed open SUDO %s %d", __FILE__, __LINE__);
        return err;
    }
    struct usb_device* device_struct = find_proper_device("0129", "0bda", devices);

    device_struct->dev = read_usb_device(device_struct->sysfs_path, &device_struct->endpoint0);

    
    char* buffer = calloc(0x40, sizeof(char));

    strcpy(buffer, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

    if(set_urb(device_struct, USBUTIL_USBDEVFS_URB_TYPE_BULK, 0x01, buffer, 0x40) != 0){
        printf("neki error u set_urb");
        free_usb_info(&device_struct->dev, device_struct->endpoint0);

        free_usb_list(devices);
    
        return 0;
    }
    if(claim_interface(device_struct, 0) != 0){
        free_usb_info(&device_struct->dev, device_struct->endpoint0);

        free_usb_list(devices);
    
        printf("interject");
        return 0;
    }

    int datasent = process_urb(device_struct);
    
    release_interface(device_struct, 0);

    printf("datasent = %d\n", datasent);
    free(buffer);

    free_usb_info(&device_struct->dev, device_struct->endpoint0);

    free_usb_list(devices);

    

    return 0;
}
