#include <kernel/global.h>
#include <task/task.h>
#include <service.h>

PRIVATE pid_t service_pid_table[SERVICES];

PUBLIC bool is_service_id(uint32_t sid)
{
    return sid >= SERVICE_ID_BASE && sid < SERVICE_ID_BASE + SERVICES;
}

PUBLIC pid_t service_id_to_pid(uint32_t sid)
{
    if (is_service_id(sid))
    {
        return service_pid_table[sid - SERVICE_ID_BASE];
    }
    return MAX_TASK;
}

PUBLIC void tick_main();
PUBLIC void mm_main();
PUBLIC void view_main();

PUBLIC void service_init()
{
    service_pid_table[TICK - SERVICE_ID_BASE] = prog_execute(tick_main,"TICK",65536)->pid;
    service_pid_table[MM   - SERVICE_ID_BASE] = task_start("MM",DEFAULT_PRIORITY,65536,mm_main,0)->pid;
    service_pid_table[VIEW - SERVICE_ID_BASE] = prog_execute(view_main,"VIEW",65536)->pid;
    return;
}