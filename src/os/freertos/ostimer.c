/****************************************************************************************
                                    INCLUDE FILES
****************************************************************************************/

#include "common_types.h"
#include "osapi.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define UNINITIALIZED 0

/****************************************************************************************
                                EXTERNAL FUNCTION PROTOTYPES
****************************************************************************************/

extern uint32 OS_FindCreator(void);
extern int32 OS_TableLock(SemaphoreHandle_t mutex, TickType_t wait_time);
extern int32 OS_TableUnlock(SemaphoreHandle_t mutex);

/****************************************************************************************
                                INTERNAL FUNCTION PROTOTYPES
****************************************************************************************/

void  OS_TicksToUsecs(TickType_t ticks, uint32 *usecs);
void  OS_UsecsToTicks(uint32 usecs, TickType_t *ticks);

/****************************************************************************************
                                     DEFINES
****************************************************************************************/

/****************************************************************************************
                                    LOCAL TYPEDEFS 
****************************************************************************************/

typedef struct 
{
   uint32              free;
   char                name[OS_MAX_API_NAME];
   uint32              creator;
   uint32              start_time;
   uint32              interval_time;
   uint32              accuracy;
   OS_TimerCallback_t  callback_ptr;
   TimerHandle_t       host_timer_handler;

} OS_timer_internal_record_t;

/****************************************************************************************
                                   GLOBAL DATA
****************************************************************************************/

static OS_timer_internal_record_t OS_timer_table[OS_MAX_TIMERS];
static uint32           os_clock_accuracy;

/*
** The Mutex for protecting the above table
*/
static SemaphoreHandle_t OS_timer_table_mut;

/****************************************************************************************
                                INITIALIZATION FUNCTION
****************************************************************************************/
int32 OS_TimerAPIInit ( void )
{
    uint32 i;
    int32  return_code = OS_SUCCESS;

    /*
    ** Mark all timers as available
    */
    for ( i = 0; i < OS_MAX_TIMERS; i++ )
    {
        OS_timer_table[i].free    = TRUE;
        OS_timer_table[i].creator = UNINITIALIZED;
        OS_timer_table[i].name[0] = '\0';
    }

    /*
    ** Store the clock accuracy for 1 tick.
    */
    OS_TicksToUsecs(1, &os_clock_accuracy);  /* TODO: took it from rtems, is it ok? */

    /*
    ** Create the Timer Table mutex
    */
    OS_timer_table_mut = xSemaphoreCreateMutex();
    if (OS_timer_table_mut == NULL)
    {
       return_code = OS_ERROR;
    }

    return(return_code);
}

/****************************************************************************************
                                INTERNAL FUNCTIONS
****************************************************************************************/
/*
** Timer Signal Handler.
** The purpose of this function is to convert the host os timer number to the 
** OSAL timer number and pass it to the User defined signal handler.
** Also it reprograms the timer with the interval time.
*/
static void OS_TimerSignalHandler(TimerHandle_t xTimer)
{
   uint32     i;
   TickType_t timeout;

   for (i = 0; i < OS_MAX_TIMERS; i++ )
   {
      if (( OS_timer_table[i].free == FALSE ) && ( xTimer == OS_timer_table[i].host_timer_handler))
      {
         /*
         ** Call the user function
         */
         (OS_timer_table[i].callback_ptr)(i);

         /*
         ** Only re-arm the timer if the interval time is greater than zero.
         */ 
         if ( OS_timer_table[i].interval_time > 0 )
         {
            /*
            ** Reprogram the timer with the interval time
            */
            OS_UsecsToTicks(OS_timer_table[i].interval_time, &timeout);

            /*
            ** Return values from xTimer...() functions are not used
            */
// TODO: switch to:
//            xTimerChangePeriodFromISR()
            xTimerChangePeriod(xTimer, timeout, portMAX_DELAY);

// TODO: switch to:
//            xTimerStartFromISR()
            xTimerStart(xTimer, portMAX_DELAY);
          }

         break;
      }
   }
}

