/**
 ******************************************************************************
 * @file    usbd_audio.c
 * @author  MCD Application Team
 * @brief   This file provides the Audio core functions.
 *
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2015 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 * @verbatim
 *
 *          ===================================================================
 *                                AUDIO Class  Description
 *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
 *           Audio Devices V1.0 Mar 18, 98".
 *           This driver implements the following aspects of the specification:
 *             - Device descriptor management
 *             - Configuration descriptor management
 *             - Standard AC Interface Descriptor management
 *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
 *             - 1 Audio Streaming Endpoint
 *             - 1 Audio Terminal Input (1 channel)
 *             - Audio Class-Specific AC Interfaces
 *             - Audio Class-Specific AS Interfaces
 *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
 *             - Audio Feature Unit (limited to Mute control)
 *             - Audio Synchronization type: Asynchronous
 *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
 *          The current audio class version supports the following audio features:
 *             - Pulse Coded Modulation (PCM) format
 *             - sampling rate: 48KHz.
 *             - Bit resolution: 16
 *             - Number of channels: 2
 *             - No volume control
 *             - Mute/Unmute capability
 *             - Asynchronous Endpoints
 *
 * @note     In HS mode and when the DMA is used, all variables and data structures
 *           dealing with the DMA during the transaction process should be 32-bit aligned.
 *
 *
 *  @endverbatim
 ******************************************************************************
 */

#include "usbd_audio.h"

#include "stm32h7xx.h"
#include "usbd_ctlreq.h"

#define AUDIO_SAMPLE_FREQ(frq) \
  (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq) \
  (uint8_t)(((frq * 2U * 2U) / 1000U) & 0xFFU), (uint8_t)((((frq * 2U * 2U) / 1000U) >> 8) & 0xFFU)

#ifdef USE_USBD_COMPOSITE
#define AUDIO_PACKET_SZE_WORD(frq) \
  (uint32_t)((((frq) * 2U * 2U) / 1000U))
#endif /* USE_USBD_COMPOSITE  */

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);

#ifndef USE_USBD_COMPOSITE
static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
#endif /* USE_USBD_COMPOSITE  */

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void *USBD_AUDIO_GetAudioHeaderDesc(uint8_t *pConfDesc);

USBD_ClassTypeDef USBD_AUDIO = {
    USBD_AUDIO_Init,
    USBD_AUDIO_DeInit,
    USBD_AUDIO_Setup,
    USBD_AUDIO_EP0_TxReady,
    USBD_AUDIO_EP0_RxReady,
    USBD_AUDIO_DataIn,
    USBD_AUDIO_DataOut,
    USBD_AUDIO_SOF,
    USBD_AUDIO_IsoINIncomplete,
    USBD_AUDIO_IsoOutIncomplete,
#ifdef USE_USBD_COMPOSITE
    NULL,
    NULL,
    NULL,
    NULL,
#else
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetDeviceQualifierDesc,
#endif /* USE_USBD_COMPOSITE  */
};

#ifndef USE_USBD_COMPOSITE
/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZ] __ALIGN_END = {
    /* Configuration 1 */
    0x09,                              /* bLength */
    USB_DESC_TYPE_CONFIGURATION,       /* bDescriptorType */
    LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ), /* wTotalLength */
    HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
    0x02, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
#if (USBD_SELF_POWERED == 1U)
    0xC0, /* bmAttributes: Bus Powered according to user configuration */
#else
    0x80, /* bmAttributes: Bus Powered according to user configuration */
