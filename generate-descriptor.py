from enum import Enum
from functools import reduce


class StringAllocator:
    def __init__(self):
        self.strings = list[str]()

    def allocate(self, string) -> "StringInstance":
        for index, item in enumerate(self.strings):
            if item == string:
                return index
        self.strings.append(string)
        return StringInstance(string, len(self.strings))


class StringInstance:
    def __init__(self, string, index):
        self.string = string
        self.index = index


class Descriptor:
    def __init__(self, description, fields: list["Field"], children: list["Descriptor"] = None):
        self.description = description
        self.fields = fields
        self.children = children if children != None else []
        self.length = reduce(lambda a, b: a + b, [f.length for f in self.fields], 0)
        self.total_length = self.length + reduce(lambda a, b: a + b, [c.total_length for c in self.children], 0)

    def generate(self):
        lines = []
        lines.append(f"/* {self.description} */")
        for field in self.fields:
            lines += field.generate(self)
        for child in self.children:
            lines.append("")
            lines += child.generate()
        return lines

    def generate_declaration(self, ident: str):
        lines = []
        lines.append(f"__ALIGN_BEGIN unsigned char {ident}[{self.total_length}] __ALIGN_END = {{")
        lines += ["  " + line for line in self.generate()]
        lines.append("};")
        return lines


class Field:
    def __init__(self, size, name, value):
        self.size = size
        self.name = name
        self.value = value
        if isinstance(value, list) or isinstance(value, str):
            self.length = size * len(value)
        else:
            self.length = size

    def generate(self, descriptor: "Descriptor"):
        value = self.value
        if isinstance(value, str):
            value = [ord(ch) for ch in value]
        if not isinstance(value, list):
            value = [value]
        lines = []
        for index, item in enumerate(value):
            comment = f"{self.name}[{index}]" if len(value) > 1 else self.name
            if isinstance(item, StringInstance):
                comment += f' = "{item.string}"'
                item = item.index
            elif isinstance(item, DescriptorLength):
                item = descriptor.total_length if item.total else descriptor.length
            elif isinstance(item, Enum):
                comment += f" = {item.name}"
                item = item.value
            spaces = " " * ((4 - self.size) * 6 + 2)
            line = [(item >> (i * 8)) & 0xFF for i in range(self.size)]
            line = "".join([f"0x{x:02X}, " for x in line])
            line = f"{line}{spaces}/* {comment} */"
            lines.append(line)
        return lines


class DescriptorLength:
    def __init__(self, total=False):
        self.total = total


class DescriptorTypes(Enum):
    DEVICE = 1
    CONFIGURATION = 2
    STRING = 3
    INTERFACE = 4
    ENDPOINT = 5


class AudioDescriptorTypes(Enum):
    UNDEFINED = 0x20
    DEVICE = 0x21
    CONFIGURATION = 0x22
    STRING = 0x23
    INTERFACE = 0x24
    ENDPOINT = 0x25


class AudioControlInterfaceDescriptorSubTypes(Enum):
    UNDEFINED = 0x00
    HEADER = 0x01
    INPUT_TERMINAL = 0x02
    OUTPUT_TERMINAL = 0x03
    MIXER_UNIT = 0x04
    SELECTOR_UNIT = 0x05
    FEATURE_UNIT = 0x06
    PROCESSING_UNIT = 0x07
    EXTENSION_UNIT = 0x08


class AudioStreamingInterfaceDescriptorSubTypes(Enum):
    UNDEFINED = 0x00
    AS_GENERAL = 0x01
    FORMAT_TYPE = 0x02
    FORMAT_SPECIFIC = 0x03


class AudioEndpointDescriptorSubTypes(Enum):
    UNDEFINED = 0x00
    EP_GENERAL = 0x01


class InterfaceClassCodes(Enum):
    AUDIO = 0x01


class AudioInterfaceSubClassCodes(Enum):
    UNDEFINED = 0x00
    AUDIOCONTROL = 0x01
    AUDIOSTREAMING = 0x02
    MIDISTREAMING = 0x03


