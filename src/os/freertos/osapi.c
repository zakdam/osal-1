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
** TODO: place in the correct line.
*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

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

#undef OS_DEBUG_PRINTF 

#define OS_SHUTDOWN_MAGIC_NUMBER    0xABADC0DE

/*
** Global data for the API
*/

/*  
** Tables for the properties of objects 
*/

/*tasks */
typedef struct
{
    int              free;
    TaskHandle_t     task_handle;  /* TODO: check */
    char             name [OS_MAX_API_NAME];
    int              creator;
    uint32           stack_size;
    uint32           priority;
    osal_task_entry  delete_hook_pointer;
}OS_task_internal_record_t;

/* function pointer type */
typedef void (*FuncPtr_t)(void);

/* Tables where the OS object information is stored */
OS_task_internal_record_t       OS_task_table          [OS_MAX_TASKS];

SemaphoreHandle_t OS_task_table_mut;

uint32          OS_printf_enabled = TRUE;
volatile uint32 OS_shutdown = FALSE;

/*
** Local Function Prototypes
*/
void    OS_ThreadKillHandler(int sig );
uint32  OS_FindCreator(void);

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
        strcpy(OS_task_table[i].name,"");
    }

   /*
   ** Initialize the module loader
   */
   #ifdef OS_INCLUDE_MODULE_LOADER
   /* TODO: implement something later if possible */
   #endif

/* TODO: implement */
//   /*
//   ** Initialize the Timer API
//   */
//   return_code = OS_TimerAPIInit();
//   if ( return_code == OS_ERROR )
//   {
//      return(return_code);
//   }

   /*
   ** Initialize the internal table Mutexes
   */

   OS_task_table_mut = xSemaphoreCreateMutex();
   if( OS_task_table_mut == NULL )
   {
      return_code = OS_ERROR;
      return(return_code);
   }

/* TODO: implement */
//   /*
//   ** File system init
//   */
//   return_code = OS_FS_Init();


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
}

/*---------------------------------------------------------------------------------------
   Name: OS_DeleteAllObjects

   Purpose: This task will delete all objects allocated by this instance of OSAL
            May be used during shutdown or by the unit tests to purge all state

   returns: no value
---------------------------------------------------------------------------------------*/
void OS_DeleteAllObjects       (void)
{
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
   /* TODO: is it enough? What about flags? */

   /* Start the scheduler itself. */
   vTaskStartScheduler();

   /* This code will only be reached if the idle task could not be created inside 
   vTaskStartScheduler(). An infinite loop is used to assist debugging by 
   ensuring this scenario does not result in main() exiting. */
   while (1);
}


/*---------------------------------------------------------------------------------------
   Name: OS_ApplicationShutdown

   Purpose: Indicates that the OSAL application should perform an orderly shutdown
       of ALL tasks, clean up all resources, and exit the application.

   returns: none

---------------------------------------------------------------------------------------*/
void OS_ApplicationShutdown(uint8 flag)
{
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
    BaseType_t         FR_status;  /* TODO: check */
    TaskHandle_t       task_handle = NULL;  /* TODO: check */

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
    xSemaphoreTake( OS_task_table_mut, 0 );  /* TODO: fix timings */

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
        xSemaphoreGive( OS_task_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /* Check to see if the name is already taken */ 
    for (i = 0; i < OS_MAX_TASKS; i++)
    {
        if ((OS_task_table[i].free == FALSE) &&
           ( strcmp((char*) task_name, OS_task_table[i].name) == 0)) 
        {
            xSemaphoreGive( OS_task_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }
    
    /* 
    ** Set the possible task Id to not free so that
    ** no other task can try to use it 
    */
    OS_task_table[possible_taskid].free = FALSE;

    xSemaphoreGive( OS_task_table_mut );

    /* TODO: floating point operation flag */

    FR_status = xTaskCreate( (TaskFunction_t) function_pointer, 
                             (char*)task_name, 
                             stack_size, 
                             NULL, 
                             priority, 
                             &task_handle );

    /*
    ** Lock
    */
    xSemaphoreTake( OS_task_table_mut, 0 );  /* TODO: fix timings */

    /* check if xTaskCreate failed */
    if (FR_status != pdPASS)
    {
      OS_task_table[possible_taskid].free = TRUE;
      xSemaphoreGive( OS_task_table_mut );
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
    xSemaphoreGive( OS_task_table_mut );

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
    return OS_SUCCESS;
}/* end OS_TaskDelete */

/*--------------------------------------------------------------------------------------
     Name: OS_TaskExit

    Purpose: Exits the calling task and removes it from the OS_task_table.

    returns: Nothing 
---------------------------------------------------------------------------------------*/

void OS_TaskExit()
{
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
    int          ret;
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

/* TODO: needs to be implemented (used later by OS_TaskGetId()) */
//    /*
//    ** Add pthread variable
//    */
//    ret = pthread_setspecific(thread_key, (void *)task_id);
//    if ( ret != 0 )
//    {
//       #ifdef OS_DEBUG_PRINTF
//          printf("OS_TaskRegister Failed during pthread_setspecific function\n");
//       #endif
//       return(OS_ERROR);
//    }

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
    return (0);
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
    return OS_SUCCESS;
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
    return OS_SUCCESS;
} /* end OS_TaskGetInfo */

/*--------------------------------------------------------------------------------------
     Name: OS_TaskInstallDeleteHandler

    Purpose: Installs a handler for when the task is deleted.

    returns: status
---------------------------------------------------------------------------------------*/

int32 OS_TaskInstallDeleteHandler(osal_task_entry function_pointer)
{
    return(OS_SUCCESS);
}/*end OS_TaskInstallDeleteHandler */

/****************************************************************************************
                                MESSAGE QUEUE API
****************************************************************************************/

/****************************************************************************************
                                  SEMAPHORE API
****************************************************************************************/

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
    return(0);
}

/*---------------------------------------------------------------------------------------
 * Name: OS_GetLocalTime
 * 
 * Purpose: This functions get the local time of the machine its on
 * ------------------------------------------------------------------------------------*/

int32 OS_GetLocalTime(OS_time_t *time_struct)
{
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
    ** TODO: check if implemented.
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
    ** TODO: check if implemented.
    */
    return(OS_SUCCESS);
}

/****************************************************************************************
                                    INT API
****************************************************************************************/

/****************************************************************************************
                               FreeRTOS specific functions
****************************************************************************************/
void vAssertCalled(unsigned long ulLine, const char * const pcFileName)
{
}

void vApplicationMallocFailedHook(void)
{
}

void vApplicationIdleHook(void)
{
}

void vApplicationTickHook(void)
{
}
