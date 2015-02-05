#include "sysHeader.h"


#if (CFG_NSIH_EN == 0)

// system controller
//#define SYSPLLCH      *(volatile unsigned long *)(0xc0010268)



// PMS for PLL0, 1

#define PLL01_PMS_1800MHZ_P     3
#define PLL01_PMS_1800MHZ_M     450
#define PLL01_PMS_1800MHZ_S     1

#define PLL01_PMS_1700MHZ_P     3
#define PLL01_PMS_1700MHZ_M     425
#define PLL01_PMS_1700MHZ_S     1

#define PLL01_PMS_1600MHZ_P     3     // pll0 {cpu clock, memory clock}
#define PLL01_PMS_1600MHZ_M     400
#define PLL01_PMS_1600MHZ_S     1

#define PLL01_PMS_1500MHZ_P     3
#define PLL01_PMS_1500MHZ_M     375
#define PLL01_PMS_1500MHZ_S     1

#define PLL01_PMS_1400MHZ_P     3 // 6 // 3
#define PLL01_PMS_1400MHZ_M     350 // 350 // 350
#define PLL01_PMS_1400MHZ_S     1 // 0 // 1

#define PLL01_PMS_1300MHZ_P     3
#define PLL01_PMS_1300MHZ_M     325
#define PLL01_PMS_1300MHZ_S     1

#define PLL01_PMS_1200MHZ_P     3
#define PLL01_PMS_1200MHZ_M     300
#define PLL01_PMS_1200MHZ_S     1

#define PLL01_PMS_1100MHZ_P     3
#define PLL01_PMS_1100MHZ_M     275
#define PLL01_PMS_1100MHZ_S     1

#define PLL01_PMS_1000MHZ_P     3
#define PLL01_PMS_1000MHZ_M     250
#define PLL01_PMS_1000MHZ_S     1

#define PLL01_PMS_933MHZ_P      4
#define PLL01_PMS_933MHZ_M      311
#define PLL01_PMS_933MHZ_S      1

#if 1
#define PLL01_PMS_800MHZ_P      3
#define PLL01_PMS_800MHZ_M      200
#define PLL01_PMS_800MHZ_S      1
#else
#define PLL01_PMS_800MHZ_P      6
#define PLL01_PMS_800MHZ_M      400
#define PLL01_PMS_800MHZ_S      1
#endif

#define PLL01_PMS_700MHZ_P      3
#define PLL01_PMS_700MHZ_M      175
#define PLL01_PMS_700MHZ_S      1

#define PLL01_PMS_666MHZ_P      2
#define PLL01_PMS_666MHZ_M      111
#define PLL01_PMS_666MHZ_S      1

#if 0
#define PLL01_PMS_600MHZ_P      3
#define PLL01_PMS_600MHZ_M      200
#define PLL01_PMS_600MHZ_S      1
#else
#define PLL01_PMS_600MHZ_P      2
#define PLL01_PMS_600MHZ_M      200
#define PLL01_PMS_600MHZ_S      2
#endif

#define PLL01_PMS_533MHZ_P      6
#define PLL01_PMS_533MHZ_M      533
#define PLL01_PMS_533MHZ_S      2

#define PLL01_PMS_400MHZ_P      3
#define PLL01_PMS_400MHZ_M      200
#define PLL01_PMS_400MHZ_S      2


// PMS for PLL2, 3
#if 1
#define PLL23_PMS_900MHZ_P      3
#define PLL23_PMS_900MHZ_M      225
#define PLL23_PMS_900MHZ_S      1
#else
#define PLL23_PMS_900MHZ_P      4
#define PLL23_PMS_900MHZ_M      300
#define PLL23_PMS_900MHZ_S      1
#endif

#define PLL23_PMS_800MHZ_P      3
#define PLL23_PMS_800MHZ_M      200
#define PLL23_PMS_800MHZ_S      1

#if 0
#define PLL23_PMS_666MHZ_P      4
#define PLL23_PMS_666MHZ_M      222
#define PLL23_PMS_666MHZ_S      1
#else
#define PLL23_PMS_666MHZ_P      2
#define PLL23_PMS_666MHZ_M      111
#define PLL23_PMS_666MHZ_S      1
#endif

