# 代码规范
* 此处的示例代码仅作为演示,非项目实际代码.
### 头文件
1. #define 保护
本项目中的所有头文件应使用`#define`防止重复包含,而不是`#pragma once`.格式为`__<FILE>_H__`.
* 假设存在名为`debug.h`的文件,则按如下方式保护:
```c
#ifndef __DEBUG_H__
#define __DEBUG_H__

...

#endif
```
2. 头文件的依赖
应尽量避免在头文件中使用`#include`,如果要在头文件中使用类型的声明,但不需要使用类型的定义,则应在头文件中使用前置声明.例如,在头文件`a.h`中需要使用`b.h`中定义的结构`base`,则在`a.h`文件中前置声明`struct base`即可,无需`#include <b.h>`.
在源文件中使用`#include`时,应尽量避免包含无用的头文件.

### 命名约定
1. 一般原则
名称应尽量清晰,不引起歧义.对于部分专有名词可保留原有的大小写,否则应按本规范命名.

好的命名应给出详细的名称,以便让代码易于理解:
```c
int number_of_tasks; // Good
int global_ticks;    // Good
```

应尽可能给出完整的名称而不是使用缩写,除非缩写的含义在项目外依旧清晰:

```c
// Good

int i; // Most people know the variable 'i' used for.
for (i = 0;i < 10;i++)
{
    ...
}

// Bad
int asdjkhas; // Nobody know what this stand for.
```
2. 文件命名
文件名称使用小写,可以包含数字和下划线`_`.源文件和头文件的名称应成对出现,例如`file.c`对应`file.h`.

3. 类型命名
类型名由小写字母和下划线组成,`struct`,`enum`,`union`类型名应分别以`_s`,`_e`,`_u`结尾,由`typedef`定义的类型以`_t`结尾:
```c
typedef int pid_t;

typedef struct base_s
{
    ...
} base_t;
```

4. 变量命名
变量名由小写字母和下划线组成,对于全局变量,可使用`g_`做为前缀.

5. 函数命名
函数名由小写字母和下划线组成.函数名应明确表示函数的功能.

6. 常量命名
常量名由大写字母和下划线组成.此处的`常量`指宏常量,枚举值等无法被改变的值,带有`const`的变量以变量命名为准.


7. 宏
宏名由大写字母和下划线组成.

8. 例外的情况:
* 在相关标准文档中有专有名称的,以对应标准中使用的名称命名.

### 代码注释
1. 注释风格
代码注释可使用`/* */`或`//`,以实际情况决定.

2. 函数注释
在函数声明处的注释应描述函数的功能,用法等,包含以下内容:
* 函数的功能.
* 函数的参数,以及参数的取值范围.
* 函数的返回值及其含义.
在函数定义处的注释应描述函数的实现细节

3. 变量注释
一般情况下,变量名已经很好的体现了变量的用途,因此无需添加注释.如果变量名无法体现变量的作用,请考虑重新命名.

### 格式
1. 行长度限制
每一行的字符数不应超过80.以下情况例外:
* 注释包含的URL或命令.
* 较长的文件路径.
* 用于头文件保护的宏定义.

2. 编码
使用utf-8编码保存文件.

3. 缩进
除非必须使用`tab`,否则一律使用空格进行缩进.每次缩进4个空格.不要在代码文件中使用`tab`.

4. 函数的声明和定义
返回类型,函数名,参数在同一行:
```c
return_type_t function_name(type arg1,type arg2)
{
    ...
}
```
如果函数名较长,使函数声明超出80字符限制,则将参数单独放置一行,参数前添加缩进:
```c
return_type_t long_long_long_long_long_function_name(
    type arg1,
    type arg2,
    type arg3)
{
    ...
}
```
注意:
* 返回类型总是与函数名在同一行.
* 左括号`(`总是紧跟在函数名后,与函数名同一行.
* 右括号`)`总是紧跟在最后一个参数后,与最后一个参数间没有空格
* 大括号`{`,`}`总是独占一行,对应的大括号的缩进相同.
* 若函数执行可能失败,则返回值应表示函数执行状态,函数输出应使用指针.

5. 函数调用
函数调用应在一行完成:
```c
    return_type_t return_value = function(arg1,arg2);
```
函数参数较多,可以每行一个参数,每一行的参数与第一个参数对齐:
```c
    return_type_t return_value = function(arg1,
                                          arg2,
                                          arg3,
                                          arg4,
                                          arg5,
                                          arg6);
```
函数名过长导致超出80字符限制,则将所有参数独立成行,参数比函数名多一次缩进:
```c
    return_type_t return_value = long_long_long_long_long_function_name(
                                    type arg1,
                                    type arg2,
                                    type arg3);

    long_long_long_long_long_function_name(
        type arg1,
        type arg2,
        type arg3);
```

6. 条件语句
`if`,`else if`语句与括号`(`之间总是存在空格.所有大括号`{` `}`独占一行:
```c
    if (condition)
    {
        ...
    }
    else if (condition)
    {
        ...
    }
    else
    {
        ...
    }
```
条件语句较短且没有else分支时可写在同一行:
```c
    if (condition) { return x;}

    // Not allowed:
    if (condition) { return x;}
    else return y;
```
即使只有一条语句,也要添加大括号`{` `}`

7. 选择语句
`switch`与左括号`(`之间总有一个空格,大括号独占一行.
`defaule`总是在最后出现:
```c
    switch (var)
    {
        case 1:
            ...
            break;
        case 2:
            ...
            break;
        default:
            ...
            break;
    }
```

8. 循环语句
`while`,`for`与左括号`(`之间总有一个空格,大括号独占一行,当语句较短时可写在同一行.
空循环应添加大括号`{}`或`continue`,而不是只写一个分号:
```c
    // Good
    for (;;)
    {

    }
    while (1) {}
    while (1) continue;

    // Bad
    while (1);
```
`do while`语句中,`while`可跟在右大括号后,之间相隔一个空格:
```c
    do
    {
        ...
    } while (condition)
```
9. 指针和成员变量
取地址`&`,解引用`*`,访问成员变量`.`,`->`前后没有空格.
声明指针变量或参数时,`*`总是靠近名称,且与名称间没有空格.

10. 逻辑表达式
逻辑运算若超出80字符限制,则进行断行,逻辑运算符位于行首:
```c
    if (condition1
        && condition2
        || condition3)
```

11. 预处理命令
预处理命令没有缩进,所有预处理命令都要从行首开始.

12. 对齐
当有多行相似的语句时,可以考虑对其以提高可读性,以结构体为例:
``` c
struct
{
    type_name       var1;
    long_type_name  var2;
    int            *var3;
}
```
成员变量类型与名称分别对齐,对其应从名称的第一个字母开始,`*`不算在内.
应确成员变量类型部分的最右侧与名称部分的最左侧只有一个空格,在此处的例子中,成员变量类型部分的最右侧为`long_type_name`右侧,名称部分的最左侧为`*var3`的左侧,两者之间存在一个空格.

### 注意
1. 尽量不要使用非标准代码.
2. 避免滥用特定编译器的拓展功能.