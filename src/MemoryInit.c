#include "sysHeader.h"

#include <nx_drex.h>
#include <nx_ddrphy.h>
#include "ddr3_sdram.h"


#define CONFIG_ODTOFF_GATELEVELINGON        0        // Not support yet.
//#define USE_HEADER
#define DDR_RW_CAL      0

#if (CFG_NSIH_EN == 0)
#include "DDR3_K4B8G1646B_MCK0.h"
#endif

#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");


extern inline void ResetCon(U32 devicenum, CBOOL en);
extern inline void DMC_Delay(int milisecond);


inline void SendDirectCommand(SDRAM_CMD cmd, U8 chipnum, SDRAM_MODE_REG mrx, U16 value)
{
    WriteIO32((U32*)&pReg_Drex->DIRECTCMD, cmd<<24 | chipnum<<20 | mrx<<16 | value);
}

void enterSelfRefresh(void)
{
    union SDRAM_MR MR;
    U32     nTemp;
    U32     nChips = 0;

#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    nChips = 0x3;
#else
    nChips = 0x1;
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        nChips = 0x3;
    else
        nChips = 0x1;
#endif


    while( ReadIO32(&pReg_Drex->CHIPSTATUS) & 0xF )
    {
        nop();
    }

    /* Send PALL command */
    SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
    DMC_Delay(100);

    // odt off
    MR.Reg          = 0;
    MR.MR2.RTT_WR   = 0;        // 0: disable, 1: RZQ/4 (60ohm), 2: RZQ/2 (120ohm)
    MR.MR2.SRT      = 0;        // self refresh normal range, if (ASR == 1) SRT = 0;
    MR.MR2.ASR      = 1;        // auto self refresh enable
#if (CFG_NSIH_EN == 0)
    MR.MR2.CWL      = (nCWL - 5);
#else
    MR.MR2.CWL      = (pSBI->DII.CWL - 5);
#endif

    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif

    MR.Reg          = 0;
    MR.MR1.DLL      = 1;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
    MR.MR1.AL       = MR1_nAL;
#else
    MR.MR1.AL       = pSBI->DII.MR1_AL;
#endif
    MR.MR1.ODS1     = 0;    // 00: RZQ/6, 01 : RZQ/7
    MR.MR1.ODS0     = 1;
    MR.MR1.QOff     = 0;
    MR.MR1.RTT_Nom2     = 0;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8
    MR.MR1.RTT_Nom1     = 1;
    MR.MR1.RTT_Nom0     = 0;
    MR.MR1.WL       = 0;
#if (CFG_NSIH_EN == 0)
    MR.MR1.TDQS     = (_DDR_BUS_WIDTH>>3) & 1;
#else
    MR.MR1.TDQS     = (pSBI->DII.BusWidth>>3) & 1;
#endif

    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif

    /* Enter self-refresh command */
    SendDirectCommand(SDRAM_CMD_REFS, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_REFS, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_REFS, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

    do
    {
        nTemp = ( ReadIO32(&pReg_Drex->CHIPSTATUS) & nChips );
    } while( nTemp );

    do
    {
        nTemp = ( (ReadIO32(&pReg_Drex->CHIPSTATUS) >> 8) & nChips );
    } while( nTemp != nChips );

    // Step 52 Auto refresh counter disable
    ClearIO32( &pReg_Drex->CONCONTROL,  (0x1 << 5));        // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1

    // Step 10  ACK, ACKB off
    SetIO32( &pReg_Drex->MEMCONTROL,    (0x1 << 0));        // clk_stop_en[0] : Dynamic Clock Control       :: 1'b0  - Always running

    DMC_Delay(1000 * 3);
}

void exitSelfRefresh(void)
{
    union SDRAM_MR MR;

    // Step 10    ACK, ACKB on
    ClearIO32( &pReg_Drex->MEMCONTROL,  (0x1 << 0));        // clk_stop_en[0]   : Dynamic Clock Control                 :: 1'b0  - Always running
    DMC_Delay(10);

    // Step 52 Auto refresh counter enable
    SetIO32( &pReg_Drex->CONCONTROL,    (0x1 << 5));        // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1
    DMC_Delay(10);

    /* Send PALL command */
    SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

    MR.Reg          = 0;
    MR.MR1.DLL      = 0;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
    MR.MR1.AL       = MR1_nAL;
#else
    MR.MR1.AL       = pSBI->DII.MR1_AL;
#endif
    MR.MR1.ODS1     = 0;    // 00: RZQ/6, 01 : RZQ/7
    MR.MR1.ODS0     = 1;
    MR.MR1.QOff     = 0;
    MR.MR1.RTT_Nom2     = 0;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8
    MR.MR1.RTT_Nom1     = 1;
    MR.MR1.RTT_Nom0     = 0;
    MR.MR1.WL       = 0;
#if (CFG_NSIH_EN == 0)
    MR.MR1.TDQS     = (_DDR_BUS_WIDTH>>3) & 1;
#else
    MR.MR1.TDQS     = (pSBI->DII.BusWidth>>3) & 1;
#endif

    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif

    // odt on
    MR.Reg          = 0;
    MR.MR2.RTT_WR   = 2;        // 0: disable, 1: RZQ/4 (60ohm), 2: RZQ/2 (120ohm)
    MR.MR2.SRT      = 0;        // self refresh normal range
    MR.MR2.ASR      = 0;        // auto self refresh disable
#if (CFG_NSIH_EN == 0)
    MR.MR2.CWL      = (nCWL - 5);
#else
    MR.MR2.CWL      = (pSBI->DII.CWL - 5);
#endif

    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif

    /* Exit self-refresh command */
    SendDirectCommand(SDRAM_CMD_REFSX, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_REFSX, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_REFSX, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

    while( ReadIO32(&pReg_Drex->CHIPSTATUS) & (0xF << 8) )
    {
        nop();
    }

    DMC_Delay(1000 * 2);
}


#if DDR_RW_CAL
void DDR3_RW_Delay_Calibration(void)
{
extern    void BurstZero(U32 *WriteAddr, U32 WData);
extern    void BurstWrite(U32 *WriteAddr, U32 WData);
extern    void BurstRead(U32 *ReadAddr, U32 *SaveAddr);
//    struct NX_DREXSDRAM_RegisterSet *pReg_Drex = (struct NX_DREXSDRAM_RegisterSet *)PHY_BASEADDR_DREX_MODULE_CH0_APB;
    struct NX_DDRPHY_RegisterSet *pDDRPHY = (struct NX_DDRPHY_RegisterSet *)PHY_BASEADDR_DREX_MODULE_CH1_APB;
    unsigned int rnw, lane, adjusted_dqs_delay=0, bit, pmin, nmin;
    unsigned int *tptr = (unsigned int *)0x40100000;
    int toggle;

    for(rnw = 0; rnw<2; rnw++)
    {
        printf("\r\nserching %s delay value......\r\n", rnw?"read":"write");
        bit = 0;
        for(bit = 0; bit<32; bit++)
        {
            unsigned int readdata[8];
            unsigned int dqs_wdelay, repeatcount;
            unsigned char pwdelay, nwdelay;
            lane = bit>>3;

            if((bit & 7) == 0)
            {
                pmin = 0x7f;
                nmin = 0x7f;
            }
            printf("bit:%02d\t", bit);
            pwdelay = 0x80;
            if(rnw ==0)
                WriteIO32(&pDDRPHY->OFFSETW_CON[0], 0x80<<(8*lane));
            else
                WriteIO32(&pDDRPHY->OFFSETR_CON[0], 0x80<<(8*lane));
            SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));           // Force DLL Resyncronization
            ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));           // Force DLL Resyncronization
            DMC_Delay(10000);
            for(dqs_wdelay = 0; dqs_wdelay<=0x7f && pwdelay==0x80; dqs_wdelay++)
            {
                repeatcount=0;
                if(rnw ==0)
                    WriteIO32(&pDDRPHY->OFFSETW_CON[0], dqs_wdelay<<(8*lane));
                else
                    WriteIO32(&pDDRPHY->OFFSETR_CON[0], dqs_wdelay<<(8*lane));
                SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
                ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
                DMC_Delay(10000);
                while(repeatcount<100)
                {
                    for(toggle=1; toggle>=0; toggle--)
                    {
                        if(toggle)
                            BurstWrite(tptr, 1<<bit);
                        else
                            BurstWrite(tptr, ~(1<<bit));
                        BurstRead(tptr, readdata);
                        if( ((readdata[0]>>bit)&0x01) == !toggle &&
                            ((readdata[1]>>bit)&0x01) == !toggle &&
                            ((readdata[2]>>bit)&0x01) == !toggle &&
                            ((readdata[3]>>bit)&0x01) == !toggle &&
                            ((readdata[4]>>bit)&0x01) ==  toggle &&
                            ((readdata[5]>>bit)&0x01) == !toggle &&
                            ((readdata[6]>>bit)&0x01) == !toggle &&
                            ((readdata[7]>>bit)&0x01) == !toggle)
                        {
                            repeatcount++;
                        }else
                        {
                            pwdelay = dqs_wdelay;
                            if(pmin > pwdelay)
                                pmin = pwdelay;
                            printf("p%d:%02d ", toggle, pwdelay);
                            repeatcount = 100;
                            toggle = -1;
                            break;
                        }
                    }
                }
            }    // dqs_wdelay
            if(rnw==0)
                WriteIO32(&pDDRPHY->OFFSETW_CON[0], 0<<(8*lane));
            else
                WriteIO32(&pDDRPHY->OFFSETR_CON[0], 0<<(8*lane));
            SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));           // Force DLL Resyncronization
            ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));           // Force DLL Resyncronization
            DMC_Delay(10000);
            nwdelay = 0;
            for(dqs_wdelay = 0x80; dqs_wdelay<=0xFF && nwdelay==0; dqs_wdelay++)
            {
                repeatcount=0;
                if(rnw == 0)
                    WriteIO32(&pDDRPHY->OFFSETW_CON[0], dqs_wdelay<<(8*lane));
                else
                    WriteIO32(&pDDRPHY->OFFSETR_CON[0], dqs_wdelay<<(8*lane));
                SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
                ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
                DMC_Delay(10000);
                while(repeatcount<100)
                {
                    for(toggle=1; toggle>=0; toggle--)
                    {
                        if(toggle)
                            BurstWrite(tptr, 1<<bit);
                        else
                            BurstWrite(tptr, ~(1<<bit));
                        BurstRead(tptr, readdata);
                        if( ((readdata[0]>>bit)&0x01) == !toggle &&
                            ((readdata[1]>>bit)&0x01) == !toggle &&
                            ((readdata[2]>>bit)&0x01) == !toggle &&
                            ((readdata[3]>>bit)&0x01) == !toggle &&
                            ((readdata[4]>>bit)&0x01) ==  toggle &&
                            ((readdata[5]>>bit)&0x01) == !toggle &&
                            ((readdata[6]>>bit)&0x01) == !toggle &&
                            ((readdata[7]>>bit)&0x01) == !toggle)
                        {
                            repeatcount++;
                        }else
                        {
                            nwdelay = dqs_wdelay & 0x7F;
                            if(nmin > nwdelay)
                                nmin = nwdelay;
                            printf("n%d:%02d  ", toggle, nwdelay);
                            repeatcount = 100;
                            toggle = -1;
                            break;
                        }
                    }
                }
            }    // dqs_wdelay

            if(pwdelay > nwdelay)    // biased to positive side
            {
                printf("margin: %2d  adj: %2d\r\n", (pwdelay - nwdelay), (pwdelay - nwdelay)>>1);
            }
            else    // biased to negative side
            {
                printf("margin: %2d  adj:-%2d\r\n", (nwdelay - pwdelay), (nwdelay - pwdelay)>>1);
            }
            if((bit & 7)==7)
            {
                printf("lane average positive min:%d negative min:%d ", pmin, nmin);
                if(pmin > nmin) // biased to positive side
                {
                    adjusted_dqs_delay |= ((pmin - nmin)>>1) << (8*lane);
                    printf("margin: %2d  adj: %2d\r\n", (pmin - nmin), (pmin - nmin)>>1);
                }
                else    // biased to negative side
                {
                    adjusted_dqs_delay |= (((nmin - pmin)>>1) | 0x80) << (8*lane);
                    printf("margin: %2d  adj:-%2d\r\n", (nmin - pmin), (nmin - pmin)>>1);
                }
            }
            if(((bit+1) & 0x7) == 0)
                printf("\n");
        }   // lane

        if(rnw == 0)
            WriteIO32(&pDDRPHY->OFFSETW_CON[0], adjusted_dqs_delay);
        else
            WriteIO32(&pDDRPHY->OFFSETR_CON[0], adjusted_dqs_delay);
        SetIO32  ( &pReg_Drex->PHYCONTROL,   (0x1   <<   3));           // Force DLL Resyncronization
        ClearIO32( &pReg_Drex->PHYCONTROL,   (0x1   <<   3));           // Force DLL Resyncronization
        printf("\r\n");
        printf("read  delay value is 0x%08X\r\n", ReadIO32(&pDDRPHY->OFFSETR_CON[0]));
        printf("write delay value is 0x%08X\r\n", ReadIO32(&pDDRPHY->OFFSETW_CON[0]));
    }
}
#endif