class AudioInterfaceProtocolCodes(Enum):
    UNDEFINED = 0x00


class USBTerminalTypes(Enum):
    UNDEFINED = 0x0100
    STREAMING = 0x0101
    VENDOR = 0x01FF


class OutputTerminalTypes(Enum):
    UNDEFINED = 0x0300
    SPEAKER = 0x0301
    HEADPHONES = 0x0302
    HEAD_MOUNTED_DISPLAY_AUDIO = 0x0303
    DESKTOP_SPEAKER = 0x0304
    ROOM_SPEAKER = 0x0305
    COMMUNICATION_SPEAKER = 0x0306
    LOW_FREQUENCY_EFFECTS_SPEAKER = 0x0307


class AudioDataFormatTypeICodes(Enum):
    UNDEFINED = 0x0000
    PCM = 0x0001
    PCM8 = 0x0002
    IEEE_FLOAT = 0x0003
    ALAW = 0x0004
    MULAW = 0x0005


class FormatTypeCodes(Enum):
    UNDEFINED = 0x00
    FORMAT_TYPE_I = 0x01
    FORMAT_TYPE_II = 0x02
    FORMAT_TYPE_III = 0x03


string_allocator = StringAllocator()

lang_code_descriptor = Descriptor(
    description="Codes Representing Languages Supported by the Device",
    fields=[
        Field(1, "bLength", DescriptorLength()),
        Field(1, "bDescriptorType", DescriptorTypes.STRING),
        Field(2, "wLANGID", [1033]),
    ],
)

device_descriptor = Descriptor(
    description="Standard Device Descriptor",
    fields=[
        Field(1, "bLength", DescriptorLength()),
        Field(1, "bDescriptorType", DescriptorTypes.DEVICE),
        Field(2, "bcdUSB", 0x0200),
        Field(1, "bDeviceClass", 0xEF),
        Field(1, "bDeviceSubClass", 0x02),
        Field(1, "bDeviceProtocol", 0x00),
        Field(1, "bMaxPacketSize0", 64),
        Field(2, "idVendor", 0x0483),
        Field(2, "idProduct", 0x5740),
        Field(2, "bcdDevice", 0x0100),
        Field(1, "iManufacturer", string_allocator.allocate("STMicroelectronics")),
        Field(1, "iProduct", string_allocator.allocate("STM32 Audio")),
        Field(1, "iSerialNumber", string_allocator.allocate("1234567890123")),
        Field(1, "bNumConfigurations", 1),
    ],
)

