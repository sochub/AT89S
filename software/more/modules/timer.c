/*  Copyright (c) 2013, laborer (laborer@126.com)
 *  Licensed under the BSD 2-Clause License.
 */


#include "common.h"
#include "timer.h"


#if defined SDCC || defined __SDCC

/* Read Timer0 as a 16-bit timer.  It's safe to call it even the timer
   is still running.  For SDCC, this routine is implemented using
   inline assembly due to the inefficiency of the code generated by
   SDCC (v3.2). */
uint16_t timer0_get16(void) __naked
{
    __asm
      get16_loop$:
        mov     a, _TH0
        mov     dpl, _TL0
        cjne    a, _TH0, get16_loop$
        mov     dph, a
        ret
    __endasm;
}

#else /* Other compiler */

/* Read Timer0 as a 16-bit timer.  It's safe to call it even the timer
   is still running */
uint16_t timer0_get16(void)
{
    uint8_t th, tl;
    
    do {
        th = TH0;
        tl = TL0;
    } while (th != TH0);

    return ((uint16_t)th << 8) | tl;
}

#endif /* Other compiler */

/* Only 8052 or better MCUs have Timer2 */
#ifdef MICROCONTROLLER_8052

/* Read Timer2 as a 16-bit timer */
uint16_t timer2_get16(void)
{
    uint8_t th, tl;
    
    do {
        th = TH2;
        tl = TL2;
    } while (th != TH2);

    return ((uint16_t)th << 8) | tl;
}

#endif /* MICROCONTROLLER_8052 */

#if defined SDCC || defined __SDCC

static uint16_t __data t0_h32;

/* Read Timer0 as a quasi-32-bit timer.  It's safe to call it even the
   timer is still running.  For SDCC, this routine is implemented
   using inline assembly due to the inefficiency of the code generated
   by SDCC (v3.2) */
uint32_t timer0_get32(void) __naked
{
    __asm
      get32_loop$:
        mov     a, _TH0
        mov     dpl, _TL0
        mov     b, _t0_h32
        mov     dph, (_t0_h32 + 1)
        jb      _TF0, get32_begin$
      get32_end$:
        cjne    a, _TH0, get32_loop$
        xch     a, dph
        ret

      get32_begin$:
        xch     a, b
        inc     a
        jnz     get32_nocarry$
        inc     dph
      get32_nocarry$:
        xch     a, b
        sjmp    get32_end$
    __endasm;
}

#else /* Other compiler */

/* The higher 16-bit of Timer0 in quasi-32-bit mode */
static uint16_t t0_h32;

/* Read Timer0 as a quasi-32-bit timer.  It's safe to call it even the
   timer is still running */
uint32_t timer0_get32(void)
{
    uint16_t    t_h32;
    uint8_t     th, tl;
    
    do {
        th = TH0;
        tl = TL0;
        /* There is a small possibility that the following statement
           yields incorrect result as this 16-bit operation is
           implemented using multiple 8-bit instructions and timer
           interrupt can happen during the execution of these
           instructions modifying t0_h32 in the middle.  To guarantee
           the correctness of function timer0_get32(...), it is better
           to invoke it in a critical region with Timer0 interrupt
           turned off.  However, this critical region causes another
           problem that t0_h32 cannot get update while the timer is
           still running and overflowing. */
        t_h32 = t0_h32;
        /* The following part is to make sure that the higher 16-bit
           of the 32-bit timer increases even if timer0_get32(...) is
           called within a critical region.  Since this 32-bit timer
           uses timer interrupt to maintain the higher 16-bit part, if
           the control flow is in a critical region (or already in an
           interrupt routine), the timer interrupt cannot update the
           higher 16-bit part.  Therefore, we need to do the update
           manually here. */
        if (TF0 == 1) {
            t_h32 += 1;
        }
    } while (th != TH0);

    return (uint32_t)t_h32 << 16 | ((uint16_t)th << 8) | tl;
}

#endif /* Other compiler */

/* Set Timer0 as a quasi-32-bit timer */
void timer0_set32(uint32_t t)
{
    TIMER0_SET16(t);
    t0_h32 = t >> 16;
}

#ifdef TIMER0_CALLBACK
extern void TIMER0_CALLBACK(void) __using 1;
#endif /* TIMER0_CALLBACK */

/* Timer0 interrupt service routine for updating the higher 16-bit in
   quasi-32-bit mode */
void timer0_interrupt(void) __interrupt TF0_VECTOR __using 1
{
    t0_h32 += 1;
#ifdef TIMER0_CALLBACK
    TIMER0_CALLBACK();
#endif /* TIMER0_CALLBACK */
}
