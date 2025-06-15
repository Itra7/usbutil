#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include "usbio.h"
#include "usbconf.h"

__u32 get_max_packet_size(const struct usb_configuration_desc* usb_configuration_desc, 
    const __u32 num_of_endpoint);
struct usb_interface_desc* get_interface_class(const struct usb_configuration_desc* usb_configuration_desc,
        const __u8 num_of_interface);
struct usb_endpoint_desc* get_endpoint_class(const struct usb_configuration_desc* usb_configuration_desc, 
            const __u32 num_of_endpoint);
inline __u8 get_interface_num_endpoints(const struct usb_interface_desc* usb_interface_desc);
inline __u32 get_endpoint_size(const struct usb_endpoint_desc* usb_endpoint_desc);
enum usb_device_speed get_device_speed(struct usb_device* usb_device);
inline __u32 get_device_capabilities(struct usb_device* usb_device);


#endif