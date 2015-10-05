#include <stdio.h>
#include "system.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"

extern void simulated_load(INT8U millisec);

#define DEBUG 1

#define HW_TIMER_PERIOD 100 /* 100ms */

/* Button Patterns */

#define GAS_PEDAL_FLAG      0x08
#define BRAKE_PEDAL_FLAG    0x04
#define CRUISE_CONTROL_FLAG 0x02
/* Switch Patterns */

#define TOP_GEAR_FLAG       0x00000002
#define ENGINE_FLAG         0x00000001

/* LED Patterns */

#define LED_RED_0 0x00000001 // Engine
#define LED_RED_1 0x00000002 // Top Gear

#define LED_GREEN_0 0x0001 // Cruise Control activated
#define LED_GREEN_2 0x0004 // Cruise Control Button
#define LED_GREEN_4 0x0010 // Brake Pedal
#define LED_GREEN_6 0x0040 // Gas Pedal

/*
 * Definition of Tasks
 */

#define TASK_STACKSIZE 2048

OS_STK StartTask_Stack[TASK_STACKSIZE]; 
OS_STK ControlTask_Stack[TASK_STACKSIZE]; 
OS_STK VehicleTask_Stack[TASK_STACKSIZE];
OS_STK ButtonIO_Stack[TASK_STACKSIZE];
OS_STK SwitchIO_Stack[TASK_STACKSIZE];
OS_STK ExtraLoad_Stack[TASK_STACKSIZE];
OS_STK Watchdog_Stack[TASK_STACKSIZE];
OS_STK OverloadDetection_Stack[TASK_STACKSIZE];

// Task Priorities
 
#define STARTTASK_PRIO           5
#define VEHICLETASK_PRIO        10
#define CONTROLTASK_PRIO        12
#define BUTTON_PRIO              8
#define SWITCH_PRIO              9
#define WATCHDOG_PRIO            6
#define OVERLOAD_DETECTION_PRIO 13    
#define EXTRA_LOAD_PRIO          7

// Task Periods

#define CONTROL_PERIOD  300
#define VEHICLE_PERIOD  300
#define POLL_PERIOD     300
/*
 * Definition of Kernel Objects 
 */

// Mailboxes
OS_EVENT *Mbox_Throttle;
OS_EVENT *Mbox_Velocity;
OS_EVENT *Mbox_Target;
OS_EVENT *Mbox_OkSignal;

// Semaphores
OS_EVENT *Sem_Veh;
OS_EVENT *Sem_Cont;
OS_EVENT *Sem_Button;
OS_EVENT *Sem_Switch;
OS_EVENT *Sem_Watchdog;
OS_EVENT *Sem_OkSignal;

// SW-Timer
OS_TMR *Tmr_Veh;
OS_TMR *Tmr_Cont;
OS_TMR *Tmr_Button;
OS_TMR *Tmr_Switch;
OS_TMR *Tmr_Watchdog;

/*
 * Types
 */
enum active {on, off};

enum active gas_pedal = off;
enum active brake_pedal = off;
enum active top_gear = off;
enum active engine = off;
enum active cruise_control = off; 

/*
 * Global variables
 */
int delay; // Delay of HW-timer 
INT16U led_green = 0; // Green LEDs
INT32U led_red = 0;   // Red LEDs

void SemCallback(void *ptmr, void *callback_arg)
{
    OS_EVENT **semaphore = (OS_EVENT**) callback_arg;
    OSSemPost(*semaphore);
    //printf("callback call: 0x%x \n", callback_arg);
}

int buttons_pressed(void)
{
  return ~IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_KEYS4_BASE);    
}

int switches_pressed(void)
{
  return IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_TOGGLES18_BASE);    
}

/*
 * ISR for HW Timer
 */
alt_u32 alarm_handler(void* context)
{
  OSTmrSignal(); /* Signals a 'tick' to the SW timers */
  
  return delay;
}

static int b2sLUT[] = {0x40, //0
                 0x79, //1
                 0x24, //2
                 0x30, //3
                 0x19, //4
                 0x12, //5
                 0x02, //6
                 0x78, //7
                 0x00, //8
                 0x18, //9
                 0x3F, //-
};

/*
 * convert int to seven segment display format
 */
int int2seven(int inval){
    return b2sLUT[inval];
}

/*
 * output current velocity on the seven segement display
 */
