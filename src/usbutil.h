#ifndef USBUTIL_H
#define USBUTIL_H


int usbutil_extract_from_file(char *, char*, struct list_of_devices***, int);
int usbutil_list_devices(struct list_of_devices**);

int usbutil_init();

int usbutil_open(const char* idProduct, const char* idVendor, const struct list_of_devices* devs);


#endif