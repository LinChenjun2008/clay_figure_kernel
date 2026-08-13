# Clay-Figure Neo

一个可以在带有UEFI的x86-64平台上运行的简易操作系统

## 编译和运行

编译本项目需要以下工具:

- make
- gcc-mingw-64
- gcc

运行本项目需要以下工具:

- qemu
- OVMF

以Debian/Ubuntu为例,使用以下命令安装所需的工具:
```bash
sudo apt-get install gcc gcc-mingw-w64-x86-64 qemu-system-x86 ovmf
```

初次编译时,使用`make init`初始化编译环境.随后通过`make all`编译项目.如果需要在虚拟机中运行,可以使用`make run`命令,在编译完成后会启动qemu虚拟机运行本项目.

在编译完成后,可以在`build/esp/`目录下找到编译后的程序.

如果要在实体机运行,请准备一个空闲的U盘并格式化为`FAT`文件系统,并将`build/esp/`下的所有文件复制到U盘根目录中.随后从U盘启动即可.

## 功能

- [x] UEFI引导程序
- [x] APIC - 高级可编程中断控制器
- [x] HPET - 高精度事件定时器
- [x] SMP - 对称多处理器

## 许可协议
本项目使用GPL-3.0 许可协议.
```
Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.
```

## 参考资料

本项目参考了以下书籍/项目:
* 郑刚.操作系统真相还原.北京:人民邮电出版社,2016.
* 川和秀実.30天自制操作系统.周自恒,李黎明,曾祥江,张文旭 译.北京:人民邮电出版社,2012.
* 于渊.Orange'S: 一个操作系统的实现.北京:电子工业出版社,2009.
* 大神 祐真.[フルスクラッチで作る!UEFIベアメタルプログラミング](https://kagurazakakotori.github.io/ubmp-cn/).神楽坂琴梨 译.
* [Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
* [osdev](https://wiki.osdev.org)
* [UEFI Spec 2.9](https://uefi.org/)
* [eXtensible Host Controller Interface for Universal Serial Bus](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)
* [Haiku](https://github.com/haiku/haiku)
* [SeaBIOS](https://github.com/coreboot/seabios.git)

---

Copyright &copy; 2026 Clay-Figure-Neo Developers

(本文档更新日期: 2026年8月13日)
