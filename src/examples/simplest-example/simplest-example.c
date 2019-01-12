/*
** simplest-example.c
**
** This is an example OSAL Application. This Application simply creates one task.
*/

#include <stdio.h>
#include "common_types.h"
#include "osapi.h"


/* Task */

#define TASK_ID         1
#define TASK_STACK_SIZE 1024
#define TASK_PRIORITY   101

uint32 task_stack[TASK_STACK_SIZE];

void task(void);

uint32 task_id;

/* ********************** MAIN **************************** */

void OS_Application_Startup(void)
{
  uint32 status;

  OS_API_Init();

  OS_printf("********If You see this, we got into OS_Application_Startup****\n");

  status = OS_TaskCreate( &task_id, "Task", task, task_stack, TASK_STACK_SIZE, TASK_PRIORITY, 0);
  if ( status != OS_SUCCESS )
  {
     OS_printf("Error creating Task\n");
  }
}

/* ********************** TASK **************************** */

void task(void)
{
    OS_printf("Starting task\n");

    OS_TaskRegister();

    while(1)
    {
    }
}