void DDR3_Init(U32 isResume)
{
//    struct NX_DREXSDRAM_RegisterSet *pReg_Drex = (struct NX_DREXSDRAM_RegisterSet *)PHY_BASEADDR_DREX_MODULE_CH0_APB;
    struct NX_DREXTZ_RegisterSet *pReg_DrexTZ = (struct NX_DREXTZ_RegisterSet *)PHY_BASEADDR_DREX_TZ_MODULE;
    struct NX_DDRPHY_RegisterSet *pDDRPHY = (struct NX_DDRPHY_RegisterSet *)PHY_BASEADDR_DREX_MODULE_CH1_APB;
//    struct NX_RSTCON_RegisterSet * const pRSTCONReg = (struct NX_RSTCON_RegisterSet *)PHY_BASEADDR_RSTCON_MODULE;
    union SDRAM_MR MR0, MR1, MR2, MR3;
    U32 DDR_AL, DDR_WL, DDR_RL;
    U32 temp;


    printf("\r\nDDR3 POR Init Start\r\n");

    DDR_AL = 0;
#if (CFG_NSIH_EN == 0)
    if (MR1_nAL > 0)
        DDR_AL = nCL - MR1_nAL;

    DDR_WL = (DDR_AL + nCWL);
    DDR_RL = (DDR_AL + nCL);
#else
    if (pSBI->DII.MR1_AL > 0)
        DDR_AL = pSBI->DII.CL - pSBI->DII.MR1_AL;

    DDR_WL = (DDR_AL + pSBI->DII.CWL);
    DDR_RL = (DDR_AL + pSBI->DII.CL);
#endif

    if (isResume == 0)
    {
        MR2.Reg         = 0;
        MR2.MR2.RTT_WR  = 2; // RTT_WR - 0: disable, 1: RZQ/4, 2: RZQ/2
        MR2.MR2.SRT     = 0; // self refresh normal range
        MR2.MR2.ASR     = 0; // auto self refresh disable
#if (CFG_NSIH_EN == 0)
        MR2.MR2.CWL     = (nCWL - 5);
#else
        MR2.MR2.CWL     = (pSBI->DII.CWL - 5);
#endif

        MR3.Reg         = 0;
        MR3.MR3.MPR     = 0;
        MR3.MR3.MPR_RF  = 0;

        MR1.Reg         = 0;
        MR1.MR1.DLL     = 0;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
        MR1.MR1.AL      = MR1_nAL;
#else
        MR1.MR1.AL      = pSBI->DII.MR1_AL;
#endif
        MR1.MR1.ODS1    = 0;    // 00: RZQ/6, 01 : RZQ/7
        MR1.MR1.ODS0    = 1;
        MR1.MR1.QOff    = 0;
        MR1.MR1.RTT_Nom2    = 0;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8
        MR1.MR1.RTT_Nom1    = 1;
        MR1.MR1.RTT_Nom0    = 0;
        MR1.MR1.WL      = 0;
#if (CFG_NSIH_EN == 0)
        MR1.MR1.TDQS    = (_DDR_BUS_WIDTH>>3) & 1;
#else
        MR1.MR1.TDQS    = (pSBI->DII.BusWidth>>3) & 1;
#endif

#if (CFG_NSIH_EN == 0)
        if (nCL > 11)
            temp = ((nCL-12) << 1) + 1;
        else
            temp = ((nCL-4) << 1);
#else
        if (pSBI->DII.CL > 11)
            temp = ((pSBI->DII.CL-12) << 1) + 1;
        else
            temp = ((pSBI->DII.CL-4) << 1);
#endif

        MR0.Reg         = 0;
        MR0.MR0.BL      = 0;
        MR0.MR0.BT      = 1;
        MR0.MR0.CL0     = (temp & 0x1);
        MR0.MR0.CL1     = ((temp>>1) & 0x7);
        MR0.MR0.DLL     = 0;//1;
#if (CFG_NSIH_EN == 0)
        MR0.MR0.WR      = MR0_nWR;
#else
        MR0.MR0.WR      = pSBI->DII.MR0_WR;
#endif
        MR0.MR0.PD      = 0;//1;
    }   // if (isResume == 0)

// Step 1. reset (Min : 10ns, Typ : 200us)
    ResetCon(RESETINDEX_OF_DREX_MODULE_CRESETn, CTRUE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_ARESETn, CTRUE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_nPRST,   CTRUE);
    DMC_Delay(0x1000);                         // wait 300ms
    ResetCon(RESETINDEX_OF_DREX_MODULE_CRESETn, CFALSE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_ARESETn, CFALSE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_nPRST,   CFALSE);
    DMC_Delay(0x1000);                         // wait 300ms
    ResetCon(RESETINDEX_OF_DREX_MODULE_CRESETn, CTRUE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_ARESETn, CTRUE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_nPRST,   CTRUE);
    DMC_Delay(0x1000);                         // wait 300ms
    ResetCon(RESETINDEX_OF_DREX_MODULE_CRESETn, CFALSE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_ARESETn, CFALSE);
    ResetCon(RESETINDEX_OF_DREX_MODULE_nPRST,   CFALSE);
    DMC_Delay(0x1000);                         // wait 300ms

    printf("PHY Version: %X\r\n", ReadIO32(&pDDRPHY->VERSION_INFO));

#if 1    // ddr simulation
// Step 2. Select Memory type : DDR3
// Check DDR3 MPR data and match it to PHY_CON[1]??
    temp = (
        (0x0    <<  29) |           // [31:29] Reserved - SBZ.
        (0x17   <<  24) |           // [28:24] T_WrWrCmd.
        (0x1    <<  22) |           // [23:22] DLL Update control 0:always, 1: depending on ctrl_flock, 2: depending on ctrl_clock, 3: don't update
        (0x0    <<  20) |           // [21:20] ctrl_upd_range.
#if (CFG_NSIH_EN == 0)
#if (tWTR == 3)     // 6 cycles
        (0x7    <<  17) |           // [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#elif (tWTR == 2)   // 4 cycles
        (0x6    <<  17) |           // [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#endif
#endif
        (0x0    <<  16) |           // [   16] ctrl_wrlvl_en[16]. Write Leveling Enable. 0:Disable, 1:Enable
//        (0x0    <<  15) |           // [   15] Reserved SBZ.
        (0x0    <<  14) |           // [   14] p0_cmd_en. 0:Issue Phase1 Read command during Read Leveling. 1:Issue Phase0
        (0x0    <<  13) |           // [   13] byte_rdlvl_en. Read Leveling 0:Disable, 1:Enable
        (0x1    <<  11) |           // [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
        (0x1    <<  10) |           // [   10] Write ODT Disable Signal during Write Calibration. 0: not change, 1: disable
        (0x1    <<   9) |           // [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
//        (0x0    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
        (0x1    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
        (0x1    <<   6) |           // [    6] ctrl_atgate.
        (0x0    <<   4) |           // [    4] ctrl_cmosrcv.
        (0x0    <<   0));           // [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

#if (CFG_NSIH_EN == 1)
    if ((pSBI->DII.TIMINGDATA >> 28) == 3)      // 6 cycles
        temp |= (0x7    <<  17);
    else if ((pSBI->DII.TIMINGDATA >> 28) == 2) // 4 cycles
        temp |= (0x6    <<  17);
#endif

    WriteIO32( &pDDRPHY->PHY_CON[0],    temp );

#if 1
#ifdef MEM_TYPE_DDR3
    temp  = ReadIO32( &pDDRPHY->LP_DDR_CON[3]) & ~0x3FFF;
    temp |= 0x105E;                                             // cmd_active= DDR3:0x105E, LPDDDR2 or LPDDDR3:0x000E
    WriteIO32( &pDDRPHY->LP_DDR_CON[3], temp);

    temp  = ReadIO32( &pDDRPHY->LP_DDR_CON[4]) & ~0x3FFF;
    temp |= 0x107F;                                             // cmd_default= DDR3:0x107F, LPDDDR2 or LPDDDR3:0x000F
    WriteIO32( &pDDRPHY->LP_DDR_CON[4], temp);
#endif

#ifdef MEM_TYPE_LPDDR23
    temp  = ReadIO32( &pDDRPHY->LP_DDR_CON[3]) & ~0x3FFF;
    temp |= 0x000E;                                             // cmd_active= DDR3:0x105E, LPDDDR2 or LPDDDR3:0x000E
    WriteIO32( &pDDRPHY->LP_DDR_CON[3], temp);

    temp  = ReadIO32( &pDDRPHY->LP_DDR_CON[4]) & ~0x3FFF;
    temp |= 0x000F;                                             // cmd_default= DDR3:0x107F, LPDDDR2 or LPDDDR3:0x000F
    WriteIO32( &pDDRPHY->LP_DDR_CON[4], temp);

    SetIO32( &pDDRPHY->OFFSETD_CON,     (1<<28) );
#endif
#endif

    printf("phy init\r\n");

/* Set WL, RL, BL */
    WriteIO32( &pDDRPHY->PHY_CON[4],
        (0x8    <<  8) |            // Burst Length(BL)
        (DDR_WL << 16) |            // T_wrdata_en (WL+1)
        (DDR_RL <<  0));            // Read Latency(RL), 800MHz:0xB, 533MHz:0x5

    /* ZQ Calibration */
    WriteIO32( &pDDRPHY->DRVDS_CON[0],        // 100: 48ohm, 101: 40ohm, 110: 34ohm, 111: 30ohm
        (0x6    <<  25) |           // Data Slice 3
        (0x6    <<  22) |           // Data Slice 2
        (0x6    <<  19) |           // Data Slice 1
        (0x6    <<  16) |           // Data Slice 0
        (0x6    <<   9) |           // CK
        (0x6    <<   6) |           // CKE
        (0x6    <<   3) |           // CS
        (0x6    <<   0));           // CA[9:0], RAS, CAS, WEN, ODT[1:0], RESET, BANK[2:0]


    // Driver Strength(zq_mode_dds), zq_clk_div_en[18]=Enable
    WriteIO32( &pDDRPHY->ZQ_CON,
//        (0x0    <<  28) |           // Reserved[31:28]. SBZ
        (0x1    <<  27) |           // zq_clk_en[27]. ZQ I/O clock enable.
        (0x6    <<  24) |           // zq_mode_dds[26:24], Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
//        (0x7    <<  24) |           // zq_mode_dds[26:24], Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
        (0x1    <<  21) |           // ODT resistor value[23:21]. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
//        (0x0    <<  21) |           // ODT resistor value[23:21]. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
        (0x0    <<  20) |           // zq_rgddr3[20]. GDDR3 mode. 0:Enable, 1:Disable
        (0x0    <<  19) |           // zq_mode_noterm[19]. Termination. 0:Enable, 1:Disable
        (0x1    <<  18) |           // zq_clk_div_en[18]. Clock Dividing Enable : 0, Disable : 1
        (0x0    <<  15) |           // zq_force-impn[17:15]
//        (0x7    <<  12) |           // zq_force-impp[14:12]
        (0x0    <<  12) |           // zq_force-impp[14:12]
        (0x30   <<   4) |           // zq_udt_dly[11:4]
        (0x1    <<   2) |           // zq_manual_mode[3:2]. 0:Force Calibration, 1:Long cali, 2:Short cali
        (0x0    <<   1) |           // zq_manual_str[1]. Manual Calibration Stop : 0, Start : 1
        (0x0    <<   0));           // zq_auto_en[0]. Auto Calibration enable

    SetIO32( &pDDRPHY->ZQ_CON,          (0x1    <<   1) );          // zq_manual_str[1]. Manual Calibration Start=1
    while( ( ReadIO32( &pDDRPHY->ZQ_STATUS ) & 0x1 ) == 0 );        //- PHY0: wait for zq_done
    ClearIO32( &pDDRPHY->ZQ_CON,        (0x1    <<   1) );          // zq_manual_str[1]. Manual Calibration Stop : 0, Start : 1

    ClearIO32( &pDDRPHY->ZQ_CON,        (0x1    <<  18) );          // zq_clk_div_en[18]. Clock Dividing Enable : 1, Disable : 0



// Step 5. dfi_init_start : High
    SetIO32( &pReg_Drex->CONCONTROL,    (0x1    <<  28) );          // DFI PHY initialization start
    while( (ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1<<3) ) == 0);   // wait for DFI PHY initialization complete
    ClearIO32( &pReg_Drex->CONCONTROL,  (0x1    <<  28) );          // DFI PHY initialization clear



    // Step 3. Set the PHY for dqs pull down mode
    WriteIO32( &pDDRPHY->LP_CON,
        (0x0    <<  16) |       // ctrl_pulld_dq[24:16]
        (0xF    <<   0));       // ctrl_pulld_dqs[8:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)


    // Step 8 : Update DLL information
    SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization


    // Step 11. MemBaseConfig
    WriteIO32( &pReg_DrexTZ->MEMBASECONFIG[0],
        (0x040      <<  16) |                   // chip_base[26:16]. AXI Base Address. if 0x20 ==> AXI base addr of memory : 0x2000_0000
#if (CFG_NSIH_EN == 0)
        (chip_mask  <<   0));                   // 256MB:0x7F0, 512MB: 0x7E0, 1GB:0x7C0, 2GB: 0x780, 4GB:0x700
#else
        (pSBI->DII.ChipMask <<   0));