void show_velocity_on_sevenseg(INT8S velocity){
  int tmp = velocity;
  int out;
  INT8U out_high = 0;
  INT8U out_low = 0;
  INT8U out_sign = 0;

  if(velocity < 0){
     out_sign = int2seven(10);
     tmp *= -1;
  }else{
     out_sign = int2seven(0);
  }

  out_high = int2seven(tmp / 10);
  out_low = int2seven(tmp - (tmp/10) * 10);
  
  out = int2seven(0) << 21 |
            out_sign << 14 |
            out_high << 7  |
            out_low;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_LOW28_BASE,out);
}

/*
 * shows the target velocity on the seven segment display (HEX5, HEX4)
 * when the cruise control is activated (0 otherwise)
 */
void show_target_velocity(INT8U target_vel)
{
  int out;
  INT8U out_high = 0;
  INT8U out_low = 0;

  out_high = int2seven(target_vel / 10);
  out_low = int2seven(target_vel - (target_vel/10) * 10);
  
  out =     out_high << 7  |
            out_low;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_HIGH28_BASE,out);
}

/*
 * indicates the position of the vehicle on the track with the four leftmost red LEDs
 * LEDR17: [0m, 400m)
 * LEDR16: [400m, 800m)
 * LEDR15: [800m, 1200m)
 * LEDR14: [1200m, 1600m)
 * LEDR13: [1600m, 2000m)
 * LEDR12: [2000m, 2400m]
 */
void show_position(INT16U position)
{
    /* position in dm, not in m! */
    if      (position < 4000)  led_red = (0xfff & led_red) | 0x1  << 17;
    else if (position < 8000)  led_red = (0xfff & led_red) | 0x3  << 16;
    else if (position < 12000) led_red = (0xfff & led_red) | 0x7  << 15;
    else if (position < 16000) led_red = (0xfff & led_red) | 0xf  << 14;
    else if (position < 20000) led_red = (0xfff & led_red) | 0x1f << 13;
    else if (position < 24000) led_red = (0xfff & led_red) | 0x3f << 12;
}

/*
 * The function 'adjust_position()' adjusts the position depending on the
 * acceleration and velocity.
 */
 INT16U adjust_position(INT16U position, INT16S velocity,
                        INT8S acceleration, INT16U time_interval)
{
  INT16S new_position = position + velocity * time_interval / 1000
    + acceleration / 2  * (time_interval / 1000) * (time_interval / 1000);

  if (new_position > 24000) {
    new_position -= 24000;
  } else if (new_position < 0){
    new_position += 24000;
  }
  
  show_position(new_position);
  return new_position;
}
 
/*
 * The function 'adjust_velocity()' adjusts the velocity depending on the
 * acceleration.
 */
INT16S adjust_velocity(INT16S velocity, INT8S acceleration,  
               enum active brake_pedal, INT16U time_interval)
{
  INT16S new_velocity;
  INT8U brake_retardation = 200;

  if (brake_pedal == off)
    new_velocity = velocity  + (float) (acceleration * time_interval) / 1000.0;
  else {
    if (brake_retardation * time_interval / 1000 > velocity)
      new_velocity = 0;
    else
      new_velocity = velocity - brake_retardation * time_interval / 1000;
  }
  
  return new_velocity;
}

/*
 * The task 'VehicleTask' updates the current velocity of the vehicle
 */
void VehicleTask(void* pdata)
{ 
  INT8U err;  
  void* msg;
  INT8U* throttle; 
  INT8S acceleration;  /* Value between 40 and -20 (4.0 m/s^2 and -2.0 m/s^2) */
  INT8S retardation;   /* Value between 20 and -10 (2.0 m/s^2 and -1.0 m/s^2) */
  INT16U position = 0; /* Value between 0 and 20000 (0.0 m and 2000.0 m)  */
  INT16S velocity = 0; /* Value between -200 and 700 (-20.0 m/s amd 70.0 m/s) */
  INT16S wind_factor;   /* Value between -10 and 20 (2.0 m/s^2 and -1.0 m/s^2) */

  printf("Vehicle task created!\n");

  while(1)
    {
      err = OSMboxPost(Mbox_Velocity, (void *) &velocity);
        
      OSSemPend(Sem_Veh, 0, &err);
      /* Non-blocking read of mailbox: 
       - message in mailbox: update throttle
       - no message:         use old throttle
      */
      msg = OSMboxPend(Mbox_Throttle, 1, &err); 
      if (err == OS_NO_ERR) 
         throttle = (INT8U*) msg;

      /* Retardation : Factor of Terrain and Wind Resistance */
      if (velocity > 0)
         wind_factor = velocity * velocity / 10000 + 1;
      else 
         wind_factor = (-1) * velocity * velocity / 10000 + 1;
         
      if (position < 4000) 
         retardation = wind_factor; // even ground
      else if (position < 8000)
          retardation = wind_factor + 15; // traveling uphill
        else if (position < 12000)
            retardation = wind_factor + 25; // traveling steep uphill
          else if (position < 16000)
              retardation = wind_factor; // even ground
            else if (position < 20000)
                retardation = wind_factor - 10; //traveling downhill
              else
                  retardation = wind_factor - 5 ; // traveling steep downhill
                  
      acceleration = *throttle / 2 - retardation;     
      position = adjust_position(position, velocity, acceleration, 300); 
      velocity = adjust_velocity(velocity, acceleration, brake_pedal, 300); 
      printf("Position: %dm\n", position / 10);
      printf("Velocity: %4.1fm/s\n", velocity /10.0);
      printf("Throttle: %dV\n", *throttle / 10);
      show_velocity_on_sevenseg((INT8S) (velocity / 10));
    }
} 
 
