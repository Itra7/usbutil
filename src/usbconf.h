#ifndef USBCONF_H
#define USBCONF_H

#include <linux/types.h>

#define SYSPATH /sys/bus/usb/devices
#define DEVICEIO_PATH /dev/bus/usb

#define MAX_LEN_MANUFACTER 64
#define MAX_NUM_OF_INTERFACES 32
#define MAX_NUM_OF_CONFIGURATION 2

struct usb_device{
    struct usb_configuration *conf[MAX_NUM_OF_CONFIGURATION];
    __u8 bNumConfiguration;
};

struct usb_configuration{
    __u16 bcdDevice;
    __u16 ConfigurationValue;
    __u16 bDeviceClass;
    __u16 bDeviceSubClass;
    __u16 bDeviceProtocol;
    __u16 bmAttributes;
    __u32 bMaxPacketSize;
    __u16 bMaxPower;
    __u16 bNumInterfaces;
    __u16 idProduct;
    __u16 idVendor;
    __u8 bNumInterfaces;
    __u8 speed;
    char manufacter[MAX_LEN_MANUFACTER];

    struct usb_interface* interface[MAX_NUM_OF_INTERFACES];
};

struct usb_interface{
    __u16 bAlternateSetting;
    __u16 bInterfaceClass;
    __u16 bInterfaceNumber;
    __u16 bInterfaceProtocol;
    __u16 bInterfaceSubClass;
    __u16 bNumEndpoints;
};





#endif