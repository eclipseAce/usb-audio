#include "usbd_core.h"

#include "stm32h7xx.h"
#include "usbd_desc.h"

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
  USB_SetupReqTypeDef *req = (USB_SetupReqTypeDef *)&hpcd->Setup[0];
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
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
              if (req->wValue <= 127 && req->wIndex == 0U && req->wLength == 0U) {
                hdev->address = (uint8_t)req->wValue;
                HAL_PCD_SetAddress(hpcd, hdev->address);
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
                  HAL_PCD_EP_Transmit(hpcd, 0x00, DeviceDescriptor, MIN(req->wLength, sizeof(DeviceDescriptor)));
                  return;

                case USB_DESC_TYPE_CONFIGURATION:
                  if (LOBYTE(req->wValue) == 1) {
                    /* send configuration 1 descriptor*/
                    HAL_PCD_EP_Transmit(hpcd, 0x00, ConfigDescriptor, MIN(req->wLength, sizeof(ConfigDescriptor)));
                    return;
                  }
                  break;

                case USB_DESC_TYPE_STRING:
                  if (LOBYTE(req->wValue) < USB_MAX_STRING_DESCRIPTORS) {
                    HAL_PCD_EP_Transmit(hpcd, 0x00, StringDescriptorPtr[LOBYTE(req->wValue)], MIN(req->wLength, StringDescriptorLen[LOBYTE(req->wValue)]));
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
            HAL_PCD_EP_Transmit(hpcd, 0x00, &hdev->configuration, 1U);
            return;
          }
          break;

        case USB_REQ_SET_CONFIGURATION:
          if (HIBYTE(req->wValue) == 0U && req->wIndex == 0U && req->wLength == 1U) {
            hdev->configuration = LOBYTE(req->wValue);
            HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0U);
            return;
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (req->wValue == 0U && req->wLength == 1U) {
            if (req->wIndex < USB_DEV_MAX_INTERFACES) {
              HAL_PCD_EP_Transmit(hpcd, 0x00, &hdev->alt_settings[req->wIndex], 1U);
              return;
            }
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (req->wLength == 0U) {
            if (req->wIndex < USB_DEV_MAX_INTERFACES) {
              hdev->alt_settings[req->wIndex] = (uint8_t)req->wValue;
              HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0U);
              return;
            }
          }
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
  HAL_PCD_EP_SetStall(hpcd, 0x00);
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