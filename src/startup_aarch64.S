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
#include "cfgBootDefine.h"


        .align 9    // 2^9 = 512 bytes sector size
/*
 * start and end of BSS
 */

.globl __bss_start__
.globl __bss_end__

/*
 * entry point of main function
 */
.global Vectors
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
        b       .//;SystemSleep     //; 20

BuildInfo:
        .word   0x68180306      //; 24, Chip name - 6818, Build num - v0.3.06
Reset_Handler:

        msr     DAIFSet, #(I_Bit|F_Bit)        //; disable interrupt & fast interrupt

        mrs     x0, MPIDR_EL1              //; Get our cpu id
        and     x12, x0, #0x3              //; cpu id
        lsr     x0, x0, #8
        and     x0, x0, #0x0F              //; check processor affinity
        orr     x12, x12, x0               //; mark cpu group 0 or 1

        cmp     x12, xzr
        b.ne    CPUBRINGUP


//;==================================================================
//; Clear SRAM
//;==================================================================
        //; Clear area of global data.
        mov     w2, wzr
        ldr     w1, =g_suspend_ready
        str     w2, [x1]
        ldr     w1, =g_wakeup_ready
        str     w2, [x1]
        ldr     w1, =g_store_stack
        str     w2, [x1]
        ldr     w1, =g_store_vaddr
        str     w2, [x1], #4
        str     w2, [x1], #4
        str     w2, [x1], #4
        str     w2, [x1], #4
        str     w2, [x1], #4
        str     w2, [x1], #4
        str     w2, [x1], #4
        str     w2, [x1], #4

#if 0
        ldr     r1, =__bss_start__                  // this is auto-relocated!

        mov     r3, #INTERNAL_SRAM_SIZE
        mov     r4, #0xFF000000
        orr     r4, r4, #0x00FF0000
        sub     r2, r3, #(4096)                     // sram_size - stack_size
        orr     r2, r2, r4
#else

        ldr     x1, =__bss_start__                  // this is auto-relocated!
        ldr     x2, =__bss_end__                    // this is auto-relocated!
#endif

        mov     x3, xzr                             // prepare zero to clear BSS

clbss_l:
        cmp     x1, x2                              // while not at end of BSS
        b.hs    clbss_e                             // higher or same
        str     x3, [x1], #8                        // clear 64-bit BSS word
        b.lo    clbss_l
clbss_e:

//;==================================================================
//; Setup stacks
//;==================================================================
CPUBRINGUP:

        mrs     x0, S3_1_c15_c2_1
        orr     x0, x0, #(1<<6)                     // [6] SMPEN
        msr     S3_1_c15_c2_1, x0

        mrs     x0, SCR_EL3
        orr     x0, x0, #(3<<1)                     // route irq, fiq to EL3
        msr     SCR_EL3, x0

        mrs     x0, SCTLR_EL3
        orr        x0, x0, #(1<<12)                 // enable icache
        msr     SCTLR_EL3, x0

        mov     w0, #0xFF000000
        orr     w0, w0, #0x00FF0000
        add     x0, x0, #INTERNAL_SRAM_SIZE

        mov     w1, #0x40                           // AArch64 stack point must be aligned by 16bytes
        sub     w2, w12, #1
        and     w2, w2, #0x7                        // cpu 0: -0x1C0, cpu 1: -0, cpu 2: -0x40,  3: -0x80, 4: -0xC0, 5: -0x100, 6: -0x140, 7: -0x180
        mul     w1, w1, w2
        sub     x0, x0, x1

        mov     sp, x0

        mov     x0, x12

        cmp     x0, xzr
        b.ne    subcpubootgo
        bl      BootMain                            //; save this in register for possible long jump
subcpubootgo:
        bl      SubCPUBoot
        b       .

//;==================================================================
//; PLL Change
//;==================================================================
        .align 6                                    //; below instruction number is 6, 2^6=64bytes

.global __pllchange
__pllchange:                                        //; r0:data r1:pll address r2:delay count
        mov     w3, #0x1000                         //; for icache prefetch
