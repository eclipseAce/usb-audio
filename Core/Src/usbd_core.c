#include "usbd_core.h"

#include <stdio.h>
#include <string.h>

#include "stm32h7xx.h"
#include "usbd_desc.h"

static void USB_EP0_Transmit(PCD_HandleTypeDef *hpcd, uint8_t *buf, uint16_t len) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  hdev->ep0_buf = buf;
  hdev->ep0_total_len = len;
  hdev->ep0_remain_len = hdev->ep0_total_len;
  hdev->ep0_packet_len = MIN(hdev->ep0_remain_len, USB_EP0_MAX_PACKET);
  HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->ep0_buf, hdev->ep0_packet_len);
}

static void USB_EP0_TransmitStatus(PCD_HandleTypeDef *hpcd) {
  HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
}

static void USB_EP0_SetStall(PCD_HandleTypeDef *hpcd) {
  HAL_PCD_EP_SetStall(hpcd, 0x00);
  HAL_PCD_EP_SetStall(hpcd, 0x80);
}

static void USB_HandleStandardRequest(PCD_HandleTypeDef *hpcd) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  USB_SetupReqTypeDef *setup = (USB_SetupReqTypeDef *)hpcd->Setup;

  switch (setup->bmRequestType & USB_REQ_RECIPIENT_MASK) {
    case USB_REQ_RECIPIENT_DEVICE:
      switch (setup->bRequest) {
        case USB_REQ_CLEAR_FEATURE:
          if (hdev->state != USB_STATE_DEFAULT                     /* not in default mode */
              && setup->wValue == USB_FEATURE_DEVICE_REMOTE_WAKEUP /* wValue == DEVICE_REMOTE_WAKEUP */
              && setup->wIndex == 0                                /* wIndex == 0 */
              && setup->wLength == 0                               /* wLength == 0 */
          ) {
            hdev->status &= (~USB_DEVICE_STATUS_REMOTE_WAKEUP_MASK);
            HAL_PCD_DeActivateRemoteWakeup(hpcd);
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_GET_CONFIGURATION:
          if (hdev->state != USB_STATE_DEFAULT /* not in default mode */
              && setup->wValue == 0            /* wValue == 0 */
              && setup->wIndex == 0            /* wIndex == 0 */
              && setup->wLength == 1U          /* wLength == 1 */
          ) {
            USB_EP0_Transmit(hpcd, &hdev->config, 1U);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          switch (setup->wValueH) {
            case USB_DESC_TYPE_DEVICE:
              USB_EP0_Transmit(hpcd, DeviceDescriptor, MIN(setup->wLength, sizeof(DeviceDescriptor)));
              break;

            case USB_DESC_TYPE_CONFIGURATION:
              USB_EP0_Transmit(hpcd, ConfigDescriptor, MIN(setup->wLength, sizeof(ConfigDescriptor)));
              break;

            case USB_DESC_TYPE_STRING:
              switch (setup->wValueL) {
                case 0:
                  USB_EP0_Transmit(hpcd, LangCodesDescriptor, MIN(setup->wLength, sizeof(LangCodesDescriptor)));
                  break;
                case 1:
                  USB_EP0_Transmit(hpcd, StringDescriptor_0, MIN(setup->wLength, sizeof(StringDescriptor_0)));
                  break;
                case 2:
                  USB_EP0_Transmit(hpcd, StringDescriptor_1, MIN(setup->wLength, sizeof(StringDescriptor_1)));
                  break;
                case 3:
                  USB_EP0_Transmit(hpcd, StringDescriptor_2, MIN(setup->wLength, sizeof(StringDescriptor_2)));
                  break;
                default:
                  USB_EP0_SetStall(hpcd);
                  break;
              }
              break;

            default:
              USB_EP0_SetStall(hpcd);
              break;
          }
          break;

        case USB_REQ_GET_STATUS:
          if (hdev->state != USB_STATE_DEFAULT /* not in default mode */
              && setup->wValue == 0            /* wValue == 0 */
              && setup->wIndex == 0            /* wIndex == 0 */
              && setup->wLength == 2U          /* wLength == 2 */
          ) {
            USB_EP0_Transmit(hpcd, (uint8_t *)&hdev->status, 2U);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_SET_ADDRESS:
          if (hdev->state != USB_STATE_CONFIGURED /* device is not in configured mode */
              && setup->wValue <= 127             /* wValue <= 127 */
              && setup->wIndex == 0               /* wIndex == 0 */
              && setup->wLength == 0              /* wLength == 0 */
          ) {
            hdev->address = (uint8_t)setup->wValue;
            hdev->state = (hdev->address == 0) ? USB_STATE_DEFAULT : USB_STATE_ADDRESS;
            HAL_PCD_SetAddress(hpcd, hdev->address);
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_SET_CONFIGURATION:
          if (hdev->state != USB_STATE_DEFAULT                /* not in default mode */
              && setup->wValueH == 0                          /* wValueH == 0 */
              && setup->wValueL <= USB_DEV_MAX_CONFIGURATIONS /* configuration value (wValueL) exists */
              && setup->wIndex == 0                           /* wIndex == 0 */
              && setup->wLength == 0                          /* wLength == 0 */
          ) {
            hdev->config = setup->wValueL;
            hdev->state = (hdev->config == 0) ? USB_STATE_ADDRESS : USB_STATE_CONFIGURED;
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_SET_DESCRIPTOR:
          USB_EP0_SetStall(hpcd);
          break;

        case USB_REQ_SET_FEATURE:
          if (hdev->state != USB_STATE_DEFAULT                     /* not in default mode */
              && setup->wValue == USB_FEATURE_DEVICE_REMOTE_WAKEUP /* wValue == DEVICE_REMOTE_WAKEUP */
              && setup->wIndex == 0                                /* wIndex == 0 */
              && setup->wLength == 0                               /* wLength == 0 */
          ) {
            hdev->status |= USB_DEVICE_STATUS_REMOTE_WAKEUP_MASK;
            HAL_PCD_ActivateRemoteWakeup(hpcd);
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        default:
          USB_EP0_SetStall(hpcd);
          break;
      }
      break;

    case USB_REQ_RECIPIENT_INTERFACE:
      switch (setup->bRequest) {
        case USB_REQ_CLEAR_FEATURE:
          USB_EP0_SetStall(hpcd);
          break;

        case USB_REQ_GET_INTERFACE:
          if (hdev->state == USB_STATE_CONFIGURED       /* is in configured mode */
              && setup->wIndex < USB_DEV_MAX_INTERFACES /* interface number (wIndex) is valid */
              && setup->wValue == 0                     /* wValue == 0 */
              && setup->wLength == 1U                   /* wLength == 1 */
          ) {
            USB_EP0_Transmit(hpcd, &hdev->alt_settings[setup->wIndex], 1U);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_GET_STATUS:
          if (hdev->state == USB_STATE_CONFIGURED       /* is in configured mode */
              && setup->wIndex < USB_DEV_MAX_INTERFACES /* interface number (wIndex) is valid */
              && setup->wValue == 0                     /* wValue == 0 */
              && setup->wLength == 2U                   /* wLength == 2 */
          ) {
            uint16_t itf_status = 0;
            USB_EP0_Transmit(hpcd, (uint8_t *)&itf_status, 2U);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_SET_FEATURE:
          USB_EP0_SetStall(hpcd);
          break;

        case USB_REQ_SET_INTERFACE:
          if (hdev->state == USB_STATE_CONFIGURED       /* is in configured mode */
              && setup->wLength == 0                    /* wLength == 1 */
              && setup->wIndex < USB_DEV_MAX_INTERFACES /* interface number is valid */
          ) {
            hdev->alt_settings[setup->wIndex] = setup->wValue;
            if (setup->wIndex == 1U) {
              if (setup->wValue == 1U) {
                // HAL_PCD_EP_Receive(hpcd, 0x01, hdev->audio_packet, USB_AUDIO_OUT_PACKET);
              } else {
                HAL_PCD_EP_Flush(hpcd, 0x81);
                HAL_PCD_EP_Flush(hpcd, 0x01);
                // HAL_PCD_EP_Close(hpcd, 0x81);
                // HAL_PCD_EP_Close(hpcd, 0x01);
                memset(hdev->audio_buf, 0, sizeof(hdev->audio_buf));
                memset(hdev->audio_packet, 0, sizeof(hdev->audio_packet));
                hdev->audio_rd_ptr = 0U;
                hdev->audio_wr_ptr = 0U;
              }
            }
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        default:
          USB_EP0_SetStall(hpcd);
          break;
      }
      break;

    case USB_REQ_RECIPIENT_ENDPOINT:
      switch (setup->bRequest) {
        case USB_REQ_CLEAR_FEATURE:
          if (hdev->state != USB_STATE_DEFAULT                             /* not in default mode */
              && setup->wValue == USB_FEATURE_ENDPOINT_HALT                /* wValue == ENDPOINT_HALT */
              && setup->wIndex < USB_DEV_MAX_ENDPOINTS                     /* endpoint number (wIndex) is valid */
              && setup->wLength == 0                                       /* wLength == 0 */
              && !(hdev->state == USB_STATE_ADDRESS && setup->wIndex != 0) /* in address mode, only endpoint 0 features are available */
          ) {
            hdev->ep_status[setup->wIndex] &= (~USB_ENDPOINT_STATUS_HALT_MASK);
            HAL_PCD_EP_ClrStall(hpcd, (uint8_t)setup->wIndex);
            HAL_PCD_EP_ClrStall(hpcd, (uint8_t)setup->wIndex & 0x80U);
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_GET_STATUS:
          if (hdev->state != USB_STATE_DEFAULT                             /* not in default mode */
              && setup->wValue == 0                                        /* wValue == 0 */
              && setup->wIndex < USB_DEV_MAX_ENDPOINTS                     /* endpoint number (wIndex) is valid */
              && setup->wLength == 2U                                      /* wLength == 2 */
              && !(hdev->state == USB_STATE_ADDRESS && setup->wIndex != 0) /* in address mode, only endpoint 0 features are available */
          ) {
            USB_EP0_Transmit(hpcd, (uint8_t *)&hdev->ep_status[setup->wIndex], 2U);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_SET_FEATURE:
          if (hdev->state != USB_STATE_DEFAULT                             /* not in default mode */
              && setup->wValue == USB_FEATURE_ENDPOINT_HALT                /* wValue == ENDPOINT_HALT */
              && setup->wIndex < USB_DEV_MAX_ENDPOINTS                     /* endpoint number (wIndex) is valid */
              && setup->wLength == 0                                       /* wLength == 0 */
              && !(hdev->state == USB_STATE_ADDRESS && setup->wIndex != 0) /* in address mode, only endpoint 0 features are available */
          ) {
            hdev->ep_status[setup->wIndex] |= USB_ENDPOINT_STATUS_HALT_MASK;
            HAL_PCD_EP_SetStall(hpcd, (uint8_t)setup->wIndex);
            HAL_PCD_EP_SetStall(hpcd, (uint8_t)setup->wIndex & 0x80U);
            USB_EP0_TransmitStatus(hpcd);
          } else {
            USB_EP0_SetStall(hpcd);
          }
          break;

        case USB_REQ_SYNCH_FRAME:
          USB_EP0_SetStall(hpcd);
          break;

        default:
          USB_EP0_SetStall(hpcd);
          break;
      }
      break;

    default:
      USB_EP0_SetStall(hpcd);
      break;
  }
}

static void USB_HandleClassRequest(PCD_HandleTypeDef *hpcd) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  USB_SetupReqTypeDef *setup = (USB_SetupReqTypeDef *)hpcd->Setup;

  if ((setup->bmRequestType & USB_REQ_RECIPIENT_MASK) != USB_REQ_RECIPIENT_INTERFACE) {
    USB_EP0_SetStall(hpcd);
    return;
  }

  switch (setup->bmRequestType & USB_REQ_RECIPIENT_MASK) {
    case USB_REQ_RECIPIENT_INTERFACE:
      if (setup->wIndexL == 1U && setup->wIndexH == 2U) {
        switch (setup->wValueH) {
          case USB_AUDIO_CS_MUTE_CONTROL:
            switch (setup->bRequest) {
              case USB_AUDIO_REQ_SET_CUR:
                HAL_PCD_EP_Receive(hpcd, 0x00, hdev->setup_buf, 1U);
                break;

              case USB_AUDIO_REQ_GET_CUR:
              case USB_AUDIO_REQ_GET_MIN:
              case USB_AUDIO_REQ_GET_MAX:
              case USB_AUDIO_REQ_GET_RES:
                if (setup->wValueL == 0) {
                  uint8_t value;
                  switch (setup->bRequest) {
                    case USB_AUDIO_REQ_GET_CUR:
                      value = hdev->audio_mute;
                      break;
                    case USB_AUDIO_REQ_GET_MIN:
                      value = 0;
                      break;
                    case USB_AUDIO_REQ_GET_MAX:
                      value = 1;
                      break;
                    case USB_AUDIO_REQ_GET_RES:
                      value = 1;
                      break;
                  }
                  USB_EP0_Transmit(hpcd, &value, 1);
                } else {
                  USB_EP0_SetStall(hpcd);
                }

              default:
                USB_EP0_SetStall(hpcd);
                break;
            }
            break;

          default:
            USB_EP0_SetStall(hpcd);
            break;
        }
      } else {
        USB_EP0_SetStall(hpcd);
      }
      break;

    case USB_REQ_RECIPIENT_ENDPOINT:
      USB_EP0_SetStall(hpcd);
      break;

    default:
      USB_EP0_SetStall(hpcd);
      break;
  }
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
  USB_SetupReqTypeDef *setup = (USB_SetupReqTypeDef *)hpcd->Setup;

  switch (setup->bmRequestType & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_STANDARD:
      USB_HandleStandardRequest(hpcd);
      break;

    case USB_REQ_TYPE_CLASS:
      USB_EP0_SetStall(hpcd);
      break;

    case USB_REQ_TYPE_VENDOR:
      USB_EP0_SetStall(hpcd);
      break;

    default:
      USB_EP0_SetStall(hpcd);
      break;
  }
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  if ((epnum & 0x0FU) == 0x00) {
  } else {
    uint16_t rx_count = (uint16_t)HAL_PCD_EP_GetRxCount(hpcd, 0x01);
    uint16_t writable = USB_AUDIO_BUFFER_SIZE - hdev->audio_wr_ptr;
    if (writable >= rx_count) {
      memcpy(&hdev->audio_buf[hdev->audio_wr_ptr], hdev->audio_packet, rx_count);
    } else {
      memcpy(&hdev->audio_buf[hdev->audio_wr_ptr], hdev->audio_packet, writable);
      memcpy(&hdev->audio_buf[0], &hdev->audio_packet[writable], rx_count - writable);
    }
    hdev->audio_wr_ptr = (hdev->audio_wr_ptr + rx_count) % USB_AUDIO_BUFFER_SIZE;

    HAL_PCD_EP_Receive(hpcd, 0x01, hdev->audio_packet, USB_AUDIO_OUT_PACKET);
  }
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  if ((epnum & 0x0FU) == 0x00) {
    if (hdev->ep0_buf != NULL) {
      hdev->ep0_remain_len -= hdev->ep0_packet_len;
      if (hdev->ep0_remain_len > 0                      /* data stage not finished */
          || hdev->ep0_packet_len == USB_EP0_MAX_PACKET /* data stage finished, and last packet len == maxpacket, send ZLP */
      ) {
        hdev->ep0_buf += hdev->ep0_packet_len;
        hdev->ep0_packet_len = MIN(hdev->ep0_remain_len, USB_EP0_MAX_PACKET);
        HAL_PCD_EP_Transmit(hpcd, 0x00, hdev->ep0_buf, hdev->ep0_packet_len);
      } else {
        hdev->ep0_buf = NULL;
        hdev->ep0_total_len = 0U;
        hdev->ep0_remain_len = 0U;
        hdev->ep0_packet_len = 0U;
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
  hdev->state = USB_STATE_DEFAULT;
  hdev->status = 0U;
  memset(hdev->ep_status, 0, sizeof(hdev->ep_status) * sizeof(uint16_t));
  hdev->address = 0U;
  hdev->config = 0U;
  memset(hdev->alt_settings, 0, sizeof(hdev->alt_settings));
  hdev->ep0_buf = NULL;
  hdev->ep0_total_len = 0U;
  hdev->ep0_remain_len = 0U;
  hdev->ep0_packet_len = 0U;
  HAL_PCD_SetAddress(hpcd, 0);
  HAL_PCD_DeActivateRemoteWakeup(hpcd);
  HAL_PCD_EP_Open(hpcd, 0x00, USB_EP0_MAX_PACKET, EP_TYPE_CTRL);
  HAL_PCD_EP_Open(hpcd, 0x80, USB_EP0_MAX_PACKET, EP_TYPE_CTRL);

  memset(hdev->audio_buf, 0, sizeof(hdev->audio_buf));
  memset(hdev->audio_packet, 0, sizeof(hdev->audio_packet));
  hdev->audio_rd_ptr = 0U;
  hdev->audio_wr_ptr = 0U;
  HAL_PCD_EP_Open(hpcd, 0x01, USB_AUDIO_OUT_PACKET, EP_TYPE_ISOC);
  HAL_PCD_EP_Open(hpcd, 0x81, 3, EP_TYPE_ISOC);

  HAL_PCD_EP_Receive(hpcd, 0x01, hdev->audio_packet, USB_AUDIO_OUT_PACKET);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
  return;
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
  USB_DeviceHandleTypeDef *hdev = (USB_DeviceHandleTypeDef *)hpcd->pData;
  if ((epnum & 0x0FU) == 0x00) {
    HAL_PCD_EP_Flush(hpcd, 0x01);
    HAL_PCD_EP_Receive(hpcd, 0x01, hdev->audio_packet, USB_AUDIO_OUT_PACKET);
  }
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