#endif              /* USBD_SELF_POWERED */
    USBD_MAX_POWER, /* MaxPower (mA) */
    /* 09 byte*/

    /* USB Speaker Standard interface descriptor */
    AUDIO_INTERFACE_DESC_SIZE,   /* bLength */
    USB_DESC_TYPE_INTERFACE,     /* bDescriptorType */
    0x00,                        /* bInterfaceNumber */
    0x00,                        /* bAlternateSetting */
    0x00,                        /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,      /* bInterfaceClass */
    AUDIO_SUBCLASS_AUDIOCONTROL, /* bInterfaceSubClass */
    AUDIO_PROTOCOL_UNDEFINED,    /* bInterfaceProtocol */
    0x00,                        /* iInterface */
    /* 09 byte*/

    /* USB Speaker Class-specific AC Interface Descriptor */
    AUDIO_INTERFACE_DESC_SIZE,       /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_HEADER,            /* bDescriptorSubtype */
    0x00, /* 1.00 */                 /* bcdADC */
    0x01,
    0x27, /* wTotalLength */
    0x00,
    0x01, /* bInCollection */
    0x01, /* baInterfaceNr */
    /* 09 byte*/

    /* USB Speaker Input Terminal Descriptor */
    AUDIO_INPUT_TERMINAL_DESC_SIZE,  /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_INPUT_TERMINAL,    /* bDescriptorSubtype */
    0x01,                            /* bTerminalID */
    0x01,                            /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
    0x01,
    0x00,       /* bAssocTerminal */
    0x02,       /* bNrChannels */
    0x03, 0x00, /* wChannelConfig */
    0x00,       /* iChannelNames */
    0x00,       /* iTerminal */
    /* 12 byte*/

    /* USB Speaker Audio Feature Unit Descriptor */
    0x09,                                      /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,           /* bDescriptorType */
    AUDIO_CONTROL_FEATURE_UNIT,                /* bDescriptorSubtype */
    AUDIO_OUT_STREAMING_CTRL,                  /* bUnitID */
    0x01,                                      /* bSourceID */
    0x01,                                      /* bControlSize */
    AUDIO_CONTROL_VOLUME | AUDIO_CONTROL_MUTE, /* bmaControls(0) */
    0,                                         /* bmaControls(1) */
    0x00,                                      /* iTerminal */
    /* 09 byte */

    /* USB Speaker Output Terminal Descriptor */
    0x09,                            /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_OUTPUT_TERMINAL,   /* bDescriptorSubtype */
    0x03,                            /* bTerminalID */
    0x01,                            /* wTerminalType  0x0301 */
    0x03,
    0x00, /* bAssocTerminal */
    0x02, /* bSourceID */
    0x00, /* iTerminal */
    /* 09 byte */

    /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
    /* Interface 1, Alternate Setting 0                                              */
    AUDIO_INTERFACE_DESC_SIZE,     /* bLength */
    USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
    0x01,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass */
    AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
    AUDIO_PROTOCOL_UNDEFINED,      /* bInterfaceProtocol */
    0x00,                          /* iInterface */
    /* 09 byte*/

    /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Operational */
    /* Interface 1, Alternate Setting 1                                           */
    AUDIO_INTERFACE_DESC_SIZE,     /* bLength */
    USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
    0x01,                          /* bInterfaceNumber */
    0x01,                          /* bAlternateSetting */
    0x01,                          /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass */
    AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
    AUDIO_PROTOCOL_UNDEFINED,      /* bInterfaceProtocol */
    0x00,                          /* iInterface */
    /* 09 byte*/

    /* USB Speaker Audio Streaming Interface Descriptor */
    AUDIO_STREAMING_INTERFACE_DESC_SIZE, /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    AUDIO_STREAMING_GENERAL,             /* bDescriptorSubtype */
    0x01,                                /* bTerminalLink */
    0x01,                                /* bDelay */
    0x01,                                /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
    0x00,
    /* 07 byte*/

    /* USB Speaker Audio Type III Format Interface Descriptor */
    0x0B,                               /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
    AUDIO_STREAMING_FORMAT_TYPE,        /* bDescriptorSubtype */
    AUDIO_FORMAT_TYPE_I,                /* bFormatType */
    0x02,                               /* bNrChannels */
    0x02,                               /* bSubFrameSize :  2 Bytes per frame (16bits) */
    16,                                 /* bBitResolution (16-bits per sample) */
    0x01,                               /* bSamFreqType only one frequency supported */
    AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ), /* Audio sampling frequency coded on 3 bytes */
    /* 11 byte*/

    /* Endpoint 1 - Standard Descriptor */
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE, /* bLength */
    USB_DESC_TYPE_ENDPOINT,            /* bDescriptorType */
    AUDIO_OUT_EP,                      /* bEndpointAddress 1 out endpoint */
    0x05,                              /* bmAttributes */
    AUDIO_PACKET_SZE(USBD_AUDIO_FREQ), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
    AUDIO_FS_BINTERVAL,                /* bInterval */
    0x00,                              /* bRefresh */
    0x00,                              /* bSynchAddress */
    /* 09 byte*/

    /* Endpoint - Audio Streaming Descriptor */
    AUDIO_STREAMING_ENDPOINT_DESC_SIZE, /* bLength */
    AUDIO_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType */
    AUDIO_ENDPOINT_GENERAL,             /* bDescriptor */
    0x00,                               /* bmAttributes */
    0x00,                               /* bLockDelayUnits */
    0x00,                               /* wLockDelay */
    0x00,
    /* 07 byte*/
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};
#endif /* USE_USBD_COMPOSITE  */