#define PLL23_PMS_614MHZ_P      2
#define PLL23_PMS_614MHZ_M      102
#define PLL23_PMS_614MHZ_S      1
#define PLL23_PMS_614MHZ_K      26214

#define PLL23_PMS_600MHZ_P      2
#define PLL23_PMS_600MHZ_M      100
#define PLL23_PMS_600MHZ_S      1

#define PLL23_PMS_550MHZ_P      3
#define PLL23_PMS_550MHZ_M      275
#define PLL23_PMS_550MHZ_S      2

#define PLL23_PMS_500MHZ_P      3
#define PLL23_PMS_500MHZ_M      250
#define PLL23_PMS_500MHZ_S      2

#define PLL23_PMS_400MHZ_P      3
#define PLL23_PMS_400MHZ_M      200
#define PLL23_PMS_400MHZ_S      2

#define PLL23_PMS_333MHZ_P      4
#define PLL23_PMS_333MHZ_M      222
#define PLL23_PMS_333MHZ_S      2

#define PLL23_PMS_400MHZ_P      3
#define PLL23_PMS_400MHZ_M      200
#define PLL23_PMS_400MHZ_S      2

#define PLL23_PMS_295MHZ_P      6
#define PLL23_PMS_295MHZ_M      295
#define PLL23_PMS_295MHZ_S      2

#define PLL23_PMS_250MHZ_P      3
#define PLL23_PMS_250MHZ_M      250
#define PLL23_PMS_250MHZ_S      3

#define PLL23_PMS_125MHZ_P      3
#define PLL23_PMS_125MHZ_M      250
#define PLL23_PMS_125MHZ_S      4

#endif

#define PLL_P   18
#define PLL_M   8
#define PLL_S   0
#define PLL_K   16


#define CLKSRC  0
#define DVO0    3
#define DVO1    9
#define DVO2    15
#define DVO3    21


extern U32 getquotient(int dividend, int divisor);
extern U32 getremainder(int dividend, int divisor);
extern struct NX_SecondBootInfo * const pSBI;
extern void __pllchange(volatile U32 data, volatile U32* addr, U32 delaycount);

struct NX_CLKPWR_RegisterSet * const clkpwr = (struct    NX_CLKPWR_RegisterSet *)PHY_BASEADDR_CLKPWR_MODULE;
static U32 __g_OSC_KHz;

void NX_CLKPWR_SetOSCFreq( U32 FreqKHz )
{
    __g_OSC_KHz = FreqKHz;
}

U32 NX_CLKPWR_GetPLLFreq(U32 PllNumber)
{
    U32 regvalue, regvalue1, nP, nM, nS, nK;
    U32 temp = 0;

    regvalue = clkpwr->PLLSETREG[PllNumber];
    regvalue1 = clkpwr->PLLSETREG_SSCG[PllNumber];
    nP = (regvalue >> PLL_P) & 0x3F;
    nM = (regvalue >> PLL_M) & 0x3FF;
    nS = (regvalue >> PLL_S) & 0xFF;
    nK = (regvalue1 >> PLL_K) & 0xFFFF;

    if ((PllNumber > 1) && nK) {
        temp = (getquotient((getquotient((nK * 1000), 65536) * __g_OSC_KHz),nP)>>nS);
    }

    temp = ((getquotient((nM * __g_OSC_KHz),nP)>>nS)*1000) + temp;
    return temp;
}

U32 NX_CLKPWR_GetSrcPll(U32 Divider)
{
    return clkpwr->DVOREG[Divider] & 0x7;
}

U32 NX_CLKPWR_GetDivideValue(U32 Divider)
{
    U32 regvalue = clkpwr->DVOREG[Divider];
    return ((((regvalue>>DVO3)&0x3F)+1)<<24) | ((((regvalue>>DVO2)&0x3F)+1)<<16) | ((((regvalue>>DVO1)&0x3F)+1)<<8) | ((((regvalue>>DVO0)&0x3F)+1)<<0);
}


