/*
** File   : osapi.c
*/

/****************************************************************************************
                                    INCLUDE FILES
****************************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <string.h>

/*
** FreeRTOS includes
*/
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <event_groups.h>

/*
** User defined include files
*/
#include "common_types.h"
#include "osapi.h"

/*
** Defines
*/
#define UNINITIALIZED 0
#define MAX_PRIORITY 255

/* depends on FreeRTOS "UBaseType_t" max value, 255 is the worst case */
#define SEM_VALUE_MAX 255

#undef OS_DEBUG_PRINTF 

#define OS_SHUTDOWN_MAGIC_NUMBER    0xABADC0DE

#define egBIN_SEM_STATE_BIT    (0x01)
#define egBIN_SEM_FLUSH_BIT    (0x02)

/*
** Global data for the API
*/

/*  
** Tables for the properties of objects 
*/

/* tasks */
typedef struct
{
    int              free;
    TaskHandle_t     task_handle;
    char             name [OS_MAX_API_NAME];
    int              creator;
    uint32           stack_size;
    uint32           priority;
    osal_task_entry  delete_hook_pointer;
}OS_task_internal_record_t;

/* queues */
typedef struct
{
    int            free;
    QueueHandle_t  queue_handle;
    uint32         max_size;
    char           name [OS_MAX_API_NAME];
    int            creator;
}OS_queue_internal_record_t;

/* Binary Semaphores */
typedef struct
{
    int                 free;
    EventGroupHandle_t  event_group_handle;
    char                name [OS_MAX_API_NAME];
    int                 creator;
    int                 max_value;
    int                 current_value;
}OS_bin_sem_internal_record_t;

/* Counting Semaphores */
typedef struct
{
    int                free;
    SemaphoreHandle_t  count_sem_handle;
    char               name [OS_MAX_API_NAME];
    int                creator;
}OS_count_sem_internal_record_t;

/* Mutexes */
typedef struct
{
    int                free;
    SemaphoreHandle_t  mut_sem_handle;
    char               name [OS_MAX_API_NAME];
    int                creator;
}OS_mut_sem_internal_record_t;

/* function pointer type */
typedef void (*FuncPtr_t)(void);

/* Tables where the OS object information is stored */
OS_task_internal_record_t       OS_task_table          [OS_MAX_TASKS];
OS_queue_internal_record_t      OS_queue_table         [OS_MAX_QUEUES];
OS_bin_sem_internal_record_t    OS_bin_sem_table       [OS_MAX_BIN_SEMAPHORES];
OS_count_sem_internal_record_t  OS_count_sem_table     [OS_MAX_COUNT_SEMAPHORES];
OS_mut_sem_internal_record_t    OS_mut_sem_table       [OS_MAX_MUTEXES];

SemaphoreHandle_t OS_task_table_mut;
SemaphoreHandle_t OS_queue_table_mut;
SemaphoreHandle_t OS_bin_sem_table_mut;
SemaphoreHandle_t OS_count_sem_table_mut;
SemaphoreHandle_t OS_mut_sem_table_mut;

static SemaphoreHandle_t idle_sem_id;

uint32          OS_printf_enabled = TRUE;
volatile uint32 OS_shutdown = FALSE;

/*
** Local Function Prototypes
*/
void    OS_ThreadKillHandler(int sig );
uint32  OS_FindCreator(void);

static void root_task( void *pvParameters );

int32 OS_TableLock(SemaphoreHandle_t mutex, TickType_t wait_time);
int32 OS_TableUnlock(SemaphoreHandle_t mutex);

/*
** Static Function Pointers
*/
static void (*root_func_p)(void);

