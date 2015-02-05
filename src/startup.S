////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009 Nexell Co., Ltd All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  Nexell informs that this code and information is provided "as is" base
//  and without warranty of any kind, either expressed or implied, including
//  but not limited to the implied warranties of merchantability and/or fitness
//  for a particular puporse.
//
//
//  Module          :
//  File            : Startup.S
//  Description     :
//  Author          : Hans
//  History         :
//                          2013-01-10      Hans
//                          2014.09.03      Hans modify for NXP5430
////////////////////////////////////////////////////////////////////////////////
#include "nx_peridot.h"


        .align 8
/*
 * start and end of BSS
 */

.globl __bss_start
.globl __bss_end__

/*
 * entry point of main function
 */
.global BootMain
.global SubCPUBoot

//;==================================================================
//; Vectors
//;==================================================================
.global Vectors
Vectors:
        b       Reset_Handler   //; 00 - Reset
        b       .               //; 04 - Undefined instructions
        b       .               //; 08 - SWI instructions
        b       .               //; 0C - Instruction fetch aborts
        b       .               //; 10 - Data access aborts
        b       .               //; 14 - Reserved (was address exception)
        b       .               //; 18 - IRQ interrupts
        b       .               //; 1C - FIQ interrupts
.global Sleep
Sleep:
        b       SystemSleep     //; 20

BuildInfo:
        .word   0x68180203      //; 24, Chip name - 6818, Build num - v0.2.03
Reset_Handler:
//;        mcr     p15, 0 r0, c8, c9, 0                //; set debug break;
        mrc     p15, 0, r12, c0, c0, 5              //; Get our cpu id
        tst     r12, #0x4400                        //; check processor affinity
        orrne   r12, r12, #4                        //; mark cpu0 or cpu1
        ands    r12, r12, #0xF                      //; Save CPU id

        msr     CPSR_c, #(Mode_SVC|I_Bit|F_Bit)

        bne     CPUBRINGUP

//;==================================================================
//; Set SMP Start Address to Invalid
//;==================================================================
        mov     r0, #0xC0000000                     //; peri base
        orr     r0, r0, #0x00010000                 //; clk N pwr offset
        orr     r0, r0, #0x00000230                 //; scratch register
        mov     r1, #0xFFFFFFFF                     //; mark to invalid address
        str     r1, [r0]                            //; set invalid jump address
//;==================================================================
//; Clear SRAM
//;==================================================================
#if 0
        mov     r1, #0xFF000000
        orr     r1, r1, #0x00FF0000
        add     r1, r1, #(INTERNAL_SRAM_SIZE/2)
        add     r2, r1, #(INTERNAL_SRAM_SIZE/2)     // half of sram
        mov     r3, #0
clearsram:
        str     r3, [r1], #4
        str     r3, [r1], #4
        str     r3, [r1], #4
        str     r3, [r1], #4
        cmp     r1, r2
        bne     clearsram
#else
        ldr     r1, =__bss_start                    // this is auto-relocated!
        ldr     r2, =__bss_end__                    // this is auto-relocated!

        mov     r3, #0x00000000                     // prepare zero to clear BSS

clbss_l:
        cmp     r1, r2                              // while not at end of BSS
        strlo   r3, [r1]                            // clear 32-bit BSS word
        addlo   r1, r1, #4                          // move to next
        blo     clbss_l
#endif

//;==================================================================
//; Setup stacks
//;==================================================================
CPUBRINGUP:

        .word   0xEC510F1F                          //; mrrc p15, 1, r0, r1, c15        - smp enable
        .word   0xE3800040                          //; orr r0, r0, #0x40
        .word   0xEC410F1F                          //; mcrr p15, 1, r0, r1, c15


        mov     r0, #0xFF000000
        orr     r0, r0, #0x00FF0000
        add     r0, r0, #INTERNAL_SRAM_SIZE

        mov     r1, #0x20
        sub     r2, r12, #1
        and     r2, r2, #0x7                        // cpu 0: -0xE0, cpu 1: -0, cpu 2: -0x20,  3: -0x40, 4: -0x60, 5: -0x80, 6: -0xA0, 7: -0xC0
        mul     r1, r1, r2
        sub     r0, r0, r1

        mov     sp, r0

        movs    r0, r12

        bleq    BootMain                            //; save this in register for possible long jump
        blne    SubCPUBoot
        b       .

//;==================================================================
//; PLL Change
//;==================================================================
        .align 4                                    //; below instruction number is 4, 16bytes

.global __pllchange
__pllchange:                                        //; r0:data r1:pll address r2:delay count
        str     r0, [r1]                            //; pll change start