#define _GET_PLL01(_MHz, _val) \
    _val = (U32)((1UL<<28)|(PLL01_PMS_##_MHz##MHZ_P<<PLL_P)|(PLL01_PMS_##_MHz##MHZ_M<<PLL_M)|(PLL01_PMS_##_MHz##MHZ_S<<PLL_S));

#define _GET_PLL23(_MHz, _val) \
    _val = (U32)((1UL<<28)|(PLL23_PMS_##_MHz##MHZ_P<<PLL_P)|(PLL23_PMS_##_MHz##MHZ_M<<PLL_M)|(PLL23_PMS_##_MHz##MHZ_S<<PLL_S));

void initClock(void)
{
#if (CFG_NSIH_EN == 0)
    U32 PLL_PMS;
#endif

    NX_CLKPWR_SetOSCFreq( OSC_KHZ );

    // pll change
#if (CFG_NSIH_EN == 0)
    // PLL0 for memory
    _GET_PLL01(500, PLL_PMS);
    clkpwr->PLLSETREG[0] = (U32)PLL_PMS;

    // PLL1 for CPU
    _GET_PLL01(800, PLL_PMS);
    clkpwr->PLLSETREG[1] = (U32)PLL_PMS;

    // PLL2 for BCLK, 3DCLK
    _GET_PLL23(614, PLL_PMS);
    clkpwr->PLLSETREG[2] = (U32)PLL_PMS;

    // PLL3 for others (Audio)
    _GET_PLL23(800, PLL_PMS);
    clkpwr->PLLSETREG[3] = (U32)PLL_PMS;

    clkpwr->PLLSETREG_SSCG[2] = PLL23_PMS_614MHZ_K<<16 | 2<<0;
//    clkpwr->PLLSETREG_SSCG[3] = PLL23_PMS_614MHZ_K<<16 | 2<<0;

    // CPUDVOREG
    clkpwr->DVOREG[0] = (U32)((NX_CLKSRC_PLL_1<<CLKSRC)|    // PLL Select
                            ((1-1)<<DVO0)|                  // FCLK ==> CPU Group 0
                            ((4-1)<<DVO1));                 // HCLK ==> CPU bus (max 250MHz)

    // CPUDVOREG
    clkpwr->DVOREG[7] = (U32)((NX_CLKSRC_PLL_1<<CLKSRC)|    // PLL Select
                            ((1-1)<<DVO0)|                  // FCLK ==> CPU Group1
                            ((4-1)<<DVO1));                 // HCLK ==> CPU bus (max 250MHz)

    // BUSDVOREG
    clkpwr->DVOREG[1] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // PLL Select
                            ((2-1)<<DVO0)|                  // BCLK ==> System bus (max 333MHz)
                            ((2-1)<<DVO1));                 // PCLK ==> Peripheral bus (max 166MHz)

    // MEMDVOREG
    clkpwr->DVOREG[2] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // PLL Select
                            ((1-1)<<DVO0)|                  // MDCLK ==> Memory DLL (max 800MHz)
                            ((1-1)<<DVO1)|                  // MCLK  ==> Memory DDR (max 800MHz)
                            ((2-1)<<DVO2)|                  // MBCLK ==> MCU bus (max 400MHz)
                            ((2-1)<<DVO3));                 // MPCLK ==> MCU Peripheral bus (max 200MHz)

    clkpwr->DVOREG[3] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // GRP3DVOREG
                            ((2-1)<<DVO0)|                  // GR3DBCLK ==> GPU bus & core (max 333MHz)
                            ((2-1)<<DVO1));                 // GR3DPCLK ==> not used

    clkpwr->DVOREG[4] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // MPEGDVOREG
                            ((2-1)<<DVO0)|                  // MPEGBCLK ==> MPEG bus & core (max 300MHz)
                            ((2-1)<<DVO1));                 // MPEGPCLK ==> MPEG control if (max 150MHz)

    clkpwr->DVOREG[5] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // DISPLAY BUS
                            ((2-1)<<DVO0)|                  // DISPLAY BCLK ==> DISPLAY bus (max 300MHz)
                            ((2-1)<<DVO1));                 // DISPLAY PCLK ==> DISPLAY control if (max 150MHz)

    clkpwr->DVOREG[6] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // HDMI (max 100MHz)
                            ((8-1)<<DVO0));                 // HDMI PCLK ==> HDMI Control if

    clkpwr->DVOREG[8] = (U32)((NX_CLKSRC_PLL_3<<CLKSRC)|    // FAST BUS (max 400MHz)
                            ((2-1)<<DVO0)|                  // FAST BUS BCLK ==> FAST bus (max 400MHz)
                            ((2-1)<<DVO1));                 // FAST BUS PCLK ==> FAST BUS control if (max 200MHz)
