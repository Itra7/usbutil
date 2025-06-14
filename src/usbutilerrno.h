#ifndef USBUTIL_ERRNO_H
#define USBUTIL_ERRNO_H

#include <stdio.h>
#include <stdarg.h>
#define USBDEBUG 1

#define USBUTIL_OUTPUT 0x0
#define USBUTIL_EOPEN 0x10
#define USBUTIL_MALLOC_FAIL 0x11
#define USBUTIL_GETKERNEL 0x12
#define USBUTIL_KERNELVERSION 0x13
#define USBUTIL_NOT_FOUND 0x14
#define USBUTIL_IOCTL_FAIL 0x15
#define USBUTIL_NOT_SUPPORTED 0x16
#define USBUTIL_OTHER 0x20


void usbutil_dbg(int ERROR_CODE, const char *format, ...);
void usbutil_printf(const char* format, ...);

static const char *error_msg[] ={
    [USBUTIL_OUTPUT] = "",   // for only output
    [USBUTIL_EOPEN] = "Failed to open file",
    [USBUTIL_MALLOC_FAIL] = "Failed to allocate memory",
    [USBUTIL_GETKERNEL] = "Failed to get kernel information",
    [USBUTIL_KERNELVERSION] = "Failed to get kernel version",
    [USBUTIL_NOT_FOUND] = "Not found",
    [USBUTIL_IOCTL_FAIL] = "Ioctl fail",
    [USBUTIL_NOT_SUPPORTED] = "Device doesn't support this operation",
    [USBUTIL_OTHER] = ""

};

#endif