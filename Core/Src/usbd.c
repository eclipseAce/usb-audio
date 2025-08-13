#include "stm32h7xx.h"

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

typedef struct usb_setup_req {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} USB_SetupReqTypedef;

__ALIGN_BEGIN uint8_t USB_DeviceDesc[] __ALIGN_END = {
    /* Device Descriptor */
    0x12,       /*bLength */
    0x01,       /*bDescriptorType*/
    0x00, 0x02, /*bcdUSB */
    0xEF,       /*bDeviceClass*/
    0x02,       /*bDeviceSubClass*/
    0x01,       /*bDeviceProtocol*/
    0x40,       /*bMaxPacketSize*/
    0x83, 0x04, /*idVendor*/
    0x40, 0x57, /*idProduct*/
    0x00, 0x01, /*bcdDevice rel. 2.00*/
    0x01,       /*Index of manufacturer  string*/
    0x02,       /*Index of product string*/
    0x03,       /*Index of serial number string*/
    0x01        /*bNumConfigurations*/
};

__ALIGN_BEGIN uint8_t USB_StrDesc_0[] __ALIGN_END = {
    /* String Descriptor 0 */
    0x04,       /* bLength: Length of descriptor in bytes */
    0x03,       /* bDescriptorType: STRING descriptor type */
    0x09, 0x04, /* wLANGID: Language ID */
};

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
  USBD_SetupReqTypedef *setup = (USBD_SetupReqTypedef *)&hpcd->Setup[0];

  switch (setup->bmRequestType & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_STANDARD:
      switch (setup->bmRequestType & USB_REQ_RECIPIENT_MASK) {
        case USB_REQ_RECIPIENT_DEVICE:
          switch (setup->bRequest) {
            case USB_REQ_GET_STATUS:
              break;
            case USB_REQ_CLEAR_FEATURE:
              break;
            case USB_REQ_SET_FEATURE:
              break;
            case USB_REQ_SET_ADDRESS:
              break;
            case USB_REQ_GET_DESCRIPTOR:
              break;
            case USB_REQ_SET_DESCRIPTOR:
              break;
            case USB_REQ_GET_CONFIGURATION:
              break;
            case USB_REQ_SET_CONFIGURATION:
              break;
            case USB_REQ_GET_INTERFACE:
              break;
            case USB_REQ_SET_INTERFACE:
              break;
            case USB_REQ_SYNCH_FRAME:
              break;
          }
          break;

        case USB_REQ_RECIPIENT_INTERFACE:
          break;

        case USB_REQ_RECIPIENT_ENDPOINT:
          break;
      }
      break;

    case USB_REQ_TYPE_CLASS:
      break;

    case USB_REQ_TYPE_VENDOR:
      break;
  }
  return;
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  return;
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  return;
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  return;
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  return;
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
  return;
}