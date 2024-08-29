#include <kernel/global.h>
#include <device/keyboard/ps2_keyboard.h>
#include <kernel/syscall.h> // send_recv
#include <lib/fifo.h>       // fifo functions
#include <std/string.h>     // memset

#include <log.h>

PRIVATE uint8_t key_buf[64];
PUBLIC fifo_t key_fifo;

PUBLIC void keyboard_main()
{
    memset(key_buf,0,64);
    init_fifo(&key_fifo,key_buf,8,64);
    ps2_keyboard_init();
    message_t msg;
    while (1)
    {
        sys_send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch (msg.type)
        {
            case RECV_FROM_INT:
                char c;
                while (!fifo_empty(&key_fifo))
                {
                    fifo_get(&key_fifo,&c);
                    pr_log("%c",c);
                }
                break;
            default:
                break;
        }
    }
}