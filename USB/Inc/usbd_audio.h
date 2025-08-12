/**
 ******************************************************************************
 * @file    usbd_audio.h
 * @author  MCD Application Team
 * @brief   header file for the usbd_audio.c file.
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
 */

#ifndef __USBD_AUDIO_H
#define __USBD_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#ifndef USBD_AUDIO_FREQ
#define USBD_AUDIO_FREQ                               48000U
#endif /* USBD_AUDIO_FREQ */

#ifndef USBD_MAX_NUM_INTERFACES
#define USBD_MAX_NUM_INTERFACES                       1U
#endif /* USBD_AUDIO_FREQ */

#ifndef AUDIO_OUT_EP
#define AUDIO_OUT_EP                                  0x01U
#endif /* AUDIO_OUT_EP */

#ifndef AUDIO_IN_EP
#define AUDIO_IN_EP                                   0x81U
#endif /* AUDIO_IN_EP */

#define USBD_AUDIO_VOL_MIN                            (int16_t)0xA000    /* -96dB */
#define USBD_AUDIO_VOL_MAX                            (int16_t)0x0000    /*   0dB */
#define USBD_AUDIO_VOL_RES                            (int16_t)0x0300    /*   3dB */

#define USB_AUDIO_CONFIG_DESC_SIZ                     0x76U
#define AUDIO_INTERFACE_DESC_SIZE                     0x09U
#define USB_AUDIO_DESC_SIZ                            0x09U
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x09U
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x07U

#define AUDIO_DESCRIPTOR_TYPE                         0x21U
#define USB_DEVICE_CLASS_AUDIO                        0x01U
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01U
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02U
#define AUDIO_PROTOCOL_UNDEFINED                      0x00U
#define AUDIO_STREAMING_GENERAL                       0x01U
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02U

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24U
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25U

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01U
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02U
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03U
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06U

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0CU
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09U
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07U

#define AUDIO_CONTROL_MUTE                            0x0001U
#define AUDIO_CONTROL_VOLUME                          0x0002U

#define AUDIO_FORMAT_TYPE_I                           0x01U
#define AUDIO_FORMAT_TYPE_III                         0x03U

#define AUDIO_ENDPOINT_GENERAL                        0x01U

#define AUDIO_REQ_SET_CUR                             0x01U
#define AUDIO_REQ_SET_MIN                             0x02U
#define AUDIO_REQ_SET_MAX                             0x03U
#define AUDIO_REQ_SET_RES                             0x04U
#define AUDIO_REQ_GET_CUR                             0x81U
#define AUDIO_REQ_GET_MIN                             0x82U
#define AUDIO_REQ_GET_MAX                             0x83U
#define AUDIO_REQ_GET_RES                             0x84U

#define AUDIO_OUT_STREAMING_CTRL                      0x02U

#define AUDIO_OUT_TC                                  0x01U
#define AUDIO_IN_TC                                   0x02U

#define AUDIO_OUT_PACKET                              (uint16_t)(((USBD_AUDIO_FREQ / 1000U + 1) * 2U * 2U))

/* Number of sub-packets in the audio transfer buffer. You can modify this value but always make sure
  that it is an even number and higher than 3 */
#define AUDIO_OUT_PACKET_NUM                          8U
/* Total size of the audio transfer buffer */
#define AUDIO_TOTAL_BUF_SIZE                          ((uint16_t)(AUDIO_OUT_PACKET * AUDIO_OUT_PACKET_NUM))

typedef struct {
  uint32_t alt_setting;
  uint8_t buffer[AUDIO_TOTAL_BUF_SIZE + AUDIO_OUT_PACKET];
  uint8_t playing;
  uint16_t rd_ptr;
  uint16_t wr_ptr;
  uint16_t fb_fnsof;
  uint32_t fb_value;
  uint32_t fb_value_norm;
  uint8_t mute;
  int16_t volume;
  USBD_SetupReqTypedef setup_req;
  uint8_t setup_data[USB_MAX_EP0_SIZE];
} USBD_AUDIO_HandleTypeDef;

/*
 * Audio Class specification release 1.0
 */

/* Table 4-2: Class-Specific AC Interface Header Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint16_t bcdADC;
  uint16_t wTotalLength;
  uint8_t bInCollection;
  uint8_t baInterfaceNr;
} __PACKED USBD_SpeakerIfDescTypeDef;

/* Table 4-3: Input Terminal Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint8_t bTerminalID;
  uint16_t wTerminalType;
  uint8_t bAssocTerminal;
  uint8_t bNrChannels;
  uint16_t wChannelConfig;
  uint8_t iChannelNames;
  uint8_t iTerminal;
} __PACKED USBD_SpeakerInDescTypeDef;

/* USB Speaker Audio Feature Unit Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint8_t bUnitID;
  uint8_t bSourceID;
  uint8_t bControlSize;
  uint16_t bmaControls;
  uint8_t iTerminal;
} __PACKED USBD_SpeakerFeatureDescTypeDef;

/* Table 4-4: Output Terminal Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint8_t bTerminalID;
  uint16_t wTerminalType;
  uint8_t bAssocTerminal;
  uint8_t bSourceID;
  uint8_t iTerminal;
} __PACKED USBD_SpeakerOutDescTypeDef;

/* Table 4-19: Class-Specific AS Interface Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint8_t bTerminalLink;
  uint8_t bDelay;
  uint16_t wFormatTag;
} __PACKED USBD_SpeakerStreamIfDescTypeDef;

/* USB Speaker Audio Type III Format Interface Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint8_t bFormatType;
  uint8_t bNrChannels;
  uint8_t bSubFrameSize;
  uint8_t bBitResolution;
  uint8_t bSamFreqType;
  uint8_t tSamFreq2;
  uint8_t tSamFreq1;
  uint8_t tSamFreq0;
} USBD_SpeakerIIIFormatIfDescTypeDef;

/* Table 4-17: Standard AC Interrupt Endpoint Descriptor */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
  uint8_t bRefresh;
  uint8_t bSynchAddress;
} __PACKED USBD_SpeakerEndDescTypeDef;

/* Table 4-21: Class-Specific AS Isochronous Audio Data Endpoint Descriptor        */
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptor;
  uint8_t bmAttributes;
  uint8_t bLockDelayUnits;
  uint16_t wLockDelay;
} __PACKED USBD_SpeakerEndStDescTypeDef;

extern USBD_ClassTypeDef USBD_AUDIO;
#define USBD_AUDIO_CLASS &USBD_AUDIO

#ifdef USE_USBD_COMPOSITE
uint32_t USBD_AUDIO_GetEpPcktSze(USBD_HandleTypeDef *pdev, uint8_t If, uint8_t Ep);
#endif /* USE_USBD_COMPOSITE */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_AUDIO_H */