#endif

#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        WriteIO32( &pReg_DrexTZ->MEMBASECONFIG[1],
            (0x080      <<  16) |               // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
            (chip_mask  <<   0));               // chip_mask[10:0]. 2048 - chip size
#endif
#else
    if(pSBI->DII.ChipNum > 1)
    {
        WriteIO32( &pReg_DrexTZ->MEMBASECONFIG[1],
            (pSBI->DII.ChipBase <<  16) |       // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
            (pSBI->DII.ChipMask <<   0));       // chip_mask[10:0]. 2048 - chip size
    }
#endif

// Step 12. MemConfig
    WriteIO32( &pReg_DrexTZ->MEMCONFIG[0],
        (0x0    <<  20) |           // bank lsb, LSB of Bank Bit Position in Complex Interleaved Mapping 0:8, 1: 9, 2:10, 3:11, 4:12, 5:13
        (0x0    <<  19) |           // rank inter en, Rank Interleaved Address Mapping
        (0x0    <<  18) |           // bit sel en, Enable Bit Selection for Randomized interleaved Address Mapping
        (0x0    <<  16) |           // bit sel, Bit Selection for Randomized Interleaved Address Mapping
        (0x2    <<  12) |           // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0:Linear(Bank, Row, Column, Width), 1:Interleaved(Row, bank, column, width), other : reserved
#if (CFG_NSIH_EN == 0)
        (chip_col   <<   8) |       // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
        (chip_row   <<   4) |       // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
#else
        (pSBI->DII.ChipCol  <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
        (pSBI->DII.ChipRow  <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
#endif
        (0x3    <<   0));           // [ 3: 0] chip_bank. Number of  Bank Address Bit. others:Reserved, 2:4bank, 3:8banks


#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        WriteIO32( &pReg_DrexTZ->MEMCONFIG[1],
            (0x0    <<  20) |       // bank lsb, LSB of Bank Bit Position in Complex Interleaved Mapping 0:8, 1: 9, 2:10, 3:11, 4:12, 5:13
            (0x0    <<  19) |       // rank inter en, Rank Interleaved Address Mapping
            (0x0    <<  18) |       // bit sel en, Enable Bit Selection for Randomized interleaved Address Mapping
            (0x0    <<  16) |       // bit sel, Bit Selection for Randomized Interleaved Address Mapping
            (0x2    <<  12) |       // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
            (chip_col   <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
            (chip_row   <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
            (0x3    <<   0));       // [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
#endif
#else
    if(pSBI->DII.ChipNum > 1) {
        WriteIO32( &pReg_DrexTZ->MEMCONFIG[1],
            (0x0    <<  20) |       // bank lsb, LSB of Bank Bit Position in Complex Interleaved Mapping 0:8, 1: 9, 2:10, 3:11, 4:12, 5:13
            (0x0    <<  19) |       // rank inter en, Rank Interleaved Address Mapping
            (0x0    <<  18) |       // bit sel en, Enable Bit Selection for Randomized interleaved Address Mapping
            (0x0    <<  16) |       // bit sel, Bit Selection for Randomized Interleaved Address Mapping
            (0x2    <<  12) |       // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
            (pSBI->DII.ChipCol  <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
            (pSBI->DII.ChipRow  <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
            (0x3    <<   0));       // [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
    }
#endif

// Step 13. Precharge Configuration
//    WriteIO32( &pReg_Drex->PRECHCONFIG0,
//        (0xF    <<    28)    |        // Timeout Precharge per Port
//        (0x0    <<    16));            // open page policy
//    WriteIO32( &pReg_Drex->PRECHCONFIG1,    0xFFFFFFFF );           //- precharge cycle
//    WriteIO32( &pReg_Drex->PWRDNCONFIG,     0xFFFF00FF );           //- low power counter



// Step 14.  AC Timing
#if (CFG_NSIH_EN == 0)
    WriteIO32( &pReg_Drex->TIMINGAREF,
        (tREFIPB    <<  16) |       //- rclk (MPCLK)
        (tREFI      <<   0));       //- refresh counter, 800MHz : 0x618

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGROW,
        (tRFC       <<  24) |
        (tRRD       <<  20) |
        (tRP        <<  16) |
        (tRCD       <<  12) |
        (tRC        <<   6) |
        (tRAS       <<   0)) ;

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGDATA,
        (tWTR       <<  28) |
        (tWR        <<  24) |
        (tRTP       <<  20) |
        (tPPD       <<  17) |
        (W2W_C2C    <<  14) |
        (R2R_C2C    <<  12) |
        (nWL        <<   8) |
        (tDQSCK     <<   4) |
        (nRL        <<   0));

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGPOWER,
        (tFAW       <<  26) |
        (tXSR       <<  16) |
        (tXP        <<   8) |
        (tCKE       <<   4) |
        (tMRD       <<   0));

#if (_DDR_CS_NUM > 1)
    WriteIO32( &pReg_Drex->ACTIMING1.TIMINGROW,
        (tRFC       <<  24) |
        (tRRD       <<  20) |
        (tRP        <<  16) |
        (tRCD       <<  12) |
        (tRC        <<   6) |
        (tRAS       <<   0)) ;

    WriteIO32( &pReg_Drex->ACTIMING1.TIMINGDATA,
        (tWTR       <<  28) |
        (tWR        <<  24) |
        (tRTP       <<  20) |
        (W2W_C2C    <<  14) |   // W2W_C2C
        (R2R_C2C    <<  12) |   // R2R_C2C
        (nWL        <<   8) |
        (tDQSCK     <<   4) |   // tDQSCK
        (nRL        <<   0));

    WriteIO32( &pReg_Drex->ACTIMING1.TIMINGPOWER,
        (tFAW       <<  26) |
        (tXSR       <<  16) |
        (tXP        <<   8) |
        (tCKE       <<   4) |
        (tMRD       <<   0));
#endif

//    WriteIO32( &pReg_Drex->TIMINGPZQ,   0x00004084 );     //- average periodic ZQ interval. Max:0x4084
    WriteIO32( &pReg_Drex->TIMINGPZQ,   tPZQ );           //- average periodic ZQ interval. Max:0x4084
#else

// Step 14.  AC Timing
    WriteIO32( &pReg_Drex->TIMINGAREF,              pSBI->DII.TIMINGAREF );                 //- refresh counter, 800MHz : 0x618

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGROW,     pSBI->DII.TIMINGROW ) ;
    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGDATA,    pSBI->DII.TIMINGDATA );
    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGPOWER,   pSBI->DII.TIMINGPOWER );

    if(pSBI->DII.ChipNum > 1)
    {
        WriteIO32( &pReg_Drex->ACTIMING1.TIMINGROW,     pSBI->DII.TIMINGROW ) ;
        WriteIO32( &pReg_Drex->ACTIMING1.TIMINGDATA,    pSBI->DII.TIMINGDATA );
        WriteIO32( &pReg_Drex->ACTIMING1.TIMINGPOWER,   pSBI->DII.TIMINGPOWER );
    }

