#ifndef USBUTIL_H
#define USBUTIL_H

#include "usbconf.h"
#include "utilities.h"
#include "usbio.h"

#define usb_interface_desc_MEMBERS_LEN 12

typedef enum {
    TYPE_U8,
    TYPE_U32,
    TYPE_STRING
} field_type;

typedef struct {
    const char *name;
    size_t offset;
    field_type type;
} field_info;

static const field_info _usb_desc[] = {
    {"bLength", offsetof(struct usb_desc, bLength), TYPE_U8},
    {"bDescriptorType", offsetof(struct usb_desc, bDescriptorType), TYPE_U8},
    {"bDeviceClass", offsetof(struct usb_desc, bDeviceClass), TYPE_U8},
    {"bDeviceSubClass", offsetof(struct usb_desc, bDeviceSubClass), TYPE_U8},
    {"bDeviceProtocol", offsetof(struct usb_desc, bDeviceProtocol), TYPE_U8},
    {"bMaxPacketSize0", offsetof(struct usb_desc, bMaxPacketSize0), TYPE_U8},
    {"bcdDevice", offsetof(struct usb_desc, bcdDevice), TYPE_STRING},
    {"iManufacturer", offsetof(struct usb_desc, iManufacturer), TYPE_U8},
    {"iProduct", offsetof(struct usb_desc, iProduct), TYPE_U8},
    {"iSerial", offsetof(struct usb_desc, iSerial), TYPE_U8},
    {"bNumConfigurations", offsetof(struct usb_desc, bNumConfigurations), TYPE_U8}
};

static const field_info _usb_cofiguration_desc[] = {
    {"bLength", offsetof(struct usb_configuration_desc, bLength), TYPE_U8},
    {"bDescriptorType", offsetof(struct usb_configuration_desc, bDescriptorType), TYPE_U8},
    {"bNumInterfaces", offsetof(struct usb_configuration_desc, bNumInterfaces), TYPE_U8},
    {"bConfigurationValue", offsetof(struct usb_configuration_desc, bConfigurationValue), TYPE_U8},
    {"iConfiguration", offsetof(struct usb_configuration_desc, iConfiguration), TYPE_U8},
    {"bmAttributes", offsetof(struct usb_configuration_desc, bmAttributes), TYPE_U8},
};

static const field_info _usb_interface_desc[] = {
    {"bLength", offsetof(struct usb_interface_desc, bLength), TYPE_U8},
    {"bDescriptorType", offsetof(struct usb_interface_desc, bDescriptorType), TYPE_U8},
    {"bInterfaceNumber", offsetof(struct usb_interface_desc, bInterfaceNumber), TYPE_U8},
    {"bAlternateSetting", offsetof(struct usb_interface_desc, bAlternateSetting), TYPE_U8},
    {"bNumEndpoints", offsetof(struct usb_interface_desc, bNumEndpoints), TYPE_U8},
    {"bInterfaceClass", offsetof(struct usb_interface_desc, bInterfaceClass), TYPE_U8},
    {"bInterfaceSubClass", offsetof(struct usb_interface_desc, bInterfaceSubClass), TYPE_U8},
    {"bInterfaceProtocol", offsetof(struct usb_interface_desc, bInterfaceProtocol), TYPE_U8},
    {"iInterface", offsetof(struct usb_interface_desc, iInterface), TYPE_U8},
};

static const field_info _usb_endpoint_desc[] = {
    {"bLength", offsetof(struct usb_endpoint_desc, bLength), TYPE_U8},
    {"type", offsetof(struct usb_endpoint_desc, type), TYPE_STRING},
    {"bEndpointAddress", offsetof(struct usb_endpoint_desc, bEndpointAddress), TYPE_U32},
    {"bmAttributes", offsetof(struct usb_endpoint_desc, bmAttributes), TYPE_U8},
    {"wMaxPacketSize", offsetof(struct usb_endpoint_desc, wMaxPacketSize), TYPE_U32},
    {"bInterval", offsetof(struct usb_endpoint_desc, bInterval), TYPE_U8},
};

void free_usb_info(struct usb_desc** dev, struct usb_endpoint_desc*);
void free_usb_list(struct usb_device *devs);
void print_usb_list(struct usb_device *devs);

struct usb_desc* read_usb_device(const char* path, struct usb_endpoint_desc**);
int sysfs_fd_open(const char* device, const char* file_name, const int flags);
int usbutil_list_devices(struct usb_device**);
int usbutil_extract_from_file(char *paths, char *d_name, struct usb_device** usb_device, int flag);
int usbutil_init();

int usbutil_open(const char* idProduct, const char* idVendor, struct usb_device* devs);
struct usb_device* find_proper_device(const char* idProduct, const char* idVendor, struct usb_device *devs);

void usbutil_get_device_speed(struct usb_device* usb_device);
int set_urb(struct usb_device* usb_device, unsigned char type, 
    unsigned char endpoint, void* buffer, int length);
int process_urb(struct usb_device* usb_device);
int claim_interface(struct usb_device* usb_device, int interface);
int release_interface(struct usb_device* usb_device, int interface);
int detach_kernel_driver_and_claim(struct usb_device* usb_device, int interface);
int attach_kernel_driver(struct usb_device* usb_device, int interface);
int detach_kernel_driver(struct usb_device* usb_device, int interface);
int reap_urb(struct usb_device* usb_device, struct usbdevfs_urb *reap);
int free_stream(struct usb_device* usb_device, struct usbdevfs_streams* stream);
int alloc_stream(struct usb_device* usb_device, struct usbdevfs_streams* stream);
int drop_privilegies(struct usb_device* usb_device, int capability);
int get_capabilites(struct usb_device* usb_device);
int clear_halt(struct usb_device* usb_device, int endpoint);
void usbutil_close(struct usb_device* head, struct usb_device* target_device);


#endif