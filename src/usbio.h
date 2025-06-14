#ifndef USBIO_H
#define USBIO_H

#include <linux/types.h>
#include <linux/magic.h>

#include "usbconf.h"

/* --------------------------------------------------------------------- */

/* usbdevfs ioctl codes */

struct usbdevfs_ctrltransfer {
	__u8 bRequestType;
	__u8 bRequest;
	__u16 wValue;
	__u16 wIndex;
	__u16 wLength;
	__u32 timeout;  /* in milliseconds */
 	void *data;
};

struct usbdevfs_bulktransfer {
	unsigned int ep;
	unsigned int len;
	unsigned int timeout; /* in milliseconds */
	void *data;
};

struct usbdevfs_setinterface {
	unsigned int interface;
	unsigned int altsetting;
};

struct usbdevfs_disconnectsignal {
	unsigned int signr;
	void *context;
};

#define USBDEVFS_MAXDRIVERNAME 255

struct usbdevfs_getdriver {
	unsigned int interface;
	char driver[USBDEVFS_MAXDRIVERNAME + 1];
};

struct usbdevfs_connectinfo {
	unsigned int devnum;
	unsigned char slow;
};

struct usbdevfs_conninfo_ex {
	__u32 size;		/* Size of the structure from the kernel's */
				/* point of view. Can be used by userspace */
				/* to determine how much data can be       */
				/* used/trusted.                           */
	__u32 busnum;           /* USB bus number, as enumerated by the    */
				/* kernel, the device is connected to.     */
	__u32 devnum;           /* Device address on the bus.              */
	__u32 speed;		/* USB_SPEED_* constants from ch9.h        */
	__u8 num_ports;		/* Number of ports the device is connected */
				/* to on the way to the root hub. It may   */
				/* be bigger than size of 'ports' array so */
				/* userspace can detect overflows.         */
	__u8 ports[7];		/* List of ports on the way from the root  */
				/* hub to the device. Current limit in     */
				/* USB specification is 7 tiers (root hub, */
				/* 5 intermediate hubs, device), which     */
				/* gives at most 6 port entries.           */
};

#define USBUTIL_USBDEVFS_URB_SHORT_NOT_OK	0x01
#define USBUTIL_USBDEVFS_URB_ISO_ASAP		0x02
#define USBUTIL_USBDEVFS_URB_BULK_CONTINUATION	0x04
#define USBUTIL_USBDEVFS_URB_NO_FSBR		0x20	/* Not used */
#define USBUTIL_USBDEVFS_URB_ZERO_PACKET	0x40
#define USBUTIL_USBDEVFS_URB_NO_INTERRUPT	0x80

#define USBUTIL_USBDEVFS_URB_TYPE_ISO		   0
#define USBUTIL_USBDEVFS_URB_TYPE_INTERRUPT	   1
#define USBUTIL_USBDEVFS_URB_TYPE_CONTROL	   2
#define USBUTIL_USBDEVFS_URB_TYPE_BULK		   3

struct usbdevfs_iso_packet_desc {
	unsigned int length;
	unsigned int actual_length;
	unsigned int status;
};

struct usbdevfs_urb {
	unsigned char type;
	unsigned char endpoint;
	int status;
	unsigned int flags;
	void *buffer;
	int buffer_length;
	int actual_length;
	int start_frame;
	union {
		int number_of_packets;	/* Only used for isoc urbs */
		unsigned int stream_id;	/* Only used with bulk streams */
	};
	int error_count;
	unsigned int signr;	/* signal to be sent on completion,
				  or 0 if none should be sent. */
	void *usercontext;
	struct usbdevfs_iso_packet_desc iso_frame_desc[];
};

/* ioctls for talking directly to drivers */
struct usbdevfs_ioctl {
	int	ifno;		/* interface 0..N ; negative numbers reserved */
	int	ioctl_code;	/* MUST encode size + direction of data so the
				 * macros in <asm/ioctl.h> give correct values */
	void *data;	/* param buffer (in, or out) */
};

/* You can do most things with hubs just through control messages,
 * except find out what device connects to what port. */
struct usbdevfs_hub_portinfo {
	char nports;		/* number of downstream ports in this hub */
	char port [127];	/* e.g. port 3 connects to device 27 */
};

