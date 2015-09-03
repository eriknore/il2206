#include <stdio.h>

extern void puttime(int* timeloc);
extern void puthex(int time);
extern void tick(int* timeloc);
extern void delay (int millisec);
extern int hexasc(int invalue);

#define TRUE 1

int timeloc = 0x5957; /* startvalue given in hexadecimal/BCD-code */

int main ()
{
    
    while (TRUE)
    {
        puttime (&timeloc);
    }
    
    return 0;
}
