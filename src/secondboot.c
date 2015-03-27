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
//  File            : SecondBoot.c
//  Description     :
//  Author          : Hans
//  History         :
//          2014-08-20  Hans modify for Peridot
//          2013-01-10  Hans
////////////////////////////////////////////////////////////////////////////////

#define __SET_GLOBAL_VARIABLES
#include "sysHeader.h"



#define EMA_VALUE           (1)     // Manual setting - 1: 1.1V, 3: 1.0V, 4: 0.95V

extern void     __pllchange(volatile U32 data, volatile U32* addr, U32 delaycount);
extern U32      iget_fcs(U32 fcs, U32 data);
extern U32      __calc_crc(void *addr, int len);
extern void     DMC_Delay(int milisecond);

//extern void     flushICache(void);
//extern void     enableICache(CBOOL enable);

extern void     enterSelfRefresh(void);
extern void     exitSelfRefresh(void);
extern void     set_bus_config(void);
extern void     set_drex_qos(void);

extern CBOOL    iUSBBOOT(struct NX_SecondBootInfo * const pTBI);
extern CBOOL    iUARTBOOT(struct NX_SecondBootInfo * const pTBI);
extern CBOOL    iSPIBOOT(struct NX_SecondBootInfo * const pTBI);
extern CBOOL    iSDXCBOOT(struct NX_SecondBootInfo * const pTBI);
extern CBOOL    iNANDBOOTEC(struct NX_SecondBootInfo * const pTBI);
extern CBOOL    iSDXCFSBOOT(struct NX_SecondBootInfo * const pTBI);
extern void     initClock(void);
extern void     initDDR3(U32);
extern CBOOL    buildinfo(void);

extern void     printClkInfo(void);

extern void     ResetCon(U32 devicenum, CBOOL en);

extern U32  g_WR_lvl;
extern U32  g_GT_cycle;
extern U32  g_GT_code;
extern U32  g_RD_vwmc;
extern U32  g_WR_vwmc;

//------------------------------------------------------------------------------
static void dowakeup(void)
{
    U32 fn, sign, phy, crc, len;
    void (*jumpkernel)(void) = 0;

    WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1);             // open alive power gate
    fn   = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE1);
    sign = ReadIO32(&pReg_Alive->ALIVESCRATCHREADREG);
    phy  = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE3);
    crc  = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE2);
    len  = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE4);
    jumpkernel = (void (*)(void))fn;

    WriteIO32(&pReg_Alive->ALIVESCRATCHRSTREG,  0xFFFFFFFF);
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST1,    0xFFFFFFFF);
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST2,    0xFFFFFFFF);
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST3,    0xFFFFFFFF);
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST4,    0xFFFFFFFF);

    if (SUSPEND_SIGNATURE == sign)
    {
        U32 ret = __calc_crc((void*)phy, len);

        printf("CRC: 0x%08X FN: 0x%08X phy: 0x%08X len: 0x%08X ret: 0x%08X\r\n", crc, fn, phy, len, ret);
        if (fn && (crc == ret))
        {
            U32 temp = 0x100000;
            printf("It's WARM BOOT\r\nJump to Kernel!\r\n");
            while(DebugIsBusy() && temp--);
            jumpkernel();
        }
    }
    else
    {
        printf("Suspend Signature is different\r\nRead Signature :0x%08X\r\n", sign);
    }

    printf("It's COLD BOOT\r\n");
}


//------------------------------------------------------------------------------
#if (CCI400_COHERENCY_ENABLE == 1)
void initCCI400(void)
{
    struct NX_CCI400_RegisterSet *pReg_CCI400 = (struct NX_CCI400_RegisterSet *)PHY_BASEADDR_CCI400_MODULE;

    //before set barrier instruction.
    WriteIO32( &pReg_CCI400->COR,   (1UL<<3) );             // protect to send barrier command to drex

    WriteIO32( &pReg_CCI400->CSI[BUSID_CS].SCR,     0 );    // snoop request disable
    WriteIO32( &pReg_CCI400->CSI[BUSID_CODA].SCR,   0 );    // snoop request disable
    WriteIO32( &pReg_CCI400->CSI[BUSID_TOP].SCR,    0 );    // snoop request disable

    WriteIO32( &pReg_CCI400->CSI[BUSID_CPUG1].SCR,  (1<<31 | 1<<30 | 1<<1 | 1<<0) );    // cpu 4~7
    WriteIO32( &pReg_CCI400->CSI[BUSID_CPUG0].SCR,  (1<<31 | 1<<30 | 1<<1 | 1<<0) );    // cpu 0~3
}
#endif  // #if (CCI400_COHERENCY_ENABLE == 1)

