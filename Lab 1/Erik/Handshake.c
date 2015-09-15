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

int task0_state = 0;
int task1_state = 0;

/* TODO */
void task0(void* pdata)
{
    INT8U err;
    int* state = pdata;
    
    /* Reset bSemaphore to have 1 key */
    OSSemSet(bSemaphore, 1, OS_ERR_NONE);
    while(1) {
        OSSemPend(aSemaphore, 0, &err);
        printf("Task 0 - State %d\n", *state);
        if(*state == 0) {
            *state = 1;
            OSSemPost(bSemaphore);
        } else {
            *state = 0;
            OSSemPost(aSemaphore);
        }
    }
}

/* TODO */
void task1(void* pdata)
{
    INT8U err;
    int* state = pdata;
    while(1) {
        OSSemPend(bSemaphore, 0, &err);
        printf("Task 1 - State %d\n", *state);
        if(*state == 1) {
            *state = 0;
            OSSemPost(aSemaphore);
        } else {
            *state = 1;
            OSSemPost(bSemaphore);
        }
    }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
  printf("Lab 1.4 - Handshake\n");
  
  aSemaphore = OSSemCreate(1);
  bSemaphore = OSSemCreate(0);

  OSTaskCreateExt
    (task0,                        // Pointer to task code
     &task0_state,                 // Pointer to argument that is
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
     &task1_state,                 // Pointer to argument that is
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