pllchangedelayloop:                                 //; this code will be already within i-cache. no bus transaction will make
        subs    r2, r2, #1                          //; wait for pll change done
        bne     pllchangedelayloop
        bx      lr

//;==================================================================
//; Self-Refresh Service
//;==================================================================
.global enterSelfRefresh                            //; this code is call linux kernel, so here is virtual memory space.
.global sleepMain
.global vddPowerOff
SystemSleep:        //; r0:alive r1:drex

    //;-----------------------
    //;   Disable MMU
    //;-----------------------

DisableMMU:
        mrc     p15, 0, r1, c1, c0, 0   //; Read control register
        bic     r1, r1, #0x1            //; Disable MMU.
        bic     r1, r1, #0x1000         //; Disable IC.
        bic     r1, r1, #0x4            //; Dsiable DC.

        ldr     r0, =PhysicalStart
        cmp     r0, #0                  //; make sure no stall on "mov pc,r0" below

        //; Disable the MMU.
        //;
        mcr     p15, 0, r1, c1, c0, 0

        //; Jump to the physical address of the 'PhysicalStart' label.
        //;
        mov     pc, r0                  //;  & jump to new physical address
        nop
        nop
        nop

        //; MMU & caches now disabled.
        //;

        .align 4
PhysicalStart:

        //; Disable IRQ & FIQ.
        mrs     r0, CPSR
        orr     r0, r0, #(I_Bit|F_Bit)
        msr     CPSR_cxsf, r0

        //; Set stack pointer
        mov     sp, #0xFF000000
        orr     sp, sp, #0x00FF0000
        add     sp, sp, #INTERNAL_SRAM_SIZE

        //; Goto sleepMain function.
        bl      sleepMain
        bl      vddPowerOff
        b       .                                   //; this time, core power will off and so cpu will die.


//;==================================================================
//; CPU 1, 2, 3, 4, 5, 6, 7 wait
//;==================================================================

.global __WFI
__WFI:
        mov     r0, #0xC0000000                     //; peri base
        orr     r0, r0, #0x00010000                 //; clk N pwr offset
        orr     r0, r0, #0x00000230                 //; scratch register

        mrc     p15, 0, r12, c0, c0, 5              //; Get our cpu id
        tst     r12, #0x4400                        //; check processor affinity
        orrne   r12, r12, #4                        //; mark cpu0 or cpu1
        and     r12, r12, #0xF                      //; Save CPU id

        WFI                                         //; wait for interrupt

        ldr     r1, [r0]                            //; get jump address
        cmp     r1, #0xFFFFFFFF                     //; check if invalid address
        beq     __WFI
        ldr     r2, [r0, #4]
        cmp     r12, r2
        bne     __WFI
        bx      r1                                  //; if valid address, get jump.

.global __WFE
__WFE:
        mov     r0, #0xC0000000                     //; peri base
        orr     r0, r0, #0x00010000                 //; clk N pwr offset
        orr     r0, r0, #0x00000230                 //; scratch register

        mrc     p15, 0, r12, c0, c0, 5              //; Get our cpu id
        tst     r12, #0x4400                        //; check processor affinity
        orrne   r12, r12, #4                        //; mark cpu0 or cpu1
        and     r12, r12, #0xF                      //; Save CPU id

        WFE                                         //; wait for event

        ldr     r1, [r0]                            //; get jump address
        cmp     r1, #0xFFFFFFFF                     //; check if invalid address
        beq     __WFE
        ldr     r2, [r0, #4]
        cmp     r12, r2
        bne     __WFE
        bx      r1                                  //; if valid address, get jump.

#if 0
.global BurstZero
BurstZero:
        push    {r2-r9, lr}
        mvn     r2, r1
        mvn     r3, r1
        mvn     r4, r1
        mvn     r5, r1
        mvn     r6, r1
        mvn     r7, r1
        mvn     r8, r1
        mvn     r9, r1
        stmia   r0!, {r2-r9}
        pop     {r2-r9, pc}

.global BurstWrite
BurstWrite:
        push    {r2-r9, lr}
        mvn     r2, r1
        mvn     r3, r1
        mvn     r4, r1
        mvn     r5, r1
        mov     r6, r1
        mvn     r7, r1
        mvn     r8, r1
        mvn     r9, r1
        stmia   r0!, {r2-r9}
        pop     {r2-r9, pc}

.global BurstRead
BurstRead:
        push    {r2-r9, lr}
        ldmia   r0!, {r2-r9}
        stmia   r1!, {r2-r9}
        pop     {r2-r9, pc}
#endif

//;==================================================================
//; End of startup.s
//;==================================================================