static uint8_t AUDIOOutEpAdd = AUDIO_OUT_EP;

extern I2S_HandleTypeDef hi2s2;

volatile uint16_t last_rd_samples;
volatile uint16_t last_wr_samples;
volatile uint16_t writable_samples;

/**
 * @brief  USBD_AUDIO_Init
 *         Initialize the AUDIO interface
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
  UNUSED(cfgidx);
  USBD_AUDIO_HandleTypeDef *haudio;

  /* Allocate Audio structure */
  haudio = (USBD_AUDIO_HandleTypeDef *)USBD_malloc(sizeof(USBD_AUDIO_HandleTypeDef));
  pdev->pClassDataCmsit[pdev->classId] = haudio;
  pdev->pClassData = haudio;

  if (haudio == NULL) {
    return (uint8_t)USBD_EMEM;
  }

#ifdef USE_USBD_COMPOSITE
  /* Get the Endpoints addresses allocated for this class instance */
  AUDIOOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_ISOC, (uint8_t)pdev->classId);
#endif /* USE_USBD_COMPOSITE */

  if (pdev->dev_speed == USBD_SPEED_HIGH) {
    pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = AUDIO_HS_BINTERVAL;
  } else { /* LOW and FULL-speed endpoints */
    pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = AUDIO_FS_BINTERVAL;
  }

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, AUDIOOutEpAdd, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET);
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].is_used = 1U;

  haudio->alt_setting = 0U;
  haudio->offset = AUDIO_OFFSET_UNKNOWN;
  haudio->wr_ptr = 0U;
  haudio->rd_ptr = 0U;
  haudio->rd_enable = 0U;

  /* Prepare Out endpoint to receive 1st packet */
  USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, haudio->buffer, AUDIO_OUT_PACKET);

  /* Start DMA Transfer */
  HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)haudio->buffer, AUDIO_TOTAL_BUF_SIZE / sizeof(uint16_t));

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Init
 *         DeInitialize the AUDIO layer
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
  UNUSED(cfgidx);

#ifdef USE_USBD_COMPOSITE
  /* Get the Endpoints addresses allocated for this class instance */
  AUDIOOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_ISOC, (uint8_t)pdev->classId);
