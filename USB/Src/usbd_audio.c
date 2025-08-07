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

#include "main.h"
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
    0 | AUDIO_CONTROL_MUTE, /* bmaControls(0) */
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
    0x02,                          /* bNumEndpoints 1 out and 1 feekback */
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

    /* Standard AS Isochronous Audio Data Endpoint Descriptor */
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE, /* bLength */
    USB_DESC_TYPE_ENDPOINT,            /* bDescriptorType */
    AUDIO_OUT_EP,                      /* bEndpointAddress 1 out endpoint */
    0x05,                              /* bmAttributes */
    AUDIO_PACKET_SZE(USBD_AUDIO_FREQ), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
    0x01,                              /* bInterval */
    0x00,                              /* bRefresh */
    AUDIO_IN_EP,                       /* bSynchAddress */
    /* 09 byte*/

    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor */
    AUDIO_STREAMING_ENDPOINT_DESC_SIZE, /* bLength */
    AUDIO_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType */
    AUDIO_ENDPOINT_GENERAL,             /* bDescriptor */
    0x00,                               /* bmAttributes */
    0x00,                               /* bLockDelayUnits */
    0x00,                               /* wLockDelay */
    0x00,
    /* 07 byte*/

    /* Standard AS Isochronous Synch Endpoint Descriptor */
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE, /* bLength */
    USB_DESC_TYPE_ENDPOINT,            /* bDescriptorType */
    AUDIO_IN_EP,                       /* bEndpointAddress 1 feekback endpoint */
    0x01,                              /* bmAttributes */
    0x03,                              /* wMaxPacketSize in Bytes 3bytes */
    0x01,                              /* bInterval */
    0x02,                              /* bRefresh 4ms = 2^2 */
    0x00,                              /* bSynchAddress */
    /* 09 byte*/
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
static uint8_t AUDIOInEpAdd = AUDIO_IN_EP;

extern I2S_HandleTypeDef hi2s2;

volatile uint16_t last_rd_samples;
volatile uint16_t last_wr_samples;
volatile uint16_t last_n_writable;
volatile uint32_t last_fb_value;

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
  if (haudio == NULL) {
    return (uint8_t)USBD_EMEM;
  }

  pdev->pClassDataCmsit[pdev->classId] = haudio;
  pdev->pClassData = haudio;

#ifdef USE_USBD_COMPOSITE
  /* Get the Endpoints addresses allocated for this class instance */
  AUDIOOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_ISOC, (uint8_t)pdev->classId);
  AUDIOInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_ISOC, (uint8_t)pdev->classId);
#endif /* USE_USBD_COMPOSITE */

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, AUDIOOutEpAdd, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET);
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].is_used = 1U;
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = 1U;

  /* Open EP IN */
  USBD_LL_OpenEP(pdev, AUDIOInEpAdd, USBD_EP_TYPE_ISOC, 3);
  pdev->ep_in[AUDIOInEpAdd & 0xFU].is_used = 1U;
  pdev->ep_in[AUDIOInEpAdd & 0xFU].bInterval = 1U;

  /* Flush feedback endpoint */
  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);

  haudio->alt_setting = 0U;
  haudio->tx_enabled = 0U;
  haudio->wr_ptr = 0U;
  haudio->rd_ptr = 0U;
  haudio->mute = 0;
  haudio->volume = USBD_AUDIO_VOL_MAX;

  haudio->fb_busy = 0U;
  haudio->fb_data[0] = (uint8_t)((AUDIO_FB_VALUE >> 8) & 0x000000FF);
  haudio->fb_data[1] = (uint8_t)((AUDIO_FB_VALUE >> 16) & 0x000000FF);
  haudio->fb_data[2] = (uint8_t)((AUDIO_FB_VALUE >> 24) & 0x000000FF);

  /* Prepare Out endpoint to receive 1st packet */
  USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, haudio->buffer, AUDIO_OUT_PACKET);

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
  AUDIOInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_ISOC, (uint8_t)pdev->classId);