config_descriptor = Descriptor(
    description="Standard Configuration Descriptor",
    fields=[
        Field(1, "bLength", DescriptorLength()),
        Field(1, "bDescriptorType", DescriptorTypes.CONFIGURATION),
        Field(2, "wTotalLength", DescriptorLength(total=True)),
        Field(1, "bNumInterfaces", 2),
        Field(1, "bConfigurationValue", 1),
        Field(1, "iConfiguration", 0),
        Field(1, "bmAttributes", 0xC0),
        Field(1, "MaxPower", 500),
    ],
    children=[
        Descriptor(
            description="Standard AC Interface Descriptor",
            fields=[
                Field(1, "bLength", DescriptorLength()),
                Field(1, "bDescriptorType", DescriptorTypes.INTERFACE),
                Field(1, "bInterfaceNumber", 0),
                Field(1, "bAlternateSetting", 0),
                Field(1, "bNumEndpoints", 0),
                Field(1, "bInterfaceClass", InterfaceClassCodes.AUDIO),
                Field(1, "bInterfaceSubClass", AudioInterfaceSubClassCodes.AUDIOCONTROL),
                Field(1, "bInterfaceProtocol", AudioInterfaceProtocolCodes.UNDEFINED),
                Field(1, "iInterface", 0),
            ],
            children=[
                Descriptor(
                    description="Class-Specific AC Interface Header Descriptor",
                    fields=[
                        Field(1, "bLength", DescriptorLength()),
                        Field(1, "bDescriptorType", AudioDescriptorTypes.INTERFACE),
                        Field(1, "bDescriptorSubtype", AudioControlInterfaceDescriptorSubTypes.HEADER),
                        Field(2, "bcdADC", 0x0100),
                        Field(2, "wTotalLength", DescriptorLength(total=True)),
                        Field(1, "bInCollection", 1),
                        Field(1, "baInterfaceNr", [1]),
                    ],
                    children=[
                        Descriptor(
                            description="Input Terminal Descriptor",
                            fields=[
                                Field(1, "bLength", DescriptorLength()),
                                Field(1, "bDescriptorType", AudioDescriptorTypes.INTERFACE),
                                Field(1, "bDescriptorSubtype", AudioControlInterfaceDescriptorSubTypes.INPUT_TERMINAL),
                                Field(1, "bTerminalID", 1),
                                Field(2, "wTerminalType", USBTerminalTypes.STREAMING),
                                Field(1, "bAssocTerminal", 0),
                                Field(1, "bNrChannels", 2),
                                Field(2, "wChannelConfig", 0x0003),
                                Field(1, "iChannelNames", 0),
                                Field(1, "iTerminal", 0),
                            ],
                        ),
                        Descriptor(
                            description="Feature Unit Descriptor",
                            fields=[
                                Field(1, "bLength", DescriptorLength()),
                                Field(1, "bDescriptorType", AudioDescriptorTypes.INTERFACE),
                                Field(1, "bDescriptorSubtype", AudioControlInterfaceDescriptorSubTypes.FEATURE_UNIT),
                                Field(1, "bUnitID", 2),
                                Field(2, "bSourceID", 1),
                                Field(1, "bControlSize", 1),
                                Field(1, "bmaControls", [0x03, 0x00, 0x00]),
                                Field(1, "iFeature", 0),
                            ],
                        ),
                        Descriptor(
                            description="Output Terminal Descriptor",
                            fields=[
                                Field(1, "bLength", DescriptorLength()),
                                Field(1, "bDescriptorType", AudioDescriptorTypes.INTERFACE),
                                Field(1, "bDescriptorSubtype", AudioControlInterfaceDescriptorSubTypes.OUTPUT_TERMINAL),
                                Field(1, "bTerminalID", 3),
                                Field(2, "wTerminalType", OutputTerminalTypes.SPEAKER),
                                Field(1, "bAssocTerminal", 0),
                                Field(1, "bSourceID", 2),
                                Field(1, "iTerminal", 0),
                            ],
                        ),
                    ],
                )
            ],
        ),
        Descriptor(
            description="Standard AS Interface Descriptor (AlternateSetting: 0)",
            fields=[
                Field(1, "bLength", DescriptorLength()),
                Field(1, "bDescriptorType", DescriptorTypes.INTERFACE),
                Field(1, "bInterfaceNumber", 1),
                Field(1, "bAlternateSetting", 0),
                Field(1, "bNumEndpoints", 0),
                Field(1, "bInterfaceClass", InterfaceClassCodes.AUDIO),
                Field(1, "bInterfaceSubClass", AudioInterfaceSubClassCodes.AUDIOSTREAMING),
                Field(1, "bInterfaceProtocol", AudioInterfaceProtocolCodes.UNDEFINED),
                Field(1, "iInterface", 0),
            ],
        ),
        Descriptor(
            description="Standard AS Interface Descriptor (AlternateSetting: 1)",
            fields=[
                Field(1, "bLength", DescriptorLength()),
                Field(1, "bDescriptorType", DescriptorTypes.INTERFACE),
                Field(1, "bInterfaceNumber", 1),
                Field(1, "bAlternateSetting", 1),
                Field(1, "bNumEndpoints", 2),
                Field(1, "bInterfaceClass", InterfaceClassCodes.AUDIO),
                Field(1, "bInterfaceSubClass", AudioInterfaceSubClassCodes.AUDIOSTREAMING),
                Field(1, "bInterfaceProtocol", AudioInterfaceProtocolCodes.UNDEFINED),
                Field(1, "iInterface", 0),
            ],
            children=[
                Descriptor(
                    description="Class-Specific AS Interface Descriptor",
                    fields=[
                        Field(1, "bLength", DescriptorLength()),
                        Field(1, "bDescriptorType", AudioDescriptorTypes.INTERFACE),
                        Field(1, "bDescriptorSubtype", AudioStreamingInterfaceDescriptorSubTypes.AS_GENERAL),
                        Field(1, "bTerminalLink", 1),
                        Field(1, "bDelay", 1),
                        Field(1, "wFormatTag", AudioDataFormatTypeICodes.PCM),
                    ],
                    children=[
                        Descriptor(
                            description="Class-Specific AS Format Type Descriptor (Type I)",
                            fields=[
                                Field(1, "bLength", DescriptorLength()),
                                Field(1, "bDescriptorType", AudioDescriptorTypes.INTERFACE),
                                Field(1, "bDescriptorSubtype", AudioStreamingInterfaceDescriptorSubTypes.FORMAT_TYPE),
                                Field(1, "bFormatType", FormatTypeCodes.FORMAT_TYPE_I),
                                Field(1, "bNrChannels", 2),
                                Field(1, "bSubframeSize", 2),
                                Field(1, "bBitResolution", 16),
                                Field(1, "bSamFreqType", 1),
                                Field(3, "tSamFreq", [48000]),
                            ],
                        )
                    ],
                ),
                Descriptor(
                    description="Standard AS Isochronous Audio Data Endpoint Descriptor",
                    fields=[
                        Field(1, "bLength", DescriptorLength()),
                        Field(1, "bDescriptorType", DescriptorTypes.ENDPOINT),
                        Field(1, "bEndpointAddress", 0x01),
                        Field(1, "bmAttributes", 0x05),
                        Field(2, "wMaxPacketSize", int(48000 / 1000 * 2 * 2)),
                        Field(1, "bInterval", 1),
                        Field(1, "bRefresh", 0),
                        Field(1, "bSynchAddress", 0x81),
                    ],
                    children=[
                        Descriptor(
                            description="Class-Specific AS Isochronous Audio Data Endpoint Descriptor",
                            fields=[
                                Field(1, "bLength", DescriptorLength()),
                                Field(1, "bDescriptorType", AudioDescriptorTypes.ENDPOINT),
                                Field(1, "bDescriptorSubtype", AudioEndpointDescriptorSubTypes.EP_GENERAL),
                                Field(1, "bmAttributes", 0x00),
                                Field(1, "bLockDelayUnits", 0),
                                Field(2, "wLockDelay", 0),
                            ],
                        ),
                    ],
                ),
                Descriptor(
                    description="Standard AS Isochronous Synch Endpoint Descriptor",
                    fields=[
                        Field(1, "bLength", DescriptorLength()),
                        Field(1, "bDescriptorType", DescriptorTypes.ENDPOINT),
                        Field(1, "bEndpointAddress", 0x81),
                        Field(1, "bmAttributes", 0x01),
                        Field(2, "wMaxPacketSize", 3),
                        Field(1, "bInterval", 1),
                        Field(1, "bRefresh", 2),
                        Field(1, "bSynchAddress", 0),
                    ],
                ),
            ],
        ),
    ],
)

descriptors = list[tuple[str, Descriptor]]()

descriptors.append(("DeviceDescriptor", device_descriptor))
descriptors.append(("ConfigDescriptor", config_descriptor))
descriptors.append(("LangCodesDescriptor", lang_code_descriptor))
for index, string in enumerate(string_allocator.strings):
    descriptor = Descriptor(
        description=f'String Descriptor of "{string}"',
        fields=[
            Field(1, "bLength", DescriptorLength()),
            Field(1, "bDescriptorType", DescriptorTypes.STRING),
            Field(2, "bString", string),
        ],
    )
    descriptors.append((f"StringDescriptor_{index}", descriptor))

lines = []
for name, descriptor in descriptors:
    lines.append("")
    lines += descriptor.generate_declaration(name)

print("\n".join(lines))