//    WriteIO32( &pReg_Drex->TIMINGPZQ,   0x00004084 );               //- average periodic ZQ interval. Max:0x4084
    WriteIO32( &pReg_Drex->TIMINGPZQ,   pSBI->DII.TIMINGPZQ );      //- average periodic ZQ interval. Max:0x4084
#endif

#if 1
#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
#if (CFG_NSIH_EN == 0)
    WriteIO32( &pDDRPHY->PHY_CON[4],    READDELAY);
    WriteIO32( &pDDRPHY->PHY_CON[6],    WRITEDELAY);
#else
    WriteIO32( &pDDRPHY->PHY_CON[4],    pSBI->DII.READDELAY);
    WriteIO32( &pDDRPHY->PHY_CON[6],    pSBI->DII.WRITEDELAY);
#endif
#endif
#if defined(ARCH_NXP5430)
#if (CFG_NSIH_EN == 0)
    WriteIO32( &pDDRPHY->OFFSETR_CON[0], READDELAY);
    WriteIO32( &pDDRPHY->OFFSETW_CON[0], WRITEDELAY);
#else
    WriteIO32( &pDDRPHY->OFFSETR_CON[0], pSBI->DII.READDELAY);
    WriteIO32( &pDDRPHY->OFFSETW_CON[0], pSBI->DII.WRITEDELAY);
#endif
#endif
#endif

    if (isResume == 0)
    {
        // Step 18, 19 :  Send NOP command.
        SendDirectCommand(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif


        // Step 20 :  Send MR2 command.
        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR2.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR2.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR2.Reg);
#endif


        // Step 21 :  Send MR3 command.
        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR3.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR3.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR3.Reg);
#endif


        // Step 22 :  Send MR1 command.
        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR1.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif


        // Step 23 :  Send MR0 command.
        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR0, MR0.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR0, MR0.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR0, MR0.Reg);
#endif


        // Step 25 : Send ZQ Init command
        SendDirectCommand(SDRAM_CMD_ZQINIT, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_ZQINIT, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_ZQINIT, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
        DMC_Delay(100);
    }   // if (isResume)


