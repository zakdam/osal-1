/******************************************************************************
** File:  bsp_start.c
**
**
**      This is governed by the NASA Open Source Agreement and may be used, 
**      distributed and modified only pursuant to the terms of that agreement. 
**
**      Copyright (c) 2004-2006, United States government as represented by the 
**      administrator of the National Aeronautics Space Administration.  
**      All rights reserved. 
**
**
** Purpose:
**   OSAL BSP main entry point.
**
******************************************************************************/

/*
** OSAL includes 
*/
#include "osapi.h"

/*
** Types and prototypes for this module
*/

/*
**  External Functions
*/
extern int32 OS_CreateRootTask(void (*func_p)(void));
extern void OS_StartScheduler(void);

/*
**  External Declarations
*/
void OS_Application_Startup(void);
                                                                           
/*
** Global variables
*/


/* root_func should be called inside FreeRTOS root_task */
static void root_func(void)
{
   /*
   ** Call application specific entry point.
   */
   OS_Application_Startup();

   /*
   ** OS_IdleLoop() will wait forever and return if
   ** someone calls OS_ApplicationShutdown(TRUE)
   */
   OS_IdleLoop();
}

                                                                                                                                                
/******************************************************************************
**  Function:  main()
**
**  Purpose:
**    BSP Application entry point.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/

int main(int argc, char *argv[])
{
   OS_CreateRootTask(root_func);
   OS_StartScheduler();

   /* Should typically never get here */
   return 0;
}