#endif /* USE_USBD_COMPOSITE */

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, AUDIOOutEpAdd);
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].is_used = 0U;
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassDataCmsit[pdev->classId] != NULL) {
    /* Stop DMA Transfer */
    HAL_I2S_DMAStop(&hi2s2);

    USBD_free(pdev->pClassDataCmsit[pdev->classId]);
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    pdev->pClassData = NULL;
  }

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Setup
 *         Handle the AUDIO specific requests
 * @param  pdev: instance
 * @param  req: usb requests
 * @retval status
 */
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_FAIL;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
  if (haudio == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  if ((req->bmRequest & 0b01111111) == 0b00100001) { /* Type: Class, Recipient: Interface */
    if (req->bmRequest == 0x01 && HIBYTE(req->wValue) == 0x01) { /* Request: SET_CUR, CS: MUTE_CONTROL */

    } else if (req->bRequest == 0x01 && HIBYTE(req->wValue) == 0x02) { /* Request: SET_CUR, CS: VOLUMN_CONTROL */

    } else if (req->bRequest == 0x81 && HIBYTE(req->wValue) == 0x01) { /* Request: GET_CUR, CS: MUTE_CONTROL */
     
    } else if (req->bRequest == 0x81 && HIBYTE(req->wValue) == 0x02) { /* Request: GET_CUR, CS: VOLUMN_CONTROL */

    } else if (req->bRequest == 0x82 && HIBYTE(req->wValue) == 0x02) { /* Request: GET_MIN, CS: VOLUMN_CONTROL */

    } else if (req->bRequest == 0x83 && HIBYTE(req->wValue) == 0x02) { /* Request: GET_MAX, CS: VOLUMN_CONTROL */

    } else if (req->bRequest == 0x84 && HIBYTE(req->wValue) == 0x02) { /* Request: GET_RES, CS: VOLUMN_CONTROL */
      
    }
  } else if ((req->bmRequest & 0b01100000) == 0b00000000) { /* Type: Standard, Recipient: Any */
    if (req->bRequest == 0) { /* Request: GET_STATUS */
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        ret = USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
      }

    } else if (req->bRequest == 1) { /* Request: CLEAR_FEATURE */
      ret = USBD_OK;

    } else if (req->bRequest == 6) { /* Request: GET_DESCRIPTOR */
      if (HIBYTE(req->wValue) == 4) { /* DescriptorType: INTERFACE */
        USBD_DescHeaderTypeDef *desc = USBD_FindDesc(pdev->pConfDesc,
                                                     AUDIO_INTERFACE_DESCRIPTOR_TYPE,
                                                     AUDIO_CONTROL_HEADER);
        if (pbuf != NULL) {
          ret = USBD_CtlSendData(pdev, pbuf, MIN(desc->bLength, req->wLength));
        }
      } else {
        ret = USBD_OK;
      }

    } else if (req->bRequest == 10) { /* Request: GET_INTERFACE */
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        ret = USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
      }

    } else if (req->bRequest == 11) { /* Request: SET_INTERFACE */
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        if (LOBYTE(req->wValue) <= USBD_MAX_NUM_INTERFACES) {
          haudio->alt_setting = (uint8_t)req->wValue;
          ret = USBD_OK;
        }
      }
    }

    if (ret == USBD_FAIL) {
      USBD_CtlError(pdev, req);
    }
    return (uint8_t)ret;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
      case USB_REQ_TYPE_CLASS:
        switch (req->bRequest) {
          case AUDIO_REQ_GET_CUR:
            AUDIO_REQ_GetCurrent(pdev, req);
            break;

          case AUDIO_REQ_SET_CUR:
            AUDIO_REQ_SetCurrent(pdev, req);
            break;

          default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;

      case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest) {
          case USB_REQ_GET_STATUS:
            if (pdev->dev_state == USBD_STATE_CONFIGURED) {
              (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
            } else {
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
            break;

          case USB_REQ_GET_DESCRIPTOR:
            if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE) {
              pbuf = (uint8_t *)USBD_AUDIO_GetAudioHeaderDesc(pdev->pConfDesc);
              if (pbuf != NULL) {
                len = MIN(USB_AUDIO_DESC_SIZ, req->wLength);
                (void)USBD_CtlSendData(pdev, pbuf, len);
              } else {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
              }
            }
            break;

          case USB_REQ_GET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED) {
              (void)USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
            } else {
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
            break;

          case USB_REQ_SET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED) {
              if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES) {
                haudio->alt_setting = (uint8_t)(req->wValue);
              } else {
                /* Call the error management function (command will be NAKed */
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
              }
            } else {
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
            break;

          case USB_REQ_CLEAR_FEATURE:
            break;

          default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;
      default:
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
    }

    return (uint8_t)ret;
  }