#if 1
{
    U32 lock0;

//    printf("\r\n########## READ/GATE Level ##########\r\n");

    SetIO32( &pDDRPHY->PHY_CON[0],        (0x1  <<  6) );           // ctrl_atgate=1
    SetIO32( &pDDRPHY->PHY_CON[0],        (0x1  << 14) );           // p0_cmd_en=1
    SetIO32( &pDDRPHY->PHY_CON[2],        (0x1  <<  6) );           // InitDeskewEn=1
    SetIO32( &pDDRPHY->PHY_CON[0],        (0x1  << 13) );           // byte_rdlvl_en=1

    temp  = ReadIO32( &pDDRPHY->PHY_CON[1]) & ~(0xF <<  16);        // rdlvl_pass_adj=4
    temp|= (0x4 <<  16);
    WriteIO32( &pDDRPHY->PHY_CON[1],    temp);

    temp  = ReadIO32( &pDDRPHY->LP_DDR_CON[3]) & ~0x3FFF;
    temp |= 0x105E;                                                 // cmd_active= DDR3:0x105E, LPDDDR2 or LPDDDR3:0x000E
    WriteIO32( &pDDRPHY->LP_DDR_CON[3],   temp);

    temp  = ReadIO32( &pDDRPHY->LP_DDR_CON[4]) & ~0x3FFF;
    temp |= 0x107F;                                                 // cmd_default= DDR3:0x107F, LPDDDR2 or LPDDDR3:0x000F
    WriteIO32( &pDDRPHY->LP_DDR_CON[4],   temp);

    temp  = ReadIO32( &pDDRPHY->PHY_CON[2]) & ~(0x7F << 16);        // rdlvl_incr_adj=1
    temp |= (0x1 <<  16);
    WriteIO32( &pDDRPHY->PHY_CON[2],    temp);

    ClearIO32( &pDDRPHY->MDLL_CON[0],   (0x1 << 5));                // ctrl_dll_on=0

    lock0 = (ReadIO32( &pDDRPHY->MDLL_CON[1]) >> 8) & 0x1FF;        // read lock value

    temp  = ReadIO32( &pDDRPHY->MDLL_CON[0]) & ~(0x1FF << 7);
    temp |= (lock0 << 7);
    WriteIO32( &pDDRPHY->MDLL_CON[0],    temp);

#if (DDR_WRITE_LEVELING_EN == 1)
    DDR_Write_Leveling();
#endif
#if (DDR_CA_CALIB_EN == 1)
    DDR_CA_Calibration();
#endif
#if (DDR_GATE_LEVELING_EN == 1)
    DDR_Gate_Leveling();
#endif
#if (DDR_READ_DQ_CALIB_EN == 1)
    DDR_Read_DQ_Calibration();
#endif
#if (DDR_WRITE_LEVELING_CALIB_EN == 1)
    DDR_Write_Leveling_Calibration();
#endif
#if (DDR_WRITE_DQ_CALIB_EN == 1)
    DDR_Write_DQ_Calibration();
#endif

    SetIO32( &pDDRPHY->MDLL_CON[0],     (0x1    <<   5));           // ctrl_dll_on=1
    SetIO32( &pDDRPHY->PHY_CON[2],      (0x1    <<  12));           // DLLDeskewEn=1

#if 0
    ClearIO32( &pDDRPHY->OFFSETD_CON,   (0x1    <<  28));           // upd_mode=0

    SetIO32  ( &pDDRPHY->OFFSETD_CON,   (0x1    <<  24));           // ctrl_resync=1
    ClearIO32( &pDDRPHY->OFFSETD_CON,   (0x1    <<  24));           // ctrl_resync=0
#endif
}
#endif  // gate leveling



    SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization

    temp = (U32)(
//        (0x0    <<  29) |           // Reserved[31:29] SBZ.
        (0x17   <<  24) |           // T_WrWrCmd.
        (0x1    <<  22) |           // DLL Update control 0:always, 1: depending on ctrl_flock, 2: depending on ctrl_clock, 3: don't update
        (0x0    <<  20) |           // ctrl_upd_range[21:20].
#if (CFG_NSIH_EN == 0)
#if (tWTR == 3)
        (0x7    <<  17) |           // T_RwRdCmd[19:17]. 6:tWTR=4cycle, 7:tWTR=6cycle
#elif (tWTR == 2)
        (0x6    <<  17) |           // T_RwRdCmd[19:17]. 6:tWTR=4cycle, 7:tWTR=6cycle
#endif
#endif
        (0x0    <<  16) |           // ctrl_wrlvl_en[16]. Write Leveling Enable. 0:Disable, 1:Enable
//        (0x0    <<  15) |           // Reserved[15] SBZ.
        (0x0    <<  14) |           // p0_cmd_en[14]. 0:Issue Phase1 Read command during Read Leveling. 1:Issue Phase0
        (0x0    <<  13) |           // byte_rdlvl_en[13]. Read Leveling 0:Disable, 1:Enable
        (0x1    <<  11) |           // ctrl_ddr_mode[12:11]. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
        (0x1    <<  10) |           // Write ODT Disable Signal during Write Calibration. 0: not change, 1: disable
        (0x1    <<   9) |           // ctrl_dfdqs[9]. 0:Single-ended DQS, 1:Differential DQS
        (0x0    <<   8) |           // ctrl_shgate[8]. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
        (0x1    <<   6) |           // ctrl_atgate[6].
        (0x0    <<   4) |           // ctrl_cmosrcv[4].
        (0x0    <<   0));           // ctrl_fnc_fb[2:0]. 000:Normal operation.

#if (CFG_NSIH_EN == 1)
    if ((pSBI->DII.TIMINGDATA >> 28) == 3)      // 6 cycles
        temp |= (0x7    <<  17);
    else if ((pSBI->DII.TIMINGDATA >> 28) == 2) // 4 cycles
        temp |= (0x6    <<  17);
