/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
Modifications Author: 2023 Mikhail Shomov
*/
#include "coremark.h"
#include "core_portme.h"
#include "gpio.h"
#include "msg.h"
#if HAS_PRINTF
    #include "printf.h"
#endif

#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;
/* Porting : Timing functions
        How to capture time and convert to seconds must be ported to whatever is
   supported by the platform. e.g. Read value from on board RTC, read value from
   cpu clock cycles performance counter etc. Sample implementation for standard
   time.h and windows.h definitions included.
*/

#if XLEN == 32
    CORETIMETYPE clock_start, clock_startH, clock_stop, clock_stopH;
#else
    CORETIMETYPE clock_start, clock_stop;
#endif

/* Define : TIMER_RES_DIVIDER
        Divider to trade off timer resolution and total time that can be
   measured.

        Use lower values to increase resolution, but make sure that overflow
   does not occur. If there are issues with the return value overflowing,
   increase this value.
        */
#define MYTIMEDIFF(fin, ini)       ((fin) - (ini))
#define TIMER_RES_DIVIDER          1
#define SAMPLE_TIME_IMPLEMENTATION 1
#define EE_TICKS_PER_SEC           (CLOCKS_PER_SEC / TIMER_RES_DIVIDER)

/** Define Host specific (POSIX), or target specific global time variables. */
static CORETIMETYPE start_time_val, stop_time_val;

/* Function : start_time
        This function will be called right before starting the timed portion of
   the benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or zeroing some system parameters - e.g. setting the cpu clocks
   cycles to 0.
*/
void start_time(void) {
    #if XLEN == 32
        __asm__ volatile ("rdcycleh %0;" : "=r"(clock_startH));     // RISC-V ISA Counters 
    #endif
    __asm__ volatile ("rdcycle %0;" : "=r"(clock_start));           // RISC-V ISA Counters 
    //TODO check overflow
}
/* Function : stop_time
        This function will be called right after ending the timed portion of the
   benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or other system parameters - e.g. reading the current value of
   cpu cycles counter.
*/
void stop_time(void) {
    #if XLEN == 32
        __asm__ volatile ("rdcycleh %0;" : "=r"(clock_stopH));     // RISC-V ISA Counters 
    #endif
    __asm__ volatile ("rdcycle %0;" : "=r"(clock_stop));           // RISC-V ISA Counters 
    //TODO check overflow
}
/* Function : get_time
        Return an abstract "ticks" number that signifies time on the system.

        Actual value returned may be cpu cycles, milliseconds or any other
   value, as long as it can be converted to seconds by <time_in_secs>. This
   methodology is taken to accommodate any hardware or simulated platform. The
   sample implementation returns millisecs by default, and the resolution is
   controlled by <TIMER_RES_DIVIDER>
*/
// CORE_TICKS get_time(void) {
//     CORE_TICKS elapsed = (CORE_TICKS)(MYTIMEDIFF(stop_time_val, start_time_val));
//     return elapsed;
// }

/* Function : time_in_secs
        Convert the ticks to seconds.

        The <secs_ret> type is used to accommodate systems with no support for
   floating point. Default implementation implemented by the EE_TICKS_PER_SEC
   macro above.
*/
secs_ret time_in_secs() {
    secs_ret retval = 0;
    ee_u32 clock = 0;
    ee_u32 clockH = 0;
    #if XLEN == 32
        if (clock_stop > clock_start) {
            clock = clock_stop - clock_start;
            clockH = clock_stopH - clock_startH;
        }
        else {
            clock = clock_start - clock_stop;
            clockH = clock_stopH - clock_startH - 1;
        }
        retval = clock / EE_TICKS_PER_SEC;
        ee_u32 remains = clock % EE_TICKS_PER_SEC;
        while (clockH > 0) {
            clockH--;
            retval += 0xFFFFFFFF / EE_TICKS_PER_SEC;
            remains += 0xFFFFFFFF % EE_TICKS_PER_SEC;
            if (remains >= EE_TICKS_PER_SEC) {
                retval += remains / EE_TICKS_PER_SEC;
                remains = remains % EE_TICKS_PER_SEC;
            }
        }
        return retval;
    #else
        return (secs_ret)((clock_stop - clock_start) / EE_TICKS_PER_SEC);
    #endif
}

ee_u32 default_num_contexts = 1;

/* Function : portable_init
        Target specific initialization code
        Test for some common mistakes.
*/
void
portable_init(core_portable *p, int *argc, char *argv[]) {
  set_gpio(NOEL_READY);
  uart_init(0xfc001000);
  printf("Hello, NOEL-V!\n");
}
/* Function : portable_fini
        Target specific final code
*/
void
portable_fini(core_portable *p) {
  printf("Bye, NOEL-V!\n");
    while (1)
        __asm__ __volatile__("ADDI x0, x0, 0");		//NOP
}
