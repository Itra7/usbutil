#ifndef USBCONF_H
#define USBCONF_H

#include <linux/types.h>
#include "usbutilList.h"
#include "utilities.h"

#define SYSPATH /sys/bus/usb/devices
#define DEVICEIO_PATH /dev/bus/usb

#define MAX_LEN_MANUFACTER 64
#define MAX_NUM_OF_INTERFACES 32
#define MAX_NUM_OF_ENDPOINTS 32
#define MAX_NUM_OF_CONFIGURATION 2

struct usb_desc{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDeviceClass;
    __u8 bDeviceSubClass;
    __u8 bDeviceProtocol;
    __u8 bMaxPacketSize0;
    __u8 iManufacturer;
    __u8 iProduct;
    __u8 iSerial;
    __u8 bNumConfigurations;
    char* bcdDevice;

    struct usb_configuration_desc *usb_configuration_desc[MAX_NUM_OF_CONFIGURATION]; 
};

struct usb_configuration_desc{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bNumInterfaces;
    __u8 bConfigurationValue;
    __u8 iConfiguration;
    __u8 bmAttributes;

    struct usb_interface_desc *usb_interface_desc[MAX_NUM_OF_INTERFACES];
};

struct usb_interface_desc{
    __u8 bLength;
    __u8 bDescriptorType;        
    __u8 bInterfaceNumber;       
    __u8 bAlternateSetting;      
    __u8 bNumEndpoints;     
    __u8 bInterfaceClass;         
    __u8 bInterfaceSubClass;      
    __u8 bInterfaceProtocol;    
    __u8 iInterface;     
    struct usb_endpoint_desc *usb_endpoint_desc[MAX_NUM_OF_ENDPOINTS];    
};

struct usb_endpoint_desc{
    __u8    bLength;                        
    __u8    bEndpointAddress;
    __u8    bmAttributes;                     
    __u32   wMaxPacketSize;
    __u8    bInterval;    
    char*   type;
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
    enum usb_device_speed device_speed;
    enum usb_device_state device_state;
    __u32 capabilities;
    int fd;
    char sysfs_path[256*2];    
    char busnum[4];
    char devnum[4];
    char idProduct[6];
    char idVendor[6];
    struct usb_desc* dev;
    struct usb_endpoint_desc* endpoint0;
    struct list_head list;
    struct kref kref; 
};


#endif

