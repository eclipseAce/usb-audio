class Field:
    def __init__(self, name, size, value, compute):
        self.name = name
        self.size = size
        self.value = value
        self.compute = compute


class FieldCollection:
    def __init__(self, description, parent=None):
        self.description = description
        self.parent = parent
        if parent != None:
            parent.children.append(self)
        self.fields = list[Field]()
        self.fields_dict = dict[str, Field]()
        self.children = list[FieldCollection]()
        self.length = 0
        self.total_length = 0

    def add(self, name, size, value=None, compute=None):
        field = Field(name, size, value, compute)
        self.fields.append(field)
        self.fields_dict[name] = field
        length = size
        if isinstance(value, list) or isinstance(value, str):
            length *= len(value)
        self.length += length
        self.total_length += length

        parent = self.parent
        while parent != None:
            parent.total_length += length
            parent = parent.parent

    def add_byte(self, name, value=None, compute=None):
        self.add(name, size=1, value=value, compute=compute)

    def add_word(self, name, value=None, compute=None):
        self.add(name, size=2, value=value, compute=compute)

    def add_medium(self, name, value=None, compute=None):
        self.add(name, size=3, value=value, compute=compute)

    def add_dword(self, name, value=None, compute=None):
        self.add(name, size=4, value=value, compute=compute)

    def to_declaration(self, identifier):
        return "\n".join(
            [
                f"__ALIGN_BEGIN unsigned char {identifier}[{self.total_length}] __ALIGN_END = {{",
                *[f"  {line}" for line in self.to_hex_block()],
                "}};",
            ]
        )

    def to_hex_block(self, leading_spaces=2):
        leading = " " * leading_spaces
        lines = [f"{leading}/* {self.description} */"]
        for field in self.fields:
            value = field.value
            if field.compute == "length":
                value = self.length
            if field.compute == "total_length":
                value = self.total_length
            if isinstance(value, str):
                value = [ord(ch) for ch in value]
            if not isinstance(value, list):
                value = [value]
            for index, item in enumerate(value):
                elems = "".join(
                    [f"0x{((item >> (i * 8)) & 0xFF):02X}, " for i in range(field.size)]
                )
                comment = f"{field.name}"
                if len(value) > 1:
                    comment += f"[{index}]"
                spaces = " " * ((4 - field.size) * 6 + 2)
                lines.append(f"{leading}{elems}{spaces}/* {comment} */")
        for child in self.children:
            lines.append("")
            lines += child.to_hex_block()
        return lines


Strings = list[tuple[str, FieldCollection]]()


def get_string_index(s):
    for index, item in enumerate(Strings):
        if item[0] == s:
            return index
    index = len(Strings) + 1
    String = FieldCollection(f"String Descriptor '{s}'")
    String.add_byte("bLength", compute="length")
    String.add_byte("bDescriptorType", value=3)
    String.add_word("bString", value=s)
    Strings.append((s, String))
    return index


Dev = FieldCollection("Standard Device Descriptor")
Dev.add_byte("bLength", compute="length")
Dev.add_byte("bDescriptorType", value=1)
Dev.add_word("bcdUSB", value=0x0200)
Dev.add_byte("bDeviceClass", value=0xEF)
Dev.add_byte("bDeviceSubClass", value=0x02)
Dev.add_byte("bDeviceProtocol", value=0x00)
Dev.add_byte("bMaxPacketSize0", value=64)
Dev.add_word("idVendor", value=0x0483)
Dev.add_word("idProduct", value=0x5740)
Dev.add_word("bcdDevice", value=0x0100)
Dev.add_byte("iManufacturer", value=get_string_index("STMicroelectronics"))
Dev.add_byte("iProduct", value=get_string_index("STM32 Audio"))
Dev.add_byte("iSerialNumber", value=get_string_index("1234567890123"))
Dev.add_byte("bNumConfigurations", value=1)

Cfg = FieldCollection("Standard Configuration Descriptor")
Cfg.add_byte("bLength", compute="length")
Cfg.add_byte("bDescriptorType", value=2)
Cfg.add_word("wTotalLength", compute="total_length")
Cfg.add_byte("bNumInterfaces", value=2)
Cfg.add_byte("bConfigurationValue", value=1)
Cfg.add_byte("iConfiguration", value=0)
Cfg.add_byte("bmAttributes", value=0xC0)
Cfg.add_byte("MaxPower", value=500)

