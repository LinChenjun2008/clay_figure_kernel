#include <kernel/global.h>
#include <kernel/syscall.h>
#include <service.h>

#include <log.h>

PRIVATE uint64_t tick = 0;

PUBLIC void tick_main()
{
    tick = 0;
    message_t msg;
    while(1)
    {
        send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch (msg.type)
        {
            case RECV_FROM_INT:
                tick += msg.m1.i1;
                break;
            case TICK_GET_TICKS:
                msg.m3.l1 = tick;
                send_recv(NR_SEND,msg.src,&msg);
            default:
                break;
        }
    }
}