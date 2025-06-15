#include <stdio.h>
#include <stdlib.h>

#include "usbconf.h"
#include "usbutil.h"
#include "usbutilerrno.h"
#include "usbutil.h"


__u32 get_max_packet_size(const struct usb_configuration_desc* usb_configuration_desc, 
                            const __u32 num_of_endpoint){
    for(int i = 0; i < usb_configuration_desc->bNumInterfaces; i++){
        struct usb_interface_desc* usb_interface_desc = usb_configuration_desc->usb_interface_desc[i];
        for(int j = 0; j < usb_interface_desc->bNumEndpoints; j++){
            struct usb_endpoint_desc* usb_endpoint_desc = usb_interface_desc->usb_endpoint_desc[j];
            if(usb_endpoint_desc->bEndpointAddress == num_of_endpoint){
                return usb_endpoint_desc->wMaxPacketSize;
            }
        }
    }

    usbutil_dbg(USBUTIL_NOT_FOUND, " Endpoint address %s %d", __FILE__, __LINE__);
    return USBUTIL_NOT_FOUND;
}

struct usb_interface_desc* get_interface_class(const struct usb_configuration_desc* usb_configuration_desc,
                                                const __u8 num_of_interface){
    for(int i = 0; i < usb_configuration_desc->bNumInterfaces; i++){
        if(usb_configuration_desc->usb_interface_desc[i]->bInterfaceNumber == num_of_interface){
            return usb_configuration_desc->usb_interface_desc[i];
        }
    }

    usbutil_dbg(USBUTIL_NOT_FOUND, " Interface class not found %s %d", __FILE__, __LINE__);
    return NULL;
}

struct usb_endpoint_desc* get_endpoint_class(const struct usb_configuration_desc* usb_configuration_desc, 
                                                const __u32 num_of_endpoint){
    for(int i = 0; i < usb_configuration_desc->bNumInterfaces; i++){
        struct usb_interface_desc* usb_interface_desc = usb_configuration_desc->usb_interface_desc[i];
        for(int j = 0; j < usb_interface_desc->bNumEndpoints; j++){
            struct usb_endpoint_desc* usb_endpoint_desc = usb_interface_desc->usb_endpoint_desc[j];
            if(usb_endpoint_desc->bEndpointAddress == num_of_endpoint){
                return usb_endpoint_desc;
            }
        }
    }

    usbutil_dbg(USBUTIL_NOT_FOUND, " Endpoint class not found %s %d", __FILE__, __LINE__);
    return NULL;
}

inline __u8 get_interface_num_endpoints(const struct usb_interface_desc* usb_interface_desc){
    return usb_interface_desc->bNumEndpoints;
}
inline __u32 get_endpoint_size(const struct usb_endpoint_desc* usb_endpoint_desc){
    return usb_endpoint_desc->wMaxPacketSize;
}

enum usb_device_speed get_device_speed(struct usb_device* usb_device){
    usbutil_get_device_speed(usb_device);
    return usb_device->device_speed;
}

inline __u32 get_device_capabilities(struct usb_device* usb_device){
    return usb_device->capabilities;
}