pllchangedelayloop:                                 //; this code will be already within i-cache. no bus transaction will make
        subs    w3, w3, #1                          //; wait for pll change done
        b.ne    notadapt
        str     w0, [x1]                            //; pll change start
        mov     w3, w2                              //; real delay time set
        cmp     w3, wzr
        b.ne    postloop
notadapt:
        cmp     w3, #0x1000
postloop:
        b.ne    pllchangedelayloop
        ret
        nop

//;==================================================================
//; Self-Refresh Service
//;==================================================================
.global enterSelfRefresh                            //; this code is call linux kernel, so here is virtual memory space.
.global sleepMain
.global vddPowerOff
//.global subCPUsleep
//.global jumpToVirtualArea

subCPUsleep:
        mov     w0, #1
        ldr     w1, =g_suspend_ready
        ldr     w2, [x1]
//;        orr     w2, w2, w0, lsl w3
        str     w2, [x1]

        wfi
        b       .
        ldr     w0, =0xfffffb60
        ldursw  x0, [x0]
        ldr     w0, =0xfffffb60
        ldrsw   x0, [x0]
        ldr     w0, =0xfffffb60
        ldr     x0, [x0]
#if 0
        ldr     w0, =g_wakeup_ready
recheck_cpu0_ready:
        ldr     w1, [x0]
        cmp     w1, #0
        b.eq    recheck_cpu0_ready

        b       jumpToVirtualArea

        .align 4
SystemSleep:                            //; r0:alive r1:drex

        //; Disable IRQ & FIQ.
        cpsid   if

        //; Store reg values
        stmfd   sp!, {w0-w4, lr}

        //; dcache off
        bl      v7_flush_cache_all
        mrs     x1, SCTLR_EL3           //; Read control register
        bic     x1, x1, #0x4            //; Disable DC.
        msr     SCTLR_EL3, x1

        mov     x4, pc
        bic     x4, x4, #0xFF
        bic     x4, x4, #0xFF00


    //;-----------------------
    //;   Disable MMU
    //;-----------------------

jumpToPhysicalArea:
DisableMMU:
//        mrc     p15, 0, r1, c1, c0, 0   //; Read control register
        bic     x1, x1, #0x1            //; Disable MMU.
//        bic     w1, w1, #0x1000         //; Disable IC.
//        bic     w1, w1, #0x4            //; Dsiable DC.

        ldr     x0, =PhysicalStart
        cmp     w0, #0                  //; make sure no stall on "mov pc,r0" below

        //; Disable the MMU.
        //;
        msr     SCTLR_EL3, x1

        //; Jump to the physical address of the 'PhysicalStart' label.
        //;
        mov     pc, x0                  //;  & jump to new physical address
        nop
        nop
        nop

        //; MMU & caches now disabled.
        //;


PhysicalStart:

        //; ALive interrupt foward to only cpu0.
        ldr     w0, =0xC0009824
        ldr     w1, [x0]
        bic     w2, w1, #0xFF
        orr     w1, w2, #0x01
        str     w1, [x0]

        //; get cpu nember
        mrs     x3, MPIDR_EL1           //; Get our cpu id
        tst     w3, #0x4400             //; check processor affinity
        orr.ne  w3, w3, #4              //; mark cpu0 or cpu1
        ands    w3, w3, #0xF            //; Save CPU id

        ldr     w0, =g_store_vaddr
        lsl     w2, w3, #2
        str     w4, [x0, w2]
        b.ne     subCPUsleep             //; if cpu0 then bypass.

#if 0
#if (MULTICORE_SLEEP_CONTROL == 1)
#if 0
        //; wait suspend status
        ldr     w0, =g_suspend_ready
        ldr     w1, =0xFE
recheck_suspend_ready:
        ldr     w2, [x0]
        cmp     w1, w2
        b.ne    recheck_suspend_ready
#else

        //; wait suspend status
        ldr     r1, =0xFE
