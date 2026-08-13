# Clay-Figure Neo

A simple operating system that can run on x86-64 platforms with UEFI support.

## Build & Run

To compile this project you will need the following tools:

- make
- gcc-mingw-64
- gcc

To run this project you will need the following tools:

- qemu
- OVMF

Taking Debian/Ubuntu as an example, install the required tools with the
following command:

```bash
sudo apt-get install gcc gcc-mingw-w64-x86-64 qemu-system-x86 ovmf
```

On the first build, use `make init` to initialize the build environment, then
use `make all` to compile the project. If you want to run it in a virtual
machine, you can use the `make run` command, which will start a QEMU virtual
machine to run this project after the compilation finishes.

After compilation, you can find the compiled programs in the `build/esp/`
directory.

To run on real hardware, prepare a spare USB drive, format it with a `FAT`
file system, and copy all the files under `build/esp/` to the root directory
of the USB drive. Then simply boot from the USB drive.

## Features

- [x] UEFI bootloader
- [x] APIC - Advanced Programmable Interrupt Controller
- [x] HPET - High Precision Event Timer
- [x] SMP - Symmetric Multi-Processor

## License

This project is licensed under the GPL-3.0 License.

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

## References

This project references the following books/projects:

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

(Last updated: August 13, 2026)