#if (MULTICORE_BRING_UP == 1)
void BringUpSlaveCPU(U32 CPUID)
{
    WriteIO32( &pReg_ClkPwr->CPURESETMODE,      0x1);
    WriteIO32( &pReg_ClkPwr->CPUPOWERONREQ,     (1 << CPUID) );
}

void SetVectorLocation(U32 CPUID, CBOOL LowHigh)
{
    U32 regvalue;
    if(CPUID & 0x4)    // cpu 4, 5, 6, 7
    {
        regvalue = ReadIO32(&pReg_Tieoff->TIEOFFREG[95]);
        if(LowHigh)
            regvalue |= 1<<(12+(CPUID & 0x3));
        else
            regvalue &= ~(1<<(12+(CPUID & 0x3)));
        WriteIO32(&pReg_Tieoff->TIEOFFREG[95], regvalue);
    }
    else    // cpu 0, 1, 2, 3
    {
        regvalue = ReadIO32(&pReg_Tieoff->TIEOFFREG[78]);
        if(LowHigh)
            regvalue |= 1<<(20+(CPUID & 0x3));
        else
            regvalue &= ~(1<<(20+(CPUID & 0x3)));
        WriteIO32(&pReg_Tieoff->TIEOFFREG[78], regvalue);
    }
}
#endif  // #if (MULTICORE_BRING_UP == 1)

void __WFI(void);
#define CPU_ALIVE_FLAG_ADDR    0xC0010238
void SubCPUBoot( U32 CPUID )
{
    volatile U32 *aliveflag = (U32*)CPU_ALIVE_FLAG_ADDR;

    WriteIO32( &pReg_GIC400->GICC.CTLR,         0x07 );     // enable cpu interface
    WriteIO32( &pReg_GIC400->GICC.PMR,          0xFF );     // all high priority
    WriteIO32( &pReg_GIC400->GICD.ISENABLER[0], 0xFF );     // enable sgi all
    WriteIO32( &pReg_ClkPwr->CPUPOWERONREQ,     0x00 );     // clear own wakeup req bit

//    printf("Hello World. I'm CPU %d!\r\n", CPUID);
    DebugPutch('0'+CPUID);
    *aliveflag = 1;

    __WFI();
}


void vddPowerOff( void )
{
#if (MULTICORE_SLEEP_CONTROL == 1)
    WriteIO32( &pReg_ClkPwr->CPUWARMRESETREQ,       0x0 );              //; clear for reset issue.

    WriteIO32( &pReg_Alive->ALIVEPWRGATEREG,        0x00000001 );       //; alive power gate open

    //----------------------------------
    // Save leveling & training values.
#if 1
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST5,        0xFFFFFFFF);        // clear - ctrl_shiftc
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST6,        0xFFFFFFFF);        // clear - ctrl_offsetC
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST7,        0xFFFFFFFF);        // clear - ctrl_offsetr
    WriteIO32(&pReg_Alive->ALIVESCRATCHRST8,        0xFFFFFFFF);        // clear - ctrl_offsetw

    WriteIO32(&pReg_Alive->ALIVESCRATCHSET5,        g_GT_cycle);        // store - ctrl_shiftc
    WriteIO32(&pReg_Alive->ALIVESCRATCHSET6,        g_GT_code);         // store - ctrl_offsetc
    WriteIO32(&pReg_Alive->ALIVESCRATCHSET7,        g_RD_vwmc);         // store - ctrl_offsetr
    WriteIO32(&pReg_Alive->ALIVESCRATCHSET8,        g_WR_vwmc);         // store - ctrl_offsetw
#endif


    WriteIO32( &pReg_Alive->VDDOFFCNTVALUERST,      0xFFFFFFFF );       //; clear delay counter, refrence rtc clock
    WriteIO32( &pReg_Alive->VDDOFFCNTVALUESET,      0x00000001 );       //; set minimum delay time for VDDPWRON pin. 1 cycle per 32.768Kh (about 30us)

    if ( ReadIO32(&pReg_Alive->ALIVEGPIODETECTPENDREG) )
    {
        __asm__ __volatile__ ("wfi");                                   //; now real entering point to stop mode.
    }
    else
    {
        WriteIO32( &pReg_Alive->VDDCTRLRSTREG,          0x00000001 );   //; vddpoweron off, start counting down.
        WriteIO32( &pReg_Alive->ALIVEGPIODETECTPENDREG, 0xFF );         //; all alive pend pending clear until power down.
        DMC_Delay(220);

        WriteIO32( &pReg_Alive->ALIVEPWRGATEREG,        0x00000000 );   //; alive power gate close
    }

    WriteIO32( &pReg_ClkPwr->CPUWARMRESETREQ,       0x0 );              //; clear for reset issue.
