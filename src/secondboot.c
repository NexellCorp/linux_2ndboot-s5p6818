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

#include <nx_cci400.h>

void    enterSelfRefresh(void);
void    exitSelfRefresh(void);
void    Clock_Init(void);
void    set_bus_config(void);
void    set_drex_qos(void);

CBOOL   iUSBBOOT(struct NX_SecondBootInfo * const pTBI);
CBOOL   iSPIBOOT(struct NX_SecondBootInfo * const pTBI);
CBOOL   iSDXCBOOT(struct NX_SecondBootInfo * const pTBI);
CBOOL   iNANDBOOTEC(struct NX_SecondBootInfo * const pTBI);
CBOOL   iSDXCFSBOOT(struct NX_SecondBootInfo * const pTBI);
void    DDR3_Init(U32);

void    DisplayClkSet(void);

void    Decrypt(U32 *SrcAddr, U32 *DestAddr, U32 Size);
void    ResetCon(U32 devicenum, CBOOL en);

extern inline U32 get_fcs(U32 fcs, U8 data);
extern inline U32 iget_fcs(U32 fcs, U32 data);
extern inline U32 __calc_crc(void *addr, int len);

extern U32 getquotient(int dividend, int divisor);
extern U32 getremainder(int dividend, int divisor);




inline void DMC_Delay(int milisecond);
U32     __GetCPUID(void);
void    __WFI();

void    BurstWrite(U32 *WriteAddr, U32 WData);
void    BurstZero(U32 *WriteAddr, U32 WData);
void    BurstRead(U32 *ReadAddr, U32 *SaveAddr);


//------------------------------------------------------------------------------
static void dowakeup(void)
{
    U32 fn, sign, phy, crc, len;
    void (*jumpkernel)(void) = 0;

    WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1);         // open alive power gate
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
void CCI400_init(void)
{
    struct NX_CCI400_RegisterSet *pCCIBUS = (struct NX_CCI400_RegisterSet *)PHY_BASEADDR_CCI400_MODULE;

    //before set barrier instruction.
    pCCIBUS->COR = 1UL<<3;// protect to send barrier command to drex

    pCCIBUS->CSI[BUSID_CS].SCR = 0;//&= ~(1<<0);        // snoop request disable

    pCCIBUS->CSI[BUSID_CODA].SCR = 0;//&= ~(1<<0);      // snoop request disable

    pCCIBUS->CSI[BUSID_TOP].SCR = 0;//&= ~(1<<0);       // snoop request disable

    pCCIBUS->CSI[BUSID_CPUG1].SCR = 1<<31 | 1<<30 | 1<<1 | 1<<0;    // cpu 4~7

    pCCIBUS->CSI[BUSID_CPUG0].SCR = 1<<31 | 1<<30 | 1<<1 | 1<<0;     // cpu 0~3
}


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

void SubCPUBoot( U32 CPUID )
{
    volatile U32 *aliveflag = (U32*)0xC0010238;

//    printf("Hello World. I'm CPU %d!\r\n", CPUID);
    DebugPutch('0'+CPUID);
    *aliveflag = 1;

    __WFI();
}


void SleepMain( void )
{
    DebugInit();
    enterSelfRefresh();
}


void vddPowerOff( void )
{
    WriteIO32( &pReg_Alive->ALIVEPWRGATEREG,    0x00000001 );   //; alive power gate open

    WriteIO32( &pReg_Alive->VDDOFFCNTVALUERST,  0xFFFFFFFF );   //; clear delay counter, refrence rtc clock
    WriteIO32( &pReg_Alive->VDDOFFCNTVALUESET,  0x00000001 );   //; no need delay time

    while(1)
    {
        WriteIO32( &pReg_Alive->VDDCTRLRSTREG,          0x01 ); //; vddpoweron off, start delay counter

        WriteIO32( &pReg_Alive->ALIVEGPIODETECTPENDREG, 0xFF ); //; all alive pend pending clear until power down.
    }                                                           //; this time, core power will off and so cpu will die.
}


