# 系统调用

clay figure kernel采用微内核设计,所有系统调用在下方的表格(表1)中列出,

对于内核进程,系统调用应通过`sys_send_recv`函数进行.对于用户进程,系统调用应使用`send_recv`函数进行.
无论何种方式进行系统调用,函数的参数均为`(uint32_t nr,pid_t src_dest,message_t *msg)`.
有关函数参数的说明,见下方表2.

表1:
调用号 | 说明
------|-----
NR_SEND | 发送一个消息到指定进程
NR_RECV | 从指定进程/中断/任意进程接收一个消息
NR_BOTH | 依次执行`NR_SEND`与`NR_RECV`

表2:
nr | src_dest | msg
---|----------|----
表1中显示的系统调用号 | 发送消息的目的地或接收消息的来源 | 指向消息结构体的指针

执行系统调用的程序应保证msg指向一个有效的message_t结构,否则可能出现意料之外的结果.

若系统调用执行成功,将返回`SYSCALL_SUCCESS`(0x80000000),否则,返回相应的错误码.
下表显示了这些错误码及其含义:
错误码 | 值 | 产生原因
------|----|--------
SYSCALL_NO_SYSCALL | 0xc0000001 | 不是一个系统调用
SYSCALL_DEADLOCK | 0xc0000002 | 将会发生死锁
SYSCALL_DEST_NOT_EXIST | 0xc0000003 | 发送消息的目的地不存在
SYSCALL_SRC_NOT_EXIST  | 0xc0000004 | 接收消息的来源不存在

[返回](../index.md)