/*---------------------------------------------------------------------------------------
   Name: OS_API_Init

   Purpose: Initialize the tables that the OS API uses to keep track of information
            about objects

   returns: OS_SUCCESS or OS_ERROR
---------------------------------------------------------------------------------------*/
int32 OS_API_Init(void)
{
   int   i;
   int32 return_code = OS_SUCCESS;

   /* Initialize Task Table */
   for(i = 0; i < OS_MAX_TASKS; i++)
   {
        OS_task_table[i].free                = TRUE;
        OS_task_table[i].creator             = UNINITIALIZED;
        OS_task_table[i].delete_hook_pointer = NULL;
        strcpy(OS_task_table[i].name, "");
    }

    /* Initialize Message Queue Table */
    for(i = 0; i < OS_MAX_QUEUES; i++)
    {
        OS_queue_table[i].free        = TRUE;
        OS_queue_table[i].creator     = UNINITIALIZED;
        strcpy(OS_queue_table[i].name, "");
    }

    /* Initialize Binary Semaphore Table */
    for(i = 0; i < OS_MAX_BIN_SEMAPHORES; i++)
    {
        OS_bin_sem_table[i].free          = TRUE;
        OS_bin_sem_table[i].creator       = UNINITIALIZED;
        strcpy(OS_bin_sem_table[i].name, "");
    }

    /* Initialize Counting Semaphore Table */
    for(i = 0; i < OS_MAX_COUNT_SEMAPHORES; i++)
    {
        OS_count_sem_table[i].free        = TRUE;
        OS_count_sem_table[i].creator     = UNINITIALIZED;
        strcpy(OS_count_sem_table[i].name, "");
    }

    /* Initialize Mutex Semaphore Table */
    for(i = 0; i < OS_MAX_MUTEXES; i++)
    {
        OS_mut_sem_table[i].free        = TRUE;
        OS_mut_sem_table[i].creator     = UNINITIALIZED;
        strcpy(OS_mut_sem_table[i].name, "");
    }

   /*
   ** Initialize the module loader
   */
   #ifdef OS_INCLUDE_MODULE_LOADER
     /* Not implemented */
   #endif

   /*
   ** Initialize the Timer API
   */
   return_code = OS_TimerAPIInit();
   if ( return_code == OS_ERROR )
   {
      return(return_code);
   }

   /*
   ** Initialize the internal table Mutexes
   */

   OS_task_table_mut = xSemaphoreCreateMutex();
   if( OS_task_table_mut == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

   OS_queue_table_mut = xSemaphoreCreateMutex();
   if( OS_queue_table_mut == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

   OS_bin_sem_table_mut = xSemaphoreCreateMutex();
   if( OS_bin_sem_table_mut == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

   OS_count_sem_table_mut = xSemaphoreCreateMutex();
   if( OS_count_sem_table == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

   OS_mut_sem_table_mut = xSemaphoreCreateMutex();
   if( OS_mut_sem_table == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

   /*
   ** File system init
   */
   /* Not implemented */

   idle_sem_id = xSemaphoreCreateBinary();
   if( idle_sem_id == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

   return_code = OS_SUCCESS;

   return(return_code);
}

/*---------------------------------------------------------------------------------------
   Name: OS_ApplicationExit

   Purpose: Indicates that the OSAL application should exit and return control to the OS
         This is intended for e.g. scripted unit testing where the test needs to end
         without user intervention.  This function does not return.

    NOTES: This exits the entire process including tasks that have been created.
       It does not return.  Production embedded code typically would not ever call this.

---------------------------------------------------------------------------------------*/
void OS_ApplicationExit(int32 Status)
{
   /* vTaskEndScheduler() implemented only for x86 PC port */
   vTaskEndScheduler();

   if (Status == OS_SUCCESS)
   {
      exit(EXIT_SUCCESS);
   }
   else
   {
      exit(EXIT_FAILURE);
   }
}

/*---------------------------------------------------------------------------------------
   Name: OS_DeleteAllObjects

   Purpose: This task will delete all objects allocated by this instance of OSAL
            May be used during shutdown or by the unit tests to purge all state

   returns: no value
---------------------------------------------------------------------------------------*/
void OS_DeleteAllObjects       (void)
{
    uint32 i;

    for (i = 0; i < OS_MAX_TASKS; ++i)
    {
        OS_TaskDelete(i);
    }
    for (i = 0; i < OS_MAX_QUEUES; ++i)
    {
        OS_QueueDelete(i);
    }
    for (i = 0; i < OS_MAX_MUTEXES; ++i)
    {
        OS_MutSemDelete(i);
    }
    for (i = 0; i < OS_MAX_COUNT_SEMAPHORES; ++i)
    {
        OS_CountSemDelete(i);
    }
    for (i = 0; i < OS_MAX_BIN_SEMAPHORES; ++i)
    {
        OS_BinSemDelete(i);
    }
    for (i = 0; i < OS_MAX_TIMERS; ++i)
    {
        OS_TimerDelete(i);
    }
#if 0  /* needs to be implemented */
    for (i = 0; i < OS_MAX_MODULES; ++i)
    {
        OS_ModuleUnload(i);
    }
    for (i = 0; i < OS_MAX_NUM_OPEN_FILES; ++i)
    {
        OS_close(i);
    }
#endif
}


/*---------------------------------------------------------------------------------------
   Name: OS_IdleLoop

   Purpose: Should be called after all initialization is done
            This thread may be used to wait for and handle external events
            Typically just waits forever until "OS_shutdown" flag becomes true.

   returns: no value
---------------------------------------------------------------------------------------*/
void OS_IdleLoop()
{
   xSemaphoreTake( idle_sem_id, portMAX_DELAY );
}


/*---------------------------------------------------------------------------------------
   Name: OS_ApplicationShutdown

   Purpose: Indicates that the OSAL application should perform an orderly shutdown
       of ALL tasks, clean up all resources, and exit the application.

   returns: none

---------------------------------------------------------------------------------------*/
void OS_ApplicationShutdown(uint8 flag)
{
   xSemaphoreGive( idle_sem_id );
}



/*
**********************************************************************************
**          TASK API
**********************************************************************************
*/

/*---------------------------------------------------------------------------------------
   Name: OS_TaskCreate

   Purpose: Creates a task and starts running it.

   returns: OS_INVALID_POINTER if any of the necessary pointers are NULL
            OS_ERR_NAME_TOO_LONG if the name of the task is too long to be copied
            OS_ERR_INVALID_PRIORITY if the priority is bad
            OS_ERR_NO_FREE_IDS if there can be no more tasks created
            OS_ERR_NAME_TAKEN if the name specified is already used by a task
            OS_ERROR if the operating system calls fail
            OS_SUCCESS if success
            
    NOTES: task_id is passed back to the user as the ID. stack_pointer is usually null.
           the flags parameter is unused.

---------------------------------------------------------------------------------------*/
int32 OS_TaskCreate (uint32 *task_id, const char *task_name, osal_task_entry function_pointer,
                      uint32 *stack_pointer, uint32 stack_size, uint32 priority,
                      uint32 flags)
{
    int                possible_taskid;
    int                i;
    BaseType_t         task_status;
    TaskHandle_t       task_handle;

    /* Check for NULL pointers */    
    if( (task_name == NULL) || (function_pointer == NULL) || (task_id == NULL) )
    {
        return OS_INVALID_POINTER;
    }
    
    /* we don't want to allow names too long*/
    /* if truncated, two names might be the same */
    if (strlen(task_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    /* Check for bad priority */
    if (priority > MAX_PRIORITY)
    {
        return OS_ERR_INVALID_PRIORITY;
    }

    /* Check Parameters */
    OS_TableLock( OS_task_table_mut, portMAX_DELAY );

    for(possible_taskid = 0; possible_taskid < OS_MAX_TASKS; possible_taskid++)
    {
        if (OS_task_table[possible_taskid].free == TRUE)
        {
            break;
        }
    }

    /* Check to see if the id is out of bounds */
    if( possible_taskid >= OS_MAX_TASKS || OS_task_table[possible_taskid].free != TRUE)
    {
        OS_TableUnlock( OS_task_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /* Check to see if the name is already taken */ 
    for (i = 0; i < OS_MAX_TASKS; i++)
    {
        if ((OS_task_table[i].free == FALSE) &&
           ( strcmp((char*) task_name, OS_task_table[i].name) == 0)) 
        {
            OS_TableUnlock( OS_task_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }
    
    /* 
    ** Set the possible task Id to not free so that
    ** no other task can try to use it 
    */
    OS_task_table[possible_taskid].free = FALSE;

    OS_TableUnlock( OS_task_table_mut );

    task_status = xTaskCreate( (TaskFunction_t) function_pointer, 
                             (char*)task_name, 
                             stack_size, 
                             NULL, 
                             priority, 
                             &task_handle );

    /*
    ** Lock
    */
    OS_TableLock( OS_task_table_mut, portMAX_DELAY );

    /* check if xTaskCreate failed */
    if (task_status != pdPASS)
    {
      OS_task_table[possible_taskid].free = TRUE;
      OS_TableUnlock( OS_task_table_mut );
      return OS_ERROR;
    }

    /* Set the task_id to the id that was found available
       Set the name of the task, the stack size, and priority */
    *task_id = possible_taskid;

    /* this Id no longer free */
    OS_task_table[*task_id].task_handle = task_handle;

    strcpy(OS_task_table[*task_id].name, task_name);
    OS_task_table[*task_id].free = FALSE;
    OS_task_table[*task_id].creator = OS_FindCreator();
    OS_task_table[*task_id].stack_size = stack_size;
    OS_task_table[*task_id].priority = priority;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_task_table_mut );

    return OS_SUCCESS;
}/* end OS_TaskCreate */


/*--------------------------------------------------------------------------------------
     Name: OS_TaskDelete

    Purpose: Deletes the specified Task and removes it from the OS_task_table.

    returns: OS_ERR_INVALID_ID if the ID given to it is invalid
             OS_ERROR if the OS delete call fails
             OS_SUCCESS if success
---------------------------------------------------------------------------------------*/
int32 OS_TaskDelete (uint32 task_id)
{
    FuncPtr_t         FunctionPointer;

    /* 
    ** Check to see if the task_id given is valid 
    */
    if (task_id >= OS_MAX_TASKS || OS_task_table[task_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /*
    ** Call the task Delete hook if there is one.
    */
    if ( OS_task_table[task_id].delete_hook_pointer != NULL)
    {
       FunctionPointer = (FuncPtr_t)OS_task_table[task_id].delete_hook_pointer;
       (*FunctionPointer)();
    }

    /* Try to delete the task */
    vTaskDelete(OS_task_table[task_id].task_handle);

    /*
     * Now that the task is deleted, remove its 
     * "presence" in OS_task_table
    */
    OS_TableLock( OS_task_table_mut, portMAX_DELAY );
    OS_task_table[task_id].free = TRUE;
    OS_task_table[task_id].task_handle = UNINITIALIZED;
    OS_task_table[task_id].name[0] = '\0';
    OS_task_table[task_id].creator = UNINITIALIZED;
    OS_task_table[task_id].stack_size = UNINITIALIZED;
    OS_task_table[task_id].priority = UNINITIALIZED;
    OS_task_table[task_id].delete_hook_pointer = NULL;
    OS_TableUnlock( OS_task_table_mut );

    return OS_SUCCESS;

}/* end OS_TaskDelete */

/*--------------------------------------------------------------------------------------
     Name: OS_TaskExit

    Purpose: Exits the calling task and removes it from the OS_task_table.

    returns: Nothing 
---------------------------------------------------------------------------------------*/

void OS_TaskExit()
{
    uint32 task_id;

    task_id = OS_TaskGetId();

    /*
    ** Lock
    */
    OS_TableLock( OS_task_table_mut, portMAX_DELAY );

    OS_task_table[task_id].free = TRUE;
    OS_task_table[task_id].task_handle = UNINITIALIZED;
    strcpy(OS_task_table[task_id].name, "");
    OS_task_table[task_id].creator = UNINITIALIZED;
    OS_task_table[task_id].stack_size = UNINITIALIZED;
    OS_task_table[task_id].priority = UNINITIALIZED;
    OS_task_table[task_id].delete_hook_pointer = NULL;
    
    /*
    ** Unlock
    */
    OS_TableUnlock( OS_task_table_mut );

    vTaskDelete( NULL );

}/*end OS_TaskExit */
/*---------------------------------------------------------------------------------------
   Name: OS_TaskDelay

   Purpose: Delay a task for specified amount of milliseconds

   returns: OS_ERROR if sleep fails or millisecond = 0
            OS_SUCCESS if success
---------------------------------------------------------------------------------------*/
int32 OS_TaskDelay(uint32 millisecond )
{
   vTaskDelay( pdMS_TO_TICKS( millisecond ) );

   return OS_SUCCESS;
}/* end OS_TaskDelay */

/*---------------------------------------------------------------------------------------
   Name: OS_TaskSetPriority

   Purpose: Sets the given task to a new priority

    returns: OS_ERR_INVALID_ID if the ID passed to it is invalid
             OS_ERR_INVALID_PRIORITY if the priority is greater than the max 
             allowed
             OS_ERROR if the OS call to change the priority fails
             OS_SUCCESS if success
---------------------------------------------------------------------------------------*/
int32 OS_TaskSetPriority (uint32 task_id, uint32 new_priority)
{
    /* Check Parameters */
    if (task_id >= OS_MAX_TASKS || OS_task_table[task_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    if (new_priority > MAX_PRIORITY)
    {
        return OS_ERR_INVALID_PRIORITY;
    }

    /* Set FreeRTOS Task Priority */
    vTaskPrioritySet( OS_task_table[task_id].task_handle, new_priority );
    /* Change the priority in the table as well */
    OS_task_table[task_id].priority = new_priority;

    return OS_SUCCESS;

} /* end OS_TaskSetPriority */


/*---------------------------------------------------------------------------------------
   Name: OS_TaskRegister
  
   Purpose: Registers the calling task id with the task by adding the var to the tcb
            It searches the OS_task_table to find the task_id corresponding to the tcb_id
            
   Returns: OS_ERR_INVALID_ID if there the specified ID could not be found
            OS_ERROR if the OS call fails
            OS_SUCCESS if success
---------------------------------------------------------------------------------------*/
int32 OS_TaskRegister (void)
{
    int          i;
    uint32       task_id;
    TaskHandle_t task_handle;

    /* 
    ** Get current task handle
    */
    task_handle = xTaskGetCurrentTaskHandle();

    /*
    ** Look our task ID in table 
    */
    for(i = 0; i < OS_MAX_TASKS; i++)
    {
       if(OS_task_table[i].task_handle == task_handle)
       {
          break;
       }
    }
    task_id = i;

    if(task_id == OS_MAX_TASKS)
    {
        return OS_ERR_INVALID_ID;
    }

    /* save "task_id" as FreeRTOS thread local storage pointer */
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)task_id );

    return OS_SUCCESS;
}/* end OS_TaskRegister */

/*---------------------------------------------------------------------------------------
   Name: OS_TaskGetId

   Purpose: This function returns the #defined task id of the calling task

   Notes: The OS_task_key is initialized by the task switch if AND ONLY IF the 
          OS_task_key has been registered via OS_TaskRegister(..).  If this is not 
          called prior to this call, the value will be old and wrong.
---------------------------------------------------------------------------------------*/
uint32 OS_TaskGetId (void)
{
    /* obtain "task_id" from FreeRTOS local storage pointer */
    return (uint32)pvTaskGetThreadLocalStoragePointer( NULL, 0 );
}/* end OS_TaskGetId */

/*--------------------------------------------------------------------------------------
    Name: OS_TaskGetIdByName

    Purpose: This function tries to find a task Id given the name of a task

    Returns: OS_INVALID_POINTER if the pointers passed in are NULL
             OS_ERR_NAME_TOO_LONG if th ename to found is too long to begin with
             OS_ERR_NAME_NOT_FOUND if the name wasn't found in the table
             OS_SUCCESS if SUCCESS
---------------------------------------------------------------------------------------*/

int32 OS_TaskGetIdByName (uint32 *task_id, const char *task_name)
{
    uint32 i;

    /*
     * Note: This function accesses the OS_task_table without locking that table's
     * semaphore.
     */
    if (task_id == NULL || task_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* we don't want to allow names too long because they won't be found at all */
    if (strlen(task_name) >= OS_MAX_API_NAME)
    {
            return OS_ERR_NAME_TOO_LONG;
    }

    for (i = 0; i < OS_MAX_TASKS; i++)
    {
        if((OS_task_table[i].free != TRUE) &&
          (strcmp(OS_task_table[i].name, (char *)task_name) == 0 ))
        {
            *task_id = i;
            return OS_SUCCESS;
        }
    }
    /* The name was not found in the table,
     *  or it was, and the task_id isn't valid anymore */
    return OS_ERR_NAME_NOT_FOUND;

}/* end OS_TaskGetIdByName */

/*---------------------------------------------------------------------------------------
    Name: OS_TaskGetInfo

    Purpose: This function will pass back a pointer to structure that contains 
             all of the relevant info (creator, stack size, priority, name) about the 
             specified task. 

    Returns: OS_ERR_INVALID_ID if the ID passed to it is invalid
             OS_INVALID_POINTER if the task_prop pointer is NULL
             OS_SUCCESS if it copied all of the relevant info over
 
---------------------------------------------------------------------------------------*/
int32 OS_TaskGetInfo (uint32 task_id, OS_task_prop_t *task_prop)
{
    /*
    ** Lock
    */
    OS_TableLock( OS_task_table_mut, portMAX_DELAY );
    /* Check to see that the id given is valid */
    if (task_id >= OS_MAX_TASKS || OS_task_table[task_id].free == TRUE)
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_task_table_mut );
        return OS_ERR_INVALID_ID;
    }

    if (task_prop == NULL)
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_task_table_mut );
        return OS_INVALID_POINTER;
    }

    /* put the info into the stucture */
    task_prop -> creator =    OS_task_table[task_id].creator;
    task_prop -> stack_size = OS_task_table[task_id].stack_size;
    task_prop -> priority =   OS_task_table[task_id].priority;
    task_prop -> OStask_id =  (uint32)OS_task_table[task_id].task_handle;

    strcpy(task_prop-> name, OS_task_table[task_id].name);
    /*
    ** Unlock
    */
    OS_TableUnlock( OS_task_table_mut );

    return OS_SUCCESS;

} /* end OS_TaskGetInfo */

/*--------------------------------------------------------------------------------------
     Name: OS_TaskInstallDeleteHandler

    Purpose: Installs a handler for when the task is deleted.

    returns: status
---------------------------------------------------------------------------------------*/

int32 OS_TaskInstallDeleteHandler(osal_task_entry function_pointer)
{
    uint32 task_id;

    task_id = OS_TaskGetId();

    if ( task_id >= OS_MAX_TASKS )
    {
       return(OS_ERR_INVALID_ID);
    }

    /*
    ** Lock
    */
    OS_TableLock( OS_task_table_mut, portMAX_DELAY );

    if ( OS_task_table[task_id].free != FALSE )
    {
       /* 
       ** Somehow the calling task is not registered 
       */
       /*
       ** Unlock
       */
       OS_TableUnlock( OS_task_table_mut );
       return(OS_ERR_INVALID_ID);
    }

    /*
    ** Install the pointer
    */
    OS_task_table[task_id].delete_hook_pointer = function_pointer;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_task_table_mut );

    return(OS_SUCCESS);

}/*end OS_TaskInstallDeleteHandler */

/****************************************************************************************
                                MESSAGE QUEUE API
****************************************************************************************/
/*---------------------------------------------------------------------------------------
   Name: OS_QueueCreate

   Purpose: Create a message queue which can be refered to by name or ID

   Returns: OS_INVALID_POINTER if a pointer passed in is NULL
            OS_ERR_NAME_TOO_LONG if the name passed in is too long
            OS_ERR_NO_FREE_IDS if there are already the max queues created
            OS_ERR_NAME_TAKEN if the name is already being used on another queue
            OS_ERROR if the OS create call fails
            OS_SUCCESS if success

   Notes: the flahs parameter is unused.
---------------------------------------------------------------------------------------*/

int32 OS_QueueCreate (uint32 *queue_id, const char *queue_name, uint32 queue_depth,
                       uint32 data_size, uint32 flags)
{
    uint32 possible_qid;
    uint32 i;
    QueueHandle_t queue_handle;

    if ( queue_id == NULL || queue_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* we don't want to allow names too long*/
    /* if truncated, two names might be the same */
    if (strlen(queue_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    /* Check Parameters */
    /*
    ** Lock
    */
    OS_TableLock( OS_queue_table_mut, portMAX_DELAY );
    for (possible_qid = 0; possible_qid < OS_MAX_QUEUES; possible_qid++)
    {
        if (OS_queue_table[possible_qid].free == TRUE)
        {
            break;
        }
    }

    if( possible_qid >= OS_MAX_QUEUES || OS_queue_table[possible_qid].free != TRUE)
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_queue_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /* Check to see if the name is already taken */
    for (i = 0; i < OS_MAX_QUEUES; i++)
    {
        if ((OS_queue_table[i].free == FALSE) &&
                strcmp ((char*)queue_name, OS_queue_table[i].name) == 0)
        {
            /*
            ** Unlock
            */
            OS_TableUnlock( OS_queue_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }

    OS_queue_table[possible_qid].free = FALSE;
    /*
    ** Unlock
    */
    OS_TableUnlock( OS_queue_table_mut );

    /* Create FreeRTOS Message Queue */
    queue_handle = xQueueCreate(queue_depth, data_size);

    /* check if message Q create failed */
    if(queue_handle == NULL)
    {
        /*
        ** Lock
        */
        OS_TableLock( OS_queue_table_mut, portMAX_DELAY );
        OS_queue_table[possible_qid].free = TRUE;
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_queue_table_mut );
        return OS_ERROR;
    }

    /* Set the queue_id to the id that was found available*/
    /* Set the name of the queue, and the creator as well */
    *queue_id = possible_qid;

    /*
    ** Lock
    */
    OS_TableLock( OS_queue_table_mut, portMAX_DELAY );

    OS_queue_table[*queue_id].queue_handle = queue_handle;
    OS_queue_table[*queue_id].max_size = data_size;
    strcpy( OS_queue_table[*queue_id].name, (char*) queue_name);
    OS_queue_table[*queue_id].creator = OS_FindCreator();

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_queue_table_mut );

    return OS_SUCCESS;

} /* end OS_QueueCreate */

/*--------------------------------------------------------------------------------------
    Name: OS_QueueDelete

    Purpose: Deletes the specified message queue.

    Returns: OS_ERR_INVALID_ID if the id passed in does not exist
             OS_ERROR if the OS call to delete the queue fails
             OS_SUCCESS if success

    Notes: If There are messages on the queue, they will be lost and any subsequent
           calls to QueueGet or QueuePut to this queue will result in errors
---------------------------------------------------------------------------------------*/

int32 OS_QueueDelete (uint32 queue_id)
{
    /*
     * Note: There is currently no semaphore protection for the simple
     * OS_queue_table accesses in this function, only the significant
     * table entry update.
     */
    /* Check to see if the queue_id given is valid */
    if (queue_id >= OS_MAX_QUEUES || OS_queue_table[queue_id].free == TRUE)
    {
       return OS_ERR_INVALID_ID;
    }

    /* Delete the queue (FreeRTOS doesn't have return code for this operation) */
    if (OS_queue_table[queue_id].queue_handle != NULL)
    {
      vQueueDelete( OS_queue_table[queue_id].queue_handle );
    }

    /*
     * Now that the queue is deleted, remove its "presence"
     * in OS_message_q_table and OS_message_q_name_table
    */
    /*
    ** Lock
    */
    OS_TableLock( OS_queue_table_mut, portMAX_DELAY );

    OS_queue_table[queue_id].free = TRUE;
    strcpy(OS_queue_table[queue_id].name, "");
    OS_queue_table[queue_id].creator = UNINITIALIZED;
    OS_queue_table[queue_id].queue_handle = UNINITIALIZED;
    OS_queue_table[queue_id].max_size = 0;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_queue_table_mut );

    return OS_SUCCESS;

} /* end OS_QueueDelete */

/*---------------------------------------------------------------------------------------
   Name: OS_QueueGet

   Purpose: Receive a message on a message queue.  Will pend or timeout on the receive.
   Returns: OS_ERR_INVALID_ID if the given ID does not exist
            OS_ERR_INVALID_POINTER if a pointer passed in is NULL
            OS_QUEUE_EMPTY if the Queue has no messages on it to be recieved
            OS_QUEUE_TIMEOUT if the timeout was OS_PEND and the time expired
            OS_QUEUE_INVALID_SIZE if the buffer passed in is too small for the maximum sized 
                                 message
            OS_SUCCESS if success
---------------------------------------------------------------------------------------*/

int32 OS_QueueGet (uint32 queue_id, void *data, uint32 size, uint32 *size_copied,
                    int32 timeout)
{
    /*
     * Note: this function accesses the OS_queue_table without locking that table's
     * semaphore.
     */
    /* msecs rounded to the closest system tick count */
    BaseType_t status;
    uint32 sys_ticks;

    /* Check Parameters */
    if ( (data == NULL) || (size_copied == NULL) )
    {
        return OS_INVALID_POINTER;
    }
    if (queue_id >= OS_MAX_QUEUES || OS_queue_table[queue_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }
    if ( size < OS_queue_table[queue_id].max_size )
    {
        /* 
        ** The buffer that the user is passing in is potentially too small
        ** RTEMS will just copy into a buffer that is too small
        */
        *size_copied = 0;
        return(OS_QUEUE_INVALID_SIZE);
    }

    /* Get Message From FreeRTOS Message Queue */
    if (timeout == OS_PEND)
    {
        status = xQueueReceive( OS_queue_table[queue_id].queue_handle, data, portMAX_DELAY );
    }
    else if (timeout == OS_CHECK)
    {
        status = xQueueReceive( OS_queue_table[queue_id].queue_handle, data, (TickType_t)0 );

        if (status == errQUEUE_EMPTY)
        {
            *size_copied = 0;
            return OS_QUEUE_EMPTY;
        }
    }
    else
    {
        sys_ticks = OS_Milli2Ticks(timeout);
        status = xQueueReceive( OS_queue_table[queue_id].queue_handle, data, (TickType_t)sys_ticks );

        if (status != pdPASS)
        {
            *size_copied = 0;
            return OS_QUEUE_TIMEOUT;
        }
    }

    if(status != pdPASS)
    {
        *size_copied = 0;
        return OS_ERROR;
    }
    else
    {
        *size_copied = size;
        return OS_SUCCESS;
    }

}/* end OS_QueueGet */

/*---------------------------------------------------------------------------------------
   Name: OS_QueuePut

   Purpose: Put a message on a message queue.

   Returns: OS_ERR_INVALID_ID if the queue id passed in is not a valid queue
            OS_INVALID_POINTER if the data pointer is NULL
            OS_QUEUE_FULL if the queue cannot accept another message
            OS_ERROR if the OS call returns an error
            OS_SUCCESS if SUCCESS

   Notes: The flags parameter is not used.  The message put is always configured to
            immediately return an error if the receiving message queue is full.
---------------------------------------------------------------------------------------*/

int32 OS_QueuePut (uint32 queue_id, const void *data, uint32 size, uint32 flags)
{
    /*
     * Note: This function accesses the OS_queue_table without locking that table's
     * semaphore.
     */
    BaseType_t status;

    /* Check Parameters */
    if(queue_id >= OS_MAX_QUEUES || OS_queue_table[queue_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    if (data == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* Put Message Into FreeRTOS Message Queue */
    status = xQueueSendToBack( OS_queue_table[queue_id].queue_handle, (void*)data, (TickType_t)0 );

    if (status != pdPASS)
    {
        if (status == errQUEUE_FULL)
        {
            return OS_QUEUE_FULL;
        }
        else
        {
            return OS_ERROR;
        }
    }
    return OS_SUCCESS;

}/* end OS_QueuePut */

/*--------------------------------------------------------------------------------------
    Name: OS_QueueGetIdByName

    Purpose: This function tries to find a queue Id given the name of the queue. The 
             id of the queue is passed back in queue_id

    Returns: OS_INVALID_POINTER if the name or id pointers are NULL
             OS_ERR_NAME_TOO_LONG the name passed in is too long
             OS_ERR_NAME_NOT_FOUND the name was not found in the table
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/

int32 OS_QueueGetIdByName (uint32 *queue_id, const char *queue_name)
{
    /*
     * Note: This function accesses the OS_queue_table without
     * locking that table's semaphore.
     */
    uint32 i;

    if(queue_id == NULL || queue_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* a name too long wouldn't have been allowed in the first place
     * so we definitely won't find a name too long*/

    if (strlen(queue_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    for (i = 0; i < OS_MAX_QUEUES; i++)
    {
        if ( OS_queue_table[i].free != TRUE &&
                (strcmp(OS_queue_table[i].name, (char*) queue_name) == 0 ))
        {
            *queue_id = i;
            return OS_SUCCESS;
        }
    }

    /* The name was not found in the table,
     *  or it was, and the queue_id isn't valid anymore */
    return OS_ERR_NAME_NOT_FOUND;

}/* end OS_QueueGetIdByName */

/*---------------------------------------------------------------------------------------
    Name: OS_QueueGetInfo

    Purpose: This function will pass back a pointer to structure that contains
             all of the relevant info (name and creator) about the specified queue.

    Returns: OS_INVALID_POINTER if queue_prop is NULL
             OS_ERR_INVALID_ID if the ID given is not  a valid queue
             OS_SUCCESS if the info was copied over correctly
---------------------------------------------------------------------------------------*/

int32 OS_QueueGetInfo (uint32 queue_id, OS_queue_prop_t *queue_prop)
{
    /*
     * Note: This function accesses the OS_queue_table without locking that table's
     * semaphore, but locks the table while copying the data.
     */
    /* Check to see that the id given is valid */
    if (queue_prop == NULL)
    {
        return OS_INVALID_POINTER;
    }

    if (queue_id >= OS_MAX_QUEUES || OS_queue_table[queue_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* put the info into the stucture */
    /*
    ** Lock
    */
    OS_TableLock( OS_queue_table_mut, portMAX_DELAY );

    queue_prop -> creator = OS_queue_table[queue_id].creator;
    strcpy(queue_prop -> name, OS_queue_table[queue_id].name);

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_queue_table_mut );

    return OS_SUCCESS;

} /* end OS_QueueGetInfo */


/****************************************************************************************
                                  SEMAPHORE API
****************************************************************************************/

/*---------------------------------------------------------------------------------------
   Name: OS_BinSemCreate

   Purpose: Creates a binary semaphore with initial value specified by
            sem_initial_value and name specified by sem_name. sem_id will be
            returned to the caller

   Returns: OS_INVALID_POINTER if sen name or sem_id are NULL
            OS_ERR_NAME_TOO_LONG if the name given is too long
            OS_ERR_NO_FREE_IDS if all of the semaphore ids are taken
            OS_ERR_NAME_TAKEN if this is already the name of a binary semaphore
            OS_SEM_FAILURE if the OS call failed
            OS_SUCCESS if success


   Notes: options is an unused parameter
---------------------------------------------------------------------------------------*/

int32 OS_BinSemCreate (uint32 *sem_id, const char *sem_name, uint32 sem_initial_value,
                        uint32 options)
{
    /* the current candidate for the new sem id */
    uint32 possible_semid;
    uint32 i;
    EventGroupHandle_t event_group_handle;

    if (sem_id == NULL || sem_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* we don't want to allow names too long*/
    /* if truncated, two names might be the same */

    if (strlen(sem_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    /* Check Parameters */
    /*
    ** Lock
    */
    OS_TableLock( OS_bin_sem_table_mut, portMAX_DELAY );

    for (possible_semid = 0; possible_semid < OS_MAX_BIN_SEMAPHORES; possible_semid++)
    {
        if (OS_bin_sem_table[possible_semid].free == TRUE)
        {
            break;
        }
    }

    if((possible_semid >= OS_MAX_BIN_SEMAPHORES) ||
       (OS_bin_sem_table[possible_semid].free != TRUE))
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_bin_sem_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /* Check to see if the name is already taken */
    for (i = 0; i < OS_MAX_BIN_SEMAPHORES; i++)
    {
        if ((OS_bin_sem_table[i].free == FALSE) &&
                strcmp ((char*)sem_name, OS_bin_sem_table[i].name) == 0)
        {
            /*
            ** Unlock
            */
            OS_TableUnlock( OS_bin_sem_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }
    
    OS_bin_sem_table[possible_semid].free  = FALSE;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_bin_sem_table_mut );

    /* Check to make sure the sem value is going to be either 0 or 1 */
    if (sem_initial_value > 1)
    {
        sem_initial_value = 1;
    }
    
    /* Create FreeRTOS Semaphore */
    event_group_handle = xEventGroupCreate();

    /*
    ** Lock
    */
    OS_TableLock( OS_bin_sem_table_mut, portMAX_DELAY );

    /* check if semBCreate failed */
    if (event_group_handle == NULL)
    {
        OS_bin_sem_table[possible_semid].free  = TRUE;
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_bin_sem_table_mut );
        return OS_SEM_FAILURE;
    }

    if (sem_initial_value == 1)
    {
      xEventGroupSetBits( event_group_handle, egBIN_SEM_STATE_BIT );
    }

    /* Set the sem_id to the one that we found available */
    /* Set the name of the semaphore,creator and free as well */

    *sem_id = possible_semid;

    OS_bin_sem_table[*sem_id].free = FALSE;
    OS_bin_sem_table[*sem_id].event_group_handle = event_group_handle;
    strcpy(OS_bin_sem_table[*sem_id].name, (char *)sem_name);
    OS_bin_sem_table[*sem_id].creator = OS_FindCreator();
    OS_bin_sem_table[*sem_id].max_value = 1;
    OS_bin_sem_table[*sem_id].current_value = sem_initial_value;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_bin_sem_table_mut );

    return OS_SUCCESS;

}/* end OS_BinSemCreate */

/*--------------------------------------------------------------------------------------
     Name: OS_BinSemDelete

    Purpose: Deletes the specified Binary Semaphore.

    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid binary semaphore
             OS_SEM_FAILURE the OS call failed
             OS_SUCCESS if success
---------------------------------------------------------------------------------------*/

int32 OS_BinSemDelete (uint32 sem_id)
{
    /*
     * Note: There is currently no semaphore protection for the simple
     * OS_bin_sem_table accesses in this function, only the significant
     * table entry update.
     */
    /* Check to see if this sem_id is valid */
    if (sem_id >= OS_MAX_BIN_SEMAPHORES || OS_bin_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* FreeRTOS doesn't have return values for this function */
    if (OS_bin_sem_table[sem_id].event_group_handle != NULL)
    {
      vEventGroupDelete( OS_bin_sem_table[sem_id].event_group_handle );
    }

    /* Remove the Id from the table, and its name, so that it cannot be found again */
    /*
    ** Lock
    */
    OS_TableLock( OS_bin_sem_table_mut, portMAX_DELAY );

    OS_bin_sem_table[sem_id].free = TRUE;
    strcpy(OS_bin_sem_table[sem_id].name, "");
    OS_bin_sem_table[sem_id].creator = UNINITIALIZED;
    OS_bin_sem_table[sem_id].event_group_handle = NULL;
    OS_bin_sem_table[sem_id].max_value = 0;
    OS_bin_sem_table[sem_id].current_value = 0;
    
    /*
    ** Unlock
    */
    OS_TableUnlock( OS_bin_sem_table_mut );

    return OS_SUCCESS;

}/* end OS_BinSemDelete */

/*---------------------------------------------------------------------------------------
    Name: OS_BinSemGive

    Purpose: The function  unlocks the semaphore referenced by sem_id by performing
             a semaphore unlock operation on that semaphore.If the semaphore value 
             resulting from this operation is positive, then no threads were blocked             
             waiting for the semaphore to become unlocked; the semaphore value is
             simply incremented for this semaphore.

    
    Returns: OS_SEM_FAILURE the semaphore was not previously  initialized or is not
             in the array of semaphores defined by the system
             OS_ERR_INVALID_ID if the id passed in is not a binary semaphore
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/
int32 OS_BinSemGive (uint32 sem_id)
{
    /*
     * Note: This function accesses the OS_bin_sem_table without locking that table's
     * semaphore.
     */
    /* Check Parameters */
    if (sem_id >= OS_MAX_BIN_SEMAPHORES || OS_bin_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* 
    ** If the sem value is not full ( 1 ) then increment it.
    */
    if ( OS_bin_sem_table[sem_id].current_value < OS_bin_sem_table[sem_id].max_value )
    {
        OS_bin_sem_table[sem_id].current_value ++;

        xEventGroupSetBits( OS_bin_sem_table[sem_id].event_group_handle, egBIN_SEM_STATE_BIT );
        xEventGroupClearBits( OS_bin_sem_table[sem_id].event_group_handle, egBIN_SEM_FLUSH_BIT );
    }

    return OS_SUCCESS;

}/* end OS_BinSemGive */

/*---------------------------------------------------------------------------------------
    Name: OS_BinSemFlush

    Purpose: The function unblocks all tasks pending on the specified semaphore. However,
             this function does not change the state of the semaphore.

    
    Returns: OS_SEM_FAILURE the semaphore was not previously  initialized or is not
             in the array of semaphores defined by the system
             OS_ERR_INVALID_ID if the id passed in is not a binary semaphore
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/
int32 OS_BinSemFlush (uint32 sem_id)
{
    /*
     * Note: This function accesses the OS_bin_sem_table without locking that table's
     * semaphore.
     */
    /* Check Parameters */
    if(sem_id >= OS_MAX_BIN_SEMAPHORES || OS_bin_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    xEventGroupSetBits( OS_bin_sem_table[sem_id].event_group_handle, egBIN_SEM_FLUSH_BIT );
    return OS_SUCCESS;

}/* end OS_BinSemFlush */

/*---------------------------------------------------------------------------------------
    Name:    OS_BinSemTake

    Purpose: The locks the semaphore referenced by sem_id by performing a
             semaphore lock operation on that semaphore.If the semaphore value
             is currently zero, then the calling thread shall not return from
             the call until it either locks the semaphore or the call is
             interrupted by a signal.

    Return:  OS_SEM_FAILURE : the semaphore was not previously initialized
             or is not in the array of semaphores defined by the system
             OS_ERR_INVALID_ID the Id passed in is not a valid binar semaphore
             OS_SEM_FAILURE if the OS call failed
             OS_SUCCESS if success

----------------------------------------------------------------------------------------*/

int32 OS_BinSemTake (uint32 sem_id)
{
    /*
     * Note: This function accesses the OS_bin_sem_table without locking that table's
     * semaphore.
     */

    EventBits_t uxBits;

    /* Check Parameters */
    if (sem_id >= OS_MAX_BIN_SEMAPHORES  || OS_bin_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    uxBits = xEventGroupWaitBits( OS_bin_sem_table[sem_id].event_group_handle,
                                  (egBIN_SEM_STATE_BIT | egBIN_SEM_FLUSH_BIT),
                                  pdFALSE,
                                  pdFALSE,
                                  portMAX_DELAY );

    if( ( uxBits & ( egBIN_SEM_STATE_BIT | egBIN_SEM_FLUSH_BIT ) ) == ( egBIN_SEM_STATE_BIT | egBIN_SEM_FLUSH_BIT ) )
    {
        /* xEventGroupWaitBits() returned because both bits were set */
        return OS_SUCCESS;
    }
    else if( ( uxBits & egBIN_SEM_STATE_BIT ) != 0 )
    {
        /* xEventGroupWaitBits() returned because just egBIN_SEM_STATE_BIT was set. */
        xEventGroupClearBits( OS_bin_sem_table[sem_id].event_group_handle, egBIN_SEM_STATE_BIT );
        OS_bin_sem_table[sem_id].current_value --;
        return OS_SUCCESS;
    }
    else if( ( uxBits & egBIN_SEM_FLUSH_BIT ) != 0 )
    {
        /* xEventGroupWaitBits() returned because just egBIN_SEM_FLUSH_BIT was set */
        return OS_SUCCESS;
    }
    else
    {
        /* xEventGroupWaitBits() returned because xTicksToWait ticks passed */
        return OS_SEM_FAILURE;
    }

    return OS_SEM_FAILURE;

}/* end OS_BinSemTake */

/*---------------------------------------------------------------------------------------
    Name: OS_BinSemTimedWait

    Purpose: The function locks the semaphore referenced by sem_id . However,
             if the semaphore cannot be locked without waiting for another process
             or thread to unlock the semaphore , this wait shall be terminated when
             the specified timeout ,msecs, expires.

    Returns: OS_SEM_TIMEOUT if semaphore was not relinquished in time
             OS_SUCCESS if success
             OS_SEM_FAILURE the semaphore was not previously initialized or is not
             in the array of semaphores defined by the system
             OS_ERR_INVALID_ID if the ID passed in is not a valid semaphore ID
----------------------------------------------------------------------------------------*/

int32 OS_BinSemTimedWait (uint32 sem_id, uint32 msecs)
{
    /*
     * Note: This function accesses the OS_bin_sem_table without locking that table's
     * semaphore.
     */
    uint32 sys_ticks;
    EventBits_t uxBits;

    /* Check Parameters */
    if( (sem_id >= OS_MAX_BIN_SEMAPHORES) || (OS_bin_sem_table[sem_id].free == TRUE) )
    {
        return OS_ERR_INVALID_ID;
    }

    sys_ticks = OS_Milli2Ticks(msecs);

    uxBits = xEventGroupWaitBits( OS_bin_sem_table[sem_id].event_group_handle,
                                  (egBIN_SEM_STATE_BIT | egBIN_SEM_FLUSH_BIT),
                                  pdFALSE,
                                  pdFALSE,
                                  sys_ticks );

    if( ( uxBits & ( egBIN_SEM_STATE_BIT | egBIN_SEM_FLUSH_BIT ) ) == ( egBIN_SEM_STATE_BIT | egBIN_SEM_FLUSH_BIT ) )
    {
        /* xEventGroupWaitBits() returned because both bits were set */
        return OS_SUCCESS;
    }
    else if( ( uxBits & egBIN_SEM_STATE_BIT ) != 0 )
    {
        /* xEventGroupWaitBits() returned because just egBIN_SEM_STATE_BIT was set */
        xEventGroupClearBits( OS_bin_sem_table[sem_id].event_group_handle, egBIN_SEM_STATE_BIT );
        OS_bin_sem_table[sem_id].current_value --;
        return OS_SUCCESS;
    }
    else if( ( uxBits & egBIN_SEM_FLUSH_BIT ) != 0 )
    {
        /* xEventGroupWaitBits() returned because just egBIN_SEM_FLUSH_BIT was set */
        return OS_SUCCESS;
    }
    else
    {
        /* xEventGroupWaitBits() returned because xTicksToWait ticks passed */
        return OS_SEM_TIMEOUT;
    }

    return OS_SEM_FAILURE;

}/* end OS_BinSemTimedWait */

/*--------------------------------------------------------------------------------------
    Name: OS_BinSemGetIdByName

    Purpose: This function tries to find a binary sem Id given the name of a bin_sem
             The id is returned through sem_id

    Returns: OS_INVALID_POINTER is semid or sem_name are NULL pointers
             OS_ERR_NAME_TOO_LONG if the name given is to long to have been stored
             OS_ERR_NAME_NOT_FOUND if the name was not found in the table
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/
int32 OS_BinSemGetIdByName (uint32 *sem_id, const char *sem_name)
{
    /*
     * Note: This function accesses the OS_bin_sem_table without locking that table's
     * semaphore.
     */
    uint32 i;

    if (sem_id == NULL || sem_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* a name too long wouldn't have been allowed in the first place
     * so we definitely won't find a name too long*/
    if (strlen(sem_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    for (i = 0; i < OS_MAX_BIN_SEMAPHORES; i++)
    {
        if ( OS_bin_sem_table[i].free != TRUE &&
           ( strcmp (OS_bin_sem_table[i].name , (char*) sem_name) == 0)
           )
        {
            *sem_id = i;
            return OS_SUCCESS;
        }
    }
    /* The name was not found in the table,
     *  or it was, and the sem_id isn't valid anymore */

    return OS_ERR_NAME_NOT_FOUND;

}/* end OS_BinSemGetIdByName */

/*---------------------------------------------------------------------------------------
    Name: OS_BinSemGetInfo

    Purpose: This function will pass back a pointer to structure that contains
             all of the relevant info( name and creator) about the specified binary
             semaphore.

    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid semaphore
             OS_INVALID_POINTER if the bin_prop pointer is null
             OS_SUCCESS if success
---------------------------------------------------------------------------------------*/

int32 OS_BinSemGetInfo (uint32 sem_id, OS_bin_sem_prop_t *bin_prop)
{
    /*
    ** Lock
    */
    OS_TableLock( OS_bin_sem_table_mut, portMAX_DELAY );
    /* Check to see that the id given is valid */
    if (sem_id >= OS_MAX_BIN_SEMAPHORES || OS_bin_sem_table[sem_id].free == TRUE)
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_bin_sem_table_mut );
        return OS_ERR_INVALID_ID;
    }

    if (bin_prop == NULL)
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_bin_sem_table_mut );
        return OS_INVALID_POINTER;
    }

    /* put the info into the structure */

    bin_prop -> creator = OS_bin_sem_table[sem_id].creator;
    bin_prop -> value = OS_bin_sem_table[sem_id].current_value;
    strcpy(bin_prop-> name, OS_bin_sem_table[sem_id].name);

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_bin_sem_table_mut );

    return OS_SUCCESS;

} /* end OS_BinSemGetInfo */

/*---------------------------------------------------------------------------------------
   Name: OS_CountSemCreate

   Purpose: Creates a counting semaphore with initial value specified by
            sem_initial_value and name specified by sem_name. sem_id will be
            returned to the caller

   Returns: OS_INVALID_POINTER if sen name or sem_id are NULL
            OS_ERR_NAME_TOO_LONG if the name given is too long
            OS_ERR_NO_FREE_IDS if all of the semaphore ids are taken
            OS_ERR_NAME_TAKEN if this is already the name of a counting semaphore
            OS_SEM_FAILURE if the OS call failed
            OS_SUCCESS if success


   Notes: The options parameter is unused. 
---------------------------------------------------------------------------------------*/

int32 OS_CountSemCreate (uint32 *sem_id, const char *sem_name, uint32 sem_initial_value,
                        uint32 options)
{
    uint32 possible_semid;
    uint32 i;
    SemaphoreHandle_t count_sem_handle;

    /*
    ** Check Parameters
    */
    if (sem_id == NULL || sem_name == NULL)
    {
        return OS_INVALID_POINTER;
    }
    
    /*
    ** Verify that the semaphore maximum value is not too high
    */ 
    if ( sem_initial_value > SEM_VALUE_MAX )
    {
        return OS_INVALID_SEM_VALUE;
    }

    /* 
    ** we don't want to allow names too long
    ** if truncated, two names might be the same 
    */
    if (strlen(sem_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    /*
    ** Lock
    */
    OS_TableLock( OS_count_sem_table_mut, portMAX_DELAY );

    for (possible_semid = 0; possible_semid < OS_MAX_COUNT_SEMAPHORES; possible_semid++)
    {
        if (OS_count_sem_table[possible_semid].free == TRUE)
            break;
    }

    if((possible_semid >= OS_MAX_COUNT_SEMAPHORES) ||
       (OS_count_sem_table[possible_semid].free != TRUE))
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_count_sem_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /* Check to see if the name is already taken */
    for (i = 0; i < OS_MAX_COUNT_SEMAPHORES; i++)
    {
        if ((OS_count_sem_table[i].free == FALSE) &&
                strcmp ((char*)sem_name, OS_count_sem_table[i].name) == 0)
        {
            /*
            ** Unlock
            */
            OS_TableUnlock( OS_count_sem_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }
    OS_count_sem_table[possible_semid].free = FALSE;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_count_sem_table_mut );

    /* Create FreeRTOS Semaphore */
    count_sem_handle = xSemaphoreCreateCounting(SEM_VALUE_MAX, sem_initial_value);

    /*
    ** Lock
    */
    OS_TableLock( OS_count_sem_table_mut, portMAX_DELAY );

    /* check if semCCreate failed */
    if(count_sem_handle == NULL)
    {

        OS_count_sem_table[possible_semid].free = TRUE;

        /*
        ** Unlock
        */ 
        OS_TableUnlock( OS_count_sem_table_mut );
        return OS_SEM_FAILURE;
    }

    /* Set the sem_id to the one that we found available */
    /* Set the name of the semaphore,creator and free as well */

    *sem_id = possible_semid;

    OS_count_sem_table[*sem_id].count_sem_handle = count_sem_handle;

    OS_count_sem_table[*sem_id].free = FALSE;
    strcpy(OS_count_sem_table[*sem_id].name , (char*) sem_name);
    OS_count_sem_table[*sem_id].creator = OS_FindCreator();

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_count_sem_table_mut );

    return OS_SUCCESS;

}/* end OS_CountSemCreate */

/*--------------------------------------------------------------------------------------
     Name: OS_CountSemDelete

    Purpose: Deletes the specified Counting Semaphore.

    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid counting semaphore
             OS_SEM_FAILURE the OS call failed
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/
int32 OS_CountSemDelete (uint32 sem_id)
{
    /* 
     * Note: This function accesses the OS_count_sem_table without locking that table's
     * semaphore.
     */

    /*
    ** Check to see if this sem_id is valid 
    */
    if (sem_id >= OS_MAX_COUNT_SEMAPHORES || OS_count_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* FreeRTOS doesn't have return values for this operation */
    if (OS_count_sem_table[sem_id].count_sem_handle != NULL)
    {
      vSemaphoreDelete(OS_count_sem_table[sem_id].count_sem_handle);
    }

    /* 
    ** Remove the Id from the table, and its name, so that it cannot be found again 
    */

    /*
    ** Lock
    */
    OS_TableLock( OS_count_sem_table_mut, portMAX_DELAY );

    OS_count_sem_table[sem_id].free = TRUE;
    strcpy(OS_count_sem_table[sem_id].name, "");
    OS_count_sem_table[sem_id].creator = UNINITIALIZED;
    OS_count_sem_table[sem_id].count_sem_handle = NULL;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_count_sem_table_mut );

    return OS_SUCCESS;

}/* end OS_CountSemDelete */

/*---------------------------------------------------------------------------------------
    Name: OS_CountSemGive

    Purpose: The function  unlocks the semaphore referenced by sem_id by performing
             a semaphore unlock operation on that semaphore.If the semaphore value 
             resulting from this operation is positive, then no threads were blocked             
             waiting for the semaphore to become unlocked; the semaphore value is
             simply incremented for this semaphore.

    
    Returns: OS_SEM_FAILURE the semaphore was not previously  initialized or is not
             in the array of semaphores defined by the system
             OS_ERR_INVALID_ID if the id passed in is not a counting semaphore
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/

int32 OS_CountSemGive (uint32 sem_id)
{
    /*
     * Note: This function accesses the OS_count_sem_table without locking that table's
     * semaphore.
     */
    int32 return_code = OS_SUCCESS;

    /* 
    ** Check Parameters 
    */
    if (sem_id >= OS_MAX_COUNT_SEMAPHORES || OS_count_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    if (xSemaphoreGive( OS_count_sem_table[sem_id].count_sem_handle ) == pdPASS)
    {
        return_code =  OS_SUCCESS;
    }
    else
    {
        return_code =  OS_SEM_FAILURE;
    }

    return(return_code);

}/* end OS_CountSemGive */

/*---------------------------------------------------------------------------------------
    Name:    OS_CountSemTake

    Purpose: The locks the semaphore referenced by sem_id by performing a
             semaphore lock operation on that semaphore.If the semaphore value
             is currently zero, then the calling thread shall not return from
             the call until it either locks the semaphore or the call is
             interrupted by a signal.

    Return:  OS_SEM_FAILURE : the semaphore was not previously initialized
             or is not in the array of semaphores defined by the system
             OS_ERR_INVALID_ID the Id passed in is not a valid countar semaphore
             OS_SEM_FAILURE if the OS call failed
             OS_SUCCESS if success

----------------------------------------------------------------------------------------*/

int32 OS_CountSemTake (uint32 sem_id)
{
    /*
     * Note: This function accesses the OS_count_sem_table without locking that table's
     * semaphore.
     */
    int32 return_code;

    /* 
    ** Check Parameters 
    */
    if (sem_id >= OS_MAX_COUNT_SEMAPHORES || OS_count_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* 
    ** Give FreeRTOS Semaphore 
    */
    if (xSemaphoreTake( OS_count_sem_table[sem_id].count_sem_handle, portMAX_DELAY ) == pdPASS)
    {
        return_code = OS_SUCCESS;
    }
    else
    {
        return_code = OS_SEM_FAILURE;
    }

    return(return_code);

}/* end OS_CountSemTake */

/*---------------------------------------------------------------------------------------
    Name: OS_CountSemTimedWait

    Purpose: The function locks the semaphore referenced by sem_id . However,
             if the semaphore cannot be locked without waiting for another process
             or thread to unlock the semaphore , this wait shall be terminated when
             the specified timeout ,msecs, expires.

    Returns: OS_SEM_TIMEOUT if semaphore was not relinquished in time
             OS_SUCCESS if success
             OS_SEM_FAILURE the semaphore was not previously initialized or is not
             in the array of semaphores defined by the system
             OS_ERR_INVALID_ID if the ID passed in is not a valid semaphore ID
----------------------------------------------------------------------------------------*/

int32 OS_CountSemTimedWait (uint32 sem_id, uint32 msecs)
{
    /*
     * Note: This function accesses the OS_count_sem_table without locking that table's
     * semaphore.
     */
    int32 return_code = OS_SUCCESS;
    uint32 sys_ticks;
    BaseType_t status;

    /* msecs rounded to the closest system tick count */
    sys_ticks = OS_Milli2Ticks(msecs);

    /* 
    ** Check Parameters 
    */
    if( (sem_id >= OS_MAX_COUNT_SEMAPHORES) || (OS_count_sem_table[sem_id].free == TRUE) )
    {
        return OS_ERR_INVALID_ID;
    }

    /* 
    ** Give FreeRTOS Semaphore 
    */
    status = xSemaphoreTake( OS_count_sem_table[sem_id].count_sem_handle, sys_ticks );
    if (status == pdPASS)
    {
       return_code = OS_SUCCESS;
    }
    /* Note: for xSemaphoreTake() there is no way to understand if timeout happend */
    else
    {
       return_code =  OS_SEM_FAILURE;
    }

    return(return_code);

}/* end OS_CountSemTimedWait */

/*--------------------------------------------------------------------------------------
    Name: OS_CountSemGetIdByName

    Purpose: This function tries to find a counting sem Id given the name of a count_sem
             The id is returned through sem_id

    Returns: OS_INVALID_POINTER is semid or sem_name are NULL pointers
             OS_ERR_NAME_TOO_LONG if the name given is to long to have been stored
             OS_ERR_NAME_NOT_FOUND if the name was not found in the table
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/

int32 OS_CountSemGetIdByName (uint32 *sem_id, const char *sem_name)
{
    /*
     * Note: This function accesses the OS_count_sem_table without locking that table's
     * semaphore.
     */
    uint32 i;

    /*
    ** Check Parameters
    */
    if (sem_id == NULL || sem_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* 
    ** A name too long wouldn't have been allowed in the first place
    ** so we definitely won't find a name too long
    */
    if (strlen(sem_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    for (i = 0; i < OS_MAX_COUNT_SEMAPHORES; i++)
    {
        if ( OS_count_sem_table[i].free != TRUE &&
           ( strcmp (OS_count_sem_table[i].name , (char*) sem_name) == 0))
        {
            *sem_id = i;
            return OS_SUCCESS;
        }
    }
    /* 
    ** The name was not found in the table,
    **  or it was, and the sem_id isn't valid anymore 
    */

    return OS_ERR_NAME_NOT_FOUND;

}/* end OS_CountSemGetIdByName */

/*---------------------------------------------------------------------------------------
    Name: OS_CountSemGetInfo

    Purpose: This function will pass back a pointer to structure that contains
             all of the relevant info( name and creator) about the specified counting
             semaphore.

    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid semaphore
             OS_INVALID_POINTER if the count_prop pointer is null
             OS_SUCCESS if success
---------------------------------------------------------------------------------------*/

int32 OS_CountSemGetInfo (uint32 sem_id, OS_count_sem_prop_t *count_prop)
{
    /* 
     * Note: This function performs some accesses to the OS_count_sem_table
     * without locking that table's semaphore.
     */

    /*
    ** Check to see that the id given is valid 
    */
    if (sem_id >= OS_MAX_COUNT_SEMAPHORES || OS_count_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    if (count_prop == NULL)
    {
        return OS_INVALID_POINTER;
    }
    
    /*
    ** Lock
    */
    OS_TableLock( OS_count_sem_table_mut, portMAX_DELAY );

    /* put the info into the stucture */
    count_prop -> creator = OS_count_sem_table[sem_id].creator;
    strcpy(count_prop -> name, OS_count_sem_table[sem_id].name);
    count_prop -> value = 0; /* will be deprecated */

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_count_sem_table_mut );

    return OS_SUCCESS;

} /* end OS_CountSemGetInfo */


/****************************************************************************************
                                  MUTEX API
****************************************************************************************/

/*---------------------------------------------------------------------------------------
    Name: OS_MutSemCreate

    Purpose: Creates a mutex semaphore initially full.

    Returns: OS_INVALID_POINTER if sem_id or sem_name are NULL
             OS_ERR_NAME_TOO_LONG if the sem_name is too long to be stored
             OS_ERR_NO_FREE_IDS if there are no more free mutex Ids
             OS_ERR_NAME_TAKEN if there is already a mutex with the same name
             OS_SEM_FAILURE if the OS call failed
             OS_SUCCESS if success

    Notes: the options parameter is not used in this implementation

---------------------------------------------------------------------------------------*/

int32 OS_MutSemCreate (uint32 *sem_id, const char *sem_name, uint32 options)
{
    uint32 possible_semid;
    uint32 i;
    SemaphoreHandle_t mut_sem_handle;

    /* Check Parameters */
    if (sem_id == NULL || sem_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* we don't want to allow names too long*/
    /* if truncated, two names might be the same */
    if (strlen(sem_name) >= OS_MAX_API_NAME)
    {
       return OS_ERR_NAME_TOO_LONG;
    }

    /*
    ** Lock
    */
    OS_TableLock( OS_mut_sem_table_mut, portMAX_DELAY );

    for (possible_semid = 0; possible_semid < OS_MAX_MUTEXES; possible_semid++)
    {
        if (OS_mut_sem_table[possible_semid].free == TRUE)
        {
            break;
        }
    }

    if( (possible_semid >= OS_MAX_MUTEXES) ||
        (OS_mut_sem_table[possible_semid].free != TRUE) )
    {
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_mut_sem_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /* Check to see if the name is already taken */
    for (i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if ((OS_mut_sem_table[i].free == FALSE) &&
            strcmp ((char*) sem_name, OS_mut_sem_table[i].name) == 0)
        {
            /*
            ** Unlock
            */
            OS_TableUnlock( OS_mut_sem_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }
    OS_mut_sem_table[possible_semid].free = FALSE;

    /* Create FreeRTOS Semaphore */

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_mut_sem_table_mut );

    mut_sem_handle = xSemaphoreCreateRecursiveMutex();

    /*
    ** Lock
    */
    OS_TableLock( OS_mut_sem_table_mut, portMAX_DELAY );

     /* check if xSemaphoreCreateMutex() failed */
    if(mut_sem_handle == NULL)
    {
        OS_mut_sem_table[possible_semid].free = TRUE;
        /*
        ** Unlock
        */
        OS_TableUnlock( OS_mut_sem_table_mut );
        return OS_SEM_FAILURE;
    }

    /* Set the sem_id to the one that we found open */
    /* Set the name of the semaphore, creator, and free as well */

    *sem_id = possible_semid;

    OS_mut_sem_table[*sem_id].mut_sem_handle = mut_sem_handle;
    strcpy(OS_mut_sem_table[*sem_id].name, (char*)sem_name);
    OS_mut_sem_table[*sem_id].free = FALSE;
    OS_mut_sem_table[*sem_id].creator = OS_FindCreator();

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_mut_sem_table_mut );

    return OS_SUCCESS;

}/* end OS_MutSemCreate */

/*--------------------------------------------------------------------------------------
     Name: OS_MutSemDelete

    Purpose: Deletes the specified Mutex Semaphore.

    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid mutex
             OS_SEM_FAILURE if the OS call failed
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/

int32 OS_MutSemDelete (uint32 sem_id)
{
    /*
     * Note: This function performs some access to the OS_mut_sem_table without
     * locking that table's semaphore.
     */
    /* Check to see if this sem_id is valid   */
    if (sem_id >= OS_MAX_MUTEXES || OS_mut_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* FreeRTOS doesn't have return values for this operation */
    if (OS_mut_sem_table[sem_id].mut_sem_handle != NULL)
    {
      vSemaphoreDelete( OS_mut_sem_table[sem_id].mut_sem_handle );
    }

    /* Delete its presence in the table */
    /*
    ** Lock
    */
    OS_TableLock( OS_mut_sem_table_mut, portMAX_DELAY );

    OS_mut_sem_table[sem_id].free = TRUE;
    OS_mut_sem_table[sem_id].mut_sem_handle = NULL;
    strcpy(OS_mut_sem_table[sem_id].name, "");
    OS_mut_sem_table[sem_id].creator = UNINITIALIZED;

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_mut_sem_table_mut );

    return OS_SUCCESS;

}/* end OS_MutSemDelete */

/*---------------------------------------------------------------------------------------
    Name: OS_MutSemGive

    Purpose: The function releases the mutex object referenced by sem_id.The
             manner in which a mutex is released is dependent upon the mutex's type
             attribute.  If there are threads blocked on the mutex object referenced by
             mutex when this function is called, resulting in the mutex becoming
             available, the scheduling policy shall determine which thread shall
             acquire the mutex.

    Returns: OS_SUCCESS if success
             OS_SEM_FAILURE if the semaphore was not previously  initialized
             OS_ERR_INVALID_ID if the id passed in is not a valid mutex

---------------------------------------------------------------------------------------*/

int32 OS_MutSemGive (uint32 sem_id)
{
    /*
     * Note: This function accesses the OS_mut_sem_table_sem without locking that table's
     * semaphore.
     */
    /* Check Parameters */
    if (sem_id >= OS_MAX_MUTEXES || OS_mut_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    /* Give FreeRTOS Semaphore */
    if (xSemaphoreGiveRecursive( OS_mut_sem_table[sem_id].mut_sem_handle ) != pdPASS)
    {
        return OS_SEM_FAILURE;
    }

    return OS_SUCCESS;

}/* end OS_MutSemGive */

/*---------------------------------------------------------------------------------------
    Name: OS_MutSemTake

    Purpose: The mutex object referenced by sem_id shall be locked by calling this
             function. If the mutex is already locked, the calling thread shall
             block until the mutex becomes available. This operation shall return
             with the mutex object referenced by mutex in the locked state with the
             calling thread as its owner.

    Returns: OS_SUCCESS if success
             OS_SEM_FAILURE if the semaphore was not previously initialized or is
             not in the array of semaphores defined by the system
             OS_ERR_INVALID_ID the id passed in is not a valid mutex
---------------------------------------------------------------------------------------*/

int32 OS_MutSemTake (uint32 sem_id)
{
   /*
    * Note: This function accesses the OS_mut_sem_table_sem without locking that table's
    * semaphore.
    */
   /* Check Parameters */
   if(sem_id >= OS_MAX_MUTEXES || OS_mut_sem_table[sem_id].free == TRUE)
   {
        return OS_ERR_INVALID_ID;
   }
   /* Take FreeRTOS Semaphore */
   if (xSemaphoreTakeRecursive( OS_mut_sem_table[sem_id].mut_sem_handle, portMAX_DELAY ) != pdPASS)
   {
        return OS_SEM_FAILURE;
   }

   return OS_SUCCESS;

}/* end OS_MutSemGive */

/*--------------------------------------------------------------------------------------
    Name: OS_MutSemGetIdByName

    Purpose: This function tries to find a mutex sem Id given the name of a bin_sem
             The id is returned through sem_id

    Returns: OS_INVALID_POINTER is semid or sem_name are NULL pointers
             OS_ERR_NAME_TOO_LONG if the name given is to long to have been stored
             OS_ERR_NAME_NOT_FOUND if the name was not found in the table
             OS_SUCCESS if success

---------------------------------------------------------------------------------------*/

int32 OS_MutSemGetIdByName (uint32 *sem_id, const char *sem_name)
{
    /*
     * Note: This function accesses the OS_mut_sem_table_sem without locking that table's
     * semaphore.
     */
    uint32 i;

    if(sem_id == NULL || sem_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* a name too long wouldn't have been allowed in the first place
     * so we definitely won't find a name too long*/

    if (strlen(sem_name) >= OS_MAX_API_NAME)
    {
       return OS_ERR_NAME_TOO_LONG;
    }

    for (i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if ((OS_mut_sem_table[i].free != TRUE) &&
           (strcmp (OS_mut_sem_table[i].name, (char*) sem_name) == 0))
        {
            *sem_id = i;
            return OS_SUCCESS;
        }
    }

    /* The name was not found in the table,
     *  or it was, and the sem_id isn't valid anymore */
    return OS_ERR_NAME_NOT_FOUND;

}/* end OS_MutSemGetIdByName */

/*---------------------------------------------------------------------------------------
    Name: OS_MutSemGetInfo

    Purpose: This function will pass back a pointer to structure that contains
             all of the relevant info( name and creator) about the specified mutex
             semaphore.

    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid semaphore
             OS_INVALID_POINTER if the mut_prop pointer is null
             OS_SUCCESS if success
---------------------------------------------------------------------------------------*/

int32 OS_MutSemGetInfo (uint32 sem_id, OS_mut_sem_prop_t *mut_prop)
{
    /*
     * Note: This function performs some accesses to the OS_mut_sem_table without
     * locking that table's semaphore, but locks the table while copying the data.
     */
    /* Check to see that the id given is valid */
    if (sem_id >= OS_MAX_MUTEXES || OS_mut_sem_table[sem_id].free == TRUE)
    {
        return OS_ERR_INVALID_ID;
    }

    if (mut_prop == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* put the info into the stucture */
    /*
    ** Lock
    */
    OS_TableLock( OS_mut_sem_table_mut, portMAX_DELAY );

    mut_prop -> creator = OS_mut_sem_table[sem_id].creator;
    strcpy(mut_prop-> name, OS_mut_sem_table[sem_id].name);

    /*
    ** Unlock
    */
    OS_TableUnlock( OS_mut_sem_table_mut );

    return OS_SUCCESS;

} /* end OS_MutSemGetInfo */

/****************************************************************************************
                                  MUTEX API
****************************************************************************************/

/****************************************************************************************
                                    INT API
****************************************************************************************/

/*---------------------------------------------------------------------------------------
   Name: OS_HeapGetInfo

   Purpose: Return current info on the heap

   Parameters:

---------------------------------------------------------------------------------------*/
int32 OS_HeapGetInfo (OS_heap_prop_t *heap_prop)
{
    if (heap_prop == NULL)
    {
        return OS_INVALID_POINTER;
    }
    /*
    ** Not implemented yet
    */
    return (OS_ERR_NOT_IMPLEMENTED);
}
/*---------------------------------------------------------------------------------------
** Name: OS_Tick2Micros
**
** Purpose:
** This function returns the duration of a system tick in micro seconds.
**
** Assumptions and Notes:
**
** Parameters: None
**
** Global Inputs: None
**
** Global Outputs: None
**
**
**
** Return Values: duration of a system tick in microseconds
---------------------------------------------------------------------------------------*/
int32 OS_Tick2Micros (void)
{
    return (0);
}

/*---------------------------------------------------------------------------------------
** Name: OS_Milli2Ticks
**
** Purpose:
** This function accepts a time interval in milliseconds, as an input and 
** returns the tick equivalent  for this time period. The tick value is 
**  rounded up.
**
** Assumptions and Notes:
**
** Parameters:
**      milli_seconds : the time interval ,in milli second , to be translated
**                      to ticks
**
** Global Inputs: None
**
** Global Outputs: None
**
**
** Return Values: the number of ticks rounded up.
---------------------------------------------------------------------------------------*/
int32 OS_Milli2Ticks(uint32 milli_seconds)
{
    uint32 num_of_ticks;

    num_of_ticks = pdMS_TO_TICKS( milli_seconds );

    return (num_of_ticks);
}

/*---------------------------------------------------------------------------------------
 * Name: OS_GetLocalTime
 * 
 * Purpose: This functions get the local time of the machine its on
 * ------------------------------------------------------------------------------------*/

int32 OS_GetLocalTime(OS_time_t *time_struct)
{
    TickType_t        tick_count;

    if (time_struct == NULL)
    {
        return OS_INVALID_POINTER;
    }

    tick_count = xTaskGetTickCount();

    time_struct -> seconds = tick_count / configTICK_RATE_HZ;
    time_struct -> microsecs = (tick_count % configTICK_RATE_HZ) * 1e6 / configTICK_RATE_HZ;

    return (OS_SUCCESS);
}/* end OS_GetLocalTime */

/*---------------------------------------------------------------------------------------
 * Name: OS_SetLocalTime
 * 
 * Purpose: This functions set the local time of the machine its on
 * ------------------------------------------------------------------------------------*/
int32 OS_SetLocalTime(OS_time_t *time_struct)
{
    return (OS_SUCCESS);
} /*end OS_SetLocalTime */

/*--------------------------------------------------------------------------------------
 * uint32 FindCreator
 * purpose: Finds the creator of the calling thread
---------------------------------------------------------------------------------------*/
uint32 OS_FindCreator(void)
{
    TaskHandle_t task_handle;
    uint32 i;
    task_handle = xTaskGetCurrentTaskHandle();

    for (i = 0; i < OS_MAX_TASKS; i++)
    {
        if (task_handle == OS_task_table[i].task_handle)
        {
            break;
        }
    }

    return i;
}

/* ---------------------------------------------------------------------------
 * Name: OS_printf 
 * 
 * Purpose: This function abstracts out the printf type statements. This is 
 *          useful for using OS- specific thats that will allow non-polled
 *          print statements for the real time systems. 
 ---------------------------------------------------------------------------*/
void OS_printf( const char *String, ...)
{
    va_list     ptr;
    char msg_buffer [OS_BUFFER_SIZE];

    if ( OS_printf_enabled == TRUE )
    {
       va_start(ptr,String);
       vsnprintf(&msg_buffer[0], (size_t)OS_BUFFER_SIZE, String, ptr);
       va_end(ptr);
    
       msg_buffer[OS_BUFFER_SIZE -1] = '\0';
       printf("%s", &msg_buffer[0]);
    }

    
}/* end OS_printf*/

/* ---------------------------------------------------------------------------
 * Name: OS_printf_disable
 * 
 * Purpose: This function disables the output to the UART from OS_printf.  
 *
 ---------------------------------------------------------------------------*/
void OS_printf_disable(void)
{
   OS_printf_enabled = FALSE;
}/* end OS_printf_disable*/

/* ---------------------------------------------------------------------------
 * Name: OS_printf_enable
 * 
 * Purpose: This function enables the output to the UART through OS_printf.  
 *
 ---------------------------------------------------------------------------*/
void OS_printf_enable(void)
{
   OS_printf_enabled = TRUE;
}/* end OS_printf_enable*/

/*---------------------------------------------------------------------------------------
 *  Name: OS_GetErrorName()
---------------------------------------------------------------------------------------*/
int32 OS_GetErrorName(int32 error_num, os_err_name_t * err_name)
{
    /*
     * Implementation note for developers:
     *
     * The size of the string literals below (including the terminating null)
     * must fit into os_err_name_t.  Always check the string length when
     * adding or modifying strings in this function.  If changing os_err_name_t
     * then confirm these strings will fit.
     */

    os_err_name_t local_name;
    uint32        return_code = OS_SUCCESS;

    if ( err_name == NULL )
    {
       return(OS_INVALID_POINTER);
    }

    switch (error_num)
    {
        case OS_SUCCESS:
            strcpy(local_name,"OS_SUCCESS"); break;
        case OS_ERROR:
            strcpy(local_name,"OS_ERROR"); break;
        case OS_INVALID_POINTER:
            strcpy(local_name,"OS_INVALID_POINTER"); break;
        case OS_ERROR_ADDRESS_MISALIGNED:
            strcpy(local_name,"OS_ADDRESS_MISALIGNED"); break;
        case OS_ERROR_TIMEOUT:
            strcpy(local_name,"OS_ERROR_TIMEOUT"); break;
        case OS_INVALID_INT_NUM:
            strcpy(local_name,"OS_INVALID_INT_NUM"); break;
        case OS_SEM_FAILURE:
            strcpy(local_name,"OS_SEM_FAILURE"); break;
        case OS_SEM_TIMEOUT:
            strcpy(local_name,"OS_SEM_TIMEOUT"); break;
        case OS_QUEUE_EMPTY:
            strcpy(local_name,"OS_QUEUE_EMPTY"); break;
        case OS_QUEUE_FULL:
            strcpy(local_name,"OS_QUEUE_FULL"); break;
        case OS_QUEUE_TIMEOUT:
            strcpy(local_name,"OS_QUEUE_TIMEOUT"); break;
        case OS_QUEUE_INVALID_SIZE:
            strcpy(local_name,"OS_QUEUE_INVALID_SIZE"); break;
        case OS_QUEUE_ID_ERROR:
            strcpy(local_name,"OS_QUEUE_ID_ERROR"); break;
        case OS_ERR_NAME_TOO_LONG:
            strcpy(local_name,"OS_ERR_NAME_TOO_LONG"); break;
        case OS_ERR_NO_FREE_IDS:
            strcpy(local_name,"OS_ERR_NO_FREE_IDS"); break;
        case OS_ERR_NAME_TAKEN:
            strcpy(local_name,"OS_ERR_NAME_TAKEN"); break;
        case OS_ERR_INVALID_ID:
            strcpy(local_name,"OS_ERR_INVALID_ID"); break;
        case OS_ERR_NAME_NOT_FOUND:
            strcpy(local_name,"OS_ERR_NAME_NOT_FOUND"); break;
        case OS_ERR_SEM_NOT_FULL:
            strcpy(local_name,"OS_ERR_SEM_NOT_FULL"); break;
        case OS_ERR_INVALID_PRIORITY:
            strcpy(local_name,"OS_ERR_INVALID_PRIORITY"); break;

        default: strcpy(local_name,"ERROR_UNKNOWN");
                 return_code = OS_ERROR;
    }

    strcpy((char*) err_name, local_name);

    return return_code;
}

/* ---------------------------------------------------------------------------
 * Name: OS_ThreadKillHandler
 * 
 * Purpose: This function allows for a task to be deleted when OS_TaskDelete
 * is called  
----------------------------------------------------------------------------*/

void OS_ThreadKillHandler(int sig)
{
}/*end OS_ThreadKillHandler */


/*
**
**   Name: OS_FPUExcSetMask
**
**   Purpose: This function sets the FPU exception mask
**
**   Notes: The exception environment is local to each task Therefore this must be
**          called for each task that that wants to do floating point and catch exceptions.
*/
int32 OS_FPUExcSetMask(uint32 mask)
{
    /*
    ** Not used.
    ** The FPU usage depends on a FreeRTOS port and its compile time configuration.
    */
    return(OS_SUCCESS);
}

/*
**
**   Name: OS_FPUExcGetMask
**
**   Purpose: This function gets the FPU exception mask
**
**   Notes: The exception environment is local to each task Therefore this must be
**          called for each task that that wants to do floating point and catch exceptions.
*/
int32 OS_FPUExcGetMask(uint32 *mask)
{
    /*
    ** Not used.
    ** The FPU usage depends on a FreeRTOS port and its compile time configuration.
    */
    return(OS_SUCCESS);
}

/****************************************************************************************
                                    INT API
****************************************************************************************/

/****************************************************************************************
                               FreeRTOS specific functions
****************************************************************************************/
/*
**
**   Name: OS_StartScheduler
**
**   Purpose: This function starts FreeRTOS scheduler 
**
**   Notes: This function is specific for FreeRTOS, so it's not mentioned in 
**          "osapi-os-core.h" header file
*/
void OS_StartScheduler(void)
{
   vTaskStartScheduler();
}

/*
**
**   Name: OS_CreateRootTask
**
**   Purpose: This function creates root task which is used to initialize OS API 
**            and start folowing tasks
**
**   Notes: This function is specific for FreeRTOS, so it's not mentioned in 
**          "osapi-os-core.h" header file
*/
int32 OS_CreateRootTask(void (*func_p)(void))
{
  BaseType_t    task_status;
  TaskHandle_t  task_handle;

  if (func_p == NULL)
  {
    return OS_INVALID_POINTER;
  }

  root_func_p = func_p;

  task_status = xTaskCreate( (TaskFunction_t) root_task, 
                             "root", 
                             4096, 
                             NULL, 
                             configMAX_PRIORITIES - 1, 
                             &task_handle );

  if (task_status != pdPASS)
  {
    return OS_ERROR;
  }

  return OS_SUCCESS;
}

/* root_task() calls root_func() which should be defined inside 
   bsp_start.c or bsp_ut.c */
static void root_task( void *pvParameters )
{
  if (root_func_p != NULL)
  {
    root_func_p();
  }

  vTaskDelete( NULL );
}


/*
** FreeRTOSConfig.h dependent functions
*/

/* Used if configASSERT() macro is defined */
void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
  taskENTER_CRITICAL();
  {
        printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
        fflush(stdout);
  }
  taskEXIT_CRITICAL();
  exit(-1);
}

/* Used only if configUSE_MALLOC_FAILED_HOOK is set to 1 */
void vApplicationMallocFailedHook(void)
{
}

/* Used only if configUSE_IDLE_HOOK is set to 1 */
void vApplicationIdleHook(void)
{
}

/* Used only if configUSE_TICK_HOOK is set to 1 */
void vApplicationTickHook(void)
{
}

/* Used only if configUSE_TIMERS and configUSE_DAEMON_TASK_STARTUP_HOOK 
   are both set to 1 */
void vApplicationDaemonTaskStartupHook(void)
{
}

/****************************************************************************************
                                  Table Locking/Unlocking
****************************************************************************************/
/*
**
**   Name: OS_TableLock
**
**   Purpose: This function locks a mutex for mutual exclusion
**
*/
int32 OS_TableLock(SemaphoreHandle_t mutex, TickType_t wait_time)
{
    if (NULL != mutex)
    {
      if (pdPASS == xSemaphoreTake( mutex, wait_time ))
      {
        return OS_SUCCESS;
      }
      else
      {
        return OS_ERROR;
      }
    }
    else
    {
      return OS_INVALID_POINTER;
    }
}

/*
**
**   Name: OS_TableUnlock
**
**   Purpose: This function unlocks the mutex
**
*/
int32 OS_TableUnlock(SemaphoreHandle_t mutex)
{
    if (NULL != mutex)
    {
      if (pdPASS == xSemaphoreGive( mutex ))
      {
        return OS_SUCCESS;
      }
      else
      {
        return OS_ERROR;
      }
    }
    else
    {
      return OS_INVALID_POINTER;
    }
}