#endif /* USE_USBD_COMPOSITE */

  /* Flush all endpoints */
  USBD_LL_FlushEP(pdev, AUDIO_OUT_EP);
  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, AUDIOOutEpAdd);
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].is_used = 0U;
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = 0U;

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, AUDIOInEpAdd);
  pdev->ep_in[AUDIOInEpAdd & 0xFU].is_used = 0U;
  pdev->ep_in[AUDIOInEpAdd & 0xFU].bInterval = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassDataCmsit[pdev->classId] != NULL) {
    USBD_free(pdev->pClassDataCmsit[pdev->classId]);
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    pdev->pClassData = NULL;
  }

  /* Stop DMA Transfer */
  HAL_I2S_DMAStop(&hi2s2);
  HAL_GPIO_WritePin(SD_MODE_GPIO_Port, SD_MODE_Pin, GPIO_PIN_RESET);

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
  USBD_DescHeaderTypeDef *pdesc;
  USBD_StatusTypeDef ret = USBD_FAIL;
  uint8_t buf[4];

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
  if (haudio == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  /* Type: Class, Recipient: Interface */
  if ((req->bmRequest & 0b01111111) == 0b00100001) {
    /* Request: SET_CUR, CS: MUTE_CONTROL */
    if (req->bRequest == 0x01 && HIBYTE(req->wValue) == 0x01) {
      haudio->setup_req = *req;
      ret = USBD_CtlPrepareRx(pdev, haudio->setup_data, req->wLength);
    }

    /* Request: SET_CUR, CS: VOLUMN_CONTROL */
    else if (req->bRequest == 0x01 && HIBYTE(req->wValue) == 0x02) {
      haudio->setup_req = *req;
      ret = USBD_CtlPrepareRx(pdev, haudio->setup_data, req->wLength);
    }

    /* Request: GET_CUR, CS: MUTE_CONTROL */
    else if (req->bRequest == 0x81 && HIBYTE(req->wValue) == 0x01) {
      ret = USBD_CtlSendData(pdev, &haudio->mute, 1);
    }

    /* Request: GET_CUR, CS: VOLUMN_CONTROL */
    else if (req->bRequest == 0x81 && HIBYTE(req->wValue) == 0x02) {
      ret = USBD_CtlSendData(pdev, (uint8_t *)&haudio->volume, 2);
    }

    /* Request: GET_MIN, CS: VOLUMN_CONTROL */
    else if (req->bRequest == 0x82 && HIBYTE(req->wValue) == 0x02) {
      *((uint16_t *)buf) = USBD_AUDIO_VOL_MIN;
      ret = USBD_CtlSendData(pdev, buf, 2);
    }

    /* Request: GET_MAX, CS: VOLUMN_CONTROL */
    else if (req->bRequest == 0x83 && HIBYTE(req->wValue) == 0x02) {
      *((uint16_t *)buf) = USBD_AUDIO_VOL_MAX;
      ret = USBD_CtlSendData(pdev, buf, 2);
    }

    /* Request: GET_RES, CS: VOLUMN_CONTROL */
    else if (req->bRequest == 0x84 && HIBYTE(req->wValue) == 0x02) {
      *((uint16_t *)buf) = USBD_AUDIO_VOL_RES;
      ret = USBD_CtlSendData(pdev, buf, 2);
    }
  }

  /* Type: Standard, Recipient: Any */
  else if ((req->bmRequest & 0b01100000) == 0b00000000) {
    /* Request: GET_STATUS */
    if (req->bRequest == 0) {
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        *((uint16_t *)buf) = 0U;
        ret = USBD_CtlSendData(pdev, buf, 2U);
      }
    }

    /* Request: CLEAR_FEATURE */
    else if (req->bRequest == 1) {
      ret = USBD_OK;
    }

    /* Request: GET_DESCRIPTOR */
    else if (req->bRequest == 6) {
      /* DescriptorType: INTERFACE */
      if (HIBYTE(req->wValue) == 4) {
        pdesc = USBD_FindDesc(pdev->pConfDesc, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_HEADER);
        if (pdesc != NULL) {
          ret = USBD_CtlSendData(pdev, (uint8_t *)pdesc, MIN(pdesc->bLength, req->wLength));
        }
      } else {
        ret = USBD_OK;
      }
    }

    /* Request: GET_INTERFACE */
    else if (req->bRequest == 10) {
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        ret = USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
      }

    }

    /* Request: SET_INTERFACE */
    else if (req->bRequest == 11) {
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        if (LOBYTE(req->wValue) <= USBD_MAX_NUM_INTERFACES) {
          haudio->alt_setting = (uint8_t)req->wValue;
          ret = USBD_OK;
        }
      }
    }
  }

  if (ret == USBD_FAIL) {
    USBD_CtlError(pdev, req);
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
  USBD_AUDIO_HandleTypeDef *haudio;

  if (epnum == (AUDIO_IN_EP & 0xf)) {
    haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
    if (haudio == NULL) {
      return (uint8_t)USBD_FAIL;
    }
    haudio->fb_busy = 0U;
  }

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
  USBD_SetupReqTypedef *req;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
  if (haudio == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  req = &haudio->setup_req;

  /* Type: Class, Recipient: Interface */
  if ((req->bmRequest & 0b01111111) == 0b00100001) {
    /* Request: SET_CUR, CS: MUTE_CONTROL */
    if (req->bRequest == 0x01 && HIBYTE(req->wValue) == 0x01) {
      haudio->mute = haudio->setup_data[0];
      HAL_GPIO_WritePin(SD_MODE_GPIO_Port, SD_MODE_Pin, haudio->mute ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }

    /* Request: SET_CUR, CS: VOLUMN_CONTROL */
    else if (req->bRequest == 0x01 && HIBYTE(req->wValue) == 0x02) {
      haudio->volume = *(uint16_t *)&haudio->setup_data[0];
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

  uint16_t n_writable;
  if (haudio->rd_ptr < haudio->wr_ptr) {
    n_writable = (haudio->rd_ptr + AUDIO_TOTAL_BUF_SIZE - haudio->wr_ptr) / 4;
  } else {
    n_writable = (haudio->rd_ptr - haudio->wr_ptr) / 4;
  }
  last_n_writable = n_writable;

  uint32_t fb_value = n_writable * 48 / (AUDIO_TOTAL_BUF_SIZE / 2);
  // if (fb_value > 49) {
  //   fb_value = 49;
  // } else if (fb_value < 47) {
  //   fb_value = 47;
  // }
  fb_value = ((fb_value) * (1 << 14)) << 8;
  last_fb_value = fb_value;

  if (haudio->fb_busy == 0U) {
    haudio->fb_data[0] = (uint8_t)((fb_value >> 8) & 0x000000FF);
		haudio->fb_data[1] = (uint8_t)((fb_value >> 16) & 0x000000FF);
		haudio->fb_data[2] = (uint8_t)((fb_value >> 24) & 0x000000FF);
    USBD_LL_Transmit(pdev, AUDIO_IN_EP, haudio->fb_data, 3U);
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

    // Start playing when half of the audio buffer is filled
    // so if you increase the buffer length too much, the audio latency will be obvious when watching video+audio
    if (haudio->tx_enabled == 0U) {
      if (haudio->wr_ptr >= AUDIO_TOTAL_BUF_SIZE / 2U) {
        haudio->tx_enabled = 1U;

        /* Start DMA Transfer */
        HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)haudio->buffer, AUDIO_TOTAL_BUF_SIZE / sizeof(uint16_t));
        HAL_GPIO_WritePin(SD_MODE_GPIO_Port, SD_MODE_Pin, GPIO_PIN_SET);
      }
    }

    /* Prepare Out endpoint to receive next audio packet */
    USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, &haudio->buffer[haudio->wr_ptr], AUDIO_OUT_PACKET);
  }

  return (uint8_t)USBD_OK;
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