#else   // #if (MULTICORE_SLEEP_CONTROL == 1)

    WriteIO32( &pReg_Alive->ALIVEPWRGATEREG,        0x00000001 );       //; alive power gate open

    WriteIO32( &pReg_Alive->VDDOFFCNTVALUERST,      0xFFFFFFFF );       //; clear delay counter, refrence rtc clock
    WriteIO32( &pReg_Alive->VDDOFFCNTVALUESET,      0x00000001 );       //; set minimum delay time for VDDPWRON pin. 1 cycle per 32.768Kh (about 30us)

    WriteIO32( &pReg_Alive->VDDCTRLRSTREG,          0x00000001 );       //; vddpoweron off, start counting down.
    DMC_Delay(220);

    WriteIO32( &pReg_Alive->ALIVEGPIODETECTPENDREG, 0xFF );             //; all alive pend pending clear until power down.
    WriteIO32( &pReg_Alive->ALIVEPWRGATEREG,        0x00000000 );       //; alive power gate close

    WriteIO32( &pReg_ClkPwr->CPUWARMRESETREQ,       0x0 );              //; clear for reset issue.

    while(1);
    {
        __asm__ __volatile__ ("wfi");                                   //; now real entering point to stop mode.
    }
#endif  // #if (MULTICORE_SLEEP_CONTROL == 1)
}


struct NX_CLKPWR_RegisterSet * const clkpwr;
void sleepMain( void )
{
#if 0
    WriteIO32( &clkpwr->PLLSETREG[1],   0x100CC801 );       //; set PLL1 - 800Mhz
//    WriteIO32( &clkpwr->PLLSETREG[1],   0x100CC802 );       //; set PLL1 - 400Mhz

    __pllchange(clkpwr->PWRMODE | 0x1<<15, &clkpwr->PWRMODE, 0x20000); //533 ==> 800MHz:#0xED00, 1.2G:#0x17000, 1.6G:#0x1E000

    {
        volatile U32 delay = 0x100000;
        while((clkpwr->PWRMODE & 0x1<<15) && delay--);    // it's never checked here, just for insure
        if( clkpwr->PWRMODE & 0x1<<15 )
        {
//            printf("pll does not locked\r\nsystem halt!\r\r\n");    // in this point, it's not initialized uart debug port yet
            while(1);        // system reset code need.
        }
    }
#endif

//    DebugInit();

    enterSelfRefresh();
    ClearIO32( &pReg_Tieoff->TIEOFFREG[76], (7<<6) );       //; Lock Drex port

    vddPowerOff();

    exitSelfRefresh();
    DMC_Delay(50);

    SetIO32  ( &pReg_Tieoff->TIEOFFREG[76], (7<<6) );       //; Unlock Drex port
}


