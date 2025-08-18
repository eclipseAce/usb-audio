#ifndef __USBD_CORE_H
#define __USBD_CORE_H

#include "stm32h7xx.h"

#ifndef LOBYTE
#define LOBYTE(x) ((uint8_t)((x) & 0x00FFU))
#endif /* LOBYTE */

#ifndef HIBYTE
#define HIBYTE(x) ((uint8_t)(((x) & 0xFF00U) >> 8U))
#endif /* HIBYTE */

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
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

#define USB_STATE_DEFAULT    0
#define USB_STATE_ADDRESS    1
#define USB_STATE_CONFIGURED 2

#define USB_DEVICE_FEATURE_REMOTE_WAKEUP 1

#define USB_DEVICE_STATUS_REMOTE_WAKEUP_MASK 0x02U

#define USB_DEV_MAX_INTERFACES     2U
#define USB_DEV_MAX_CONFIGURATIONS 1U

typedef struct usb_setup_req {
  uint8_t bmRequestType;
  uint8_t bRequest;
  union {
    uint16_t wValue;
    struct {
      uint8_t DescriptorIndex;
      uint8_t DescriptorType;
    };
  };
  uint16_t wIndex;
  uint16_t wLength;
} USB_SetupReqTypeDef;

typedef struct usb_device_handle {
  uint8_t state;
  uint16_t dev_status;
  uint8_t address;
  uint8_t configuration;
  uint8_t alt_settings[USB_DEV_MAX_INTERFACES];

  uint8_t *buf;
  uint16_t len;
  uint16_t remain_len;
  uint16_t transmit_len;
} USB_DeviceHandleTypeDef;

#endif /* __USBD_CORE_H */