StdAcIf = FieldCollection("Standard AC Interface Descriptor", parent=Cfg)
StdAcIf.add_byte("bLength", compute="length")
StdAcIf.add_byte("bDescriptorType", value=4)
StdAcIf.add_byte("bInterfaceNumber", value=0)
StdAcIf.add_byte("bAlternateSetting", value=0)
StdAcIf.add_byte("bNumEndpoints", value=0)
StdAcIf.add_byte("bInterfaceClass", value=0x01)
StdAcIf.add_byte("bInterfaceSubClass", value=0x01)
StdAcIf.add_byte("bInterfaceProtocol", value=0)
StdAcIf.add_byte("iInterface", value=0)

CsAcIfHdr = FieldCollection('Class-Specific AC Interface Header Descriptor', parent=StdAcIf)
CsAcIfHdr.add_byte("bLength", compute="length")
CsAcIfHdr.add_byte("bDescriptorType", value=0x24)
CsAcIfHdr.add_byte("bDescriptorSubtype", value=0x01)
CsAcIfHdr.add_word("bcdADC", value=0x0100)
CsAcIfHdr.add_word("wTotalLength", compute='total_length')
CsAcIfHdr.add_byte("bInCollection", value=1)
CsAcIfHdr.add_byte("baInterfaceNr", value=[1])

It = FieldCollection('Input Terminal Descriptor', parent=CsAcIfHdr)
It.add_byte('bLength', compute='length')
It.add_byte('bDescriptorType', value=0x24)
It.add_byte('bDescriptorSubtype', value=0x02)
It.add_byte('bTerminalID', value=1)
It.add_word('wTerminalType', value=0x0101)
It.add_byte('bAssocTerminal', value=0)
It.add_byte('bNrChannels', value=2)
It.add_word('wChannelConfig', value=0x0003)
It.add_byte('iChannelNames', value=0)
It.add_byte('iTerminal', value=0)

Fu = FieldCollection('Feature Unit Descriptor', parent=CsAcIfHdr)
Fu.add_byte('bLength', compute='length')
Fu.add_byte('bDescriptorType', value=0x24)
Fu.add_byte('bDescriptorSubtype', value=0x06)
Fu.add_byte('bUnitID', value=2)
Fu.add_byte('bSourceID', value=1)
Fu.add_byte('bControlSize', value=1)
Fu.add_byte('bmaControls', value=[0x03, 0x00, 0x00])
Fu.add_byte('iFeature', value=0)

Ot = FieldCollection('Output Terminal Descriptor', parent=CsAcIfHdr)
Ot.add_byte('bLength', compute='length')
Ot.add_byte('bDescriptorType', value=0x24)
Ot.add_byte('bDescriptorSubtype', value=0x03)
Ot.add_byte('bTerminalID', value=3)
Ot.add_word('wTerminalType', value=0x0301)
Ot.add_byte('bAssocTerminal', value=0)
Ot.add_byte('bSourceID', value=2)
Ot.add_byte('iTerminal', value=0)

StdAsIf_0 = FieldCollection("Standard AS Interface Descriptor (AlternateSetting: 0)", parent=Cfg)
StdAsIf_0.add_byte("bLength", compute="length")
StdAsIf_0.add_byte("bDescriptorType", value=4)
StdAsIf_0.add_byte("bInterfaceNumber", value=1)
StdAsIf_0.add_byte("bAlternateSetting", value=0)
StdAsIf_0.add_byte("bNumEndpoints", value=0)
StdAsIf_0.add_byte("bInterfaceClass", value=0x01)
StdAsIf_0.add_byte("bInterfaceSubClass", value=0x02)
StdAsIf_0.add_byte("bInterfaceProtocol", value=0)
StdAsIf_0.add_byte("iInterface", value=0)