recheck_suspend_ready:
        ldr     r0, =0xC0011168         //; TIEOFF90
        ldr     r2, [r0]
        and     r2, r2, #0xF

        ldr     r0, =0xC00111AC         //; TIEOFF107
        ldr     r3, [r0]
        and     r3, r3, #0xF

        orr     r2, r2, r3, lsl #4

        cmp     r1, r2
        bne     recheck_suspend_ready
#endif
#endif  //; #if (MULTICORE_SLEEP_CONTROL == 1)
#endif


        //; Store stack pointer
        ldr     w0, =g_store_stack
        str     sp, [x0]

        //; Set stack pointer
        mov     sp, #0xFF000000
        orr     sp, sp, #0x00FF0000
        add     sp, sp, #INTERNAL_SRAM_SIZE

        //; Store link address
        push    {lr}


        //; Goto sleepMain function.
        bl      sleepMain

        //; Awake other cpus.
        ldr     w0, =0xC0009F00
        ldr     w1, =(0xFE << 16)
        str     w1, [x0]

        //; Set cpu0 status already.
        ldr     w0, =g_wakeup_ready
        mov     w1, #1
        str     w1, [x0]

        //; Restore link address
        pop     {lr}

        //; Restore stack pointer
        ldr     w0, =g_store_stack
        ldr     sp, [x0]

    //;-----------------------
    //;   Eisable MMU
    //;-----------------------

        .align 4
jumpToVirtualArea:
EnableMMU:
        ldr     w0, =g_store_vaddr

        //; get cpu nember
        mrs     x3, MPIDR_EL1           //; Get our cpu id
        tst     w3, #0x4400             //; check processor affinity
        orr.ne   w3, w3, #4             //; mark cpu0 or cpu1
        and     w3, w3, #0xF            //; Save CPU id

        lsl     w2, w3, #2
        ldr     w4, [w0, w2]

        mrs     x1, SCTLR_EL3           //; Read control register
        orr     w1, w1, #0x1            //; Enable MMU.
        orr     w1, w1, #0x1000         //; Enable IC.
        orr     w1, w1, #0x4            //; Enable DC.

        ldr     w0, =VirtualStart
        bic     w0, #0xFF000000
        bic     w0, #0x00FF0000
        orr     w0, w0, w4
        cmp     w0, #0                  //; make sure no stall on "mov pc,r0" below

        //; Enable the MMU.
        //;
        msr     SCTLR_EL3, x1

        //; Jump to the virtual address of the 'VirtualStart' label.
        //;
        mov     pc, x0                  //;  & jump to new virtual address
        nop
        nop
        nop

        //; MMU & caches now enabled.
        //;


VirtualStart:
        //; Restore reg values
        ldmfd   sp!, {w0-w4, lr}

        mov     pc, lr
        b       .

#endif

#if 0
.global BurstZero
BurstZero:
        push    {x2-x9, lr}
        mvn     x2, x1
        mvn     x3, x1
        mvn     x4, x1
        mvn     x5, x1
        mvn     x6, x1
        mvn     x7, x1
        mvn     x8, x1
        mvn     x9, x1
        stmia   x0!, {x2-x9}
        pop     {x2-x9, pc}

.global BurstWrite
BurstWrite:
        push    {x2-x9, lr}
        mvn     x2, x1
        mvn     x3, x1
        mvn     x4, x1
        mvn     x5, x1
        mov     x6, x1
        mvn     x7, x1
        mvn     x8, x1
        mvn     x9, x1
        stmia   x0!, {x2-x9}
        pop     {x2-x9, pc}

.global BurstRead
BurstRead:
        push    {x2-x9, lr}
        ldmia   x0!, {x2-x9}
        stmia   x1!, {x2-x9}
        pop     {x2-x9, pc}
#endif

#if 0
/*
 *  v7_flush_dcache_all()
 *
 *  Flush the whole D-cache.
 *
 *  Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
 *
 *  - mm    - mm_struct describing address space
 */
