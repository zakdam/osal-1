/******************************************************************************
** File:  bsp_ut.c
**
**
**      This is governed by the NASA Open Source Agreement and may be used,
**      distributed and modified only pursuant to the terms of that agreement.
**
**      Copyright (c) 2004-2015, United States government as represented by the
**      administrator of the National Aeronautics Space Administration.
**      All rights reserved.
**
**
** Purpose:
**   BSP unit test implementation functions.
**
******************************************************************************/

/*
 * NOTE - This entire source file is only relevant for unit testing.
 * It should not be included in a "normal" BSP build.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utbsp.h"
#include "uttest.h"
#include "osapi.h"

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
**  Local Variables
*/
static uint32 CurrVerbosity = (2 << UTASSERT_CASETYPE_PASS) - 1;

void UT_BSP_Setup(const char *Name)
{
    UT_BSP_DoText(UTASSERT_CASETYPE_BEGIN, Name);

    /*
    ** Create local directories for "disk" mount points
    **  See bsp_voltab for the values
    **  
    **  Currently not used since there is no FS support yet.
    */
}

void UT_BSP_StartTestSegment(uint32 SegmentNumber, const char *SegmentName)
{
    char ReportBuffer[128];

    snprintf(ReportBuffer,sizeof(ReportBuffer), "%02u %s", (unsigned int)SegmentNumber, SegmentName);
    UT_BSP_DoText(UTASSERT_CASETYPE_BEGIN, ReportBuffer);
}

void UT_BSP_DoText(uint8 MessageType, const char *OutputMessage)
{
   const char *Prefix;

   if ((CurrVerbosity >> MessageType) & 1)
   {
      switch(MessageType)
      {
      case UTASSERT_CASETYPE_ABORT:
         Prefix = "ABORT";
         break;
      case UTASSERT_CASETYPE_FAILURE:
         Prefix = "FAIL";
         break;
      case UTASSERT_CASETYPE_MIR:
         Prefix = "MIR";
         break;
      case UTASSERT_CASETYPE_TSF:
         Prefix = "TSF";
         break;
      case UTASSERT_CASETYPE_TTF:
          Prefix = "TTF";
          break;
      case UTASSERT_CASETYPE_NA:
         Prefix = "N/A";
         break;
      case UTASSERT_CASETYPE_BEGIN:
         printf("\n"); /* add a bit of extra whitespace between tests */
         Prefix = "BEGIN";
         break;
      case UTASSERT_CASETYPE_END:
         Prefix = "END";
         break;
      case UTASSERT_CASETYPE_PASS:
         Prefix = "PASS";
         break;
      case UTASSERT_CASETYPE_INFO:
         Prefix = "INFO";
         break;
      case UTASSERT_CASETYPE_DEBUG:
         Prefix = "DEBUG";
         break;
      default:
         Prefix = "OTHER";
         break;
      }
      printf("[%5s] %s\n",Prefix,OutputMessage);
   }

   /*
    * If any ABORT (major failure) message is thrown,
    * then actually call abort() to stop the test and dump a core
    */
   if (MessageType == UTASSERT_CASETYPE_ABORT)
   {
       abort();
   }
}

void UT_BSP_DoReport(const char *File, uint32 LineNum, uint32 SegmentNum, uint32 TestSeq, uint8 MessageType, const char *SubsysName, const char *ShortDesc)
{
    uint32 FileLen;
    const char *BasePtr;
    char ReportBuffer[128];

    FileLen = strlen(File);
    BasePtr = File + FileLen;
    while (FileLen > 0)
    {
        --BasePtr;
        --FileLen;
        if (*BasePtr == '/' || *BasePtr == '\\')
        {
            ++BasePtr;
            break;
        }
    }

    snprintf(ReportBuffer,sizeof(ReportBuffer), "%02u.%03u %s:%u - %s",
            (unsigned int)SegmentNum, (unsigned int)TestSeq,
            BasePtr, (unsigned int)LineNum, ShortDesc);

    UT_BSP_DoText(MessageType, ReportBuffer);
}

void UT_BSP_DoTestSegmentReport(const char *SegmentName, const UtAssert_TestCounter_t *TestCounters)
{
    char ReportBuffer[128];

    snprintf(ReportBuffer,sizeof(ReportBuffer),
            "%02u %-20s TOTAL::%-4u  PASS::%-4u  FAIL::%-4u   MIR::%-4u   TSF::%-4u   N/A::%-4u\n",
            (unsigned int)TestCounters->TestSegmentCount,
            SegmentName,
            (unsigned int)TestCounters->TotalTestCases,
            (unsigned int)TestCounters->CaseCount[UTASSERT_CASETYPE_PASS],
            (unsigned int)TestCounters->CaseCount[UTASSERT_CASETYPE_FAILURE],
            (unsigned int)TestCounters->CaseCount[UTASSERT_CASETYPE_MIR],
            (unsigned int)TestCounters->CaseCount[UTASSERT_CASETYPE_TSF],
            (unsigned int)TestCounters->CaseCount[UTASSERT_CASETYPE_NA]);


    UT_BSP_DoText(UTASSERT_CASETYPE_END, ReportBuffer);
}

void UT_BSP_EndTest(const UtAssert_TestCounter_t *TestCounters)
{
   /*
    * Only output a "summary" if there is more than one test Segment.
    * Otherwise it is a duplicate of the report already given.
    */
   if (TestCounters->TestSegmentCount > 1)
   {
       UT_BSP_DoTestSegmentReport("SUMMARY", TestCounters);
   }

   printf("COMPLETE: %u tests Segment(s) executed\n\n", (unsigned int)TestCounters->TestSegmentCount);

   /*
    * Since FreeRTOS simulation is running on Linux PC as any regular application,
    * we may simply call exit(0) to finish tests.
    */
   exit(0);
}

/* root_func should be called inside FreeRTOS root_task */
static void root_func(void)
{
   UT_BSP_Setup("PC-FREERTOS UNIT TEST");

   /*
   ** Call application specific entry point.
   */
   OS_Application_Startup();

   /*
   ** In unit test mode, call the UtTest_Run function (part of UT Assert library)
   */
   UtTest_Run();
   UT_BSP_EndTest(UtAssert_GetCounters());
}

/******************************************************************************
**  Function:  main()
**
**  Purpose:
**    BSP Unit Test Application entry point.
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

