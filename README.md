# spd_platformer
[**中文文档**](https://github.com/C-Hidery/spd_platformer/blob/master/README_ZH.md)

The modified version of tool 'spreadtrum_flash'

spd_platformer is a maintenance release for spreadtrum_flash

[spreadtrum_flash](https://github.com/TomKing062/spreadtrum_flash)

---

Run this before making on linux:

``` bash
sudo apt update
# Ubuntu/Obsidian
sudo apt install libusb-1.0-0-dev
# Android(Termux)
pkg install termux-api libusb clang git
```

Then make:
``` bash
make
```

Use on Termux:

``` bash
# Search OTG device
termux-usb -l
[
  "/dev/bus/usb/xxx/xxx"
]
# Authorize OTG device
termux-usb -r /dev/bus/usb/xxx/xxx
# Run
termux-usb -e './spd_platformer --usb-fd' /dev/bus/usb/xxx/xxx
```
