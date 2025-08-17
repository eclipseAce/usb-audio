#include "usbd_core.h"

#include <string.h>

#include "stm32h7xx.h"
#include "usbd_desc.h"

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
  USB_SetupReqTypeDef *req = (USB_SetupReqTypeDef *)&hpcd->Setup[0];
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  uint8_t buf[64];

  switch (req->bmRequestType & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_STANDARD:
      switch (req->bmRequestType & USB_REQ_RECIPIENT_MASK) {
        case USB_REQ_RECIPIENT_DEVICE:
          switch (req->bRequest) {
            case USB_REQ_CLEAR_FEATURE:
              break;

            case USB_REQ_GET_CONFIGURATION:
              break;

            case USB_REQ_GET_DESCRIPTOR:
              switch (req->DescriptorType) {
                case USB_DESC_TYPE_DEVICE:
                  hdev->buf = DeviceDescriptor;
                  hdev->total_len = sizeof(DeviceDescriptor);
                  hdev->transmit_len = MIN(hdev->total_len, hpcd->IN_ep[0].maxpacket);
                  HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->buf, hdev->transmit_len);
                  break;

                case USB_DESC_TYPE_CONFIGURATION:
                  hdev->buf = ConfigDescriptor;
                  hdev->total_len = sizeof(ConfigDescriptor);
                  hdev->transmit_len = MIN(hdev->total_len, hpcd->IN_ep[0].maxpacket);
                  HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->buf, hdev->transmit_len);
                  break;

                case USB_DESC_TYPE_STRING:
                  hdev->buf = StringDescriptorPtr[req->DescriptorIndex];
                  hdev->total_len = StringDescriptorLen[req->DescriptorIndex];
                  hdev->transmit_len = MIN(hdev->total_len, hpcd->IN_ep[0].maxpacket);
                  HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->buf, hdev->transmit_len);
                  break;
              }
              break;

            case USB_REQ_GET_STATUS:
              break;

            case USB_REQ_SET_ADDRESS:
              HAL_PCD_SetAddress(hpcd, (uint8_t)(req->wValue & 0x7F));
              HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
              break;

            case USB_REQ_SET_CONFIGURATION:
              break;

            case USB_REQ_SET_DESCRIPTOR:
              break;

            case USB_REQ_SET_FEATURE:
              break;
          }
          break;

        case USB_REQ_RECIPIENT_INTERFACE:
          switch (req->bRequest) {
            case USB_REQ_CLEAR_FEATURE:
              break;

            case USB_REQ_GET_INTERFACE:
              break;

            case USB_REQ_GET_STATUS:
              break;

            case USB_REQ_SET_FEATURE:
              break;

            case USB_REQ_SET_INTERFACE:
              break;
          }
          break;

        case USB_REQ_RECIPIENT_ENDPOINT:
          switch (req->bRequest) {
            case USB_REQ_CLEAR_FEATURE:
              break;

            case USB_REQ_GET_STATUS:
              break;

            case USB_REQ_SET_FEATURE:
              break;

            case USB_REQ_SYNCH_FRAME:
              break;
          }
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
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  if ((epnum & 0x0FU) == 0x00) {
    if (hdev->buf != NULL) {
      hdev->total_len -= hdev->transmit_len;
      if (hdev->total_len > 0) {
        hdev->buf += hdev->transmit_len;
        hdev->transmit_len = MIN(hdev->total_len, hpcd->IN_ep[0].maxpacket);
        HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->buf, hdev->transmit_len);
        HAL_PCD_EP_Receive(hpcd, 0x00, NULL, 0U);
      } else {
        if (hdev->transmit_len == hpcd->IN_ep[0].maxpacket) {
          HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
          HAL_PCD_EP_Receive(hpcd, 0x00, NULL, 0U);
        }
        hdev->buf = NULL;
        hdev->transmit_len = 0U;
      }
    }
  }
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  hdev->configuration = 0U;
  memset(hdev->alt_settings, 0, sizeof(hdev->alt_settings));
  hdev->buf = NULL;
  hdev->total_len = 0U;
  hdev->transmit_len = 0U;

  HAL_PCD_EP_Open(hpcd, 0x00, 8, EP_TYPE_CTRL);
  HAL_PCD_EP_Open(hpcd, 0x80, 8, EP_TYPE_CTRL);
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