#endif

    WriteIO32( &pDDRPHY->PHY_CON[0],    temp );

    /* Send PALL command */
    SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif


    temp = (U32)(
        (0x0    <<  29) |           // [31:29] pause_ref_en : Refresh command issue Before PAUSE ACKNOLEDGE
        (0x0    <<  28) |           // [   28] sp_en        : Read with Short Preamble in Wide IO Memory
        (0x0    <<  27) |           // [   27] pb_ref_en    : Per bank refresh for LPDDR4/LPDDR3
//        (0x0    <<  25) |           // [26:25] reserved     : SBZ
        (0x0    <<  24) |           // [   24] pzq_en       : DDR3 periodic ZQ(ZQCS) enable
//        (0x0    <<  23) |           // [   23] reserved     : SBZ
        (0x3    <<  20) |           // [22:20] bl           : Memory Burst Length                       :: 3'h3  - 8
#if (CFG_NSIH_EN == 0)
        ((_DDR_CS_NUM-1)        <<  16) |   // [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
#else
        ((pSBI->DII.ChipNum-1)  <<  16) |   // [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
#endif
        (0x2    <<  12) |           // [15:12] mem_width    : Width of Memory Data Bus                  :: 4'h2  - 32bits
        (0x6    <<   8) |           // [11: 8] mem_type     : Type of Memory                            :: 4'h6  - ddr3
        (0x0    <<   6) |           // [ 7: 6] add_lat_pall : Additional Latency for PALL in cclk cycle :: 2'b00 - 0 cycle
        (0x0    <<   5) |           // [    5] dsref_en     : Dynamic Self Refresh                      :: 1'b0  - Disable
//        (0x0    <<   4) |           // [    4] Reserved     : SBZ
        (0x0    <<   2) |           // [ 3: 2] dpwrdn_type  : Type of Dynamic Power Down                :: 2'b00 - Active/precharge power down
        (0x0    <<   1) |           // [    1] dpwrdn_en    : Dynamic Power Down                        :: 1'b0  - Disable
        (0x0    <<   0));           // [    0] clk_stop_en  : Dynamic Clock Control                     :: 1'b0  - Always running

    if (isResume)
        temp |= (0x1    <<   0);

    WriteIO32( &pReg_Drex->MEMCONTROL,  temp );

    WriteIO32( &pReg_Drex->PHYCONTROL,
        (0x1    <<  31) |           // [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
        (0x1    <<  30) |           // [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
        (0x1    <<  29) |           // [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
        (0x0    <<  24) |           // [28:24] ctrl_pd. Input Gate for Power Down.
        (0x0    <<   8) |           // [    8] Termination Type for Memory Write ODT (0:single, 1:both chip ODT)
        (0x0    <<   7) |           // [    7] Resync Enable During PAUSE Handshaking
        (0x0    <<   4) |           // [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
        (0x0    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
        (0x0    <<   2) |           // [    2] Drive Memory DQ Bus Signals
        (0x0    <<   1) |           // [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1
        (0x0    <<   0));           // [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1

    temp = (U32)(
        (0x0    <<  28) |           // [   28] dfi_init_start
        (0xFFF  <<  16) |           // [27:16] timeout_level0
        (0x1    <<  12) |           // [14:12] rd_fetch
        (0x1    <<   8) |           // [    8] empty
        (0x0    <<   6) |           // [ 7: 6] io_pd_con
        (0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
        (0x0    <<   3) |           // [    3] update_mode - Update Interface in DFI.
        (0x0    <<   1) |           // [ 2: 1] clk_ratio
        (0x0    <<   0));           // [    0] ca_swap

    if (isResume)
        temp &= ~(0x1    <<   5);

    WriteIO32( &pReg_Drex->CONCONTROL,  temp );
    WriteIO32( &pReg_Drex->CGCONTROL,
        (0x0    <<   4) |           // [    4] phy_cg_en
        (0x0    <<   3) |           // [    3] memif_cg_en
        (0x0    <<   2) |           // [    2] scg_sg_en
        (0x0    <<   1) |           // [    1] busif_wr_cg_en
        (0x0    <<   0));           // [    0] busif_rd_cg_en

#if DDR_RW_CAL
    DDR3_RW_Delay_Calibration();
#endif

#else    // ddr simulation
{
    //1.3.4 DDR3 with PHY V6
    //1. Apply power. RESET# pin of memory needs to be maintained for minimum 200us with stable power.
    //   CKE is pulled 'Low' anytime before RESET# being de-asserted (min. time 10ns)

    //2. Set the PHY for DDR3 operation mode, RL/WL/BL register and proceed ZQ calibration. Refer to "INITIALIZATION" in PHY manual.
    // - After power-up and system PLL locking time, system reset(rst_n) is released.
    // - Select Memory Type (=PHY_CON0[12:11]).
    //   + ctrl_ddr_mode=2'b11 (LPDDR3)
    //   + ctrl_ddr_mode=2'b10 (LDDR2)
    //   + ctrl_ddr_mode=2'b00 (DDR2)
    //   + ctrl_ddr_mode=2'b01 (DDR3)
    WriteIO32( pDDRPHY->PHY_CON[0],
        (0x17   <<  24) |       // [28:24] T_WrWrCmd
        (0x1    <<  22) |       // [23:22] ctrl_upd_mode
        (0x0    <<  20) |       // [21:20] ctrl_upd_range
        (0x6    <<  17) |       // [19:17] T_WrRdCmd
        (0x0    <<  16) |       // [   16] wrlvl_mode
        (0x0    <<  14) |       // [   14] p0_cmd_en
        (0x0    <<  13) |       // [   13] byte_rdlvl_en
        (0x1    <<  11) |       // [12:11] ctrl_ddr_mode    0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
        (0x1    <<  10) |       // [   10] ctrl_wr_dis
        (0x1    <<   9) |       // [    9] ctrl_dfdqs
        (0x1    <<   8) |       // [    8] ctrl_shgate
        (0x0    <<   7) |       // [    7] Reserved (ctrl_ckdis removed?)
        (0x1    <<   6) |       // [    6] ctrl_atgate
        (0x0    <<   5) |       // [    5] Reserved (ctrl_read_dis removed?)
        (0x0    <<   4) |       // [    4] ctrl_cmosrcv
        (0x0    <<   3) |       // [    3] Reserved (ctrl_read_width removed?)
        (0x0    <<   0));       // [ 2: 0] ctrl_fnc_fb

    // NOTE: If ctrl_ddr_mode[1]=1'b1, cmd_active=14'h000E(=LP_DDR_CON3[13:0]), cmd_default=14'h000F(=LP_DDR_CON4[13:0]) upd_mode=1'b1(=OFFSETD_CON0[28])

    // - Set Read Latency(RL), Burst Length(BL) and Write Latency(WL)
    //   + Set RL in PHY_CON4[4:0].
    //   + Set BL in PHY_CON4[12:8].
    //   + Set WL in PHY_CON4[20:16].
    WriteIO32( pDDRPHY->PHY_CON[4],
        (0x8    <<  16) |       // [20:16] WL = CWL + AL
        (0x8    <<   8) |       // [12: 8] BL
        (0xB    <<   0));       // [ 4: 0] RL

    // - ZQ Calibration(Please refer to "8.5 ZQ I/O CONTROL PROCEDURE" for more details)
    //   + Enable and Disable "zq_clk_div_en" in ZQ_CON0[18]
    //   + Enable "zq_manual_str" in ZQ_CON0[1]
    //   + Wait until "zq_cal_done"(ZQ_CON1[0]) is enabled.
    //   + Disable "zq_manual_str" in ZQ_CON0[1]
    WriteIO32( &pDDRPHY->ZQ_CON,
        (0x1    <<  27) |       // [   27] zq_clk_en
        (0x7    <<  24) |       // [26:24] zq_mode_dds
        (0x0    <<  21) |       // [23:21] zq_mode_term
        (0x0    <<  20) |       // [   20] zq_rgddr
        (0x0    <<  19) |       // [   19] zq_mode_noterm
        (0x1    <<  18) |       // [   18] zq_clk_div_en    - Clock dividing enable
        (0x0    <<  15) |       // [17:15] zq_force_impn
        (0x7    <<  12) |       // [14:12] zq_force_impp
        (0x30   <<   4) |       // [11: 4] zq_udt_dly
        (0x1    <<   2) |       // [ 3: 2] zq_manual_mode
        (0x1    <<   1) |       // [    1] zq_manual_str    - Manual calibration start
        (0x0    <<   0));       // [    0] zq_auto_en

    while( (ReadIO32(&pDDRPHY->ZQ_STATUS) & 0x1) != 0x1 );       // [    0] zq_done    - ZQ Callbration is finished

    WriteIO32( &pDDRPHY->ZQ_CON,
        (0x1    <<  27) |       // [   27] zq_clk_en
        (0x7    <<  24) |       // [26:24] zq_mode_dds
        (0x0    <<  21) |       // [23:21] zq_mode_term
        (0x0    <<  20) |       // [   20] zq_rgddr3
        (0x0    <<  19) |       // [   19] zq_mode_noterm
        (0x0    <<  18) |       // [   18] zq_clk_div_en    - Clock dividing enable
        (0x0    <<  15) |       // [17:15] zq_force_impn
        (0x7    <<  12) |       // [14:12] zq_force_impp
        (0x30   <<   4) |       // [11: 4] zq_udt_dly
        (0x1    <<   2) |       // [ 3: 2] zq_manual_mode
        (0x0    <<   1) |       // [    1] zq_manual_str    - Manual calibration start
        (0x0    <<   0));       // [    0] zq_auto_en

    //3. Assert the ConControl.dfi_init_start field to high but leave as default value for other fields.(aref_en and io_pd_con should be off.)
    //   Clock gating in CGControl should be disabled in initialization and training se-quence.
    SetIO32( &pReg_Drex->CONCONTROL,    (0x1    << 28));    // [   28] dfi_init_start    - DFI PHY initialization strat

    //4. Wait for the PhyStatus0.dfi_init_complete field to change to '1'.
    while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & 0x8 ) != 0x8 );    // [    3] dfi_init_complete    - DFI PHY initialization complete

    //5. Deassert the ConControl.dfi_init_start field to low.
    ClearIO32( &pReg_Drex->CONCONTROL,    (0x1    << 28));    // [   28] dfi_init_start    - DFI PHY initialization strat

    //6. Set the PHY for dqs pulldown mode. (Refer to PHY manual)
    // + Enable DQS pull down mode
    //   - Set "ctrl_pulld_dqs=9'h1FF" (=LP_CON0[8:0]) in case of using 72bit PHY.
    //   - Please be careful that DQS pull down can be disabled only after Gate Leveling is done.
    WriteIO32( &pDDRPHY->LP_CON,
        (0x0    <<  16) |       // [24:16] ctrl_pulld_dq
        (0xF    <<   0));       // [ 8: 0] ctrl_pulld_dqs    - 0: pull-up for PDQS/NDQS signal 1: pull-down for PDQS/NDQS signal

    // + Memory Controller should assert "dfi_ctrlupd_req" after "dfi_init_complete" is set.
    //   - Please keep "Ctrl-Initiated Update" mode until finishing Leveling and Training.

    //7. Set the PhyControl0.fp_resync bit-field to '1' to update DLL information.
    SetIO32  ( &pReg_Drex->PHYCONTROL,    (0x1    <<  3));    // [    3] fp_resync    - Froce DLL Resyncronization

    //8. Set the PhyControl0.fp_resync bit-field to '0'.
    ClearIO32( &pReg_Drex->PHYCONTROL,    (0x1    <<  3));    // [    3] fp_resync    - Froce DLL Resyncronization

    //9. Set the MemBaseConfig0 register and MemBaseConfig1 register if needed.
    WriteIO32( &pReg_DrexTZ->MEMBASECONFIG[0],
        (0x40   <<  16) |       // [26:16] chip_base    - AXI Base Address        0x80000000
        (0x7C0  <<   0));       // [10: 0] chip_mask    - AXI Base Address Mask    1GB
//    WriteIO32( &pReg_DrexTZ->MEMBASECONFIG[0],
//        (0x40   <<  16) |       // [26:16] chip_base    - AXI Base Address        0x40000000
//        (0x780  <<   0));       // [10: 0] chip_mask    - AXI Base Address Mask    2GB

    WriteIO32( &pReg_DrexTZ->MEMCONFIG[0],
        (0x0    <<  20) |       // [22:20] bank_lsb
        (0x0    <<  19) |       // [   19] rank_inter_en
        (0x0    <<  18) |       // [   18] bit_sel_en
        (0x2    <<  16) |       // [17:16] bit_sel
        (0x2    <<  12) |       // [15:12] chip_map     - 2 : Split Column Interleaved
        (0x3    <<   8) |       // [11: 8] chip_col     - 3 : 10 bits
        (0x3    <<   4) |       // [ 7: 4] chip_row     - 3 : 15 bits
        (0x3    <<   0));       // [ 3: 0] chip_bank    - 3 : 8  Banks


    //10. Set the PrechConfig and PwrdnConfig registers.
//    WriteIO32( &pReg_Drex->PRECHCONFIG0,    0x00000000 );
//    WriteIO32( &pReg_Drex->PRECHCONFIG1,    0xFFFFFFFF );
//    WriteIO32( &pReg_Drex->PWRDNCONFIG ,    0xFFFF00FF );

    //11. Set the TimingAref, TimingRow, TimingData and TimingPower registers according to memory AC parameters.
    WriteIO32( &pReg_Drex->TIMINGAREF,
        (0x98   <<  16) |       // [31:16] t_refpb  - 0.4875 us (rclk)
        (0x618  <<   0));       // [15: 0] t_refi   - 7.8    us (rclk)

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGROW,
        (0x68   <<  24) |       // [31:24] tRFC     - 260   ns  (cclk)
        (0x4    <<  20) |       // [23:20] tRRD     - 7.5   ns  (cclk)
        (0x6    <<  16) |       // [19:16] tRP      - 13.75 ns  (cclk)
        (0x6    <<  12) |       // [15:12] tRCD     - 13.75 ns  (cclk)
        (0x14   <<   6) |       // [11: 6] tRC      - 48.75 ns  (cclk)
        (0xF    <<   0));       // [ 5: 0] tRAS     - 35    ns  (cclk)

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGDATA,
        (0x4    <<  28) |       // [31:28] tWTR     - 7.5 ns
        (0x6    <<  24) |       // [27:24] tWR      - 15  ns
        (0x4    <<  20) |       // [23:20] tRTP     - 7.5 ns
        (0x0    <<  17) |       // [   17] tPPD     - for LPDDR3
        (0x0    <<  14) |       // [   14] t_w2w_c2c -
        (0x0    <<  12) |       // [   12] t_r2r_c2c -
        (0x8    <<   8) |       // [11: 8] WL       - AL + CWL
        (0x0    <<   4) |       // [ 7: 4] dqsck    - DDR3 : 0
        (0xB    <<   0));       // [ 3: 0] RL       - AL + CL

    WriteIO32( &pReg_Drex->ACTIMING0.TIMINGPOWER,
        (0x10   <<  26) |       // [31:26] tFAW     - 40 ns
        (0x20   <<  16) |       // [25:16] tXSR     @sei ?? 512 tCK
        (0x0A   <<   8) |       // [15: 8] tXP      @sei ?? 3 tCK
        (0x2    <<   4) |       // [ 7: 4] tCKE     -  5 ns
        (0x4    <<   0));       // [ 3: 0] tMRD     -  4 tCK

    //12. If QoS scheme is required, set the QosControl0~15 and QosConfig0~15 registers.
    //--------------------------------------
    // @modified by choiyk 2014/02/12
    // for display stresstest. QOS15 is timingout 0
    // and set display's qos to 15 (0xF)