/*
 * The task 'ControlTask' is the main task of the application. It reacts
 * on sensors and generates responses.
 */

void turn_off_cruise_control(); /* pre declaration */

void ControlTask(void* pdata)
{
  INT8U err;
  INT8U throttle = 0; /* Value between 0 and 80, which is interpreted as between 0.0V and 8.0V */
  void* msg;
  INT16S* current_velocity;
  INT8S diff;
  
  turn_off_cruise_control();

  printf("Control Task created!\n");

  while(1)
    {
      msg = OSMboxPend(Mbox_Velocity, 0, &err);
      current_velocity = (INT16S*) msg;
      
      msg = OSMboxAccept(Mbox_Target);
      if(msg == NULL)
        diff = 0;
      else 
        diff = *((INT16S *) msg) - *current_velocity;
        
      OSSemPend(Sem_Cont, 0, &err);

      /* P-controller starts here */
      if (cruise_control == on) {
        if(diff < -5) { 
            throttle = 0;
        } else if(diff > 5) {
            throttle = 80;
        } else {
            throttle = 10;
        }
      } else if (gas_pedal == on) {
        throttle = 80;
      } else {
        throttle = 0;
      }
      
      err = OSMboxPost(Mbox_Throttle, (void *) &throttle);
      
      IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE, led_green);
    }
}

void turn_off_cruise_control()
{
    led_green = led_green & ~LED_GREEN_2; // LEDG2 off
    cruise_control = off;
    show_target_velocity(0);
}

// button-polling task
void ButtonIO(void* pdata)
{
    INT8U err;
    INT16S target;
    INT16S* current;
    int keys;
  while(1)
  {  
    OSSemPend(Sem_Button,0,&err);
    keys = buttons_pressed();
    if( keys & CRUISE_CONTROL_FLAG ) {  /* key 1 */
        if(cruise_control == on) {
            turn_off_cruise_control();
            target = 0; 
        } else if(top_gear == on) {
            current = (INT16S*) OSMboxPend(Mbox_Velocity, 0, &err);
            if( *current >= 200) {
                target = *current;
                show_target_velocity((INT8U) (target / 10));
                led_green = led_green | LED_GREEN_2; // LEDG2 on
                cruise_control = on;
            }
        }
    }
    
    if( keys & BRAKE_PEDAL_FLAG ) {     /* key 2 */
        if(brake_pedal == off) {
            brake_pedal = on;
            led_green = led_green | LED_GREEN_4; // LEDG4 on
            turn_off_cruise_control();
            //printf("brake pedal down\n");
        }
    } else {
        if(brake_pedal == on) {
            led_green = led_green & ~LED_GREEN_4; // LEDG4 off
            brake_pedal = off;
            //printf("brake pedal up\n");
        }
    }
    
    if( (keys & GAS_PEDAL_FLAG) && engine == on) {       /* key 3 */
        if(gas_pedal == off) {
            gas_pedal = on;
            led_green = led_green | LED_GREEN_6; // LEDG6 on
            turn_off_cruise_control();
            //printf("gas pedal down\n");
        }
    } else {
        if(gas_pedal == on) {
            led_green = led_green & ~LED_GREEN_6; // LEDG6 off
            gas_pedal = off;
            //printf("gas pedal up\n");
        }
    }
    
    //printf("ButtonIO working \n");
    if(cruise_control == on)
        err = OSMboxPost(Mbox_Target, (void *) &target);
  }
}

