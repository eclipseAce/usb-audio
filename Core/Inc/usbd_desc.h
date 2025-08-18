#include "stm32h7xx.h"

#define USB_MAX_STRING_DESCRIPTORS 4

extern uint8_t DeviceDescriptor[18];

extern uint8_t ConfigDescriptor[119];

extern uint8_t LangCodesDescriptor[4];

extern uint8_t StringDescriptor_0[38];

extern uint8_t StringDescriptor_1[24];

extern uint8_t StringDescriptor_2[64];

extern uint8_t *StringDescriptorPtr[USB_MAX_STRING_DESCRIPTORS];

extern uint16_t StringDescriptorLen[USB_MAX_STRING_DESCRIPTORS];