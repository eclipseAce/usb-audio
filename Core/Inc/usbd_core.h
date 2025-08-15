#ifndef __USBD_CORE_H
#define __USBD_CORE_H

#include "stm32h7xx.h"

#ifndef LOBYTE
#define LOBYTE(x)  ((uint8_t)((x) & 0x00FFU))
#endif /* LOBYTE */

#ifndef HIBYTE
#define HIBYTE(x)  ((uint8_t)(((x) & 0xFF00U) >> 8U))
#endif /* HIBYTE */

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#define USB_REQ_TYPE_STANDARD 0x00U
#define USB_REQ_TYPE_CLASS    0x20U
#define USB_REQ_TYPE_VENDOR   0x40U
#define USB_REQ_TYPE_MASK     0x60U

#define USB_REQ_RECIPIENT_DEVICE    0x00U
#define USB_REQ_RECIPIENT_INTERFACE 0x01U
#define USB_REQ_RECIPIENT_ENDPOINT  0x02U
#define USB_REQ_RECIPIENT_MASK      0x1FU

#define USB_REQ_GET_STATUS        0x00U
#define USB_REQ_CLEAR_FEATURE     0x01U
#define USB_REQ_SET_FEATURE       0x03U
#define USB_REQ_SET_ADDRESS       0x05U
#define USB_REQ_GET_DESCRIPTOR    0x06U
#define USB_REQ_SET_DESCRIPTOR    0x07U
#define USB_REQ_GET_CONFIGURATION 0x08U
#define USB_REQ_SET_CONFIGURATION 0x09U
#define USB_REQ_GET_INTERFACE     0x0AU
#define USB_REQ_SET_INTERFACE     0x0BU
#define USB_REQ_SYNCH_FRAME       0x0CU

#define USB_DESC_TYPE_DEVICE        0x01U
#define USB_DESC_TYPE_CONFIGURATION 0x02U
#define USB_DESC_TYPE_STRING        0x03U
#define USB_DESC_TYPE_INTERFACE     0x04U
#define USB_DESC_TYPE_ENDPOINT      0x05U

#define USB_DEV_MAX_INTERFACES   2U

typedef struct usb_setup_req {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} USB_SetupReqTypeDef;

typedef struct usb_device_handle {
  uint8_t configuration;
  uint8_t alt_settings[USB_DEV_MAX_INTERFACES];
} USB_DeviceHandleTypeDef;

#endif /* __USBD_CORE_H */