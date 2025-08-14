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

#define USB_DEV_STATE_DEFAULT    0x00U
#define USB_DEV_STATE_ADDRESS    0x01U
#define USB_DEV_STATE_CONFIGURED 0X02U

typedef struct usb_setup_req {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} USB_SetupReqTypeDef;

typedef struct usb_handle {
  uint8_t state;
  uint8_t dev_addr;
} USB_HandleTypeDef;

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
  USB_SetupReqTypeDef *req = (USB_SetupReqTypeDef *)&hpcd->Setup[0];
  USB_HandleTypeDef *hdev = (USB_HandleTypeDef *)hpcd->pData;
  uint8_t buf[64];
  uint8_t req_type = req->bmRequestType & USB_REQ_TYPE_MASK;
  uint8_t req_recipient = req->bmRequestType & USB_REQ_RECIPIENT_MASK;

  switch (req_type) {
    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest) {
        case USB_REQ_GET_STATUS:
          switch (req_recipient) {
            case USB_REQ_RECIPIENT_DEVICE:
            case USB_REQ_SET_INTERFACE:
            case USB_REQ_RECIPIENT_ENDPOINT:
              *(uint16_t *)buf = 0U;
              HAL_PCD_EP_Transmit(hpcd, 0x00, buf, 2U);
              return;
          }
          break;

        case USB_REQ_CLEAR_FEATURE:
          switch (req_recipient) {
            case USB_REQ_RECIPIENT_DEVICE:
            case USB_REQ_SET_INTERFACE:
            case USB_REQ_RECIPIENT_ENDPOINT:
              HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0U);
              return;
          }
          break;

        case USB_REQ_SET_FEATURE:
          switch (req_recipient) {
            case USB_REQ_RECIPIENT_DEVICE:
            case USB_REQ_SET_INTERFACE:
            case USB_REQ_RECIPIENT_ENDPOINT:
              HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0U);
              return;
          }
          break;

        case USB_REQ_SET_ADDRESS:
          switch (req_recipient) {
            case USB_REQ_RECIPIENT_DEVICE:
              if (req->wValue <= 127 && req->wValue == 0U && req->wLength == 0U) {
                hdev->dev_addr = (uint8_t)req->wValue;
                hdev->state = USB_DEV_STATE_ADDRESS;
                HAL_PCD_SetAddress(hpcd, hdev->dev_addr);
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0U);
                return;
              }
              break;
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          switch (req_recipient) {
            case USB_REQ_RECIPIENT_DEVICE:
              switch (HIBYTE(req->wValue)) {
                case USB_DESC_TYPE_DEVICE:
                  /* send device descriptor */
                  HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, MIN(req->wLength, 0));
                  return;

                case USB_DESC_TYPE_CONFIGURATION:
                  if (LOBYTE(req->wValue) == 1) {
                    /* send configuration 1 descriptor*/
                    HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, MIN(req->wLength, 0));
                    return;
                  }
                  break;

                case USB_DESC_TYPE_STRING:
                  if (LOBYTE(req->wValue) == 0) {
                    /* send lang code descriptor */
                    HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, MIN(req->wLength, 0));
                    return;
                  }
                  if (LOBYTE(req->wValue) <= 3) {
                    /* send string N descriptor */
                    HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, MIN(req->wLength, 0));
                    return;
                  }
                  break;
              }
              break;
          }
          break;

        case USB_REQ_SET_DESCRIPTOR:
          /* not supported */
          break;

        case USB_REQ_GET_CONFIGURATION:
          if (req->wValue == 0U && req->wIndex == 0U && req->wLength == 1U) {
            if (hdev->state == USB_DEV_STATE_CONFIGURED) {
              buf[0] = 1U;
            } else {
              buf[0] = 0U;
            }
            HAL_PCD_EP_Transmit(hpcd, 0x00, &buf[0], 1U);
            return;
          }
          break;

        case USB_REQ_SET_CONFIGURATION:
          if (HIBYTE(req->wValue) == 0U && req->wIndex == 0U && req->wLength == 1U) {
            if (LOBYTE(req->wValue) == 0U) {
              hdev->state = USB_DEV_STATE_ADDRESS;
            } else {
              hdev->state = USB_DEV_STATE_CONFIGURED;
            }
            HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 1U);
            return;
          }
          break;

        case USB_REQ_GET_INTERFACE:
          break;

        case USB_REQ_SET_INTERFACE:
          break;

        case USB_REQ_SYNCH_FRAME:
          break;
      }
      break;

    case USB_REQ_TYPE_CLASS:
      break;

    case USB_REQ_TYPE_VENDOR:
      break;
  }
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