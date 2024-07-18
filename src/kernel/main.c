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

extern uint64_t global_ticks;

void ktask()
{
    uint32_t color = 0x00000000;
    uint32_t xsize = 20;
    uint32_t ysize = 20;
    uint32_t *buf = allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    while(1)
    {
        fill(buf,xsize * ysize * sizeof(uint32_t),xsize,ysize,apic_id() * 20,0);
        color += 100;
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
    prog_execute("k task2",DEFAULT_PRIORITY,4096,ktask);
    while(1)
    {
        __asm__ ("sti\n\t""hlt");
    };
}