#ifndef USE_USBD_COMPOSITE
/**
 * @brief  USBD_AUDIO_GetCfgDesc
 *         return configuration descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length) {
  *length = (uint16_t)sizeof(USBD_AUDIO_CfgDesc);

  return USBD_AUDIO_CfgDesc;
}
#endif /* USE_USBD_COMPOSITE  */

/**
 * @brief  USBD_AUDIO_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  UNUSED(pdev);
  UNUSED(epnum);

  /* Only OUT data are processed */
  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_EP0_RxReady
 *         handle EP0 Rx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev) {
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR) {
    /* In this driver, to simplify code, only SET_CUR request is managed */

    if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL) {
      haudio->control.cmd = 0U;
      haudio->control.len = 0U;
    }
  }

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_EP0_TxReady
 *         handle EP0 TRx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev) {
  UNUSED(pdev);

  /* Only OUT control data are processed */
  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_SOF
 *         handle SOF event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev) {
  UNUSED(pdev);
  USBD_AUDIO_HandleTypeDef *haudio;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  haudio->rd_ptr = AUDIO_TOTAL_BUF_SIZE - ((DMA_Stream_TypeDef *)hi2s2.hdmatx->Instance)->NDTR * sizeof(uint16_t);
  uint16_t rd_samples = haudio->rd_ptr / 4;
  uint16_t wr_samples = haudio->wr_ptr / 4;

  static uint16_t last_rd = 0;
  static uint16_t last_wr = 0;
  last_rd_samples = rd_samples > last_rd ? rd_samples - last_rd : rd_samples + AUDIO_TOTAL_BUF_SIZE / 4 - last_rd;
  last_wr_samples = wr_samples > last_wr ? wr_samples - last_wr : wr_samples + AUDIO_TOTAL_BUF_SIZE / 4 - last_wr;
  last_rd = rd_samples;
  last_wr = wr_samples;

  if (haudio->rd_ptr < haudio->wr_ptr) {
    writable_samples = (haudio->rd_ptr + AUDIO_TOTAL_BUF_SIZE - haudio->wr_ptr) / 4;
  } else {
    writable_samples = (haudio->rd_ptr - haudio->wr_ptr) / 4;
  }

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_IsoINIncomplete
 *         handle data ISO IN Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  UNUSED(pdev);
  UNUSED(epnum);

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_IsoOutIncomplete
 *         handle data ISO OUT Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  USBD_AUDIO_HandleTypeDef *haudio;

  if (pdev->pClassDataCmsit[pdev->classId] == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  /* Prepare Out endpoint to receive next audio packet */
  (void)USBD_LL_PrepareReceive(pdev, epnum, &haudio->buffer[haudio->wr_ptr], AUDIO_OUT_PACKET);

  return (uint8_t)USBD_OK;
}

/**
 * @brief  USBD_AUDIO_DataOut
 *         handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t packet_size;

#ifdef USE_USBD_COMPOSITE
  /* Get the Endpoints addresses allocated for this class instance */
  AUDIOOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_ISOC, (uint8_t)pdev->classId);