.global v7_flush_dcache_all
v7_flush_dcache_all:
        dmb                                     //; ensure ordering with previous memory accesses
        mrc     p15, 1, w0, c0, c0, 1           //; read clidr
        ands    w3, w0, #0x7000000              //; extract loc from clidr
        mov     w3, w3, lsr #23                 //; left align loc bit field
        b.eq     finished                       //; if loc is 0, then no need to clean
        mov     w10, #0                         //; start clean at cache level 0
loop1:
        add     w2, w10, w10, lsr #1            //; work out 3x current cache level
        mov     w1, w0, lsr w2                  //; extract cache type bits from clidr
        and     w1, w1, #7                      //; mask of the bits for current cache only
        cmp     w1, #2                          //; see what cache we have at this level
        blt     skip                            //; skip if no cache, or just i-cache
#ifdef CONFIG_PREEMPT
        save_and_disable_irqs_notrace r9        //; make cssr&csidr read atomic
#endif
        mcr     p15, 2, w10, c0, c0, 0          //; select current cache level in cssr
        isb                                     //; isb to sych the new cssr&csidr
        mrc     p15, 1, w1, c0, c0, 0           //; read the new csidr
#ifdef CONFIG_PREEMPT
        restore_irqs_notrace w9
#endif
        and     w2, w1, #7                      //; extract the length of the cache lines
        add     w2, w2, #4                      //; add 4 (line length offset)
        ldr     w4, =0x3ff
        ands    w4, w4, w1, lsr #3              //; find maximum number on the way size
        clz     w5, w4                          //; find bit position of way size increment
        ldr     w7, =0x7fff
        ands    w7, w7, w1, lsr #13             //; extract max number of the index size
loop2:
        mov     w9, w4                          //; create working copy of max way size
loop3:
        orr     w11, w10, w9, lsl w5            //; factor way and cache number into r11
        orr     w11, w11, w7, lsl w2            //; factor index number into r11
        mcr     p15, 0, w11, c7, c14, 2         //; clean & invalidate by set/way
        subs    w9, w9, #1                      //; decrement the way
        b.ge     loop3
        subs    w7, w7, #1                      //; decrement the index
        b.ge     loop2
skip:
        add     w10, w10, #2                    //; increment cache number
        cmp     w3, w10
        b.gt     loop1
finished:
        mov     w10, #0                         //; swith back to cache level 0
        mcr     p15, 2, w10, c0, c0, 0          //; select current cache level in cssr
        dsb
        isb
        mov     pc, lr


/*
 *  v7_flush_cache_all()
 *
 *  Flush the entire cache system.
 *  The data cache flush is now achieved using atomic clean / invalidates
 *  working outwards from L1 cache. This is done using Set/Way based cache
 *  maintenance instructions.
 *  The instruction cache can still be invalidated back to the point of
 *  unification in a single instruction.
 *
 */
.global v7_flush_cache_all
v7_flush_cache_all:
        stmfd   sp!, {w4-w5, w7, w9-w11, lr}
        bl      v7_flush_dcache_all
        mov     w0, #0
        mcr     p15, 0, w0, c7, c1, 0           //; invalidate I-cache inner shareable
        mcr     p15, 0, w0, c7, c5, 0           //; I+BTB cache invalidate
        ldmfd   sp!, {w4-w5, w7, w9-w11, lr}
        mov     pc, lr

#endif

//;==================================================================
//; Global data
//;==================================================================
        .align 2
        .long   0x12345678

.global g_suspend_ready
g_suspend_ready:
        .long   0x00000000

.global g_wakeup_ready
g_wakeup_ready:
        .long   0x00000000

.global g_store_stack
g_store_stack:
        .long   0x00000000

.global g_store_vaddr
g_store_vaddr:
        .long   0x00000000      //; CPU 0
        .long   0x00000000
        .long   0x00000000
        .long   0x00000000
        .long   0x00000000
        .long   0x00000000
        .long   0x00000000
        .long   0x00000000

//;==================================================================
//; End of startup.s
//;==================================================================
