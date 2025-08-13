class Device:
    def __init__(self, **kwargs):
        pass


class Configuration:
    def __init__(self, **kwargs):
        pass


Device(
    # bLength=0,
    # bDescriptorType=0,
    bcdUSB=0x0210,
    bDeviceClass=0xEF,
    bDeviceSubClass=0x02,
    bDeviceProtocol=0x01,
    bMaxPacketSize0=64,
    idVendor=0x0483,
    idProduct=0x5740,
    bcdDevice=0x0100,
    strManufacturer="STMicroelectronics",
    strProduct="STM32 Audio",
    strSerialNumber="1234567890123",
    bNumConfigurations=1,
)

Configuration(
    # bLength=0,
    # bDescriptorType=0,
    # wTotalLength=0,
    # bNumInterfaces=2,
    bConfigurationValue=1,
    strConfiguration=None,
    bmAttributes=0xC0,
    MaxPower=500,
    Interfaces=[
        
    ],
)


class AudioControl:
    pass


class InputTerminal:
    pass


class OutputTerminal:
    pass


class FeatureUnit:
    pass


AudioControl(
    ADC=0x0100,
    InterfaceNr=[1],
    TerminalAndUnits=[
        InputTerminal(
            TerminalID=1,
            TerminalType=0x0101,
            AssocTerminal=0,
            NrChannels=1,
            ChannelConfig=0x0000,
            ChannelNames=None,
            Terminal=None,
        ),
        FeatureUnit(
            UnitID=2,
            SourceID=1,
            ControlSize=1,
            Controls=[0x03, 0x00, 0x00],
            Feature=None,
        ),
        OutputTerminal(
            TerminalID=3,
            TerminalType=0x0301,
            AssocTerminal=0,
            SourceID=2,
            Terminal=None,
        ),
    ],
)
