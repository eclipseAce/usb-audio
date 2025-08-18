#include "usbd_desc.h"

__ALIGN_BEGIN uint8_t DeviceDescriptor[18] __ALIGN_END = {
  /* Standard Device Descriptor */
  0x12,                     /* bLength */
  0x01,                     /* bDescriptorType = DEVICE */
  0x00, 0x02,               /* bcdUSB */
  0xEF,                     /* bDeviceClass */
  0x02,                     /* bDeviceSubClass */
  0x00,                     /* bDeviceProtocol */
  0x40,                     /* bMaxPacketSize0 */
  0x83, 0x04,               /* idVendor */
  0x40, 0x57,               /* idProduct */
  0x00, 0x01,               /* bcdDevice */
  0x01,                     /* iManufacturer = "STMicroelectronics" */
  0x02,                     /* iProduct = "STM32 Audio" */
  0x03,                     /* iSerialNumber = "1234567890123" */
  0x01,                     /* bNumConfigurations */
};

__ALIGN_BEGIN uint8_t ConfigDescriptor[118] __ALIGN_END = {
  /* Standard Configuration Descriptor */
  0x09,                     /* bLength */
  0x02,                     /* bDescriptorType = CONFIGURATION */
  0x76, 0x00,               /* wTotalLength */
  0x02,                     /* bNumInterfaces */
  0x01,                     /* bConfigurationValue */
  0x00,                     /* iConfiguration */
  0xC0,                     /* bmAttributes */
  0xF4,                     /* MaxPower */
  
  /* Standard AC Interface Descriptor */
  0x09,                     /* bLength */
  0x04,                     /* bDescriptorType = INTERFACE */
  0x00,                     /* bInterfaceNumber */
  0x00,                     /* bAlternateSetting */
  0x00,                     /* bNumEndpoints */
  0x01,                     /* bInterfaceClass = AUDIO */
  0x01,                     /* bInterfaceSubClass = AUDIOCONTROL */
  0x00,                     /* bInterfaceProtocol = UNDEFINED */
  0x00,                     /* iInterface */
  
  /* Class-Specific AC Interface Header Descriptor */
  0x09,                     /* bLength */
  0x24,                     /* bDescriptorType = INTERFACE */
  0x01,                     /* bDescriptorSubtype = HEADER */
  0x00, 0x01,               /* bcdADC */
  0x27, 0x00,               /* wTotalLength */
  0x01,                     /* bInCollection */
  0x01,                     /* baInterfaceNr */
  
  /* Input Terminal Descriptor */
  0x0C,                     /* bLength */
  0x24,                     /* bDescriptorType = INTERFACE */
  0x02,                     /* bDescriptorSubtype = INPUT_TERMINAL */
  0x01,                     /* bTerminalID */
  0x01, 0x01,               /* wTerminalType = STREAMING */
  0x00,                     /* bAssocTerminal */
  0x01,                     /* bNrChannels */
  0x00, 0x00,               /* wChannelConfig */
  0x00,                     /* iChannelNames */
  0x00,                     /* iTerminal */
  
  /* Feature Unit Descriptor */
  0x09,                     /* bLength */
  0x24,                     /* bDescriptorType = INTERFACE */
  0x06,                     /* bDescriptorSubtype = FEATURE_UNIT */
  0x02,                     /* bUnitID */
  0x01,                     /* bSourceID */
  0x01,                     /* bControlSize */
  0x00,                     /* bmaControls[0] */
  0x00,                     /* bmaControls[1] */
  0x00,                     /* iFeature */
  
  /* Output Terminal Descriptor */
  0x09,                     /* bLength */
  0x24,                     /* bDescriptorType = INTERFACE */
  0x03,                     /* bDescriptorSubtype = OUTPUT_TERMINAL */
  0x03,                     /* bTerminalID */
  0x01, 0x03,               /* wTerminalType = SPEAKER */
  0x00,                     /* bAssocTerminal */
  0x02,                     /* bSourceID */
  0x00,                     /* iTerminal */
  
  /* Standard AS Interface Descriptor (AlternateSetting: 0) */
  0x09,                     /* bLength */
  0x04,                     /* bDescriptorType = INTERFACE */
  0x01,                     /* bInterfaceNumber */
  0x00,                     /* bAlternateSetting */
  0x00,                     /* bNumEndpoints */
  0x01,                     /* bInterfaceClass = AUDIO */
  0x02,                     /* bInterfaceSubClass = AUDIOSTREAMING */
  0x00,                     /* bInterfaceProtocol = UNDEFINED */
  0x00,                     /* iInterface */
  
  /* Standard AS Interface Descriptor (AlternateSetting: 1) */
  0x09,                     /* bLength */
  0x04,                     /* bDescriptorType = INTERFACE */
  0x01,                     /* bInterfaceNumber */
  0x01,                     /* bAlternateSetting */
  0x02,                     /* bNumEndpoints */
  0x01,                     /* bInterfaceClass = AUDIO */
  0x02,                     /* bInterfaceSubClass = AUDIOSTREAMING */
  0x00,                     /* bInterfaceProtocol = UNDEFINED */
  0x00,                     /* iInterface */
  
  /* Class-Specific AS Interface Descriptor */
  0x07,                     /* bLength */
  0x24,                     /* bDescriptorType = INTERFACE */
  0x01,                     /* bDescriptorSubtype = AS_GENERAL */
  0x01,                     /* bTerminalLink */
  0x01,                     /* bDelay */
  0x01, 0x00,               /* wFormatTag = PCM */
  
  /* Class-Specific AS Format Type Descriptor (Type I) */
  0x0B,                     /* bLength */
  0x24,                     /* bDescriptorType = INTERFACE */
  0x02,                     /* bDescriptorSubtype = FORMAT_TYPE */
  0x01,                     /* bFormatType = FORMAT_TYPE_I */
  0x02,                     /* bNrChannels */
  0x02,                     /* bSubframeSize */
  0x10,                     /* bBitResolution */
  0x01,                     /* bSamFreqType */
  0x80, 0xBB, 0x00,         /* tSamFreq */
  
  /* Standard AS Isochronous Audio Data Endpoint Descriptor */
  0x09,                     /* bLength */
  0x05,                     /* bDescriptorType = ENDPOINT */
  0x01,                     /* bEndpointAddress */
  0x05,                     /* bmAttributes */
  0xC0, 0x00,               /* wMaxPacketSize */
  0x01,                     /* bInterval */
  0x00,                     /* bRefresh */
  0x81,                     /* bSynchAddress */
  
  /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor */
  0x07,                     /* bLength */
  0x25,                     /* bDescriptorType = ENDPOINT */
  0x01,                     /* bDescriptorSubtype = EP_GENERAL */
  0x00,                     /* bmAttributes */
  0x00,                     /* bLockDelayUnits */
  0x00, 0x00,               /* wLockDelay */
  
  /* Standard AS Isochronous Synch Endpoint Descriptor */
  0x09,                     /* bLength */
  0x05,                     /* bDescriptorType = ENDPOINT */
  0x81,                     /* bEndpointAddress */
  0x01,                     /* bmAttributes */
  0x03, 0x00,               /* wMaxPacketSize */
  0x01,                     /* bInterval */
  0x02,                     /* bRefresh */
  0x00,                     /* bSynchAddress */
};

