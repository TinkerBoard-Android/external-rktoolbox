# rktoolbox

发布版本：0,2

日期：2019.11

作者邮箱：yp.xiao@rock-chips.com

文件密级：内部资料

---
**前言**

**概述**
rktoolbox旨在提供一个各个模块调试接口的总入口平台。

**产品版本**

| **芯片名称** | **内核版本**|
| ---------|--------------------------------|
| 全平台| 与内核无关 |

**读者对象**

本文档（本指南）主要适用于以下对象：
软件开发工程师

**修订记录**

| **日期**    | **版本** | **作者** | **修改说明**         |
| --------- | ------ | ------ | ---------------- |
| 2019.11.07 | V0.1   | 肖亚鹏 | 初始版本  |
| 2019.11.13 | V0.2   | 肖亚鹏 | 新增模块添加说明  |

---

[TOC]

---
## 简介
**1.传参区分指令**
当我们在shell下键入命令时，是shell创建一个进程运行对应你键入的程序，这些命令程序之间没什么联系，故他们之间公共的代码需要在每个命令二进制程序中都有拷贝，而toybox相当于shell下各种命令的集合体，我们可以看成是它们链接而成的，这样这些命令之间公共的部分就只有一份拷贝了，从而可以减少这些命令程序的二进制文件总量，这些在象嵌入式系统这些资源紧张的系统中是很有必要的。那busybox又是怎样区分不同命令的呢。有两种方法：

    1.用toybox命令 命令的参数,如 toybox ls -al
    2.通常用的一种是添加软链接为每个命令建立一个链接文件，如 ln -s toybox ls 执行时就执行./ls命令就可以

第一种方式实现原理很容易知道，命令是作为busybox的参数传入，然后就可以调用具体参数对应的指令。

第二种方式可能难理解一点，由符号链接我们可知，当键入具体命令时，都是执行toybox ，如当键入ls是，由于ls是toybox 的链接，其实执行的是toybox ，那怎样来区分不同命令呢，其实当我们键入ls，ls这个字符串就作为toybox 的第一个参数传入了，只是它隐晦一点，在第一种情况下，命令是通过第二个参数传入了。

**2.了解toybox原理**

   ```
   lrwxr-xr-x 1 root   shell          6 2009-01-01 00:00 ps -> toybox
   lrwxr-xr-x 1 root   shell          6 2009-01-01 00:00 pwd -> toybox
   lrwxr-xr-x 1 root   shell          6 2009-01-01 00:00 xxd -> toybox
   lrwxr-xr-x 1 root   shell          6 2009-01-01 00:00 yes -> toybox
   ```
可以看到，ps命令其实只是一个链接名称，程序执行的时候还是调用toybox，这里有一个比较巧妙的设计，比如执行ps，会调用起toybox程序，同时ps会作为参数传入，调用起里面的ps_main()方法。
toybox中采用了函数指针的处理方法去进行方法调用，采用toy_list结构体来保存命令（ps）和函数（ps_main）的映射。

   ```
   vi /external/toybox/toys.h

   extern struct toy_list {
     char *name;
     void (*toy_main)(void);
     char *options;
     unsigned flags;
   } toy_list[];
   ```

映射的初始化在 /external/toybox/main.c ,这里使用了字符串拼接的方法

   ```
   #undef NEWTOY
   #undef OLDTOY
   #define NEWTOY(name, opts, flags) {#name, name##_main, OPTSTR_##name, flags},
   #define OLDTOY(name, oldname, flags) \
     {#name, oldname##_main, OPTSTR_##oldname, flags},

   struct toy_list toy_list[] = {
   #include "generated/newtoys.h" //映射表文件
   };

   generated/newtoys.h
   USE_TOYBOX(NEWTOY(toybox, NULL, TOYFLAG_STAYROOT))
   USE_SH(OLDTOY(-sh, sh, 0))
   USE_SH(OLDTOY(-toysh, sh, 0))
   USE_TRUE(OLDTOY(:, true, TOYFLAG_NOFORK|TOYFLAG_NOHELP))
   USE_ACPI(NEWTOY(acpi, "abctV", TOYFLAG_USR|TOYFLAG_BIN))
       USE_GROUPADD(OLDTOY(addgroup, groupadd, TOYFLAG_NEEDROOT|TOYFLAG_SBIN))
   USE_USERADD(OLDTOY(adduser, useradd, TOYFLAG_NEEDROOT|TOYFLAG_UMASK|TOYFLAG_SBIN))
   USE_ARP(NEWTOY(arp, "vi:nDsdap:A:H:[+Ap][!sd]", TOYFLAG_USR|TOYFLAG_BIN))
   USE_ARPING(NEWTOY(arping, "<1>1s:I:w#<0c#<0AUDbqf[+AU][+Df]", TOYFLAG_USR|TOYFLAG_SBIN))
   USE_ASCII(NEWTOY(ascii, 0, TOYFLAG_USR|TOYFLAG_BIN))
   USE_BASE64(NEWTOY(base64, "diw#<0=76[!dw]", TOYFLAG_USR|TOYFLAG_BIN))
   ```

## rktoolbox

集合工具一般有两种方式来调用各个模块的功能：

1. 所有模块工具编译在一个bin文件中。这种方式缺点就是bin文件会比较大，优点是没有依赖文件，一个文件拷贝到哪里都可以使用。
2. 主入口通过映射表来动态调用模块，单模块编译成bin文件，复制到system/bin目录.这种方式和第一种优缺点刚好相反，但是有个好处就是不用关注模块是提供的源码还是bin文件都可以兼容


考虑到不一定所有模块都能提供源码，所以使用第二种方式。

```
映射表结构体:

enum{
    E_DUMP_API_FLAGS = 0x01,
};

#define SYSTEM_PATH    "/system/bin/rk_"
#define VENDOR_PATH    "/vendor/bin/rk_"

static struct {
    const char *name;
      const char *path;
    const char *info;
      int flags； //是否支持标准接口，或者支持哪些标准接口
} tools[] = {
    #define TOOL(name, flags, info) { #name, SYSTEM_PATH#name, #info, flags},
    #include "tools.h"
    #undef TOOL
    { 0, 0 },
};
```

映射文件添加 ，利用脚本对映射文件name属性排序，方便运行时使用二分查找法查找对应指令。

**调用方式**
所有模块：rktoolbox -dump   调用所有支持dump api的模块

```
 foreach tool tools:
    if tool.flags & DUMP_API_FLAG == 1
          printf("HEAD")
          exec(tool.path + argv[3]);
          printf("end")
```

单个模块：rktoolbox media -dump  对应模块dump 接口。  exec("/system/bin/media -dump")

**模块添加**
添加新的模块这里提供两种方式：

1.可以提供的源码的情况，可以参考deviceinfo目录

```
 a.在rktoolbox下创建对应的目录并把源码拷贝到目录下
 b.根据情况修改模块参数适配统一接口
 c.修改tools.h添加模块映射表信息
 d.编写Android.mk和rktoolbox一起编译，编译出的模块名字需要添加 rk_ 前缀
```

2.只提供bin文件，可以参考dr-g目录
```
a.在rktoolbox下创建对应的目录并把bin拷贝到目录下
b.根据情况是否添加转接模块以便适配统一接口
c.修改tools.h添加模块映射表信息
d.编写Android.mk和rktoolbox一起编译，编译出的模块名字需要添加 rk_ 前缀
```
