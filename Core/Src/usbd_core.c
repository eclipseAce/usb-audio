#include "usbd_core.h"

#include <stdio.h>
#include <string.h>

#include "stm32h7xx.h"
#include "usbd_desc.h"

static void USB_EP0_Transmit(PCD_HandleTypeDef *hpcd, uint8_t *buf, uint16_t len) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  hdev->buf = buf;
  hdev->len = len;
  hdev->remain_len = len;
  hdev->transmit_len = MIN(hdev->remain_len, hpcd->IN_ep[0].maxpacket);
  HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->buf, hdev->transmit_len);
}

static void USB_EP0_SetStall(PCD_HandleTypeDef *hpcd) {
  HAL_PCD_EP_SetStall(hpcd, 0x00);
  HAL_PCD_EP_SetStall(hpcd, 0x80);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
  USB_SetupReqTypeDef *req = (USB_SetupReqTypeDef *)&hpcd->Setup[0];
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  uint8_t temp[64];

  switch (req->bmRequestType & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_STANDARD:
      switch (req->bmRequestType & USB_REQ_RECIPIENT_MASK) {
        case USB_REQ_RECIPIENT_DEVICE:
          switch (req->bRequest) {
            case USB_REQ_CLEAR_FEATURE:
              if (hdev->state != USB_STATE_DEFAULT                   /* not in default mode */
                  && req->wLength == 0                               /* wLength == 0 */
                  && req->wIndex == 0                                /* wIndex == 0 */
                  && req->wValue == USB_FEATURE_DEVICE_REMOTE_WAKEUP /* wValue == DEVICE_REMOTE_WAKEUP */
              ) {
                hdev->device_status &= (~USB_DEVICE_STATUS_REMOTE_WAKEUP_MASK);
                HAL_PCD_DeActivateRemoteWakeup(hpcd);
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
                return;
              }
              break;

            case USB_REQ_GET_CONFIGURATION:
              if (hdev->state != USB_STATE_DEFAULT /* not in default mode */
                  && req->wLength == 1U            /* wLength == 1 */
                  && req->wIndex == 0              /* wIndex == 0 */
                  && req->wValue == 0              /* wValue == 0 */
              ) {
                USB_EP0_Transmit(hpcd, &hdev->configuration, 1U);
                return;
              }
              return;

            case USB_REQ_GET_DESCRIPTOR:
              switch (req->DescriptorType) {
                case USB_DESC_TYPE_DEVICE:
                  USB_EP0_Transmit(hpcd, DeviceDescriptor, MIN(req->wLength, sizeof(DeviceDescriptor)));
                  return;

                case USB_DESC_TYPE_CONFIGURATION:
                  USB_EP0_Transmit(hpcd, ConfigDescriptor, MIN(req->wLength, sizeof(ConfigDescriptor)));
                  return;

                case USB_DESC_TYPE_STRING:
                  USB_EP0_Transmit(hpcd, StringDescriptorPtr[req->DescriptorIndex], MIN(req->wLength, StringDescriptorLen[req->DescriptorIndex]));
                  return;
              }
              break;

            case USB_REQ_GET_STATUS:
              if (hdev->state != USB_STATE_DEFAULT /* not in default mode */
                  && req->wLength == 2U            /* wLength == 2 */
                  && req->wIndex == 0              /* wIndex == 0 */
                  && req->wValue == 0              /* wValue == 0 */
              ) {
                USB_EP0_Transmit(hpcd, &hdev->device_status, 2U);
                return;
              }
              break;

            case USB_REQ_SET_ADDRESS:
              if (hdev->state != USB_STATE_CONFIGURED /* device is not in configured mode */
                  && req->wLength == 0                /* wLength == 0 */
                  && req->wIndex == 0                 /* wIndex == 0 */
                  && req->wValue <= 127               /* wValue <= 127 */
              ) {
                hdev->address = (uint8_t)req->wValue;
                hdev->state = (hdev->address == 0) ? USB_STATE_DEFAULT : USB_STATE_ADDRESS;
                HAL_PCD_SetAddress(hpcd, hdev->address);
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
                return;
              }
              return;

            case USB_REQ_SET_CONFIGURATION:
              if (hdev->state != USB_STATE_DEFAULT                      /* not in default mode */
                  && req->wLength == 0                                  /* wLength == 0 */
                  && req->wIndex == 0                                   /* wIndex == 0 */
                  && req->DescriptorType == 0                           /* LOBYTE(wValue) == 0 */
                  && req->DescriptorIndex <= USB_DEV_MAX_CONFIGURATIONS /* configuration value exists */
              ) {
                hdev->configuration = req->DescriptorIndex;
                hdev->state = (hdev->configuration == 0) ? USB_STATE_ADDRESS : USB_STATE_CONFIGURED;
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
                return;
              }
              return;

            case USB_REQ_SET_DESCRIPTOR:
              /* not supported */
              break;

            case USB_REQ_SET_FEATURE:
              if (hdev->state != USB_STATE_DEFAULT                   /* not in default mode */
                  && req->wLength == 0                               /* wLength == 0 */
                  && req->wIndex == 0                                /* wIndex == 0 */
                  && req->wValue == USB_FEATURE_DEVICE_REMOTE_WAKEUP /* wValue == DEVICE_REMOTE_WAKEUP */
              ) {
                hdev->device_status |= USB_DEVICE_STATUS_REMOTE_WAKEUP_MASK;
                HAL_PCD_ActivateRemoteWakeup(hpcd);
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
                return;
              }
              break;
          }
          break;

        case USB_REQ_RECIPIENT_INTERFACE:
          switch (req->bRequest) {
            case USB_REQ_CLEAR_FEATURE:
              /* not supported */
              break;

            case USB_REQ_GET_INTERFACE:
              if (hdev->state == USB_STATE_CONFIGURED     /* is in configured mode */
                  && req->wLength == 1U                   /* wLength == 1 */
                  && req->wValue == 0                     /* wValue == 0 */
                  && req->wIndex < USB_DEV_MAX_INTERFACES /* interface number is valid */
              ) {
                HAL_PCD_EP_Transmit(hpcd, 0x00, &hdev->alterante_settings[req->wIndex], 1U);
                return;
              }
              break;

            case USB_REQ_GET_STATUS:
              if (hdev->state == USB_STATE_CONFIGURED     /* is in configured mode */
                  && req->wLength == 2U                   /* wLength == 2 */
                  && req->wValue == 0                     /* wValue == 0 */
                  && req->wIndex < USB_DEV_MAX_INTERFACES /* interface number is valid */
              ) {
                *(uint16_t *)temp = 0U;
                HAL_PCD_EP_Transmit(hpcd, 0x00, temp, 2U);
                return;
              }
              break;

            case USB_REQ_SET_FEATURE:
              /* not supported */
              break;

            case USB_REQ_SET_INTERFACE:
              if (hdev->state == USB_STATE_CONFIGURED     /* is in configured mode */
                  && req->wLength == 0                    /* wLength == 1 */
                  && req->wIndex < USB_DEV_MAX_INTERFACES /* interface number is valid */
              ) {
                hdev->alterante_settings[req->wIndex] = req->wValue;
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
                return;
              }
              break;
          }
          break;

        case USB_REQ_RECIPIENT_ENDPOINT:
          switch (req->bRequest) {
            case USB_REQ_CLEAR_FEATURE:
              if (hdev->state != USB_STATE_DEFAULT            /* not in default mode */
                  && req->wLength == 0                        /* wLength == 0 */
                  && req->wIndex < USB_DEV_MAX_ENDPOINTS      /* endpoint number is valid */
                  && req->wValue == USB_FEATURE_ENDPOINT_HALT /* wValue == ENDPOINT_HALT */
              ) {
                if (hdev->state == USB_STATE_ADDRESS && req->wIndex != 0) {
                  break; /* in address mode, only endpoint 0 features are available */
                }
                hdev->endpoint_status[req->wIndex] &= (~USB_ENDPOINT_STATUS_HALT_MASK);
                HAL_PCD_EP_ClrStall(hpcd, (uint8_t)req->wIndex);
                HAL_PCD_EP_ClrStall(hpcd, (uint8_t)req->wIndex & 0x80U);
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
              }
              break;

            case USB_REQ_GET_STATUS:
              if (hdev->state != USB_STATE_DEFAULT       /* not in default mode */
                  && req->wLength == 2U                  /* wLength == 2 */
                  && req->wIndex < USB_DEV_MAX_ENDPOINTS /* endpoint number is valid */
                  && req->wValue == 0                    /* wValue == 0 */
              ) {
                if (hdev->state == USB_STATE_ADDRESS && req->wIndex != 0) {
                  break; /* in address mode, only endpoint 0 features are available */
                }
                *(uint16_t *)temp = (uint16_t)hdev->endpoint_status[req->wIndex];
                HAL_PCD_EP_Transmit(hpcd, 0x00, temp, 2);
              }
              break;

            case USB_REQ_SET_FEATURE:
              if (hdev->state != USB_STATE_DEFAULT            /* not in default mode */
                  && req->wLength == 0                        /* wLength == 0 */
                  && req->wIndex < USB_DEV_MAX_ENDPOINTS      /* endpoint number is valid */
                  && req->wValue == USB_FEATURE_ENDPOINT_HALT /* wValue == ENDPOINT_HALT */
              ) {
                if (hdev->state == USB_STATE_ADDRESS && req->wIndex != 0) {
                  break; /* in address mode, only endpoint 0 features are available */
                }
                hdev->endpoint_status[req->wIndex] |= USB_ENDPOINT_STATUS_HALT_MASK;
                HAL_PCD_EP_SetStall(hpcd, (uint8_t)req->wIndex);
                HAL_PCD_EP_SetStall(hpcd, (uint8_t)req->wIndex & 0x80U);
                HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
              }
              break;

            case USB_REQ_SYNCH_FRAME:
              /* not supported */
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
  USB_EP0_SetStall(hpcd);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  printf("DataOut\r\n");

  return;
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  if ((epnum & 0x0FU) == 0x00) {
    if (hdev->buf != NULL) {
      hdev->remain_len -= hdev->transmit_len;
      if (hdev->remain_len > 0 || hdev->transmit_len == hpcd->IN_ep[0].maxpacket) {
        hdev->buf += hdev->transmit_len;
        hdev->transmit_len = MIN(hdev->remain_len, hpcd->IN_ep[0].maxpacket);
        HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->buf, hdev->transmit_len);
      } else {
        hdev->buf = NULL;
        hdev->len = 0U;
        hdev->remain_len = 0U;
        hdev->transmit_len = 0U;
        HAL_PCD_EP_Receive(hpcd, 0x00, NULL, 0);
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
  memset(hdev->alterante_settings, 0, sizeof(hdev->alterante_settings));
  hdev->buf = NULL;
  hdev->len = 0U;
  hdev->remain_len = 0U;
  hdev->transmit_len = 0U;

  HAL_PCD_EP_Open(hpcd, 0x00, 64, EP_TYPE_CTRL);
  HAL_PCD_EP_Open(hpcd, 0x80, 64, EP_TYPE_CTRL);
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