
# 编译器设置
CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -pedantic -Wno-unused
LIBS = -lm -lpthread
APPNAME = spd_platformer

# 检测操作系统 - 在 Termux 中，uname -s 返回 "Linux"
UNAME_S := $(shell uname -s)

# Termux 特殊处理
ifneq (,$(wildcard /data/data/com.termux/files/usr))
    # 检测到 Termux 环境
    IS_TERMUX = 1
    # Termux 通常不需要链接 rt 库，因为 clock_gettime 在 libc 中
else
    IS_TERMUX = 0
    # 常规 Linux 处理
    ifeq ($(UNAME_S),Linux)
        LIBS += -lrt
        CFLAGS += -D_POSIX_C_SOURCE=199309L
    endif
endif

# LibUSB 配置 - 在 Termux 中可能需要额外安装
LIBUSB = 1
CFLAGS += -DUSE_LIBUSB=$(LIBUSB)

ifeq ($(LIBUSB), 1)
    # 在 Termux 中，libusb 可能需要从仓库安装
    ifeq ($(IS_TERMUX),1)
        # Termux 中的 libusb 包名可能是 libusb 或 libusb1
        LIBS += -lusb-1.0
    else
        LIBS += -lusb-1.0
    endif
endif

# 目标文件
SRCS = spd_main.c common.c
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(APPNAME)

# 链接目标程序
$(APPNAME): $(OBJS)
	$(CC) -s $(CFLAGS) -o $@ $^ $(LIBS)

# 编译源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@