//------------------------------------------------------------------------------
void BootMain( U32 CPUID )
{
    struct NX_SecondBootInfo    TBI;
    struct NX_SecondBootInfo * pTBI = &TBI;    // third boot info
    CBOOL Result = CFALSE;
    register volatile U32 temp;
    U32 sign, isResume = 0;

    WriteIO32( &pReg_ClkPwr->CPUWARMRESETREQ,   0x00 );     // clear for reset issue.

    // Set EMA for CPU Cluster0
    temp  = ReadIO32( &pReg_Tieoff->TIEOFFREG[94] ) & ~((0x7 << 23) | (0x7 << 17));
    temp |= ((EMA_VALUE << 23) | (EMA_VALUE << 17));
    WriteIO32( &pReg_Tieoff->TIEOFFREG[94], temp);

    // Set EMA for CPU Cluster1
    temp  = ReadIO32( &pReg_Tieoff->TIEOFFREG[111] ) & ~((0x7 << 23) | (0x7 << 17));
    temp |= ((EMA_VALUE << 23) | (EMA_VALUE << 17));
    WriteIO32( &pReg_Tieoff->TIEOFFREG[111], temp);

#if 1    // rom boot does not cross usb connection
    ClearIO32( &pReg_Tieoff->TIEOFFREG[13], (1<<3) );       //nUtmiResetSync = 0
    ClearIO32( &pReg_Tieoff->TIEOFFREG[13], (1<<2) );       //nResetSync = 0
    SetIO32  ( &pReg_Tieoff->TIEOFFREG[13], (3<<7) );       //POR_ENB=1, POR=1

    ResetCon(RESETINDEX_OF_USB20OTG_MODULE_i_nRST, CTRUE);  // reset on
#endif

    initClock();

    //--------------------------------------------------------------------------
    // Debug Console
    //--------------------------------------------------------------------------
    DebugInit();


    //--------------------------------------------------------------------------
    // build information. version, build time and date
    //--------------------------------------------------------------------------
#if 1
    buildinfo();
#else
    if ( buildinfo() == CFALSE )
    {
        printf( "WARNING : NHIS mismatch...!!!\r\n" );
        while(1);
    }
#endif

    //--------------------------------------------------------------------------
    // print clock information
    //--------------------------------------------------------------------------
    printClkInfo();

    //--------------------------------------------------------------------------
    // get VID & PID for USBD
    //--------------------------------------------------------------------------
    temp = ReadIO32(PHY_BASEADDR_ECID_MODULE + (3<<2));
    g_USBD_VID = (temp >> 16) & 0xFFFF;
    g_USBD_PID = (temp & 0xFFFF);

    if ((g_USBD_VID == 0) || (g_USBD_PID == 0))
    {
        g_USBD_VID = USBD_VID;
        g_USBD_PID = USBD_PID;
    }
    else
    {
        g_USBD_VID = 0x04E8;
        g_USBD_PID = 0x1234;
    }
    SYSMSG("USBD VID = %04X, PID = %04X\r\n", g_USBD_VID, g_USBD_PID);


    WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1);
    sign = ReadIO32(&pReg_Alive->ALIVESCRATCHREADREG);
    if ((SUSPEND_SIGNATURE == sign) && ReadIO32(&pReg_Alive->WAKEUPSTATUS))
    {
        isResume = 1;
    }

    initDDR3(isResume);

    if (isResume)
    {
        exitSelfRefresh();
    }

    printf( "DDR3 Init Done!\r\n" );

    set_bus_config();
    set_drex_qos();

#if (CCI400_COHERENCY_ENABLE == 1)
    printf( "CCI Init!\r\n" );
    initCCI400();
#endif

    WriteIO32( &pReg_GIC400->GICC.CTLR, 0x07 );     // enable cpu interface
    WriteIO32( &pReg_GIC400->GICC.PMR,  0xFF );     // all high priority
    WriteIO32( &pReg_GIC400->GICD.CTLR, 0x03 );     // distinbutor enable

    printf( "Wakeup CPU " );

#if (MULTICORE_BRING_UP == 1)
{
    volatile U32 *aliveflag = (U32*)CPU_ALIVE_FLAG_ADDR;
    int CPUNumber, retry = 0;

    for (CPUNumber = 1; CPUNumber < 8; )
    {
        register volatile U32 delay;
        *aliveflag = 0;
        delay = 0x10000;
        SetVectorLocation(CPUNumber, CTRUE);    // CTRUE: High Vector(0xFFFF0000), CFALSE: Low Vector (0x0)
        BringUpSlaveCPU(CPUNumber);
        DMC_Delay(1000);
        while((*aliveflag == 0) && (--delay));
        if(delay == 0)
        {
            if(retry > 3)
            {
                printf("maybe cpu %d is dead. -_-;\r\n", CPUNumber);
                CPUNumber++;    // try next cpu bringup
            }
            else
            {
                printf("cpu %d is not bringup, retry\r\n", CPUNumber);
                retry++;
            }
        }
        else
        {
            retry = 0;
            CPUNumber++;    // try next cpu bringup
        }
        DMC_Delay(10000);
    }
}
#endif  // #if (MULTICORE_BRING_UP == 1)

    printf( "\r\nCPU Wakeup done! WFI is expected.\r\n" );
    printf( "CPU%d is Master!\r\n\n", CPUID );


    if (isResume)
    {
        printf( " DDR3 SelfRefresh exit Done!\r\n0x%08X\r\n", ReadIO32(&pReg_Alive->WAKEUPSTATUS) );
        dowakeup();
    }
    WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 0);

    if (pSBI->SIGNATURE != HEADER_ID)
        printf( "2nd Boot Header is invalid, Please check it out!\r\n" );