void SwitchIO(void* pdata)
{
  INT8U err;
  int switches;
  INT16S* vel;
  
  while(1)
  {
    OSSemPend(Sem_Switch,0,&err);
    //printf("SwitchIO working \n");
    switches = switches_pressed() & 0x3;
    if(switches & ENGINE_FLAG) {
        if(engine == off) {
            led_red = led_red | LED_RED_0;
            engine = on;
            //printf("Turning engine on\n");
        } // else do nothing
    } else {
        if(engine == on) {
            vel = (INT16S*) OSMboxPend(Mbox_Velocity, 0, &err);
            OSMboxPost(Mbox_Velocity, vel);
            if(*vel == 0) {
                led_red = led_red & ~LED_RED_0;
                engine = off;
                //printf("Turning engine off\n");
            }
        } // else do nothing
            
    }
    
    if(switches & TOP_GEAR_FLAG) {
        if(top_gear == off) {
            led_red = led_red | LED_RED_1;
            top_gear = on;
            //printf("Turning top gear on\n");
        } // else do nothing
    } else {
        if(top_gear == on) {
            led_red = led_red & ~LED_RED_1;
            top_gear = off;
            turn_off_cruise_control();
            //printf("Turning top gear off\n");
        } // else do nothing
    }
  }
}

/* Extra load will be modelled as a delay between 0-100ms set by
 * the switches SW4-SW9 interpreted as a binary number and dubbled. 
 * The extra load is therefore measured in increments of 2ms.
 */
 
void simulated_load2(int delay)
{
    OSTimeDly(delay);
}
 
void ExtraLoad(void *pdata)
{
  // With POLL_PERIOD = 300 then 1% extra load = 3ms
  INT16U extra_load, factor = POLL_PERIOD / 100;
  INT32S switches;
  while(1)
  {
    switches = (switches_pressed() >> 4) & 0x3F; // mask SW9-SW4
    extra_load = (INT8U) switches << 1; // extra_load = 2 * <value of switches>
    if(extra_load > 100)
        extra_load = 100; // 100% is max load
    // set LEDR9-LEDR4 to 0 and then mask with switches
    led_red = (led_red & ~0x3F0) | (switches << 4);
    IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE, led_red);
    
    /* now simulate the work by delaying */
    extra_load *= factor;
    simulated_load(extra_load);
    // yield the rest of the time
    OSTimeDlyHMSM(0,0,0, (POLL_PERIOD-extra_load));
  } 
}

void Watchdog(void *pdata)
{
    INT8U err;
    char *magic; // actually not checked
    while(1)
    {
        OSSemPend(Sem_Watchdog, 0, &err);
        // post to semaphore blocking the ok signal
        OSSemPost(Sem_OkSignal);
        // wait for the signal with a timeout
        // note timeout in ticks 1000 ticks/s => 1 tick = 1 ms
        magic = (char *) OSMboxPend(Mbox_OkSignal, POLL_PERIOD, &err); // max, change later!
        // overload error message if it timed out
        if(err == OS_ERR_TIMEOUT) {
            printf("WARNING: Overload detected!\n");
        }
        // could check magic value here...
    }   
}

void OverloadDetection(void *pdata)
{
    INT8U err;
    char magic[] = "magic";
    while(1)
    {
        OSSemPend(Sem_OkSignal, 0, &err);
        OSMboxPost(Mbox_OkSignal, &magic);
    }   
}
    
/* 
 * The task 'StartTask' creates all other tasks kernel objects and
 * deletes itself afterwards.
 */ 