#else

    clkpwr->PLLSETREG[0] = pSBI->PLL[0] | (1UL<<28);
    clkpwr->PLLSETREG[1] = pSBI->PLL[1] | (1UL<<28);
    clkpwr->PLLSETREG[2] = pSBI->PLL[2] | (1UL<<28);
    clkpwr->PLLSETREG[3] = pSBI->PLL[3] | (1UL<<28);

    clkpwr->PLLSETREG_SSCG[2] = pSBI->PLLSPREAD[0];
    clkpwr->PLLSETREG_SSCG[3] = pSBI->PLLSPREAD[1];

    clkpwr->DVOREG[0] = pSBI->DVO[0];
    clkpwr->DVOREG[1] = pSBI->DVO[1];
    clkpwr->DVOREG[2] = pSBI->DVO[2];
    clkpwr->DVOREG[3] = pSBI->DVO[3];
    clkpwr->DVOREG[4] = pSBI->DVO[4];
    clkpwr->DVOREG[5] = pSBI->DVO[5];
    clkpwr->DVOREG[6] = pSBI->DVO[6];
    clkpwr->DVOREG[7] = pSBI->DVO[7];
    clkpwr->DVOREG[8] = pSBI->DVO[8];
#endif

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
}

#if 0
void PLLDynamicChange(U32 Freq)
{
//======================= dynamic pll change =================================

//		1. change PLL0 clock to Oscillator Clock
//			 PLLSETREG0.NPLLBYPASS = 0
//			 CLKMODEREG.UPDATE_PLL[0] =1
//			 while(CLKMODEREG.WAIT_UPDATE_PLL) {	// wait for change update pll; }

	clkpwr->PLLSETREG[CPU_CLKSRC] &= ~(1<<28);	// pll bypass on, xtal clock use

	clkpwr->CLKMODEREG0 = (1<<CPU_CLKSRC);	// update pll

	while(clkpwr->CLKMODEREG0 & (1<<31));	// wait for change update pll



//		2. PLL Power Down & PMS value setting
//			PLLSETREG0.PD = 1;
//			PLLSETREG0.PMS= (U32)((PLL_PMS_400MHZ_S<<PLL_S) | (PLL_PMS_400MHZ_M<<PLL_M) | (PLL_PMS_400MHZ_P<<PLL_P));
//			CLKMODEREG.UPDATE_PLL[0] =1
//			while(CLKMODEREG.WAIT_UPDATE_PLL) {  // wait for change update pll; }
//			for ( 1 usec )  wait for pll power down.

	clkpwr->PLLSETREG[CPU_CLKSRC] = (U32)((1UL<<29)|	// power down
											(0UL<<28)|	// clock bypass on, xtal clock use
											(PLL_PMS_800MHZ_S<<PLL_S) | (PLL_PMS_800MHZ_M<<PLL_M) | (PLL_PMS_800MHZ_P<<PLL_P));
	clkpwr->CLKMODEREG0 = (1<<CPU_CLKSRC);	// update pll
	while(clkpwr->CLKMODEREG0 & (1<<31));	// wait for change update pll
	DMC_Delay(100);	// 1us



//		3. Update PLL & wait PLL locking
//			PLLSETREG0.PD = 0;
//			CLKMODEREG.UPDATE_PLL[0] =1
//			while(CLKMODEREG.WAIT_UPDATE_PLL) {	// wait for change update pll; }
//			for ( 1000usec ) wait for pll locking.
	clkpwr->PLLSETREG[CPU_CLKSRC] &= ~((U32)(1UL<<29)); // pll power up

	clkpwr->CLKMODEREG0 = (1<<CPU_CLKSRC);	// update pll

	while(clkpwr->CLKMODEREG0 & (1<<31));	// wait for change update pll

	DMC_Delay(100000);	// 1000us



//		4. Change to PLL clock
//			 PLLSETREG0.NPLLBYPASS = 1; // Change PLL clock
//			 CLKMODEREG.UPDATE_PLL[0] =1
//			 while(CLKMODEREG.WAIT_UPDATE_PLL) {	// wait for change update pll; }

	clkpwr->PLLSETREG[CPU_CLKSRC] |= (1<<28);	// pll bypass off, pll clock use

	clkpwr->CLKMODEREG0 = (1<<CPU_CLKSRC);	// update pll

	while(clkpwr->CLKMODEREG0 & (1<<31));	// wait for change update pll
}
#endif