#if 0
{
    int i;
    volatile U32 *ptr = (U32*)0x40000000, data;
    printf("memory test start!\r\n");
    printf("\r\nmemory write data to own address\r\n");
    for(i=0; i<0x10000000; i++)
    {
        ptr[i] = (U32)ptr+i;
        if((i & 0x3FFFFF) == 0)
            printf("0x%08X:\r\n", ptr+i);
    }
    printf("\r\nmemory compare with address and own data\r\n");
    for(i=0; i<0x10000000; i++)
    {
        if(ptr[i] != (U32)ptr+i)
            printf("0x%08X: %08x\r\n", (U32)(ptr+i), *(ptr+i));
        if((i & 0xFFFFF) == 0)
            printf("0x%08X:\r\n", ptr+i);
    }

    printf("bit shift test....\r\n");
    for(i=0; i<32; i++)
        ptr[i] = 1<<i;
    for(i=0; i<32; i++)
        printf("0x%8X\r\n", ptr[i]);

    printf("\r\nmemory test done\r\n");
}
#endif

#if 1
#ifdef LOAD_FROM_USB
    printf( "Loading from usb...\r\n" );
    Result = iUSBBOOT(pTBI);            // for USB boot
#endif
#ifdef LOAD_FROM_SPI
    printf( "Loading from spi...\r\n" );
    Result = iSPIBOOT(pTBI);            // for SPI boot
#endif
#ifdef LOAD_FROM_SDMMC
    printf( "Loading from sdmmc...\r\n" );
    Result = iSDXCBOOT(pTBI);           // for SD boot
#endif
#ifdef LOAD_FROM_SDFS
    printf( "Loading from sd FATFS...\r\n" );
    Result = iSDXCFSBOOT(pTBI);         // for SDFS boot
#endif
#ifdef LOAD_FROM_NAND
    printf( "Loading from nand...\r\n" );
    Result = iNANDBOOTEC(pTBI);         // for NAND boot
#endif
#ifdef LOAD_FROM_UART
    printf( "Loading from uart...\r\n" );
    Result = iUARTBOOT(pTBI);           // for UART boot
#endif

#else

    switch(pSBI->DBI.SPIBI.LoadDevice)
    {
    case BOOT_FROM_USB:
        printf( "Loading from usb...\r\n" );
        Result = iUSBBOOT(pTBI);        // for USB boot
        break;
    case BOOT_FROM_SPI:
        printf( "Loading from spi...\r\n" );
        Result = iSPIBOOT(pTBI);        // for SPI boot
        break;
    case BOOT_FROM_NAND:
        printf( "Loading from nand...\r\n" );
        Result = iNANDBOOTEC(pTBI);     // for NAND boot
        break;
    case BOOT_FROM_SDMMC:
        printf( "Loading from sdmmc...\r\n" );
        Result = iSDXCBOOT(pTBI);       // for SD boot
        break;
    case BOOT_FROM_SDFS:
        printf( "Loading from sd FATFS...\r\n" );
        Result = iSDXCFSBOOT(pTBI);     // for SDFS boot
        break;
    case BOOT_FROM_UART:
        printf( "Loading from uart...\r\n" );
        Result = iUARTBOOT(pTBI);       // for UART boot
        break;
    }
#endif


#if 0   // for memory test
    {
        U32 *pSrc = (U32 *)pTBI->LAUNCHADDR;
        U32 *pDst = (U32 *)(0x50000000);
        int i;

        for (i = 0; i < (int)(pTBI->LOADSIZE >> 2); i++)
        {
            pDst[i] = pSrc[i];
        }

        for (i = 0; i < (int)(pTBI->LOADSIZE >> 2); i++)
        {
            if (pDst[i] != pSrc[i])
            {
                printf( "Copy check faile...\r\n" );
                break;
            }
        }
    }
#endif


    if(Result)
    {
        void (*pLaunch)(U32,U32) = (void(*)(U32,U32))pTBI->LAUNCHADDR;
        printf( " Image Loading Done!\r\n" );
        printf( "Launch to 0x%08X\r\n", (U32)pLaunch );
        temp = 0x10000000;
        while(DebugIsBusy() && temp--);
        pLaunch(0, 4330);
    }

    printf( " Image Loading Failure Try to USB boot\r\n" );
    temp = 0x10000000;
    while(DebugIsBusy() && temp--);
}
