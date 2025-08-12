# Clay Figure Kernel

# 目录
* [编译工具](#编译工具)
* [编译](#编译)
* [运行](#运行)

# 编译工具
在Linux上，你可以使用下方的命令安装所需程序：
```sh
sudo apt-get install gcc g++ gcc-mingw-w64-x86-64 qemu-system-x86 ovmf
```
在windows上，从以下网站下载所需的工具[^windows_tools]:

* [x86_64-elf-tools-windows](https://github.com/lordmilko/i686-elf-tools/releases)
* [Mingw64](https://www.mingw-w64.org/)
* [Qemu](https://www.qemu.org)

# 编译
1. 获取源代码
``` bash
git clone https://github.com/linchenjun2008/clay_figure_kernel.git
```

2. 准备所需的[编译工具](#编译工具)

编辑`scripts/target.mk`，将其中的`ESP_DIR`设为一个正确的路径，用于保存编译结果。

进入`tools`目录，编译其中的`kallsyms.c`和`imgcopy.c`
```bash
mkdir build
gcc tools/kallsyms.c -o build/kallsyms
gcc tools/imgcopy.c -o build/imgcopy
```

3. 开始编译

首次编译先执行`make init`，完成后执行`make`开始编译。
```bash
make init
make all
```
如果一切顺利，你将在`ESP_DIR`对应的路径中找到编译结果。

# 运行

执行`make run`，将通过qemu模拟器运行:
```bash
make run
```
若要在物理机上运行，只需将`ESP_DIR`对应的路径中的文件复制到用于启动的磁盘，将该磁盘作为引导设备启动即可（请确定该磁盘为`FAT`类的文件系统）。

[^windows_tools]: 需要修改`scripts/tools_def.mk`使脚本能够正确运行工具,或者按照`scripts/tools_def.mk`中的描述安装工具