//    WriteIO32( DREX_QOSCONTROL15, 0 );
    //--------------------------------------


    //13. Confirm that after RESET# is de-asserted, 500 us have passed before CKE becomes active.
    //14. Confirm that clocks(CK, CK#) need to be started and stabilized for at least 10 ns or 5 tCK (which is larger) before CKE goes active.

    //15. Issue a NOP command using the DirectCmd register to assert and to hold CKE to a logic high level.
    SendDirectCommand(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);

    //16. Wait for tXPR(max(5nCK,tRFC(min)+10ns)) or set tXP to tXPR value before step 16.
    //    If the system set tXP to tXPR, then the system must set tXP to proper value before normal memory operation.
    DMC_Delay(100);

    //17. Issue an EMRS2 command using the DirectCmd register to program the operating parameters. Dynamic ODT should be disabled. A10 and A9 should be low.
    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2,
        (0x2    <<   9) |       // Dynamic ODT(Rtt)[10:9]. 0:Disable, 1:RZQ/4(60ohm), 2:RZQ/2(120ohm), 3:Reserved
        (0x0    <<   8) |       // Reserved[8] SBZ
        (0x0    <<   7) |       // Self Refresh Temperature[7]. 0:Normal(0~85), 1:Extended(0~95)
        (0x0    <<   6) |       // Auto Self Refresh[6]. 0:Disabled, 1:Enabled
        (0x3    <<   3) |       // CAS Write Latency(CWL).
                                // 0:5ck(tCK>2.5ns), 1:6ck(2.5ns>tCK>1.875ns), 2:7ck(1.875ns>tCK>2.5ns), 3:8ck(1.5ns>tCK>1.25ns,
                                // 4:9ck(1.25ns>tCK>1.07ns), 5:10ck(1.07ns>tCK>0.935ns), 6:11ck(0.935ns>tCK>0.833ns), 7:12ck(0.833ns>tCK>0.75ns)
        (0x0    <<   0));       // Partial Array Self-Refresh(Option)

    //18. Issue an EMRS3 command using the DirectCmd register to program the operating parameters.
    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3,
        (0x0    <<   3) |       // Reserved SBZ[14:3].
        (0x0    <<   2) |       // MPR enable. 0:Normal DRAM operation, 1:Dataflow from MPR
        (0x0    <<   0));       // MPR Read function. 0:Predefined pattern. 1~3: Reserved

    //19. Issue an EMRS command using the DirectCmd register to enable the memory DLL.
    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1,
        (0x0    <<  12) |       // Qoff[12]. 0:Enable, 1:Disable
        (0x0    <<  11) |       // TDQS[11]. 0:Disable(x4, x16), 1:Enable(x8 only)
        (0x0    <<  10) |       // Reserved SBZ[10]
        (0x0    <<   9) |       // Rtt(ODT)[9,6,2]. 000:Disable, 001:RZQ/4, 010:RZQ/2
        (0x1    <<   6) |       // Rtt(ODT)[9,6,2]. 011:RZQ/6, 100:RZQ/12, 101:RZQ/8
        (0x0    <<   2) |       // Rtt(ODT)[9,6,2]. 110, 111:Reserved
        (0x0    <<   8) |       // Reserved SBZ[8]
        (0x0    <<   7) |       // Write Leveling[7]. 0:Disable, 1:Enable
        (0x0    <<   5) |       // Output Drive Strength[5,1]. 00:RZQ/6, 01:RZQ/7
        (0x1    <<   1) |       // Output Drive Strength[5,1]. 10, 11 : Reserved
        (0x0    <<   3) |       // Additive Latency(AL)[4:3]. 0:AL=0, 1:AL=CL-1, 2:AL=CL-2, 3:Reserved
        (0x0    <<   0));       // DLL[0]. 0:Enable, 1:Disable

    //20. Issue a MRS command using the DirectCmd register to reset the memory DLL.
    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR0,
        (0x0    <<  12) |       // Precharge PD. 0:DLL off(slow exit), 1:DLL on(fast exit)
        (0x6    <<   9) |       // Write Recovery(WR)[11:9]. 0:Reserved, 1:5, 2:6, 3:7, 4:8, 5:10, 6:12, 7:14 (### 12clk test)
        (0x1    <<   8) |       // DLL Reset[8]. 0:No, 1:Reset
        (0x0    <<   7) |       // Reserved[7] SBZ
        (0x1    <<   6) |       // CAS Latency[6:4],[2]. 0010:5, 0100:6, 0110:7
        (0x1    <<   5) |       // CAS Latency[6:4],[2]. 1000:8, 1010:9, 1100:10
        (0x1    <<   4) |       // CAS Latency[6:4],[2]. 1110:11, 0001:12, 0011:13
        (0x0    <<   2) |       // CAS Latency[6:4],[2]. others:Reserved
        (0x1    <<   3) |       // Read Burst Type[3]. 0:Sequential, 1:Interleave
        (0x0    <<   0));       // Burst Length[1:0]. 0:Fixed 8, 1:4or8, 2:Fixed4, 3:Reserved
                                                                                     // Precharge P/D_DLL Off, WR : 5, DLL Reset Off, CL : 11, Read Burst Type : Interleave, Burst Length : 8

    //21. Issues a MRS command using the DirectCmd register to program the operating parameters without resetting the memory DLL.
    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR0,
        (0x0    <<  12) |       // Precharge PD. 0:DLL off(slow exit), 1:DLL on(fast exit)
        (0x6    <<   9) |       // Write Recovery(WR)[11:9]. 0:Reserved, 1:5, 2:6, 3:7, 4:8, 5:10, 6:12, 7:14 (### 12clk test)
        (0x0    <<   8) |       // DLL Reset[8]. 0:No, 1:Reset
        (0x0    <<   7) |       // Reserved[7] SBZ
        (0x1    <<   6) |       // CAS Latency[6:4],[2]. 0010:5, 0100:6, 0110:7
        (0x1    <<   5) |       // CAS Latency[6:4],[2]. 1000:8, 1010:9, 1100:10
        (0x1    <<   4) |       // CAS Latency[6:4],[2]. 1110:11, 0001:12, 0011:13
        (0x0    <<   2) |       // CAS Latency[6:4],[2]. others:Reserved
        (0x1    <<   3) |       // Read Burst Type[3]. 0:Sequential, 1:Interleave
        (0x0    <<   0));       // Burst Length[1:0]. 0:Fixed 8, 1:4or8, 2:Fixed4, 3:Reserved
                                                                // Precharge P/D_DLL Off, WR : 5, DLL Reset Off, CL : 11, Read Burst Type : Interleave, Burst Length : 8

    //22. Issues a ZQINIT commands using the DirectCmd register.
    SendDirectCommand(SDRAM_CMD_ZQINIT, 0, (SDRAM_MODE_REG)CNULL, CNULL);

    //23. If there are more external memory chips, perform steps 17 ~ 24 procedures for other memory device.
    //24. If any leveling/training is needed, enable ctrl_atgate, p0_cmd_en, InitDeskewEn and byte_rdlvl_en. Disable ctrl_dll_on and set ctrl_force value. (Refer to PHY manual)
    //25. If write leveling is not needed, skip this procedure. If write leveling is needed, set DDR3 into write leveling mode using MRS direct command, set ODT pin high and tWLO using WRLVL_CONFIG0 regis-ter(offset=0x120) and set the related PHY SFR fields through PHY APB I/F(Refer to PHY manual). To gener-ate 1 cycle pulse of dfi_wrdata_en_p0, write 0x1 to WRLVL_CONFIG1 register(Offset addr=0x124). To read the value of memory data, use CTRL_IO_RDATA(offset = 0x150). If write leveling is finished, disable write leveling mode in PHY register and set ODT pin low and disable write leveling mode of DDR3.
    //26. If gate leveling is not needed, skip 27 ~ 28. If gate leveling is needed, set DDR3 into MPR mode using MRS direct command and set the related PHY SFR fields through PHY APB I/F. Do the gate leveling. (Refer to PHY manual)
    //27. If gate leveling is finished, set DDR3 into normal operation mode using MRS command and disable DQS pull-down mode.(Refer to PHY manual)
    //28. If read leveling is not needed skip 29 ~ 30. If read leveling is needed, set DDR3 into MPR mode using MRS direct command and set proper value to PHY control register. Do the read leveling..(Refer to PHY manual)
    //29. If read leveling is finished, set DDR3 into normal operation mode using MRS direct command.
    //30. If write training is not needed, skip 31. If write training is nedded, set the related PHY SFR fields through PHY APB I/F..(Refer to PHY manual). To issue ACT command, enable and disable WrtraCon-fig.write_training_en . Refer to this register definition for row and bank address. Do write training. (Refer to PHY manual)
    //31. After all leveling/training are completed, enable ctrl_dll_on. (Refer to PHY manual)

    //32. Set the PhyControl0.fp_resync bit-field to '1' to update DLL information.
    SetIO32  ( &pReg_Drex->PHYCONTROL,    (0x1    <<  3));        // [    3] fp_resync - Froce DLL Resyncronization

    //33. Set the PhyControl0.fp_resync bit-field to '0'.
    ClearIO32( &pReg_Drex->PHYCONTROL,    (0x1    <<  3));        // [    3] fp_resync - Froce DLL Resyncronization

    //34. Disable PHY gating control through PHY APB I/F if necessary(ctrl_atgate, refer to PHY manual).
    WriteIO32( &pDDRPHY->PHY_CON[0],
        (0x17   <<  24) |       // [28:24] T_WrWrCmd
        (0x1    <<  22) |       // [23:22] ctrl_upd_mode
        (0x0    <<  20) |       // [21:20] ctrl_upd_range
        (0x6    <<  17) |       // [19:17] T_WrRdCmd
        (0x0    <<  16) |       // [   16] wrlvl_mode
        (0x0    <<  14) |       // [   14] p0_cmd_en
        (0x0    <<  13) |       // [   13] byte_rdlvl_en
        (0x1    <<  11) |       // [12:11] ctrl_ddr_mode
        (0x1    <<  10) |       // [   10] ctrl_wr_dis
        (0x1    <<   9) |       // [    9] ctrl_dfdqs
//        (0x1    <<   8) |       // [    8] ctrl_shgate
        (0x0    <<   8) |       // [    8] ctrl_shgate
        (0x0    <<   7) |       // [    7] Reserved (ctrl_ckdis removed?)
        (0x1    <<   6) |       // [    6] ctrl_atgate 0 : Controller generate ctrl_gate_p*, ctrl_read_p*, 1: PHY generate ctrl_gate_p* ctrl_read_p*
        (0x0    <<   5) |       // [    5] Reserved (ctrl_read_dis removed?)
        (0x0    <<   4) |       // [    4] ctrl_cmosrcv
        (0x0    <<   3) |       // [    3] Reserved (ctrl_read_width removed?)
        (0x0    <<   0));       // [ 2: 0] ctrl_fnc_fb


    //35. Issue PALL to all chips using direct command. This is an important step if write training has been done.
    SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);

    //36. Set the MemControl and PhyControl0 register.
    WriteIO32( &pReg_Drex->MEMCONTROL,
        (0x0    <<  29) |       // [   29] pause_ref_en
        (0x0    <<  28) |       // [   28] sp_en
        (0x0    <<  27) |       // [   27] pb_ref_en
        (0x0    <<  24) |       // [   24] pzq_en
        (0x3    <<  20) |       // [22:20] bl    - Memory burst length  1: 2, 2: 4, 3: 8;
        (0x0    <<  16) |       // [19:16] num_chip
        (0x2    <<  12) |       // [15:12] mem_width    - Width of Memory data bus 2: 32bit, 4:128bit
        (0x6    <<   8) |       // [11: 8] mem_type    - Tytpe of Memory 5:LPDDR2, 6:DDR3, 7:LPDDR3
        (0x0    <<   6) |       // [ 7: 6] add_lat_pall
        (0x0    <<   5) |       // [    5] dsref_en
        (0x0    <<   2) |       // [ 3: 2] dpwrdn_type
        (0x0    <<   1) |       // [    1] dpwrdn_en
        (0x0    <<   0));       // [    0] clk_stop_en

    WriteIO32( &pReg_Drex->PHYCONTROL,
        (0x1    <<  31) |       // [   31] mem_term_en - termination Enable Memory Wirte ODT 0: Disable, 1: Enable // ??
        (0x1    <<  30) |       // [   30] phy_term_en - Termination Eanble for PHY // ??
   //     (0x1    <<  29) |       // [   29] ctrl_shgate - Duration of DQS Gating Signal
        (0x0    <<  29) |       // [   29] ctrl_shgate - Duration of DQS Gating Signal
        (0x0    <<  24) |       // [28:24] ctrl_pd
        (0x0    <<   8) |       // [    8] mem_term_type
        (0x0    <<   7) |       // [    7] pause_resync_en
        (0x0    <<   4) |       // [ 6: 4] dqs_delay
        (0x0    <<   3) |       // [    3] fp_resync
        (0x0    <<   2) |       // [    2] drv_bus_en
        (0x0    <<   1) |       // [    1] sl_dll_dyn_con
        (0x0    <<   0));       // [    0] mem_term_chips

       //37. Set the ConControl register. aref_en should be turn on.
    WriteIO32( &pReg_Drex->CONCONTROL,
        (0x0    <<  28) |       // [   28] dfi_init_start
        (0xFFF  <<  16) |       // [27:16] timeout_level0
        (0x1    <<  12) |       // [14:12] rd_fetch
        (0x1    <<   8) |       // [    8] empty
        (0x0    <<   6) |       // [ 7: 6] io_pd_con
        (0x1    <<   5) |       // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
        (0x0    <<   3) |       // [    3] update_mode - Update Interface in DFI.
        (0x0    <<   1) |       // [ 2: 1] clk_ratio
        (0x0    <<   0));       // [    0] ca_swap

       //38. Set the CGControl register for clock gating enable.
    WriteIO32( &pReg_Drex->CGCONTROL,
        (0x0    <<   4) |       // [    4] phy_cg_en
        (0x0    <<   3) |       // [    3] memif_cg_en
        (0x0    <<   2) |       // [    2] scg_sg_en
        (0x0    <<   1) |       // [    1] busif_wr_cg_en
        (0x0    <<   0));       // [    0] busif_rd_cg_en
}
#endif    // ddr simulation
    printf("\r\n\r\n");
}
