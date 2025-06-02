#ifndef USBCONF_H
#define USBCONF_H

#include <linux/types.h>
#include "usbutilList.h"
#include "utilities.h"

#define SYSPATH /sys/bus/usb/devices
#define DEVICEIO_PATH /dev/bus/usb

#define MAX_LEN_MANUFACTER 64
#define MAX_NUM_OF_INTERFACES 32
#define MAX_NUM_OF_CONFIGURATION 2


#define INIT_DL_LIST(list) init_list(list);

struct usb_device_desc{
    __u8 bNumConfiguration;
};

struct usb_configuration_desc{
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
    __u8 speed;
    char manufacter[MAX_LEN_MANUFACTER];

};

struct usb_interface_desc{
    __u16 bAlternateSetting;
    __u16 bInterfaceClass;
    __u16 bInterfaceNumber;
    __u16 bInterfaceProtocol;
    __u16 bInterfaceSubClass;
    __u16 bNumEndpoints;
};

struct list_of_devices{
    char busnum[4];
    char devnum[4];
    char idProduct[6];
    char idVendor[6];
    struct list_head list;
};

enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,			/* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,		/* usb 1.1 */
	USB_SPEED_HIGH,				/* usb 2.0 */
	USB_SPEED_WIRELESS,			/* wireless (usb 2.5) */
	USB_SPEED_SUPER,			/* usb 3.0 */
	USB_SPEED_SUPER_PLUS,			/* usb 3.1 */
};


enum usb_device_state {
	/* NOTATTACHED isn't in the USB spec, and this state acts
	 * the same as ATTACHED ... but it's clearer this way.
	 */
	USB_STATE_NOTATTACHED = 0,

	/* chapter 9 and authentication (wireless) device states */
	USB_STATE_ATTACHED,
	USB_STATE_POWERED,			/* wired */
	USB_STATE_RECONNECTING,			/* auth */
	USB_STATE_UNAUTHENTICATED,		/* auth */
	USB_STATE_DEFAULT,			/* limited function */
	USB_STATE_ADDRESS,
	USB_STATE_CONFIGURED,			/* most functions */

	USB_STATE_SUSPENDED
};


struct usb_device{

    __u8 device_speed;
    __u8 device_state;
    __u32 capabilities;
    int fd;

    struct kref kref; 
};

#endif