/******************************************************************************
 **  Function:  OS_UsecToTicks
 **
 **  Purpose:  Convert Microseconds to a number of ticks.
 **
 */
void  OS_UsecsToTicks(uint32 usecs, TickType_t *ticks)
{
   TickType_t ticks_per_sec = configTICK_RATE_HZ;
   uint32     usecs_per_tick;

   usecs_per_tick = (1000000)/ticks_per_sec;

   if ( usecs < usecs_per_tick )
   {
      *ticks = 1;
   }
   else
   {
      *ticks = usecs / usecs_per_tick;
      /* Need to round up?? */ 
   }
}

/******************************************************************************
 **  Function:  OS_TicksToUsec
 **
 **  Purpose:  Convert a number of Ticks to microseconds
 **
 */
void  OS_TicksToUsecs(TickType_t ticks, uint32 *usecs)
{
   TickType_t ticks_per_sec = configTICK_RATE_HZ;
   uint32     usecs_per_tick;

   usecs_per_tick = (1000000)/ticks_per_sec;

   *usecs = ticks * usecs_per_tick;
}

/****************************************************************************************
                                   Timer API
****************************************************************************************/

/******************************************************************************
**  Function:  OS_TimerCreate
**
**  Purpose:  Create a new OSAL Timer
**
**  Arguments:
**
**  Return:
*/
int32 OS_TimerCreate(uint32 *timer_id, const char *timer_name, uint32 *clock_accuracy, OS_TimerCallback_t  callback_ptr)
{
    uint32 possible_tid;
    int32 i;
    TimerHandle_t possible_host_timer_handler;

    /*
    ** Check Parameters
    */
    if (timer_id == NULL || timer_name == NULL || clock_accuracy == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* 
    ** Verify callback parameter
    */
    if (callback_ptr == NULL)
    {
        return OS_TIMER_ERR_INVALID_ARGS;
    }

    /*
    ** we don't want to allow names too long
    ** if truncated, two names might be the same 
    */
    if (strlen(timer_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    OS_TableLock( OS_timer_table_mut, portMAX_DELAY );

    for (possible_tid = 0; possible_tid < OS_MAX_TIMERS; possible_tid++)
    {
        if (OS_timer_table[possible_tid].free == TRUE)
        {
            break;
        }
    }

    if (possible_tid >= OS_MAX_TIMERS || OS_timer_table[possible_tid].free != TRUE)
    {
        OS_TableUnlock( OS_timer_table_mut );
        return OS_ERR_NO_FREE_IDS;
    }

    /*
    ** Check to see if the name is already taken 
    */
    for (i = 0; i < OS_MAX_TIMERS; i++)
    {
        if ((OS_timer_table[i].free == FALSE) &&
             strcmp((char*) timer_name, OS_timer_table[i].name) == 0)
        {
            OS_TableUnlock( OS_timer_table_mut );
            return OS_ERR_NAME_TAKEN;
        }
    }

    /*
    ** Set the possible timer Id to not free so that
    ** no other task can try to use it 
    */
    OS_timer_table[possible_tid].free = FALSE;
    OS_TableUnlock( OS_timer_table_mut );

    OS_timer_table[possible_tid].creator = OS_FindCreator();
    strncpy(OS_timer_table[possible_tid].name, timer_name, OS_MAX_API_NAME);
    OS_timer_table[possible_tid].start_time = 0;
    OS_timer_table[possible_tid].interval_time = 0;
    OS_timer_table[possible_tid].callback_ptr = callback_ptr;

    /*
    ** Create the timer
    ** FreeRTOS prohibits to create a timer with 0 period, so we are using 1 second.
    */
//    status = timer_create(CLOCK_REALTIME, NULL, (timer_t *)&(OS_timer_table[possible_tid].host_timer_handler));
    possible_host_timer_handler = xTimerCreate(timer_name, pdMS_TO_TICKS(1000), pdFALSE, NULL /* void * const pvTimerID */, OS_TimerSignalHandler);
    if (possible_host_timer_handler == NULL)
    {
        OS_timer_table[possible_tid].free = TRUE;
        return (OS_TIMER_ERR_UNAVAILABLE);
    }

    /*
    ** Save FreeRTOS timer handler
    */
    OS_timer_table[possible_tid].host_timer_handler = possible_host_timer_handler;

    /*
    ** Return the clock accuracy to the user
    */
    *clock_accuracy = os_clock_accuracy;

    /*
    ** Return timer ID 
    */
    *timer_id = possible_tid;

    return OS_SUCCESS;
}

/******************************************************************************
**  Function:  OS_TimerSet
**
**  Purpose:  
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
int32 OS_TimerSet(uint32 timer_id, uint32 start_time, uint32 interval_time)
{
   BaseType_t status;
   TickType_t timeout;

   /* 
   ** Check to see if the timer_id given is valid 
   */
   if (timer_id >= OS_MAX_TIMERS)
   {
      return OS_ERR_INVALID_ID;
   }

   OS_TableLock( OS_timer_table_mut, portMAX_DELAY );

   if ( OS_timer_table[timer_id].free == TRUE)
   {
      OS_TableUnlock( OS_timer_table_mut );
      return OS_ERR_INVALID_ID;
   }
   /*
   ** Round up the accuracy of the start time and interval times 
   */
   if ((start_time > 0) && ( start_time < os_clock_accuracy ))
   {
      start_time = os_clock_accuracy;
   }

   if ((interval_time > 0) && ( interval_time < os_clock_accuracy ))
   {
      interval_time = os_clock_accuracy;
   }

   /*
   ** Save the start and interval times 
   */
   OS_timer_table[timer_id].start_time = start_time;
   OS_timer_table[timer_id].interval_time = interval_time;

   OS_UsecsToTicks(start_time, &timeout);

   /*
   ** Program the real timer
   */
   status = xTimerChangePeriod(OS_timer_table[timer_id].host_timer_handler, timeout, portMAX_DELAY);
   OS_TableUnlock( OS_timer_table_mut );
   if (status != pdPASS) 
   {
      return ( OS_TIMER_ERR_INTERNAL);
   }

   /*
   ** Start the real timer
   */
   status = xTimerStart(OS_timer_table[timer_id].host_timer_handler, portMAX_DELAY);
   if (status != pdPASS) 
   {
      return ( OS_TIMER_ERR_INTERNAL);
   }

   return OS_SUCCESS;
}

/******************************************************************************
**  Function:  OS_TimerDelete
**
**  Purpose: 
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
int32 OS_TimerDelete(uint32 timer_id)
{
    BaseType_t status;

    /*
    ** Check to see if the timer_id given is valid 
    */
    if (timer_id >= OS_MAX_TIMERS)
    {
       return OS_ERR_INVALID_ID;
    }

    OS_TableLock( OS_timer_table_mut, portMAX_DELAY );

    if (OS_timer_table[timer_id].free == TRUE)
    {
        OS_TableUnlock( OS_timer_table_mut );
        return OS_ERR_INVALID_ID;
    }

    OS_timer_table[timer_id].free = TRUE;
    OS_TableUnlock( OS_timer_table_mut );

    /*
    ** Delete the timer
    */
    status = xTimerDelete( OS_timer_table[timer_id].host_timer_handler, portMAX_DELAY );
    if (status != pdPASS)
    {
        return (OS_TIMER_ERR_INTERNAL);
    }

    return OS_SUCCESS;
}

/***********************************************************************************
**
**    Name: OS_TimerGetIdByName
**
**    Purpose: This function tries to find a Timer Id given the name 
**             The id is returned through timer_id
**
**    Returns: OS_INVALID_POINTER if timer_id or timer_name are NULL pointers
**             OS_ERR_NAME_TOO_LONG if the name given is to long to have been stored
**             OS_ERR_NAME_NOT_FOUND if the name was not found in the table
**             OS_SUCCESS if success
**             
*/
int32 OS_TimerGetIdByName (uint32 *timer_id, const char *timer_name)
{
    uint32 i;

    if (timer_id == NULL || timer_name == NULL)
    {
        return OS_INVALID_POINTER;
    }

    /* 
    ** a name too long wouldn't have been allowed in the first place
    ** so we definitely won't find a name too long
    */
    if (strlen(timer_name) >= OS_MAX_API_NAME)
    {
        return OS_ERR_NAME_TOO_LONG;
    }

    OS_TableLock( OS_timer_table_mut, portMAX_DELAY );
    for (i = 0; i < OS_MAX_TIMERS; i++)
    {
        if (OS_timer_table[i].free != TRUE &&
                (strcmp (OS_timer_table[i].name , (char*) timer_name) == 0))
        {
            *timer_id = i;
            OS_TableUnlock( OS_timer_table_mut );
            return OS_SUCCESS;
        }
    }

    OS_TableUnlock( OS_timer_table_mut );
    /* 
    ** The name was not found in the table,
    **  or it was, and the sem_id isn't valid anymore 
    */
    return OS_ERR_NAME_NOT_FOUND;

}/* end OS_TimerGetIdByName */

/***************************************************************************************
**    Name: OS_TimerGetInfo
**
**    Purpose: This function will pass back a pointer to structure that contains 
**             all of the relevant info( name and creator) about the specified timer.
**             
**    Returns: OS_ERR_INVALID_ID if the id passed in is not a valid timer 
**             OS_INVALID_POINTER if the timer_prop pointer is null
**             OS_SUCCESS if success
*/
int32 OS_TimerGetInfo (uint32 timer_id, OS_timer_prop_t *timer_prop)
{
    if (timer_prop == NULL)
    {
        return OS_INVALID_POINTER;
    }
    /*
    ** Check to see that the id given is valid 
    */
    if (timer_id >= OS_MAX_TIMERS)
    {
        return OS_ERR_INVALID_ID;
    }

    OS_TableLock( OS_timer_table_mut, portMAX_DELAY );

    if (OS_timer_table[timer_id].free == TRUE)
    {
        OS_TableUnlock( OS_timer_table_mut );
        return OS_ERR_INVALID_ID;
    }

    /* 
    ** put the info into the structure
    */
    timer_prop -> creator       = OS_timer_table[timer_id].creator;
    strcpy(timer_prop -> name, OS_timer_table[timer_id].name);
    timer_prop -> start_time    = OS_timer_table[timer_id].start_time;
    timer_prop -> interval_time = OS_timer_table[timer_id].interval_time;
    timer_prop -> accuracy      = OS_timer_table[timer_id].accuracy;

    OS_TableUnlock( OS_timer_table_mut );

    return OS_SUCCESS;

} /* end OS_TimerGetInfo */

/****************************************************************
 * TIME BASE API
 *
 * This is not implemented by this OSAL, so return "OS_ERR_NOT_IMPLEMENTED"
 * for all calls defined by this API.  This is necessary for forward
 * compatibility (runtime code can check for this return code and use
 * an alternative API where appropriate).
 */

int32 OS_TimeBaseCreate(uint32 *timer_id, const char *timebase_name, OS_TimerSync_t external_sync)
{
    return OS_ERR_NOT_IMPLEMENTED;
}

int32 OS_TimeBaseSet(uint32 timer_id, uint32 start_time, uint32 interval_time)
{
    return OS_ERR_NOT_IMPLEMENTED;
}

int32 OS_TimeBaseDelete(uint32 timer_id)
{
    return OS_ERR_NOT_IMPLEMENTED;
}

int32 OS_TimeBaseGetIdByName (uint32 *timer_id, const char *timebase_name)
{
    return OS_ERR_NOT_IMPLEMENTED;
}

int32 OS_TimerAdd(uint32 *timer_id, const char *timer_name, uint32 timebase_id, OS_ArgCallback_t  callback_ptr, void *callback_arg)
{
    return OS_ERR_NOT_IMPLEMENTED;
}
