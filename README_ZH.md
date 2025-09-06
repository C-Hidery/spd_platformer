# spd_platformer

工具'spreadtrum_flash'的修改版

spd_platformer是它的一个维护版本（原仓库已存档）

[spreadtrum_flash](https://github.com/TomKing062/spreadtrum_flash)

---

在Linux上执行make时请先运行这个：

``` bash
sudo apt update
# Ubuntu/Obsidian
sudo apt install libusb-1.0-0-dev
# Android(Termux)
pkg install termux-api libusb clang git
```

然后make:
``` bash
make
```

在Termux上使用:

``` bash
# 搜索OTG设备
termux-usb -l
[
  "/dev/bus/usb/xxx/xxx"
]
# 授权OTG设备
termux-usb -r /dev/bus/usb/xxx/xxx
# 运行
termux-usb -e './spd_platformer --usb-fd' /dev/bus/usb/xxx/xxx
```

---

***修改的命令:***

    part_table

**等同于命令`partition_list`**

    exec_addr [BINARY FILE] [ADDR]
    
**你需要同时提供文件路径和地址**

    exec <ADDR>

**你需要在执行FDL1时提供FDL1的地址**

