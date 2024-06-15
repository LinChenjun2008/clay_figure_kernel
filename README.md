# Clay Figure Kernel
# 目录
* [编译工具](#编译工具)
* [编译](#编译)
* [运行](#运行)
* [参考资料](#参考资料)

# 编译工具
在Linux上，你可以使用下方的命令安装所需程序：
```sh
sudo apt-get install gcc g++ gcc-mingw-w64-x86_64 qemu-system-x86 ovmf
```
在windows上，从以下网站下载所需的工具:

* [x86_64-elf-tools](https://github.com/lordmilko/i686-elf-tools/releases)
* [Mingw64](https://www.mingw-w64.org/)
* [Qemu](https://www.qemu.org)

或者下载精简过的[BuildTools](https://gitee.com/linchenjun2008/build_tools)(请将其中所有的压缩文件解压缩)。

 * 注：以下部分均假设在Windows环境下进行。
# 编译

1. 获取源代码：
```bash
mkdir project
cd ./project
git clone https://github.com/linchenjun2008/clay_figure_kernel.git
```
2. 准备[工具](#编译工具)，并确保目录结构是下面这样的：
```
project
    ├─clay_figure_kernel
    │  ├─.vscode
    │  ├─build
    │  ├─docs
    │  └─src
    │      └─...
    ├─run
    │  └─esp
    │      ├─EFI
    │      │  └─Boot
    │      └─Kernel
    │          └─resource
    └─tools
        ├─MinGW64
        │  ├─bin
        │  ├─include
        │  ├─lib
        │  ├─libexec
        │  ├─share
        │  └─x86_64-w64-mingw32
        ├─qemu
        │  └─...
        └─x86_64-elf-tools
            ├─bin
            ├─include
            ├─libexec
            ├─share
            └─x86_64-elf
```
如果你将编译工具（`tools`）安装在了其他位置，请编辑`build/tools_def.txt`中的工具路径，将其设为实际的安装位置。

3. 进入`build`目录,开始编译:
```bash
cd ./clay_figure_kernel/build/
make
```
如果一切顺利，你将在`run/esp`中找到编译结果。

# 运行
进入`build`目录,执行`make run`:
```bash
cd ./clay_figure_kernel/build/
make run
```
若要在物理机上运行，只需将`run/esp`中的文件复制到用于启动的磁盘，将该磁盘作为引导设备启动即可（请确定该磁盘为`FAT`类的文件系统）。

# 参考资料
* 郑刚.操作系统真相还原.北京:人民邮电出版社,2016.
* 川和秀实.30天自制操作系统.周自恒,李黎明,曾祥江,张文旭 译.北京:人民邮电出版社,2012.
* 于渊.Orange'S: 一个操作系统的实现.北京:电子工业出版社,2009.
* 大神 祐真.[フルスクラッチで作る!UEFIベアメタルプログラミング](https://kagurazakakotori.github.io/ubmp-cn/).神楽坂琴梨 译.
* [osdev](https://wiki.osdev.org)
* [GuideOS](https://github.com/Codetector1374/GuideOS)
* eXtensible Host Controller Interface for Universal Serial Bus