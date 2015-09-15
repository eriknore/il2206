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

/* Definition of Task Priorities */
#define TASK0_PRIORITY      6  // highest priority
#define TASK1_PRIORITY      7 

OS_EVENT* aSemaphore;
OS_EVENT* bSemaphore;

int *sharedAddress;

OS_MEM *sharedMem;
int buf[2];

/* TODO */
void task0(void* pdata)
{
    INT8U err;
    
    while(1) {
        OSSemPend(aSemaphore, 0, &err);
        printf("Sending     : %d\n", ++(*sharedAddress));
        OSSemPost(bSemaphore);
        OSSemPend(aSemaphore, 0, &err);
        printf("Receiving   : %d\n", *sharedAddress);
        *sharedAddress *= -1;
        OSSemPost(aSemaphore);
    }
}

/* TODO */
void task1(void* pdata)
{
    INT8U err;
        
    while(1) {
        OSSemPend(bSemaphore, 0, &err);
        *sharedAddress *= (-1);
        OSSemPost(aSemaphore);
    }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
  printf("Lab 2.5 - Shared Memory\n");
  
  INT8U err;
  aSemaphore = OSSemCreate(1);
  bSemaphore = OSSemCreate(0);
  
  sharedMem = OSMemCreate(&buf[0], 2, sizeof(int), &err);
  if(err != OS_ERR_NONE) {
    printf("Error: could not allocate memory! Code = %d\n", err);
    return -1;
  }
  
  sharedAddress = OSMemGet(sharedMem, &err);
  if(err != OS_ERR_NONE) {
    printf("Error: could not get address, code = %d\n", err);
    return -1;
  }
  
  *sharedAddress = 0; /* init value */

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
        
  OSStart();
  return 0;
}