/* System and bus capability flags */
#define USBUTIL_USBDEVFS_CAP_ZERO_PACKET		0x01
#define USBUTIL_USBDEVFS_CAP_BULK_CONTINUATION		0x02
#define USBUTIL_USBDEVFS_CAP_NO_PACKET_SIZE_LIM		0x04
#define USBUTIL_USBDEVFS_CAP_BULK_SCATTER_GATHER	0x08
#define USBUTIL_USBDEVFS_CAP_REAP_AFTER_DISCONNECT	0x10
#define USBUTIL_USBDEVFS_CAP_MMAP			0x20
#define USBUTIL_USBDEVFS_CAP_DROP_PRIVILEGES		0x40
#define USBUTIL_USBDEVFS_CAP_CONNINFO_EX		0x80
#define USBUTIL_USBDEVFS_CAP_SUSPEND			0x100


/*
 * USB directions
 *
 * This bit flag is used in endpoint descriptors' bEndpointAddress field.
 * It's also one of three fields in control requests bRequestType.
 */
#define USB_DIR_OUT			0		/* to device */
#define USB_DIR_IN			0x80		/* to host */

/*
 * Endpoints
 */
#define USB_ENDPOINT_NUMBER_MASK	0x0f	/* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_XFERTYPE_MASK	0x03	/* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3
#define USB_ENDPOINT_MAX_ADJUSTABLE	0x80


/*----------------------------*/

/**
 * usb_endpoint_num - get the endpoint's number
 * @epd: endpoint to be checked
 *
 * Returns @epd's number: 0 to 15.
 */
static __inline__ int usb_endpoint_num(const struct usb_endpoint_desc *epd){
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}

/**
 * usb_endpoint_type - get the endpoint's transfer type
 * @epd: endpoint to be checked
 *
 * Returns one of USB_ENDPOINT_XFER_{CONTROL, ISOC, BULK, INT} according
 * to @epd's transfer type.
 */
static __inline__ int usb_endpoint_type(const struct usb_endpoint_desc *epd){
	return epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
}

/**
 * usb_endpoint_dir_in - check if the endpoint has IN direction
 * @epd: endpoint to be checked
 *
 * Returns true if the endpoint is of type IN, otherwise it returns false.
 */
static __inline__ int usb_endpoint_dir_in(const struct usb_endpoint_desc *epd){
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

/**
 * usb_endpoint_dir_out - check if the endpoint has OUT direction
 * @epd: endpoint to be checked
 *
 * Returns true if the endpoint is of type OUT, otherwise it returns false.
 */
static __inline__ int usb_endpoint_dir_out(
				const struct usb_endpoint_desc *epd){
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

/**
 * usb_endpoint_xfer_bulk - check if the endpoint has bulk transfer type
 * @epd: endpoint to be checked
 *
 * Returns true if the endpoint is of type bulk, otherwise it returns false.
 */
static __inline__ int usb_endpoint_xfer_bulk(
				const struct usb_endpoint_desc *epd){
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK);
}

/**
 * usb_endpoint_xfer_control - check if the endpoint has control transfer type
 * @epd: endpoint to be checked
 *
 * Returns true if the endpoint is of type control, otherwise it returns false.
 */
static __inline__ int usb_endpoint_xfer_control(
				const struct usb_endpoint_desc *epd){
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_CONTROL);
}

/**
 * usb_endpoint_xfer_int - check if the endpoint has interrupt transfer type
 * @epd: endpoint to be checked
 *
 * Returns true if the endpoint is of type interrupt, otherwise it returns
 * false.
 */
static __inline__ int usb_endpoint_xfer_int(
				const struct usb_endpoint_desc *epd){
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT);
}

/**
 * usb_endpoint_xfer_isoc - check if the endpoint has isochronous transfer type
 * @epd: endpoint to be checked
 *
 * Returns true if the endpoint is of type isochronous, otherwise it returns
 * false.
 */
static __inline__ int usb_endpoint_xfer_isoc(
				const struct usb_endpoint_desc *epd){
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC);
}

/* USBDEVFS_DISCONNECT_CLAIM flags & struct */

/* disconnect-and-claim if the driver matches the driver field */
#define USBUTIL_USBDEVFS_DISCONNECT_CLAIM_IF_DRIVER	0x01
/* disconnect-and-claim except when the driver matches the driver field */
#define USBUTIL_USBDEVFS_DISCONNECT_CLAIM_EXCEPT_DRIVER	0x02

struct usbdevfs_disconnect_claim {
	unsigned int interface;
	unsigned int flags;
	char driver[USBDEVFS_MAXDRIVERNAME + 1];
};

struct usbdevfs_streams {
	unsigned int num_streams; /* Not used by USBDEVFS_FREE_STREAMS */
	unsigned int num_eps;
	unsigned char eps[];
};

/*
 * USB_SPEED_* values returned by USBDEVFS_GET_SPEED are defined in
 * linux/usb/ch9.h
 */

