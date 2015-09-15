// File: TwoTasks.c 

#include <stdio.h>
#include "includes.h"
#include <string.h>

#define DEBUG 1

/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task0_stk[TASK_STACKSIZE];
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    stat_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define TASK0_PRIORITY      6  // highest priority
#define TASK1_PRIORITY      7
#define TASK_STAT_PRIORITY 12  // lowest priority 

OS_EVENT *aSemapaphore;

void printStackSize(INT8U prio)
{
    INT8U err;
    INT8U err_sem;
    OS_STK_DATA stk_data;
    
    err = OSTaskStkChk(prio, &stk_data);
    if (err == OS_NO_ERR) 
    {
        if (DEBUG == 1) {
           OSSemPend(aSemapaphore, 0, &err_sem);
           printf("Task Priority %d - Used: %d; Free: %d\n", 
                   prio, stk_data.OSFree, stk_data.OSUsed);
           OSSemPost(aSemapaphore);
        }
    }
    else
    {
        if (DEBUG == 1)
           printf("Stack Check Error!\n");    
    }
}

/* Prints a message and sleeps for given time interval */
void task0(void* pdata)
{
  INT8U err;
  while (1)
  { 
    char text1[] = "Hello from Task1\n";
    int i;
    
    OSSemPend(aSemapaphore, 0, &err);
    for (i = 0; i < strlen(text1); i++)
        putchar(text1[i]);
    OSSemPost(aSemapaphore);
    OSTimeDlyHMSM(0, 0, 0, 11); // Context Switch to next task

                               // Task will go to the ready state

                               // after the specified delay
  }
}

/* Prints a message and sleeps for given time interval */
void task1(void* pdata)
{
  INT8U err;
  while (1)
  { 
    char text2[] = "Hello from Task2\n";
    int i;
    
    OSSemPend(aSemapaphore, 0, &err);
    for (i = 0; i < strlen(text2); i++)
        putchar(text2[i]);
    OSSemPost(aSemapaphore);
    OSTimeDlyHMSM(0, 0, 0, 4);
  }
}

/* Printing Statistics */
void statisticTask(void* pdata)
{
    while(1)
    {
        printStackSize(TASK0_PRIORITY);
        printStackSize(TASK1_PRIORITY);
        printStackSize(TASK_STAT_PRIORITY);
    }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
  printf("Lab 3 - Two Tasks\n");
  
  aSemapaphore = OSSemCreate(1);

  OSTaskCreateExt
    (task0,                        // Pointer to task code
     NULL,                         // Pointer to argument that is
                                   // passed to task
     &task0_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
     TASK0_PRIORITY,               // Desired Task priority
     TASK0_PRIORITY,               // Task ID
     &task0_stk[0],                // Pointer to bottom of task stack
     TASK_STACKSIZE,               // Stacksize
     NULL,                         // Pointer to user supplied memory
                                   // (not needed here)
     OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
     OS_TASK_OPT_STK_CLR           // Stack Cleared                                 
    );
               
  OSTaskCreateExt
    (task1,                        // Pointer to task code
     NULL,                         // Pointer to argument that is
                                   // passed to task
     &task1_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
     TASK1_PRIORITY,               // Desired Task priority
     TASK1_PRIORITY,               // Task ID
     &task1_stk[0],                // Pointer to bottom of task stack
     TASK_STACKSIZE,               // Stacksize
     NULL,                         // Pointer to user supplied memory
                                   // (not needed here)
     OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
     OS_TASK_OPT_STK_CLR           // Stack Cleared                       
    );  
    
  if (DEBUG == 1)
  {
    OSTaskCreateExt
      (statisticTask,                // Pointer to task code
       NULL,                         // Pointer to argument that is
                                     // passed to task
       &stat_stk[TASK_STACKSIZE-1],  // Pointer to top of task stack
       TASK_STAT_PRIORITY,           // Desired Task priority
       TASK_STAT_PRIORITY,           // Task ID
       &stat_stk[0],                 // Pointer to bottom of task stack
       TASK_STACKSIZE,               // Stacksize
       NULL,                         // Pointer to user supplied memory
                                     // (not needed here)
       OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
       OS_TASK_OPT_STK_CLR           // Stack Cleared                              
      );
  }  
    
  OSStart();
  return 0;
}
