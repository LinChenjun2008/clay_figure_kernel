/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
*/
#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <device/cpu.h>
#include <service.h>
#include <std/stdio.h>

#include <log.h>

#include <ulib.h>

PUBLIC boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

PRIVATE void ktask()
{
    uint32_t color = 0x00000000;
    uint32_t xsize = 10;
    uint32_t ysize = 10;
    uint32_t *buf = allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    while (1)
    {
        fill(buf,xsize * ysize * sizeof(uint32_t),xsize,ysize,apic_id() * 10,0);
        color = color ? color << 8 : 0xff;
        uint32_t x,y;
        for (y = 0;y < ysize;y++)
        {
            for (x = 0;x < xsize;x++)
            {
                *(buf + y * xsize + x) = color;
            }
        }
    };
}

extern taskmgr_t *tm;
PRIVATE bool print_task_vrun_time(list_node_t *node,uint64_t arg)
{
    task_struct_t *task = CONTAINER_OF(task_struct_t,general_tag,node);
    if (task != NULL)
    {
        pr_log("| %s: %d",task->name,task->vrun_time);
    }
    return arg;
}
PRIVATE void print_vrun_time()
{
    pr_log("\1");
    list_traversal(&tm->core[0].task_list,print_task_vrun_time,0);
    pr_log(" | \n");
}

extern volatile uint64_t global_ticks;

PRIVATE void print_ticks()
{
    while(1)
    {
        uint64_t ticks = global_ticks + 1000;
        while (ticks > global_ticks);
        print_vrun_time();
        __asm__ ("sti\n\t""hlt");
    };
}

PUBLIC void kernel_main()
{
    pr_log(K_NAME " - " K_VERSION "\n");
    init_all();
    prog_execute("k task",DEFAULT_PRIORITY,4096,ktask);
    task_start("vrtime",DEFAULT_PRIORITY,4096,print_ticks,0);
    while(1)
    {
        task_block(TASK_BLOCKED);
        __asm__ ("sti\n\t""hlt");
    };
}

PUBLIC void ap_kernel_main()
{
    ap_init_all();
    char name[31];
    sprintf(name,"k task %d",apic_id());
    uint64_t ticks = global_ticks + 1000 * apic_id();
    while (global_ticks > ticks) continue;
    prog_execute(name,DEFAULT_PRIORITY,4096,ktask);
    while (1)
    {
        task_block(TASK_BLOCKED);
        __asm__ ("sti\n\t""hlt");
    };
}