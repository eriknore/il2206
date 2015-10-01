3/*
  Functions.c

  Ingo Sander, 2005-10-04
  Johan Wennlund, 2008-09-19

*/

#include <stdio.h>
#include "system.h"
#include "alt_types.h"
#include <time.h>
#include <sys/alt_timestamp.h>
#include <sys/alt_cache.h>

#define N 8192

#define M 32


alt_u32 ticks;
alt_u32 time_1;
alt_u32 time_2;
alt_u32 timer_overhead;

float microseconds(int ticks)
{
  return (float) 1000000 * (float) ticks / (float) alt_timestamp_freq();
}

void initArray(int x[], int n)
{
  int i;
  
  for(i = 0; i < n; i++)
    x[i] = i;
}


void start_measurement()
{
      /* Flush caches */
      alt_dcache_flush_all();
      alt_icache_flush_all();   
      /* Measure */
      alt_timestamp_start();
      time_1 = alt_timestamp();
}

void stop_measurement()
{
      time_2 = alt_timestamp();
      ticks = time_2 - time_1;
}

int main ()
{
  int w[8192];
  
  int a, b;
  int size = M;
  
  printf("Working Set\n\n");
  printf("Information about the system:\n");
  printf("\n");
  printf("Processor Type: %s\n", NIOS2_CPU_IMPLEMENTATION);
  printf("Size Instruction Cache: %d\n", NIOS2_ICACHE_SIZE);
  printf("Line Size Instruction Cache: %d\n", NIOS2_ICACHE_LINE_SIZE);
  printf("Size Data Cache: %d\n", NIOS2_DCACHE_SIZE);
  printf("Line Size Data Cache: %d\n\n\n", NIOS2_DCACHE_LINE_SIZE);

  /* Check if timer available */
  if (alt_timestamp_start() < 0)
    printf("No timestamp device available!");
  else
    {

      /* Print Information about the system */
      printf("Information about the system:\n");
      printf("\n");

      /* Print frequency and period */
      printf("Timestamp frequency: %3.1f MHz\n", (float)alt_timestamp_freq()/1000000.0);
      printf("Timestamp period:    %f ms\n\n", 1000.0/(float)alt_timestamp_freq());  


      /* Calculate Timer Overhead */
      // Average of 10 measurements */
      int i;
      timer_overhead = 0;
      for (i = 0; i < 10; i++) {      
        start_measurement();
        stop_measurement();
        timer_overhead = timer_overhead + time_2 - time_1;
      }
      timer_overhead = timer_overhead / 10;
        
      printf("Timer overhead in ticks: %d\n", (int) timer_overhead);
      printf("Timer overhead in ms:    %f\n", 
	     1000.0 * (float)timer_overhead/(float)alt_timestamp_freq());

       
    // === Task 1 : Block Sizes ===

    // Function 1.1
    initArray(w, 8192);
    printf("Function 1.1: ");
    start_measurement();    
    for (i = 0; i < 128; i++)
       w[i]++;
    stop_measurement();
    printf("%5.2f us", (float) microseconds(ticks - timer_overhead));
    printf("(%d ticks)\n", (int) (ticks - timer_overhead)); 
        
    
      printf("Done!\n");

  }    
  return 0;
}