void printClkInfo(void)
{
    SYSMSG(" PLL0: %d   PLL1: %d   PLL2: %d   PLL3: %d\r\n\r\n",
            NX_CLKPWR_GetPLLFreq(0),
            NX_CLKPWR_GetPLLFreq(1),
            NX_CLKPWR_GetPLLFreq(2),
            NX_CLKPWR_GetPLLFreq(3));

    SYSMSG(" Divider0 PLL: %d CPU Group0:%d   CPU BUS:%d\r\n",
            NX_CLKPWR_GetSrcPll(0),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(0)), ((NX_CLKPWR_GetDivideValue(0)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(0))
                    ,((NX_CLKPWR_GetDivideValue(0)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(0)>> 8)&0x3F)));

    SYSMSG(" Divider7 PLL: %d CPU Group1:%d   CPU BUS:%d\r\n",
            NX_CLKPWR_GetSrcPll(7),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(7)), ((NX_CLKPWR_GetDivideValue(7)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(7))
                    ,((NX_CLKPWR_GetDivideValue(7)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(7)>> 8)&0x3F)));

    SYSMSG(" Divider8 PLL: %d FAST BUS BCLK:%d   PCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(8),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(8)),((NX_CLKPWR_GetDivideValue(8)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(8))
                    ,((NX_CLKPWR_GetDivideValue(8)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(8)>> 8)&0x3F)));

    SYSMSG(" Divider1 PLL: %d BCLK:%d   PCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(1),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(1)),((NX_CLKPWR_GetDivideValue(1)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(1))
                    ,((NX_CLKPWR_GetDivideValue(1)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(1)>> 8)&0x3F)));

    SYSMSG(" Divider5 PLL: %d DISPLAY BCLK:%d   PCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(5),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(5)),((NX_CLKPWR_GetDivideValue(5)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(5))
                    ,((NX_CLKPWR_GetDivideValue(5)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(5)>> 8)&0x3F)));

    SYSMSG(" Divider2 PLL: %d MDCLK:%d   MCLK:%d   \r\n\t\t MBCLK:%d   MPCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(2),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(2))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(2))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 8)&0x3F)),
            getquotient(getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(2))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 8)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(2)>>16)&0x3F)),
            getquotient(getquotient(getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(2))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(2)>> 8)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(2)>>16)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(2)>>24)&0x3F)));

    SYSMSG(" Divider3 PLL: %d G3D BCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(3),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(3)),((NX_CLKPWR_GetDivideValue(3)>> 0)&0x3F)));

    SYSMSG(" Divider4 PLL: %d MPEG BCLK:%d   MPEG PCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(4),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(4)),((NX_CLKPWR_GetDivideValue(4)>> 0)&0x3F)),
            getquotient(getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(4))
                    ,((NX_CLKPWR_GetDivideValue(4)>> 0)&0x3F))
                    ,((NX_CLKPWR_GetDivideValue(4)>> 8)&0x3F)));

    SYSMSG(" Divider6 PLL: %d HDMI PCLK:%d\r\n",
            NX_CLKPWR_GetSrcPll(6),
            getquotient(NX_CLKPWR_GetPLLFreq(NX_CLKPWR_GetSrcPll(6)),((NX_CLKPWR_GetDivideValue(6)>> 0)&0x3F)));
}