void StartTask(void* pdata)
{
  INT8U err;
  void* context;

  static alt_alarm alarm;     /* Is needed for timer ISR function */
  
  /* Base resolution for SW timer : HW_TIMER_PERIOD ms */
  delay = alt_ticks_per_second() * HW_TIMER_PERIOD / 1000; 
  printf("delay in ticks %d\n", delay);

  /* 
   * Create Hardware Timer with a period of 'delay' 
   */
  if (alt_alarm_start (&alarm,
      delay,
      alarm_handler,
      context) < 0)
      {
          printf("No system clock available!n");
      }

  /* 
   * Create and start Software Timer 
   */
  // need x_PERIOD in 1/10th of 1s => x_PERIOD/100 
  Tmr_Cont = OSTmrCreate(0, (CONTROL_PERIOD/100), OS_TMR_OPT_PERIODIC, SemCallback, &Sem_Cont, NULL, &err);
  Tmr_Veh = OSTmrCreate(0, (VEHICLE_PERIOD/100), OS_TMR_OPT_PERIODIC, SemCallback, &Sem_Veh, NULL, &err);
  Tmr_Button = OSTmrCreate(0, (POLL_PERIOD/100), OS_TMR_OPT_PERIODIC, SemCallback, &Sem_Button, NULL, &err);
  Tmr_Switch = OSTmrCreate(0, (POLL_PERIOD/100), OS_TMR_OPT_PERIODIC, SemCallback, &Sem_Switch, NULL, &err);
  Tmr_Watchdog = OSTmrCreate(0, (POLL_PERIOD/100), OS_TMR_OPT_PERIODIC, SemCallback, &Sem_Watchdog, NULL, &err);
  
  OSTmrStart(Tmr_Cont, &err);
  OSTmrStart(Tmr_Veh, &err);
  OSTmrStart(Tmr_Button, &err);
  OSTmrStart(Tmr_Switch, &err);
  OSTmrStart(Tmr_Watchdog, &err);  
  
  /*
   * Creation of Kernel Objects
   */
  
  // Mailboxes
  Mbox_Throttle = OSMboxCreate((void*) 0); /* Empty Mailbox - Throttle */
  Mbox_Velocity = OSMboxCreate((void*) 0); /* Empty Mailbox - Velocity */
  Mbox_Target   = OSMboxCreate((void*) 0); /* Empty Mailbox - Target */
  Mbox_OkSignal = OSMboxCreate((void*) 0); /* Empty Mailbox - Detection */
  
  // Semaphores
  Sem_Veh = OSSemCreate(1);
  Sem_Cont = OSSemCreate(1);
  Sem_Button = OSSemCreate(1);
  Sem_Switch = OSSemCreate(1);
  Sem_Watchdog = OSSemCreate(1);
  Sem_OkSignal = OSSemCreate(0); // incremented by watchdog
   
  /*
   * Create statistics task
   */

  OSStatInit();

  /* 
   * Creating Tasks in the system 
   */


  err = OSTaskCreateExt(
       ControlTask, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &ControlTask_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       CONTROLTASK_PRIO,
       CONTROLTASK_PRIO,
       (void *)&ControlTask_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
       VehicleTask, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &VehicleTask_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       VEHICLETASK_PRIO,
       VEHICLETASK_PRIO,
       (void *)&VehicleTask_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
       ButtonIO, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &ButtonIO_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       BUTTON_PRIO,
       BUTTON_PRIO,
       (void *)&ButtonIO_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
       SwitchIO, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &SwitchIO_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       SWITCH_PRIO,
       SWITCH_PRIO,
       (void *)&SwitchIO_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);
       
   err = OSTaskCreateExt(
       ExtraLoad, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &ExtraLoad_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       EXTRA_LOAD_PRIO,
       EXTRA_LOAD_PRIO,
       (void *)&ExtraLoad_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);
       
   err = OSTaskCreateExt(
       Watchdog, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &Watchdog_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       WATCHDOG_PRIO,
       WATCHDOG_PRIO,
       (void *)&Watchdog_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);
       
    err = OSTaskCreateExt(
       OverloadDetection, // Pointer to task code
       NULL,        // Pointer to argument that is
                    // passed to task
       &OverloadDetection_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack
       OVERLOAD_DETECTION_PRIO,
       OVERLOAD_DETECTION_PRIO,
       (void *)&OverloadDetection_Stack[0],
       TASK_STACKSIZE,
       (void *) 0,
       OS_TASK_OPT_STK_CHK);
  
  printf("All Tasks and Kernel Objects generated!\n");

  /* Task deletes itself */

  OSTaskDel(OS_PRIO_SELF);
}

/*
 *
 * The function 'main' creates only a single task 'StartTask' and starts
 * the OS. All other tasks are started from the task 'StartTask'.
 *
 */

int main(void) {
    
  printf("Lab: Cruise Control\n");
 
  OSTaskCreateExt(
     StartTask, // Pointer to task code
         NULL,      // Pointer to argument that is
                    // passed to task
         (void *)&StartTask_Stack[TASK_STACKSIZE-1], // Pointer to top
                             // of task stack 
         STARTTASK_PRIO,
         STARTTASK_PRIO,
         (void *)&StartTask_Stack[0],
         TASK_STACKSIZE,
         (void *) 0,  
         OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
         
  OSStart();
  
  return 0;
}