#define USBUTIL_USBDEVFS_CONTROL           _IOWR('U', 0, struct usbdevfs_ctrltransfer)
#define USBUTIL_USBDEVFS_CONTROL32           _IOWR('U', 0, struct usbdevfs_ctrltransfer32)
#define USBUTIL_USBDEVFS_BULK              _IOWR('U', 2, struct usbdevfs_bulktransfer)
#define USBUTIL_USBDEVFS_BULK32              _IOWR('U', 2, struct usbdevfs_bulktransfer32)
#define USBUTIL_USBDEVFS_RESETEP           _IOR('U', 3, unsigned int)
#define USBUTIL_USBDEVFS_SETINTERFACE      _IOR('U', 4, struct usbdevfs_setinterface)
#define USBUTIL_USBDEVFS_SETCONFIGURATION  _IOR('U', 5, unsigned int)
#define USBUTIL_USBDEVFS_GETDRIVER         _IOW('U', 8, struct usbdevfs_getdriver)
#define USBUTIL_USBDEVFS_SUBMITURB         _IOR('U', 10, struct usbdevfs_urb)
#define USBUTIL_USBDEVFS_SUBMITURB32       _IOR('U', 10, struct usbdevfs_urb32)
#define USBUTIL_USBDEVFS_DISCARDURB        _IO('U', 11)
#define USBUTIL_USBDEVFS_REAPURB           _IOW('U', 12, void *)
#define USBUTIL_USBDEVFS_REAPURB32         _IOW('U', 12, __u32)
#define USBUTIL_USBDEVFS_REAPURBNDELAY     _IOW('U', 13, void *)
#define USBUTIL_USBDEVFS_REAPURBNDELAY32   _IOW('U', 13, __u32)
#define USBUTIL_USBDEVFS_DISCSIGNAL        _IOR('U', 14, struct usbdevfs_disconnectsignal)
#define USBUTIL_USBDEVFS_DISCSIGNAL32      _IOR('U', 14, struct usbdevfs_disconnectsignal32)
#define USBUTIL_USBDEVFS_CLAIMINTERFACE    _IOR('U', 15, unsigned int)
#define USBUTIL_USBDEVFS_RELEASEINTERFACE  _IOR('U', 16, unsigned int)
#define USBUTIL_USBDEVFS_CONNECTINFO       _IOW('U', 17, struct usbdevfs_connectinfo)
#define USBUTIL_USBDEVFS_IOCTL             _IOWR('U', 18, struct usbdevfs_ioctl)
#define USBUTIL_USBDEVFS_IOCTL32           _IOWR('U', 18, struct usbdevfs_ioctl32)
#define USBUTIL_USBDEVFS_HUB_PORTINFO      _IOR('U', 19, struct usbdevfs_hub_portinfo)
#define USBUTIL_USBDEVFS_RESET             _IO('U', 20)
#define USBUTIL_USBDEVFS_CLEAR_HALT        _IOR('U', 21, unsigned int)
#define USBUTIL_USBDEVFS_DISCONNECT        _IO('U', 22)
#define USBUTIL_USBDEVFS_CONNECT           _IO('U', 23)
#define USBUTIL_USBDEVFS_CLAIM_PORT        _IOR('U', 24, unsigned int)
#define USBUTIL_USBDEVFS_RELEASE_PORT      _IOR('U', 25, unsigned int)
#define USBUTIL_USBDEVFS_GET_CAPABILITIES  _IOR('U', 26, __u32)
#define USBUTIL_USBDEVFS_DISCONNECT_CLAIM  _IOR('U', 27, struct usbdevfs_disconnect_claim)
#define USBUTIL_USBDEVFS_ALLOC_STREAMS     _IOR('U', 28, struct usbdevfs_streams)
#define USBUTIL_USBDEVFS_FREE_STREAMS      _IOR('U', 29, struct usbdevfs_streams)
#define USBUTIL_USBDEVFS_DROP_PRIVILEGES   _IOW('U', 30, __u32)
#define USBUTIL_USBDEVFS_GET_SPEED         _IO('U', 31)
/*
 * Returns struct usbdevfs_conninfo_ex; length is variable to allow
 * extending size of the data returned.
 */
#define USBUTIL_USBDEVFS_CONNINFO_EX(len)  _IOC(_IOC_READ, 'U', 32, len)
#define USBUTIL_USBDEVFS_FORBID_SUSPEND    _IO('U', 33)
#define USBUTIL_USBDEVFS_ALLOW_SUSPEND     _IO('U', 34)
#define USBUTIL_USBDEVFS_WAIT_FOR_RESUME   _IO('U', 35)

#endif 