#endif /* USE_USBD_COMPOSITE */

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == AUDIOOutEpAdd) {
    /* Get received data packet length */
    packet_size = (uint16_t)USBD_LL_GetRxDataSize(pdev, epnum);

    /* Increment the Buffer pointer or roll it back when all buffers are full */
    haudio->wr_ptr += packet_size;

    if (haudio->wr_ptr >= AUDIO_TOTAL_BUF_SIZE) {
      /* All buffers are full: roll back */
      haudio->wr_ptr = 0U;
    }

    /* Prepare Out endpoint to receive next audio packet */
    USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, &haudio->buffer[haudio->wr_ptr], AUDIO_OUT_PACKET);
  }

  return (uint8_t)USBD_OK;
}

/**
 * @brief  AUDIO_Req_GetCurrent
 *         Handles the GET_CUR Audio control request.
 * @param  pdev: device instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL) {
    return;
  }

  (void)USBD_memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);

  /* Send the current mute state */
  (void)USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, USB_MAX_EP0_SIZE));
}

/**
 * @brief  AUDIO_Req_SetCurrent
 *         Handles the SET_CUR Audio control request.
 * @param  pdev: device instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL) {
    return;
  }

  if (req->wLength != 0U) {
    haudio->control.cmd = AUDIO_REQ_SET_CUR;                            /* Set the request value */
    haudio->control.len = (uint8_t)MIN(req->wLength, USB_MAX_EP0_SIZE); /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);                         /* Set the request target unit */

    /* Prepare the reception of the buffer over EP0 */
    (void)USBD_CtlPrepareRx(pdev, haudio->control.data, haudio->control.len);
  }
}

#ifndef USE_USBD_COMPOSITE
/**
 * @brief  DeviceQualifierDescriptor
 *         return Device Qualifier descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length) {
  *length = (uint16_t)sizeof(USBD_AUDIO_DeviceQualifierDesc);

  return USBD_AUDIO_DeviceQualifierDesc;
}
#endif /* USE_USBD_COMPOSITE  */

#ifdef USE_USBD_COMPOSITE
/**
 * @brief  USBD_AUDIO_GetEpPcktSze
 * @param  pdev: device instance (reserved for future use)
 * @param  If: Interface number (reserved for future use)
 * @param  Ep: Endpoint number (reserved for future use)
 * @retval status
 */
uint32_t USBD_AUDIO_GetEpPcktSze(USBD_HandleTypeDef *pdev, uint8_t If, uint8_t Ep) {
  uint32_t mps;

  UNUSED(pdev);
  UNUSED(If);
  UNUSED(Ep);

  mps = AUDIO_PACKET_SZE_WORD(USBD_AUDIO_FREQ);

  /* Return the wMaxPacketSize value in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  return mps;
}
#endif /* USE_USBD_COMPOSITE */

/**
 * @brief  USBD_AUDIO_GetAudioHeaderDesc
 *         This function return the Audio descriptor
 * @param  pdev: device instance
 * @param  pConfDesc:  pointer to Bos descriptor
 * @retval pointer to the Audio AC Header descriptor
 */
static void *USBD_AUDIO_GetAudioHeaderDesc(uint8_t *pConfDesc, uint8_t type, uint8_t subType) {
  USBD_ConfigDescTypeDef *desc = (USBD_ConfigDescTypeDef *)(void *)pConfDesc;
  USBD_DescHeaderTypeDef *pdesc = (USBD_DescHeaderTypeDef *)(void *)pConfDesc;
  uint8_t *pAudioDesc = NULL;
  uint16_t ptr;

  if (desc->wTotalLength > desc->bLength) {
    ptr = desc->bLength;

    while (ptr < desc->wTotalLength) {
      pdesc = USBD_GetNextDesc((uint8_t *)pdesc, &ptr);
      if ((pdesc->bDescriptorType == AUDIO_INTERFACE_DESCRIPTOR_TYPE) &&
          (pdesc->bDescriptorSubType == AUDIO_CONTROL_HEADER)) {
        pAudioDesc = (uint8_t *)pdesc;
        break;
      }
    }
  }
  return pAudioDesc;
}