__ALIGN_BEGIN uint8_t LangCodesDescriptor[4] __ALIGN_END = {
  /* Codes Representing Languages Supported by the Device */
  0x04,                     /* bLength */
  0x03,                     /* bDescriptorType = STRING */
  0x09, 0x04,               /* wLANGID */
};

__ALIGN_BEGIN uint8_t StringDescriptor_0[38] __ALIGN_END = {
  /* String Descriptor of "STMicroelectronics" */
  0x26,                     /* bLength */
  0x03,                     /* bDescriptorType = STRING */
  0x53, 0x00,               /* bString[0] */
  0x54, 0x00,               /* bString[1] */
  0x4D, 0x00,               /* bString[2] */
  0x69, 0x00,               /* bString[3] */
  0x63, 0x00,               /* bString[4] */
  0x72, 0x00,               /* bString[5] */
  0x6F, 0x00,               /* bString[6] */
  0x65, 0x00,               /* bString[7] */
  0x6C, 0x00,               /* bString[8] */
  0x65, 0x00,               /* bString[9] */
  0x63, 0x00,               /* bString[10] */
  0x74, 0x00,               /* bString[11] */
  0x72, 0x00,               /* bString[12] */
  0x6F, 0x00,               /* bString[13] */
  0x6E, 0x00,               /* bString[14] */
  0x69, 0x00,               /* bString[15] */
  0x63, 0x00,               /* bString[16] */
  0x73, 0x00,               /* bString[17] */
};

__ALIGN_BEGIN uint8_t StringDescriptor_1[24] __ALIGN_END = {
  /* String Descriptor of "STM32 Audio" */
  0x18,                     /* bLength */
  0x03,                     /* bDescriptorType = STRING */
  0x53, 0x00,               /* bString[0] */
  0x54, 0x00,               /* bString[1] */
  0x4D, 0x00,               /* bString[2] */
  0x33, 0x00,               /* bString[3] */
  0x32, 0x00,               /* bString[4] */
  0x20, 0x00,               /* bString[5] */
  0x41, 0x00,               /* bString[6] */
  0x75, 0x00,               /* bString[7] */
  0x64, 0x00,               /* bString[8] */
  0x69, 0x00,               /* bString[9] */
  0x6F, 0x00,               /* bString[10] */
};

__ALIGN_BEGIN uint8_t StringDescriptor_2[28] __ALIGN_END = {
  /* String Descriptor of "1234567890123" */
  0x1C,                     /* bLength */
  0x03,                     /* bDescriptorType = STRING */
  0x31, 0x00,               /* bString[0] */
  0x32, 0x00,               /* bString[1] */
  0x33, 0x00,               /* bString[2] */
  0x34, 0x00,               /* bString[3] */
  0x35, 0x00,               /* bString[4] */
  0x36, 0x00,               /* bString[5] */
  0x37, 0x00,               /* bString[6] */
  0x38, 0x00,               /* bString[7] */
  0x39, 0x00,               /* bString[8] */
  0x30, 0x00,               /* bString[9] */
  0x31, 0x00,               /* bString[10] */
  0x32, 0x00,               /* bString[11] */
  0x33, 0x00,               /* bString[12] */
};