StdAsIf_1 = FieldCollection("Standard AS Interface Descriptor (AlternateSetting: 1)", parent=Cfg)
StdAsIf_1.add_byte("bLength", compute="length")
StdAsIf_1.add_byte("bDescriptorType", value=4)
StdAsIf_1.add_byte("bInterfaceNumber", value=1)
StdAsIf_1.add_byte("bAlternateSetting", value=1)
StdAsIf_1.add_byte("bNumEndpoints", value=2)
StdAsIf_1.add_byte("bInterfaceClass", value=0x01)
StdAsIf_1.add_byte("bInterfaceSubClass", value=0x02)
StdAsIf_1.add_byte("bInterfaceProtocol", value=0)
StdAsIf_1.add_byte("iInterface", value=0)

CsAsIf_1 = FieldCollection("Class-Specific AS Interface Descriptor", parent=StdAsIf_1)
CsAsIf_1.add_byte("bLength", compute="length")
CsAsIf_1.add_byte('bDescriptorType', value=0x24)
CsAsIf_1.add_byte('bDescriptorSubtype', value=0x01)
CsAsIf_1.add_byte('bTerminalLink', value=1)
CsAsIf_1.add_byte('bDelay', value=1)
CsAsIf_1.add_word('wFormatTag', value=0x0001)

CsAsFmtType_1 = FieldCollection("Class-Specific AS Format Type Descriptor (Type I)", parent=CsAsIf_1)
CsAsFmtType_1.add_byte("bLength", compute="length")
CsAsFmtType_1.add_byte('bDescriptorType', value=0x24)
CsAsFmtType_1.add_byte('bDescriptorSubtype', value=0x01)
CsAsFmtType_1.add_byte('bFormatType', value=0x01)
CsAsFmtType_1.add_byte('bNrChannels', value=2)
CsAsFmtType_1.add_byte('bSubframeSize', value=2)
CsAsFmtType_1.add_byte('bBitResolution', value=16)
CsAsFmtType_1.add_byte('bSamFreqType', value=1)
CsAsFmtType_1.add_medium('tSamFreq', value=[48000])


StdAsIsoAudioDataEp = FieldCollection("Standard AS Isochronous Audio Data Endpoint Descriptor", parent=StdAsIf_1)
StdAsIsoAudioDataEp.add_byte('bLength', compute='length')
StdAsIsoAudioDataEp.add_byte('bDescriptorType', value=5)
StdAsIsoAudioDataEp.add_byte('bEndpointAddress', value=0x01)
StdAsIsoAudioDataEp.add_byte('bmAttributes', value=0x05)
StdAsIsoAudioDataEp.add_word('wMaxPacketSize', value=int(48000/1000*2*2))
StdAsIsoAudioDataEp.add_byte('bInterval', value=1)
StdAsIsoAudioDataEp.add_byte('bRefresh', value=0)
StdAsIsoAudioDataEp.add_byte('bSynchAddress', value=0x81)

CsAsIsoAudioDataEp = FieldCollection("Class-Specific AS Isochronous Audio Data Endpoint Descriptor", parent=StdAsIsoAudioDataEp)
CsAsIsoAudioDataEp.add_byte('bLength', compute='length')
CsAsIsoAudioDataEp.add_byte('bDescriptorType', value=0x25)
CsAsIsoAudioDataEp.add_byte('bDescriptorSubtype', value=0x01)
CsAsIsoAudioDataEp.add_byte('bmAttributes', value=0x00)
CsAsIsoAudioDataEp.add_byte('bLockDelayUnits', value=0)
CsAsIsoAudioDataEp.add_word('wLockDelay', value=0)

StdAsIsoSynchEp = FieldCollection("Standard AS Isochronous Synch Endpoint Descriptor", parent=StdAsIf_1)
StdAsIsoSynchEp.add_byte('bLength', compute='length')
StdAsIsoSynchEp.add_byte('bDescriptorType', value=5)
StdAsIsoSynchEp.add_byte('bEndpointAddress', value=0x81)
StdAsIsoSynchEp.add_byte('bmAttributes', value=0x01)
StdAsIsoSynchEp.add_word('wMaxPacketSize', 3)
StdAsIsoSynchEp.add_byte('bInterval', value=1)
StdAsIsoSynchEp.add_byte('bRefresh', value=2)
StdAsIsoSynchEp.add_byte('bSynchAddress', value=0)

for index, item in enumerate(Strings):
    print(item[1].to_declaration(f"USB_StringDesc_{item[0]}"))
print(Dev.to_declaration("USB_DeviceDesc"))
print(Cfg.to_declaration("USB_ConfigDesc"))
