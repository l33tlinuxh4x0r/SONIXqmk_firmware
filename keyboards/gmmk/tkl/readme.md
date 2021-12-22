# GMMK TKL Mechanical Keyboard

Hardware Supported: SN32F268
Revisions
rev1: IS31FL3731    , NOT SUPPORTED, red led color only
rev2: VS12L03A (I2C), SN before SNSS12116xxxxx
rev3: VS12L17A (SPI), SN equal or after  SNSS12116xxxxx

Make example for this keyboard (after setting up your build environment):

    to build ANSI keymap: (87 keys)
    make gmmk/tkl:default
    make gmmk/tkl/rev2:default
    make gmmk/tkl/rev3:default
    
    to build ISO keymap: (88 keys)
    make gmmk/tkl:iso
    make gmmk/tkl/rev2:iso
    make gmmk/tkl/rev3:iso

    to build VIA:
    make gmmk/tkl/rev2:via
    make gmmk/tkl/rev3:via
    
    For VIA + OpenRGB,
    https://github.com/gloryhzw/qmk_tool/wiki/VIA-OpenRGB-for-GMMK

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).
