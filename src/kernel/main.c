#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <device/cpu.h>
#include <service.h>

#include <log.h>

#include <ulib.h>

boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

void ktask()
{
    uint32_t color = 0x00000000;
    uint32_t xsize = 10;
    uint32_t ysize = 10;
    uint32_t *buf = allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    while(1)
    {
        fill(buf,xsize * ysize * sizeof(uint32_t),xsize,ysize,apic_id() * 10,0);
        color = color ? color << 8 : 0xff;
        uint32_t x,y;
        for(y = 0;y < ysize;y++)
        {
            for (x = 0;x < xsize;x++)
            {
                *(buf + y * xsize + x) = color;
            }
        }
    };
}

void kernel_main()
{
    global_log_cnt = 0;
    pr_log("\1 Kernel initializing.\n");
    init_all();
    pr_log("\1 Kernel initializing done.\n");
    char s[64];
    cpu_name(s);
    pr_log("\1 CPU: %s.\n",s);
    prog_execute("k task",DEFAULT_PRIORITY,4096,ktask);
    while(1)
    {
        __asm__ ("sti\n\t""hlt");
    };
}

void ap_kernel_main()
{
    ap_init_all();
    char name[31];
    sprintf(name,"k task %d",apic_id());
    prog_execute(name,DEFAULT_PRIORITY,4096,ktask);
    while(1)
    {
        __asm__ ("sti\n\t""hlt");
    };
}