//------------------------------------------------------------------------------
void BootMain( U32 CPUID )
{
    struct NX_SecondBootInfo    TBI;
    struct NX_SecondBootInfo * pTBI = &TBI;    // third boot info
    CBOOL Result = CFALSE;
    register volatile U32 tmp;
    U32 CPUNumber, sign, isResume = 0;
    volatile U32 *aliveflag = (U32*)0xC0010238;

#if 0
    pReg_GPIO       = (struct NX_GPIO_RegisterSet   (*)[])PHY_BASEADDR_GPIOA_MODULE;
    pReg_Alive      = (struct NX_ALIVE_RegisterSet      *)PHY_BASEADDR_ALIVE_MODULE;
    pReg_Tieoff     = (struct NX_TIEOFF_RegisterSet     *)PHY_BASEADDR_TIEOFF_MODULE;
    pReg_ClkPwr     = (struct NX_CLKPWR_RegisterSet     *)PHY_BASEADDR_CLKPWR_MODULE;
    pReg_UartClkGen = (struct NX_CLKGEN_RegisterSet     *)PHY_BASEADDR_CLKGEN22_MODULE;
    pReg_Uart       = (struct NX_UART_RegisterSet       *)PHY_BASEADDR_UART0_MODULE;
#if defined(ARCH_NXP5430)
    pReg_Drex       = (struct NX_DREXSDRAM_RegisterSet  *)PHY_BASEADDR_DREX_MODULE_CH0_APB;
#endif
#endif

#if 1    // rom boot does not cross usb connection
    ClearIO32( &pReg_Tieoff->TIEOFFREG[13], (1<<3) );       //nUtmiResetSync = 0
    ClearIO32( &pReg_Tieoff->TIEOFFREG[13], (1<<2) );       //nResetSync = 0
    SetIO32  ( &pReg_Tieoff->TIEOFFREG[13], (3<<7) );       //POR_ENB=1, POR=1

    ResetCon(RESETINDEX_OF_USB20OTG_MODULE_i_nRST, CTRUE);  // reset on
#endif

    Clock_Init();

    //--------------------------------------------------------------------------
    // Debug Console
    //--------------------------------------------------------------------------
    DebugInit();


    printf( "\r\n" );
    printf( "--------------------------------------------------------------------------------\r\n" );
    printf( " Second Boot by Nexell Co. : Ver0.2 - Built on %s %s\r\n", __DATE__, __TIME__ );
    printf( "--------------------------------------------------------------------------------\r\n" );

    DisplayClkSet();

    //--------------------------------------------------------------------------
    // get VID & PID for USBD
    //--------------------------------------------------------------------------
    tmp = ReadIO32(PHY_BASEADDR_ECID_MODULE + (3<<2));
    g_USBD_VID = (tmp >> 16) & 0xFFFF;
    g_USBD_PID = (tmp & 0xFFFF);

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
    printf("USBD VID = %04X, PID = %04X\r\n", g_USBD_VID, g_USBD_PID);


    WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1);
    sign = ReadIO32(&pReg_Alive->ALIVESCRATCHREADREG);
    if ((SUSPEND_SIGNATURE == sign) && ReadIO32(&pReg_Alive->WAKEUPSTATUS))
    {
        isResume = 1;
    }

    DDR3_Init(isResume);
    if (isResume)
    {
        exitSelfRefresh();
    }

    printf( "DDR3 Init Done!\r\n" );

    set_bus_config();
    set_drex_qos();

    printf( "CCI Init!\r\n" );
    CCI400_init();

    printf( "Wakeup CPU " );
    for (CPUNumber = 1; CPUNumber < 8; CPUNumber++)
    {
        register volatile U32 delay;
        *aliveflag = 0;
        delay = 0x100000;
        SetVectorLocation(CPUNumber, CTRUE);    // CTRUE: High Vector(0xFFFF0000), CFALSE: Low Vector (0x0)
        BringUpSlaveCPU(CPUNumber);
        DMC_Delay(1000);
        while((*aliveflag == 0) && (--delay));
        if(delay == 0)
            printf("CPU %d is not aliving %d\r\n", CPUNumber, delay);
        DMC_Delay(10000);
    }

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

#if 1
    if(Result)
    {
        void (*pLaunch)(U32,U32) = (void(*)(U32,U32))pTBI->LAUNCHADDR;
        printf( " Image Loading Done!\r\n" );
        printf( "Launch to 0x%08X\r\n", (U32)pLaunch );
        tmp = 0x10000000;
        while(DebugIsBusy() && tmp--);
        pLaunch(0, 4330);
    }

    printf( " Image Loading Failure Try to USB boot\r\n" );
    tmp = 0x10000000;
    while(DebugIsBusy() && tmp--);
#endif
}
