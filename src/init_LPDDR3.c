#include "sysHeader.h"

#include <nx_drex.h>
#include <nx_ddrphy.h>
#include "nx_reg_base.h"
#include "ddr3_sdram.h"

#define DDR_NEW_LEVELING_TRAINING       (1)

#if defined(CHIPID_NXP4330)
#define DDR_WRITE_LEVELING_EN           (0)
#define DDR_CA_SWAP_MODE                (0)
#define DDR_CA_CALIB_EN                 (1)     // for LPDDR3
#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_LEVELING_CALIB_EN     (0)     // for Fly-by
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_GATE_LVL_COMPENSATION_EN    (0)     // Do not use. for Test.
#define DDR_READ_DQ_COMPENSATION_EN     (1)
#define DDR_WRITE_DQ_COMPENSATION_EN    (1)


#define DDR_RESET_GATE_LVL              (1)
#define DDR_RESET_READ_DQ               (1)
#define DDR_RESET_WRITE_DQ              (1)

#define DDR_RESET_QOS1                  (1)     // Release version is '1'
#endif  // #if defined(CHIPID_NXP4330)

#if defined(CHIPID_S5P4418)
#define DDR_WRITE_LEVELING_EN           (0)
#define DDR_CA_SWAP_MODE                (0)
#define DDR_CA_CALIB_EN                 (1)     // for LPDDR3
#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_LEVELING_CALIB_EN     (0)     // for Fly-by
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_GATE_LVL_COMPENSATION_EN    (0)     // Do not use. for Test.
#define DDR_READ_DQ_COMPENSATION_EN     (1)
#define DDR_WRITE_DQ_COMPENSATION_EN    (1)


#define DDR_RESET_GATE_LVL              (1)
#define DDR_RESET_READ_DQ               (1)
#define DDR_RESET_WRITE_DQ              (1)

#define DDR_RESET_QOS1                  (1)     // Release version is '1'
#endif  // #if defined(CHIPID_S5P4418)


#define CFG_DDR_LOW_FREQ                (1)
#define CFG_ODT_ENB                     (0)


#if 1   //(CFG_NSIH_EN == 0)
#include "LPDDR3_K4E6E304EB_EGCE.h"
#endif

#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");


extern void setMemPLL(int);

U32 g_DDRLock;
U32 g_GateCycle;
U32 g_GateCode;
U32 g_RDvwmc;
U32 g_WRvwmc;
U32 g_CAvwmc;


inline void DMC_Delay(int milisecond)
{
    register volatile  int    count;

    for (count = 0; count < milisecond; count++)
    {
        nop();
    }
}

inline void SendDirectCommand(SDRAM_CMD cmd, U8 chipnum, SDRAM_MODE_REG mrx, U16 value)
{
#if defined(MEM_TYPE_DDR3)
    WriteIO32( &pReg_Drex->DIRECTCMD, (U32)(cmd<<24 | chipnum<<20 | mrx<<16 | value) );
#endif
#if defined(MEM_TYPE_LPDDR23)
    WriteIO32( &pReg_Drex->DIRECTCMD, (U32)( (cmd<<24) | ((chipnum & 1)<<20) | ((mrx>>6) & 0x3) | (((mrx>>3) & 0x7)<<16) | ((mrx & 0x7)<<10) | ((value & 0xFF)<<2) ));
#endif
}

#if (CONFIG_SUSPEND_RESUME == 1)
void enterSelfRefresh(void)
{
//    union SDRAM_MR MR;
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


#if 0
    // Send MR16 PASR_Bank command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 16, 0);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 16, 0);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 16, 0);
#endif

    // Send MR17 PASR_Seg command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 17, 0);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 17, 0);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 17, 0);
#endif
#endif



#if 1
    do
    {
        nTemp = ( ReadIO32(&pReg_Drex->CHIPSTATUS) & nChips );
    } while( nTemp );

    do
    {
        nTemp = ( (ReadIO32(&pReg_Drex->CHIPSTATUS) >> 8) & nChips );
    } while( nTemp != nChips );
#else

    // for self-refresh check routine.
    while( 1 )
    {
        nTemp = ReadIO32(&pReg_Drex->CHIPSTATUS);
        if (nTemp)
            MEMMSG("ChipStatus = 0x%04x\r\n", nTemp);
    }
#endif


    // Step 52 Auto refresh counter disable
    ClearIO32( &pReg_Drex->CONCONTROL,  (0x1    <<   5) );          // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1

    // Step 10  ACK, ACKB off
    SetIO32( &pReg_Drex->MEMCONTROL,    (0x1    <<   0) );          // clk_stop_en[0] : Dynamic Clock Control   :: 1'b0  - Always running

    DMC_Delay(1000 * 3);
}

void exitSelfRefresh(void)
{
//    union SDRAM_MR MR;

    // Step 10    ACK, ACKB on
    ClearIO32( &pReg_Drex->MEMCONTROL,  (0x1    <<   0) );          // clk_stop_en[0] : Dynamic Clock Control   :: 1'b0  - Always running
    DMC_Delay(10);

    // Step 52 Auto refresh counter enable
    SetIO32( &pReg_Drex->CONCONTROL,    (0x1    <<   5) );          // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1
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


#if 0
    MR.Reg          = 0;
    MR.MR1.DLL      = 0;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
    MR.MR1.AL       = MR1_nAL;
#else
    MR.MR1.AL       = pSBI->DII.MR1_AL;
#endif
    MR.MR1.ODS1     = pSBI->DDR3_DSInfo.MR1_ODS & (1 << 1);
    MR.MR1.ODS0     = pSBI->DDR3_DSInfo.MR1_ODS & (1 << 0);
    MR.MR1.RTT_Nom2 = pSBI->DDR3_DSInfo.MR1_RTT_Nom & (1 << 2);
    MR.MR1.RTT_Nom1 = pSBI->DDR3_DSInfo.MR1_RTT_Nom & (1 << 1);
    MR.MR1.RTT_Nom0 = pSBI->DDR3_DSInfo.MR1_RTT_Nom & (1 << 0);
    MR.MR1.QOff     = 0;
    MR.MR1.WL       = 0;
#if 0
#if (CFG_NSIH_EN == 0)
    MR.MR1.TDQS     = (_DDR_BUS_WIDTH>>3) & 1;
#else
    MR.MR1.TDQS     = (pSBI->DII.BusWidth>>3) & 1;
#endif
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
    MR.MR2.RTT_WR   = pSBI->DDR3_DSInfo.MR2_RTT_WR;
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
#endif



#if 0
    while( ReadIO32(&pReg_Drex->CHIPSTATUS) & (0xF << 8) )
    {
        nop();
    }
#endif

    DMC_Delay(1000 * 2);
}
#endif // #if (CONFIG_SUSPEND_RESUME == 1)

#if (DDR_NEW_LEVELING_TRAINING == 1)
#if (DDR_WRITE_LEVELING_EN == 1)
void DDR_Write_Leveling(void)
{
#if 0
    MEMMSG("\r\n########## Write Leveling ##########\r\n");

#else
#if defined(MEM_TYPE_DDR3)
    union SDRAM_MR MR1;
#endif
    U32 temp;

    MEMMSG("\r\n########## Write Leveling - Start ##########\r\n");

    SetIO32( &pReg_DDRPHY->PHY_CON[26+1],       (0x3    <<   7) );          // cmd_default, ODT[8:7]=0x3
    SetIO32( &pReg_DDRPHY->PHY_CON[0],          (0x1    <<  16) );          // wrlvl_mode[16]=1

#if defined(MEM_TYPE_DDR3)
    /* Set MPR mode enable */
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
    MR1.MR1.WL      = 1;
#if (CFG_NSIH_EN == 0)
    MR1.MR1.TDQS    = (_DDR_BUS_WIDTH>>3) & 1;
#else
    MR1.MR1.TDQS    = (pSBI->DII.BusWidth>>3) & 1;
#endif

    SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR1.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif
#endif

#if 0
    // Send NOP command.
    SendDirectCommand(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#endif

    temp = ( (0x8 << 24) | (0x8 << 17) | (0x8 << 8) | (0x8 << 0)  );        // PHY_CON30[30:24] = ctrl_wrlvl_code3, PHY_CON30[23:17] = ctrl_wrlvl_code2, PHY_CON30[14:8] = ctrl_wrlvl_code1, PHY_CON30[6:0] = ctrl_wrlvl_code0
    WriteIO32( &pReg_DDRPHY->PHY_CON[30+1],     temp );
    MEMMSG("ctrl_wrlvl_code = 0x%08X\r\n", temp);

    // SDLL update.
    SetIO32  ( &pReg_DDRPHY->PHY_CON[30+1],     (0x1    <<  16) );          // wrlvl_enable[16]=1, ctrl_wrlvl_resync
    ClearIO32( &pReg_DDRPHY->PHY_CON[30+1],     (0x1    <<  16) );          // wrlvl_enable[16]=0, ctrl_wrlvl_resync

    temp = ReadIO32( &pReg_DDRPHY->PHY_CON[30+1] );                         // PHY_CON30[30:24] = ctrl_wrlvl_code3, PHY_CON30[23:17] = ctrl_wrlvl_code2, PHY_CON30[14:8] = ctrl_wrlvl_code1, PHY_CON30[6:0] = ctrl_wrlvl_code0
    MEMMSG("ctrl_wrlvl_code = 0x%08X\r\n", temp);

    ClearIO32( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<  16) );          // wrlvl_mode[16]=0
    ClearIO32( &pReg_DDRPHY->PHY_CON[26+1],     (0x3    <<   7) );          // cmd_default, ODT[8:7]=0x0

    MEMMSG("\r\n########## Write Leveling - End ##########\r\n");
#endif
}
#endif

#if (DDR_CA_CALIB_EN == 1)
//for LPDDR3
//*** Response for Issuing MR41 - CA Calibration Enter1
    //- CA Data Pattern transfered at Rising Edge   : CA[9:0]=0x3FF     => CA[8:5],CA[3:0] of Data Pattern transfered through MR41 is returned to DQ (CA[3:0]={DQ[6],DQ[4],DQ[2],DQ[0]}=0xF, CA[8:5]={DQ[14],DQ[12],DQ[10],DQ[8]}=0xF)
    //- CA Data Pattern transfered at Falling Edge  : CA[9:0]=0x000     => CA[8:5],CA[3:0] of Data Pattern transfered through MR41 is returned to DQ (CA[3:0]={DQ[7],DQ[5],DQ[3],DQ[1]}=0x0, CA[8:5]={DQ[15],DQ[13],DQ[11],DQ[9]}=0x0)
    //- So response(ctrl_io_rdata) from MR41 is "0x5555".
//*** Response for Issuing MR48 - CA Calibration Enter2
    //- CA Data Pattern transfered at Rising Edge   : CA[9], CA[4]=0x3  => CA[9],CA[4] of Data Pattern transfered through MR48 is returned to DQ (CA[9]=DQ[8]=0x1, CA[4]=DQ[0]}=0x1)
    //- CA Data Pattern transfered at Falling Edge  : CA[9], CA[4]=0x0  => CA[9],CA[4] of Data Pattern transfered through MR48 is returned to DQ (CA[9]=DQ[9]=0x0, CA[4]=DQ[1]}=0x0)
    //- So response(ctrl_io_rdata) from MR48 is "0x0101".
#define RESP_MR41   0x5555
#define RESP_MR48   0x0101

#define MASK_MR41   0xFFFF
#define MASK_MR48   0x0303

CBOOL DDR_CA_Calibration(void)
{
    CBOOL   ret = CFALSE;
    U32     lock_div4 = (g_DDRLock >> 2);
    U32     offsetd;
    U32     vwml, vwmr, vwmc;
    U32     temp, mr41, mr48;
    int     find_vmw;
    int     code;


    code = 0x8;                                                     // CMD SDLL Code default value "ctrl_offsetd"=0x8
    find_vmw = 0;
    vwml = vwmr = vwmc = 0;


    MEMMSG("\r\n########## CA Calibration - Start ##########\r\n");

#if (DDR_CA_SWAP_MODE == 1)
    SetIO32  ( &pReg_Tieoff->TIEOFFREG[3],      (0x1    <<  26) );  // drex_ca_swap[26]=1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[24+1],     (0x1    <<   0) );  // ca_swap_mode[0]=1
#endif

    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<  16) );  // ctrl_wrlvl_en(wrlvl_mode)[16]="1" (Enable)
    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    <<  23) );  // rdlvl_ca_en(ca_cal_mode)[23]="1" (Enable)

    SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif


    while (1)
    {
        WriteIO32( &pReg_DDRPHY->PHY_CON[10],   code );

        SetIO32  ( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );          // ctrl_resync[24]=1
        ClearIO32( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );          // ctrl_resync[24]=0
        DMC_Delay(0x80);

        SendDirectCommand(SDRAM_CMD_MRS, 0, 41, 0xA4 );         //- CH0 : Send MR41 to start CA calibration for LPDDR3 : MA=0x29 OP=0xA4, 0x50690
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 41, 0xA4);          //- CH1 : Send MR41 to start CA calibration for LPDDR3 : MA=0x29 OP=0xA4, 0x50690
#else
        if (pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, 41, 0xA4);      //- CH1 : Send MR41 to start CA calibration for LPDDR3 : MA=0x29 OP=0xA4, 0x50690
#endif
#endif

        SetIO32  ( &pReg_Drex->CACAL_CONFIG[0], (0x1    <<   0) );  // deassert_cke[0]=1 : CKE pin is "LOW"
        temp = ((0x3FF  <<  12) |                               // dfi_address_p0[24]=0x3FF
                (tADR   <<   4) |
                (0x1    <<   0) );                              // deassert_cke[0]=1 : CKE pin is "LOW"
        WriteIO32( &pReg_Drex->CACAL_CONFIG[0], temp );

        WriteIO32( &pReg_Drex->CACAL_CONFIG[1], 0x00000001 );       // cacal_csn(dfi_csn_p0)[0]=1 : generate one pulse CSn(Low and High), cacal_csn field need not to return to "0" and whenever this field is written in "1", one pulse is genrerated.
//        DMC_Delay(0x80);

        mr41 = ReadIO32( &pReg_Drex->CTRL_IO_RDATA ) & MASK_MR41;

        ClearIO32( &pReg_Drex->CACAL_CONFIG[0], (0x1    <<   0) );  // deassert_cke[0]=0 : CKE pin is "HIGH" - Normal operation
        DMC_Delay(0x80);



        SendDirectCommand(SDRAM_CMD_MRS, 0, 48, 0xC0);              // CH0 : Send MR48 to start CA calibration for LPDDR3 : MA=0x30 OP=0xC0, 0x60300
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 48, 0xC0);              // CH1 : Send MR48 to start CA calibration for LPDDR3 : MA=0x30 OP=0xC0, 0x60300
#else
        if (pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, 48, 0xC0);          // CH1 : Send MR48 to start CA calibration for LPDDR3 : MA=0x30 OP=0xC0, 0x60300
#endif
#endif

        SetIO32  ( &pReg_Drex->CACAL_CONFIG[0], (0x1    <<   0) );  // deassert_cke[0]=1 : CKE pin is "LOW"
        WriteIO32( &pReg_Drex->CACAL_CONFIG[1], 0x00000001 );       // cacal_csn(dfi_csn_p0)[0]=1 : generate one pulse CSn(Low and High), cacal_csn field need not to return to "0" and whenever this field is written in "1", one pulse is genrerated.

        mr48 = ReadIO32( &pReg_Drex->CTRL_IO_RDATA ) & MASK_MR48;

        ClearIO32( &pReg_Drex->CACAL_CONFIG[0], (0x1    <<   0) );  // deassert_cke[0]=0 : CKE pin is "HIGH" - Normal operation


        if (find_vmw < 0x3)
        {
            if ( (mr41 == RESP_MR41) && (mr48 == RESP_MR48) )
            {
                find_vmw++;
                if(find_vmw == 0x1)
                {
                    vwml = code;
                }
//                printf("+ %d\r\n", code);
            }
            else
            {
                find_vmw = 0x0;                                 //- 첫 번째 PASS로부터 연속 3회 PASS 하지 못하면 연속 3회 PASS가 발생할 때까지 Searching 다시 시작하도록 "find_vmw" = "0"으로 초기화.
//                printf("- %d\r\n", code);
            }
        }
        else if( (mr41 != RESP_MR41) || (mr48 != RESP_MR48) )
        {
            find_vmw = 0x4;
            vwmr = code - 1;
//            printf("-- %d\r\n", code);
            printf("mr41 = 0x%08X, mr48 = 0x%08X\r\n", mr41, mr48);
            break;
        }

        code++;

        if (code == 256)
        {
            MEMMSG("[Error] CA Calibration : code %d\r\n", code);

            goto ca_error_ret;
        }

    }

    lock_div4 = (g_DDRLock >> 2);

    vwmc = (vwml + vwmr) >> 1;
    code = (int)(vwmc - lock_div4);     //- (g_DDRLock >> 2) means "T/4", lock value means the number of delay cell for one period

    offsetd = (vwmc & 0xFF);

    ret = CTRUE;


ca_error_ret:

    if (ret == CFALSE)
    {
        WriteIO32( &pReg_DDRPHY->PHY_CON[10],   0x08 );
    }
    else
    {
        WriteIO32( &pReg_DDRPHY->PHY_CON[10],   offsetd );
    }

    SetIO32  ( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );      // ctrl_resync[24]=0x1 (HIGH)
    ClearIO32( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );      // ctrl_resync[24]=0x0 (LOW)

    ClearIO32( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<  16) );      // ctrl_wrlvl_en(wrlvl_mode)[16]="0"(Disable)


    // Exiting Calibration Mode of LPDDR3 using MR42
    SendDirectCommand(SDRAM_CMD_MRS, 0, 42, 0xA8);                  // CH0 : Send MR42 to exit from CA calibration mode for LPDDR3, MA=0x2A OP=0xA8, 0x50AA0
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 42, 0xA8);                  // CH1 : Send MR42 to exit from CA calibration mode for LPDDR3, MA=0x2A OP=0xA8, 0x50AA0
#else
    if (pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 42, 0xA8);              // CH1 : Send MR42 to exit from CA calibration mode for LPDDR3, MA=0x2A OP=0xA8, 0x50AA0
#endif
#endif

    MEMMSG("\r\n########## CA Calibration - End ##########\r\n");

    return ret;
}
#endif

#if (DDR_GATE_LEVELING_EN == 1)
CBOOL DDR_Gate_Leveling(U32 isResume)
{
#if defined(MEM_TYPE_DDR3)
    union SDRAM_MR MR;
#endif
    volatile U32 cal_count = 0;
#if (DDR_RESET_GATE_LVL == 1)
    U32     lock_div4;
    U32     i;
    U8      gate_cycle[4];
    U8      gate_code[4];
    int     offsetc[4];
#endif
    U32     temp;
    CBOOL   ret = CTRUE;

    MEMMSG("\r\n########## Gate Leveling - Start ##########\r\n");

    ClearIO32( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<  5) );               // ctrl_read_disable[5]=0. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
//    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<  5) );               // ctrl_read_disable[5]= 1. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.

    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1    << 13) );               // byte_rdlvl_en[13]=1

    if (isResume == 0)
    {
        SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if defined(MEM_TYPE_DDR3)
        /* Set MPR mode enable */
        MR.Reg          = 0;
        MR.MR3.MPR      = 1;
        MR.MR3.MPR_RF   = 0;

        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#endif
#if defined(MEM_TYPE_LPDDR23)
        temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
        temp |= 0x00FF;                                                     // rdlvl_rddata_adj[15:0]
    //    temp |= 0x0001;                                                     // rdlvl_rddata_adj[15:0]
        WriteIO32( &pReg_DDRPHY->PHY_CON[1],    temp );

        SendDirectCommand(SDRAM_CMD_MRR, 0, 32, 0x00);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRR, 1, 32, 0x00);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRR, 1, 32, 0x00);
#endif
#endif  // #if defined(MEM_TYPE_LPDDR23)
    }


    SetIO32( &pReg_DDRPHY->PHY_CON[2],      (0x1    <<  24) );              // rdlvl_gate_en[24] = 1
    SetIO32( &pReg_DDRPHY->PHY_CON[0],      (0x5    <<   6) );              // ctrl_shgate[8] = 1, ctrl_atgate[6] = 1
#if defined(MEM_TYPE_DDR3)
    ClearIO32( &pReg_DDRPHY->PHY_CON[1],    (0xF    <<  20) );              // ctrl_gateduradj[23:20] = DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
#endif
#if defined(MEM_TYPE_LPDDR23)
    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1]) & ~(0xF <<  20);            // ctrl_gateduradj[23:20] = DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
    temp |= (0xB    << 20);
    WriteIO32( &pReg_DDRPHY->PHY_CON[1],    temp );
#endif

    /* Update reset DLL */
    WriteIO32( &pReg_Drex->RDLVL_CONFIG,    (0x1    <<   0) );              // ctrl_rdlvl_gate_en[0] = 1

    if (isResume)
    {
        while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 );      // rdlvl_complete[14] = 1
        WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0 );                            // ctrl_rdlvl_gate_en[0] = 0
    }
    else
    {
        while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 )   // rdlvl_complete[14] : Wating until READ leveling is complete
        {
            if (cal_count > 100)                                            // Failure Case
            {
                WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                 // ctrl_rdlvl_data_en[0]=0 : Stopping it after completion of READ leveling.

                ret = CFALSE;
                goto gate_err_ret;
            }
            else
            {
                DMC_Delay(0x100);
            }

            cal_count++;
        }


        nop();
        WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                         // ctrl_rdlvl_data_en[0]=0 : Stopping it after completion of READ leveling.

        //- Checking the calibration failure status
        //- After setting PHY_CON5(0x14) to "0xC", check whether PHY_CON21 is zero or nor. If PHY_CON21(0x58) is zero, calibration is normally complete.

        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    VWM_FAIL_STATUS );          // readmodecon[7:0] = 0xC
        cal_count = 0;
        do {
            if (cal_count > 100)
            {
                MEMMSG("\r\n\nRD VWM_FAIL_STATUS Checking : fail...!!!\r\n");
                ret = CFALSE;                                               // Failure Case
                goto gate_err_ret;
            }
            else if (cal_count)
            {
                DMC_Delay(0x100);
            }

            cal_count++;
        } while( ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] ) != 0x0 );

        //------------------------------------------------------------------------------------------------------------------------
        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    GATE_CENTER_CYCLE);
        g_GateCycle = ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] );

        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    GATE_CENTER_CODE);
        g_GateCode  = ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] );
        //------------------------------------------------------------------------------------------------------------------------
    }

#if (DDR_RESET_GATE_LVL == 1)
    for (i = 0; i < 4; i++)
    {
        gate_cycle[i] = ((g_GateCycle >> (8*i)) & 0xFF);
        gate_code[i]  = ((g_GateCode  >> (8*i)) & 0xFF);
    }

    lock_div4 = (g_DDRLock >> 2);
    for (i = 0; i < 4; i++)
    {
        offsetc[i] = (int)(gate_code[i] - lock_div4);
        if (offsetc[i] < 0)
        {
            offsetc[i] *= -1;
            offsetc[i] |= 0x80;
        }
    }

#if (DDR_GATE_LVL_COMPENSATION_EN == 1)
{
    int max, min, val;
    int i, inx;

//    temp = ( ((U8)offsetc[3] << 24) | ((U8)offsetc[2] << 16) | ((U8)offsetc[1] << 8) | (U8)offsetc[0] );
//    printf("GATE code : Org value = 0x%08X\r\n", temp);

    for (inx = 0; inx < 4; inx++)
    {
        for (i = 1; i < 4; i++)
        {
            if (offsetc[i-1] > offsetc[i])
            {
               max          = offsetc[i-1];
               offsetc[i-1] = offsetc[i];
               offsetc[i]   = max;
            }
        }
    }

#if 0
    for (inx = 0; inx < 4; inx++)
    {
        printf( "Sorted Value[%d] = %d\r\n", inx, offsetc[inx] );
    }
#endif

    if ( offsetc[1] > offsetc[2])
    {
        max = offsetc[1];
        min = offsetc[2];
    }
    else
    {
        max = offsetc[2];
        min = offsetc[1];
    }

    if ( (max - min) > 5)
    {
        val = min;
    }
    else
    {
        val = max;
    }

    for (inx = 0; inx < 4; inx++)
    {
        offsetc[inx] = val;
    }
}
#endif  // #if (DDR_GATE_LVL_COMPENSATION_EN == 1)

    temp = ( ((U8)offsetc[3] << 24) | ((U8)offsetc[2] << 16) | ((U8)offsetc[1] << 8) | (U8)offsetc[0] );
    WriteIO32( &pReg_DDRPHY->PHY_CON[8], temp );    // ctrl_offsetc

    temp = ( ((U8)gate_cycle[3] << 15) | ((U8)gate_cycle[2] << 10) | ((U8)gate_cycle[1] << 5) | (U8)gate_cycle[0] );
    WriteIO32( &pReg_DDRPHY->PHY_CON[3], temp );    // ctrl_shiftc
#endif  // #if (DDR_RESET_GATE_LVL == 1)

gate_err_ret:

    WriteIO32( &pReg_DDRPHY->PHY_CON[5],    0x0 );                          // readmodecon[7:0] = 0x0

    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization

    if (isResume == 0)
    {
#if defined(MEM_TYPE_DDR3)

        /* Set MPR mode disable */
        MR.Reg          = 0;
        MR.MR3.MPR      = 0;
        MR.MR3.MPR_RF   = 0;

        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#endif  // #if defined(MEM_TYPE_DDR3)
    }

    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<   5) );              // ctrl_read_disable[5]= 1. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
//    ClearIO32( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<   5) );              // ctrl_read_disable[5]=0. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.

    MEMMSG("\r\n########## Gate Leveling - End ##########\r\n");

    return ret;
}
#endif  // #if (DDR_GATE_LEVELING_EN == 1)

#if (DDR_READ_DQ_CALIB_EN == 1)
CBOOL DDR_Read_DQ_Calibration(U32 isResume)
{
#if defined(MEM_TYPE_DDR3)
    union SDRAM_MR MR;
#endif
    volatile U32 cal_count = 0;
#if (DDR_RESET_READ_DQ == 1)
    U32     lock_div4;
    U32     i;
    U8      rlvl_vwmc[4];
    int     offsetr[4];
#endif
    U32     temp;
    CBOOL   ret = CTRUE;


    MEMMSG("\r\n########## Read DQ Calibration - Start ##########\r\n");

    if (isResume == 0)
    {
        SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
    }

#if defined(MEM_TYPE_DDR3)
    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
//    temp |= 0xFF00;                                                     // rdlvl_rddata_adj[15:0]
    temp |= 0x0100;                                                     // rdlvl_rddata_adj[15:0]
    WriteIO32( &pReg_DDRPHY->PHY_CON[1],    temp );


    if (isResume == 0)
    {
        /* Set MPR mode enable */
        MR.Reg          = 0;
        MR.MR3.MPR      = 1;
        MR.MR3.MPR_RF   = 0;

        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
    }

    WriteIO32( &pReg_DDRPHY->PHY_CON[24+1],
        (0x0    << 16) |    // [31:16] ddr3_default
        (0x0    <<  1) |    // [15: 1] ddr3_address
        (0x0    <<  0) );   // [    0] ca_swap_mode
#endif
#if defined(MEM_TYPE_LPDDR23)
    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
//    temp |= 0x00FF;                                                     // rdlvl_rddata_adj[15:0]
    temp |= 0x0001;                                                     // rdlvl_rddata_adj[15:0]
    WriteIO32( &pReg_DDRPHY->PHY_CON[1],    temp );

    SendDirectCommand(SDRAM_CMD_MRR, 0, 32, 0x00);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRR, 1, 32, 0x00);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRR, 1, 32, 0x00);
#endif
#endif  // #if defined(MEM_TYPE_LPDDR23)

    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  25) );              // rdlvl_en[25]=1

    WriteIO32( &pReg_Drex->RDLVL_CONFIG,    (0x1    <<   1) );              // ctrl_rdlvl_data_en[1]=1 : Starting READ leveling

    if (isResume)
    {
        while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 );  // rdlvl_complete[14] : Wating until READ leveling is complete
        WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                         // ctrl_rdlvl_data_en[1]=0 : Stopping it after completion of READ leveling.
    }
    else
    {
        while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 )   // rdlvl_complete[14] : Wating until READ leveling is complete
        {
            if (cal_count > 100)                                            // Failure Case
            {
                WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                 // ctrl_rdlvl_data_en[1]=0 : Stopping it after completion of READ leveling.

                ret = CFALSE;
                goto rd_err_ret;
            }
            else
            {
                DMC_Delay(0x100);
            }

            cal_count++;
        }

        nop();
        WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                         // ctrl_rdlvl_data_en[1]=0 : Stopping it after completion of READ leveling.

        //- Checking the calibration failure status
        //- After setting PHY_CON5(0x14) to "0xC", check whether PHY_CON21 is zero or nor. If PHY_CON21(0x58) is zero, calibration is normally complete.

        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    VWM_FAIL_STATUS );          // readmodecon[7:0] = 0xC
        cal_count = 0;
        do {
            if (cal_count > 100)
            {
                MEMMSG("\r\n\nRD VWM_FAIL_STATUS Checking : fail...!!!\r\n");
                ret = CFALSE;                                               // Failure Case
                goto rd_err_ret;
            }
            else if (cal_count)
            {
                DMC_Delay(0x100);
            }

            cal_count++;
        } while( ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] ) != 0x0 );

        //*** Read DQ Calibration Valid Window Margin
        //------------------------------------------------------------------------------------------------------------------------
        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    RD_VWMC);
        g_RDvwmc = ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] );
        //------------------------------------------------------------------------------------------------------------------------
    }

#if (DDR_RESET_READ_DQ == 1)
    for (i = 0; i < 4; i++)
    {
        rlvl_vwmc[i]   = ((g_RDvwmc >> (8*i)) & 0xFF);
    }

//*** Forcing manually offsetr to stop the calibration mode after completion of READ DQ Cal

    lock_div4 = (g_DDRLock >> 2);
    for (i = 0; i < 4; i++)
    {
        offsetr[i] = (int)(rlvl_vwmc[i] - lock_div4);
        if (offsetr[i] < 0)
        {
            MEMMSG("offsetr%d=%d\r\n", i, offsetr[i]);
            offsetr[i] *= -1;
            offsetr[i] |= 0x80;
        }
    }

#if (DDR_READ_DQ_COMPENSATION_EN == 1)
{
    int max, min, val;
    int i, inx;

//    temp = ( ((U8)offsetr[3] << 24) | ((U8)offsetr[2] << 16) | ((U8)offsetr[1] << 8) | (U8)offsetr[0] );
//    printf("RD DQ : Org value = 0x%08X\r\n", temp);

    for (inx = 0; inx < 4; inx++)
    {
        for (i = 1; i < 4; i++)
        {
            if (offsetr[i-1] > offsetr[i])
            {
               max          = offsetr[i-1];
               offsetr[i-1] = offsetr[i];
               offsetr[i]   = max;
            }
        }
    }

#if 0
    for (inx = 0; inx < 4; inx++)
    {
        printf( "Sorted Value[%d] = %d\r\n", inx, offsetr[inx] );
    }
#endif

    if ( offsetr[1] > offsetr[2])
    {
        max = offsetr[1];
        min = offsetr[2];
    }
    else
    {
        max = offsetr[2];
        min = offsetr[1];
    }

    if ( (max - min) > 5)
    {
        val = min;
    }
    else
    {
        val = max;
    }

    for (inx = 0; inx < 4; inx++)
    {
        offsetr[inx] = val;
    }
}
#endif  // #if (DDR_READ_DQ_COMPENSATION_EN == 1)

    if (isResume == 1)
    {
        temp = g_RDvwmc;
    }
    else
    {
        temp = ( ((U8)offsetr[3] << 24) | ((U8)offsetr[2] << 16) | ((U8)offsetr[1] << 8) | (U8)offsetr[0] );
        g_RDvwmc = temp;
    }
    WriteIO32( &pReg_DDRPHY->PHY_CON[4], temp );

    //*** Resync
    //*** Update READ SDLL Code (ctrl_offsetr) : Make "ctrl_resync" HIGH and LOW
    SetIO32  ( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );      // ctrl_resync[24]=0x1 (HIGH)
    ClearIO32( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );      // ctrl_resync[24]=0x0 (LOW)
#endif  // #if (DDR_RESET_READ_DQ == 1)


rd_err_ret:

    //*** Set PHY0.CON2.rdlvl_en : setting MUX as "0" to force manually the result value of READ leveling.
    ClearIO32( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  25) );      // rdlvl_en[25]=0
    WriteIO32( &pReg_DDRPHY->PHY_CON[5],    0x0 );                  // readmodecon[7:0] = 0x0

    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization


    if (isResume == 0)
    {
#if defined(MEM_TYPE_DDR3)

        /* Set MPR mode disable */
        MR.Reg          = 0;
        MR.MR3.MPR      = 0;
        MR.MR3.MPR_RF   = 0;

        SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
        if(pSBI->DII.ChipNum > 1)
            SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#endif
    }

    MEMMSG("\r\n########## Read DQ Calibration - End ##########\r\n");

    return ret;
}
#endif  // #if (DDR_READ_DQ_CALIB_EN == 1)

#if (DDR_WRITE_LEVELING_CALIB_EN == 1)
void DDR_Write_Leveling_Calibration(void)
{
    MEMMSG("\r\n########## Write Leveling Calibration - Start ##########\r\n");

}
#endif

#if (DDR_WRITE_DQ_CALIB_EN == 1)
CBOOL DDR_Write_DQ_Calibration(U32 isResume)
{
    volatile U32 cal_count = 0;
    U32     lock_div4;
    U32     i;
    U8      wlvl_vwmc[4];
    int     offsetw[4];
    U32     temp;
    CBOOL   ret = CTRUE;

    MEMMSG("\r\n########## Write DQ Calibration - Start ##########\r\n");


#if 1
    SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

    DMC_Delay(0x100);
#endif


    while( ReadIO32(&pReg_Drex->CHIPSTATUS) & 0xF )         //- Until chip_busy_state
    {
        nop();
    }

    WriteIO32( &pReg_Drex->WRTRA_CONFIG,
        (0x0    << 16) |    // [31:16] row_addr
        (0x0    <<  1) |    // [ 3: 1] bank_addr
        (0x1    <<  0) );   // [    0] write_training_en

#if defined(MEM_TYPE_DDR3)
    WriteIO32( &pReg_DDRPHY->PHY_CON[24+1],
        (0x0    << 16) |    // [31:16] ddr3_default
        (0x0    <<  1) |    // [15: 1] ddr3_address
        (0x0    <<  0) );   // [    0] ca_swap_mode

    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
    if (ReadIO32( &pReg_DDRPHY->PHY_CON[0] ) & (0x1 << 13) )
        temp |= 0x0100;
    else
        temp |= 0xFF00;
    WriteIO32( &pReg_DDRPHY->PHY_CON[1],        temp );
#endif
#if defined(MEM_TYPE_LPDDR23)
    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
    if (ReadIO32( &pReg_DDRPHY->PHY_CON[0] ) & (0x1 << 13) )
        temp |= 0x0001;
    else
        temp |= 0x00FF;
    WriteIO32( &pReg_DDRPHY->PHY_CON[1],        temp );
#endif

    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    << 14) );   // p0_cmd_en[14] = 1

    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    << 26) );   // wr_deskew_con[26] = 1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    << 27) );   // wr_deskew_en[27] = 1

    if (isResume)
    {
        while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1   << 14) ) == 0 );  //- wait, rdlvl_complete[14]
        ClearIO32( &pReg_Drex->WRTRA_CONFIG,    (0x1    <<  0) );   // write_training_en[0] = 0

        MEMMSG("\r\n########## Write DQ Calibration - End ##########\r\n");
    }
    else
    {
        while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1   << 14) ) == 0 ) //- wait, rdlvl_complete[14]
        {
            if (cal_count > 100) {                                          // Failure Case
                //cal_error = 1;
                ClearIO32( &pReg_Drex->WRTRA_CONFIG, (0x1    <<  0) );      // write_training_en[0]=0 : Stopping it after completion of WRITE leveling.
                MEMMSG("\r\n\nWD DQ CAL Status Checking : fail...!!!\r\n");

                ret = CFALSE;
                goto wr_err_ret;
            }
            else
            {
                DMC_Delay(0x100);
                //MEMMSG("r\n\nWD DQ CAL Status Checking : %d\n", cal_count);
            }

            cal_count++;
        }

        nop();
        ClearIO32( &pReg_Drex->WRTRA_CONFIG,    (0x1    <<  0) );       // write_training_en[0] = 0

        //- Checking the calibration failure status
        //- After setting PHY_CON5(0x14) to "0xC", check whether PHY_CON21 is zero or nor. If PHY_CON21(0x58) is zero, calibration is normally complete.

        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    VWM_FAIL_STATUS );      // readmodecon[7:0] = 0xC
        cal_count = 0;
        do {
            if (cal_count > 100)
            {
                MEMMSG("\r\n\nWR VWM_FAIL_STATUS Checking : fail...!!!\r\n");
                ret = CFALSE;                                           // Failure Case
                goto wr_err_ret;
            }
            else if (cal_count)
            {
                DMC_Delay(0x100);
            }


            cal_count++;
        } while( ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] ) != 0x0 );

        //*** Write DQ Calibration Valid Window Margin
        //------------------------------------------------------------------------------------------------------------------------
    //    WriteIO32( &pReg_DDRPHY->PHY_CON[5],    VWM_CENTER);
        WriteIO32( &pReg_DDRPHY->PHY_CON[5],    WR_VWMC);
        g_WRvwmc = ReadIO32( &pReg_DDRPHY->PHY_CON[19+1] );
        //------------------------------------------------------------------------------------------------------------------------
    }

#if (DDR_RESET_WRITE_DQ == 1)
    for (i = 0; i < 4; i++)
    {
        wlvl_vwmc[i]   = ((g_WRvwmc >> (8*i)) & 0xFF);
    }


//*** Forcing manually offsetr to stop the calibration mode after completion of WRITE DQ Cal

    lock_div4 = (g_DDRLock >> 2);
    for (i = 0; i < 4; i++)
    {
        offsetw[i] = (int)(wlvl_vwmc[i] - lock_div4);
        if (offsetw[i] < 0)
        {
            offsetw[i] *= -1;
            offsetw[i] |= 0x80;
        }
    }

#if (DDR_WRITE_DQ_COMPENSATION_EN == 1)
{
    int max, min, val;
    int i, inx;

//    temp = ( ((U8)offsetw[3] << 24) | ((U8)offsetw[2] << 16) | ((U8)offsetw[1] << 8) | (U8)offsetw[0] );
//    printf("WR DQ : Org value = 0x%08X\r\n", temp);

    for (inx = 0; inx < 4; inx++)
    {
        for (i = 1; i < 4; i++)
        {
            if (offsetw[i-1] > offsetw[i])
            {
               max          = offsetw[i-1];
               offsetw[i-1] = offsetw[i];
               offsetw[i]   = max;
            }
        }
    }

#if 0
    for (inx = 0; inx < 4; inx++)
    {
        printf( "Sorted Value[%d] = %d\r\n", inx, offsetw[inx] );
    }
#endif

    if ( offsetw[1] > offsetw[2])
    {
        max = offsetw[1];
        min = offsetw[2];
    }
    else
    {
        max = offsetw[2];
        min = offsetw[1];
    }
    temp = (max - min);

    if ( (max - min) > 5)
    {
        val = min;
    }
    else
    {
        val = max;
    }

    for (inx = 0; inx < 4; inx++)
    {
        offsetw[inx] = val;
    }
}
#endif  // #if (DDR_WRITE_DQ_COMPENSATION_EN == 1)

    if (isResume == 1)
    {
        temp = g_WRvwmc;
    }
    else
    {
        temp = ( ((U8)offsetw[3] << 24) | ((U8)offsetw[2] << 16) | ((U8)offsetw[1] << 8) | (U8)offsetw[0] );
        g_WRvwmc = temp;
    }
    WriteIO32( &pReg_DDRPHY->PHY_CON[6], temp );

    //*** Resync
    //*** Update WRITE SDLL Code (ctrl_offsetr) : Make "ctrl_resync" HIGH and LOW
    SetIO32  ( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );              // ctrl_resync[24]=0x1 (HIGH)
    ClearIO32( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24) );              // ctrl_resync[24]=0x0 (LOW)
#endif  // #if (DDR_RESET_WRITE_DQ == 1)

wr_err_ret:

    ClearIO32( &pReg_DDRPHY->PHY_CON[2],    (0x3    << 26) );               // wr_deskew_en[27] = 0, wr_deskew_con[26] = 0

    WriteIO32( &pReg_DDRPHY->PHY_CON[5],    0x0 );                          // readmodecon[7:0] = 0x0

    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization

    MEMMSG("\r\n########## Write DQ Calibration - End ##########\r\n");

    return ret;
}
#endif  // #if (DDR_WRITE_DQ_CALIB_EN == 1)
#endif  // #if (DDR_NEW_LEVELING_TRAINING == 1)


void init_LPDDR3(U32 isResume)
{
    union SDRAM_MR MR1, MR2, MR3, MR11;
    U32 DDR_WL, DDR_RL;
    U32 i, lock_div4;
    U32 temp;


    // Set 200Mhz to PLL3.
#if (CFG_DDR_LOW_FREQ == 1)
#if defined(MEM_TYPE_LPDDR23)
    while(DebugIsBusy());
    setMemPLL(0);
#endif
#endif

    MR1.Reg  = 0;
    MR2.Reg  = 0;
    MR3.Reg  = 0;
    MR11.Reg = 0;

    MEMMSG("\r\nLPDDR3 POR Init Start\r\n");

    // Step 1. reset (Min : 10ns, Typ : 200us)
    ClearIO32( &pReg_RstCon->REGRST[0],     (0x7    <<  26) );
    DMC_Delay(0x1000);                                          // wait 300ms
    SetIO32  ( &pReg_RstCon->REGRST[0],     (0x7    <<  26) );
    DMC_Delay(0x1000);                                          // wait 300ms
    ClearIO32( &pReg_RstCon->REGRST[0],     (0x7    <<  26) );
    DMC_Delay(0x1000);                                          // wait 300ms
    SetIO32  ( &pReg_RstCon->REGRST[0],     (0x7    <<  26) );
//    DMC_Delay(0x10000);                                        // wait 300ms

    ClearIO32( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
    DMC_Delay(0x1000);                                          // wait 300ms
    SetIO32  ( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
    DMC_Delay(0x1000);                                          // wait 300ms
    ClearIO32( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
    DMC_Delay(0x1000);                                          // wait 300ms
    SetIO32  ( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
    DMC_Delay(0x10000);                                         // wait 300ms

//    MEMMSG("PHY Version: %X\r\n", ReadIO32(&pReg_DDRPHY->VERSION_INFO));

    if (isResume)
    {
        WriteIO32(&pReg_Alive->ALIVEPWRGATEREG,     1);             // open alive power gate
        WriteIO32(&pReg_Alive->ALIVEPWRGATEREG,     1);             // open alive power gate

        g_GateCycle = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE5);    // read - ctrl_shiftc
        g_GateCode  = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE6);    // read - ctrl_offsetc
        g_RDvwmc    = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE7);    // read - ctrl_offsetr
        g_WRvwmc    = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE8);    // read - ctrl_offsetw
        g_CAvwmc    = ReadIO32(&pReg_RTC->RTCSCRATCH);              // read - ctrl_offsetd

        WriteIO32(&pReg_Alive->ALIVEPWRGATEREG,     0);             // close alive power gate
        WriteIO32(&pReg_Alive->ALIVEPWRGATEREG,     0);             // close alive power gate
    }

    if (!g_GateCycle || !g_RDvwmc || !g_WRvwmc)
        isResume = 0;


#if 0
#if (CFG_NSIH_EN == 0)
    MEMMSG("READDELAY   = 0x%08X\r\n", READDELAY);
    MEMMSG("WRITEDELAY  = 0x%08X\r\n", WRITEDELAY);
#else
    MEMMSG("READDELAY   = 0x%08X\r\n", pSBI->DII.READDELAY);
    MEMMSG("WRITEDELAY  = 0x%08X\r\n", pSBI->DII.WRITEDELAY);
#endif
#endif

#if (CFG_NSIH_EN == 0)
    //pSBI->LvlTr_Mode    = ( LVLTR_WR_LVL | LVLTR_CA_CAL | LVLTR_GT_LVL | LVLTR_RD_CAL | LVLTR_WR_CAL );
    //pSBI->LvlTr_Mode    = ( LVLTR_GT_LVL | LVLTR_RD_CAL | LVLTR_WR_CAL );
    pSBI->LvlTr_Mode    = LVLTR_GT_LVL;
    //pSBI->LvlTr_Mode    = 0;
#endif

#if (CFG_NSIH_EN == 0)
#if 1   // Common
//    pSBI->DDR3_DSInfo.MR2_RTT_WR    = 2;    // RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
//    pSBI->DDR3_DSInfo.MR1_ODS       = 1;    // ODS - 00: RZQ/6, 01 : RZQ/7
//    pSBI->DDR3_DSInfo.MR1_RTT_Nom   = 2;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8

    pSBI->PHY_DSInfo.DRVDS_Byte3    = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte2    = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte1    = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte0    = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_CK       = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_CKE      = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_CS       = PHY_DRV_STRENGTH_240OHM;
    pSBI->PHY_DSInfo.DRVDS_CA       = PHY_DRV_STRENGTH_240OHM;

    pSBI->PHY_DSInfo.ZQ_DDS         = PHY_DRV_STRENGTH_48OHM;
    pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_120OHM;
#endif

#if 0   // DroneL 720Mhz
    pSBI->DDR3_DSInfo.MR2_RTT_WR    = 1;    // RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
    pSBI->DDR3_DSInfo.MR1_ODS       = 1;    // ODS - 00: RZQ/6, 01 : RZQ/7
    pSBI->DDR3_DSInfo.MR1_RTT_Nom   = 3;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8

    pSBI->PHY_DSInfo.DRVDS_Byte3    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte2    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte1    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte0    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_CK       = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_CKE      = PHY_DRV_STRENGTH_30OHM;
    pSBI->PHY_DSInfo.DRVDS_CS       = PHY_DRV_STRENGTH_30OHM;
    pSBI->PHY_DSInfo.DRVDS_CA       = PHY_DRV_STRENGTH_30OHM;

    pSBI->PHY_DSInfo.ZQ_DDS         = PHY_DRV_STRENGTH_40OHM;
//    pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_80OHM;
    pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_60OHM;
#endif

#if 0   // DroneL 800Mhz
    pSBI->DDR3_DSInfo.MR2_RTT_WR    = 2;    // RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
    pSBI->DDR3_DSInfo.MR1_ODS       = 1;    // ODS - 00: RZQ/6, 01 : RZQ/7
    pSBI->DDR3_DSInfo.MR1_RTT_Nom   = 3;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8

    pSBI->PHY_DSInfo.DRVDS_Byte3    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte2    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte1    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_Byte0    = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_CK       = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.DRVDS_CKE      = PHY_DRV_STRENGTH_30OHM;
    pSBI->PHY_DSInfo.DRVDS_CS       = PHY_DRV_STRENGTH_30OHM;
    pSBI->PHY_DSInfo.DRVDS_CA       = PHY_DRV_STRENGTH_30OHM;

    pSBI->PHY_DSInfo.ZQ_DDS         = PHY_DRV_STRENGTH_40OHM;
    pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_120OHM;
#endif
#endif


    MR1.LP_MR1.BL       = 3;

    MR2.LP_MR2.WL_SEL   = 0;
    MR2.LP_MR2.WR_LVL   = 0;

#if (CFG_NSIH_EN == 0)
    DDR_WL = nWL;
    DDR_RL = nRL;

    if (MR1_nWR > 9)
    {
        MR1.LP_MR1.WR   = (MR1_nWR - 10) & 0x7;
        MR2.LP_MR2.WRE  = 1;
    }
    else
    {
        MR1.LP_MR1.WR   = (MR1_nWR - 2) & 0x7;
        MR2.LP_MR2.WRE  = 0;
    }

    if (MR2_RLWL < 6)
    {
        MR2.LP_MR2.RL_WL    = 4;
    }
    else
    {
        MR2.LP_MR2.RL_WL    = (MR2_RLWL - 2);
    }

    MR3.LP_MR3.DS       = 2;

#if (CFG_ODT_ENB == 1)
    MR11.LP_MR11.DQ_ODT = 2;    // DQ ODT - 0: Disable, 1: Rzq/4, 2: Rzq/2, 3: Rzq/1
#endif
    MR11.LP_MR11.PD_CON = 0;
#else

    DDR_WL = (pSBI->DII.TIMINGDATA >> 8) & 0xF;
    DDR_RL = (pSBI->DII.TIMINGDATA & 0xF);

    if (pSBI->DII.MR0_WR > 9)
    {
        MR1.LP_MR1.WR   = (pSBI->DII.MR0_WR - 10) & 0x7;
        MR2.LP_MR2.WRE  = 1;
    }
    else
    {
        MR1.LP_MR1.WR   = (pSBI->DII.MR0_WR - 2) & 0x7;
        MR2.LP_MR2.WRE  = 0;
    }

    if (pSBI->DII.MR1_AL < 6)
    {
        MR2.LP_MR2.RL_WL    = 4;
    }
    else
    {
        MR2.LP_MR2.RL_WL    = (pSBI->DII.MR1_AL - 2);
    }

    MR3.LP_MR3.DS       = pSBI->LPDDR3_DSInfo.MR3_DS;

#if (CFG_ODT_ENB == 1)
    MR11.LP_MR11.DQ_ODT = pSBI->LPDDR3_DSInfo.MR11_DQ_ODT;
#endif
    MR11.LP_MR11.PD_CON = pSBI->LPDDR3_DSInfo.MR11_PD_CON;
#endif


// Step 2. Select Memory type : LPDDR3
// Check LPDDR3 MPR data and match it to PHY_CON[1]??

#if defined(MEM_TYPE_LPDDR23)
#if (DDR_CA_SWAP_MODE == 1)
    WriteIO32( &pReg_DDRPHY->PHY_CON[22+1],     0x00000041 );           // lpddr2_addr[19:0] = 0x041
#else
    WriteIO32( &pReg_DDRPHY->PHY_CON[22+1],     0x00000208 );           // lpddr2_addr[19:0] = 0x208
#endif

    WriteIO32( &pReg_DDRPHY->PHY_CON[1],
        (0x0    <<  28) |           // [31:28] ctrl_gateadj
        (0x9    <<  24) |           // [27:24] ctrl_readadj
        (0xB    <<  20) |           // [23:20] ctrl_gateduradj  :: DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
        (0x1    <<  16) |           // [19:16] rdlvl_pass_adj
        (0x0001 <<   0) );          // [15: 0] rdlvl_rddata_adj :: LPDDR3 : 0x0001 or 0x00FF
//        (0x00FF <<   0) );          // [15: 0] rdlvl_rddata_adj :: LPDDR3 : 0x0001 or 0x00FF
#endif

    WriteIO32( &pReg_DDRPHY->PHY_CON[2],
        (0x0    <<  28) |           // [31:28] ctrl_readduradj
        (0x0    <<  27) |           // [   27] wr_deskew_en
        (0x0    <<  26) |           // [   26] wr_deskew_con
        (0x0    <<  25) |           // [   25] rdlvl_en
        (0x0    <<  24) |           // [   24] rdlvl_gate_en
        (0x0    <<  23) |           // [   23] rdlvl_ca_en
        (0x1    <<  16) |           // [22:16] rdlvl_incr_adj
        (0x0    <<  14) |           // [   14] wrdeskew_clear
        (0x0    <<  13) |           // [   13] rddeskew_clear
        (0x0    <<  12) |           // [   12] dlldeskew_en
        (0x2    <<  10) |           // [11:10] rdlvl_start_adj - Right shift, valid value: 1 or 2
        (0x1    <<   8) |           // [ 9: 8] rdlvl_start_adj - Left shift,  valid value: 0 ~ 2
        (0x0    <<   6) |           // [    6] initdeskewen
        (0x0    <<   0) );          // [ 1: 0] rdlvl_gateadj

    temp = (
        (0x0    <<  29) |           // [31:29] Reserved - SBZ.
        (0x17   <<  24) |           // [28:24] T_WrWrCmd.
//        (0x0    <<  22) |           // [23:22] Reserved - SBZ.
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
#if defined(MEM_TYPE_LPDDR23)
        (0x3    <<  11) |           // [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
#endif
//        (0x0    <<  10) |           // [   10] Reserved - SBZ.
        (0x1    <<   9) |           // [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
//        (0x1    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
        (0x0    <<   7) |           // [    7] ctrl_ckdis. 0:Clock output Enable, 1:Disable
//        (0x1    <<   6) |           // [    6] ctrl_atgate.
#if defined(MEM_TYPE_DDR3)
        (0x1    <<   5) |           // [    5] ctrl_read_disable. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
#endif
        (0x0    <<   4) |           // [    4] ctrl_cmosrcv.
        (0x0    <<   3) |           // [    3] ctrl_read_width.
        (0x0    <<   0));           // [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

#if (CFG_NSIH_EN == 1)
    if ((pSBI->DII.TIMINGDATA >> 28) == 3)      // 6 cycles
        temp |= (0x7    <<  17);
    else if ((pSBI->DII.TIMINGDATA >> 28) == 2) // 4 cycles
        temp |= (0x6    <<  17);
#endif
    WriteIO32( &pReg_DDRPHY->PHY_CON[0],    temp );

    MEMMSG("phy init\r\n");

/*  Set ZQ clock div    */
    WriteIO32( &pReg_DDRPHY->PHY_CON[40+1], 0x07);

/*  Set ZQ Timer    */
//    WriteIO32( &pReg_DDRPHY->PHY_CON[41+1], 0xF0);

/* Set WL, RL, BL */
    WriteIO32( &pReg_DDRPHY->PHY_CON[42+1],
        (0x8    <<   8) |       // Burst Length(BL)
        (DDR_RL <<   0));       // Read Latency(RL) - 800MHz:0xB, 533MHz:0x5

/* Set WL  */
#if defined(MEM_TYPE_LPDDR23)
    temp = ((0x105E <<  16) | 0x000E );                             // cmd_active= DDR3:0x105E, LPDDDR2 or LPDDDR3:0x000E
    WriteIO32( &pReg_DDRPHY->PHY_CON[25+1], temp);

    temp = (((DDR_WL+1) <<  16) | 0x000F );                         // T_wrdata_en, In LPDDR3 (WL+1)
    WriteIO32( &pReg_DDRPHY->PHY_CON[26+1], temp);
#endif  // #if defined(MEM_TYPE_LPDDR23)

    /* ZQ Calibration */
#if 0
    WriteIO32( &pReg_DDRPHY->PHY_CON[39+1],         // 100: 48ohm, 101: 40ohm, 110: 34ohm, 111: 30ohm
        (pSBI->PHY_DSInfo.DRVDS_Byte3 <<  25) |     // [27:25] Data Slice 3
        (pSBI->PHY_DSInfo.DRVDS_Byte2 <<  22) |     // [24:22] Data Slice 2
        (pSBI->PHY_DSInfo.DRVDS_Byte1 <<  19) |     // [21:19] Data Slice 1
        (pSBI->PHY_DSInfo.DRVDS_Byte0 <<  16) |     // [18:16] Data Slice 0
        (pSBI->PHY_DSInfo.DRVDS_CK    <<   9) |     // [11: 9] CK
        (pSBI->PHY_DSInfo.DRVDS_CKE   <<   6) |     // [ 8: 6] CKE
        (pSBI->PHY_DSInfo.DRVDS_CS    <<   3) |     // [ 5: 3] CS
        (pSBI->PHY_DSInfo.DRVDS_CA    <<   0));     // [ 2: 0] CA[9:0], RAS, CAS, WEN, ODT[1:0], RESET, BANK[2:0]
#else
    WriteIO32( &pReg_DDRPHY->PHY_CON[39+1],     0x00 );
#endif

    // Driver Strength(zq_mode_dds), zq_clk_div_en[18]=Enable
    WriteIO32( &pReg_DDRPHY->PHY_CON[16],
        (0x1    <<  27) |                       // [   27] zq_clk_en. ZQ I/O clock enable.
#if 0
        (PHY_DRV_STRENGTH_48OHM     <<  24) |   // [26:24] zq_mode_dds, Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
        (PHY_DRV_STRENGTH_120OHM    <<  21) |   // [23:21] ODT resistor value. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
#else
        (pSBI->PHY_DSInfo.ZQ_DDS    <<  24) |   // [26:24] zq_mode_dds, Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
        (pSBI->PHY_DSInfo.ZQ_ODT    <<  21) |   // [23:21] ODT resistor value. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
#endif
        (0x0    <<  20) |                       // [   20] zq_rgddr3. GDDR3 mode. 0:Enable, 1:Disable
#if defined(MEM_TYPE_DDR3)
        (0x0    <<  19) |                       // [   19] zq_mode_noterm. Termination. 0:Enable, 1:Disable
#endif
#if defined(MEM_TYPE_LPDDR23)
        (0x1    <<  19) |                       // [   19] zq_mode_noterm. Termination. 0:Enable, 1:Disable
#endif
        (0x1    <<  18) |                       // [   18] zq_clk_div_en. Clock Dividing Enable : 0, Disable : 1
        (0x0    <<  15) |                       // [17:15] zq_force-impn
        (0x0    <<  12) |                       // [14:12] zq_force-impp
        (0x30   <<   4) |                       // [11: 4] zq_udt_dly
        (0x1    <<   2) |                       // [ 3: 2] zq_manual_mode. 0:Force Calibration, 1:Long cali, 2:Short cali
        (0x0    <<   1) |                       // [    1] zq_manual_str. Manual Calibration Stop : 0, Start : 1
        (0x0    <<   0));                       // [    0] zq_auto_en. Auto Calibration enable

    SetIO32( &pReg_DDRPHY->PHY_CON[16],     (0x1    <<   1) );          // zq_manual_str[1]. Manual Calibration Start=1
    while( ( ReadIO32( &pReg_DDRPHY->PHY_CON[17+1] ) & 0x1 ) == 0 );    //- PHY0: wait for zq_done
    ClearIO32( &pReg_DDRPHY->PHY_CON[16],   (0x1    <<   1) );          // zq_manual_str[1]. Manual Calibration Stop : 0, Start : 1

//    ClearIO32( &pReg_DDRPHY->PHY_CON[16],   (0x1    <<  18) );          // zq_clk_div_en[18]. Clock Dividing Enable : 1, Disable : 0


    // Step 3. Set the PHY for dqs pull down mode
    WriteIO32( &pReg_DDRPHY->PHY_CON[14],
        (0x0    <<   8) |           // ctrl_pulld_dq[11:8]
        (0xF    <<   0));           // ctrl_pulld_dqs[7:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)
    // Step 4. ODT
    WriteIO32( &pReg_Drex->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
        (0x1    <<  31) |           // [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
        (0x1    <<  30) |           // [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
        (0x0    <<   0) |           // [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
#if defined(MEM_TYPE_DDR3)
        (0x1    <<  29) |           // [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
#endif
        (0x0    <<  24) |           // [28:24] ctrl_pd. Input Gate for Power Down.
//        (0x0    <<   7) |           // [23: 7] Reserved - SBZ
        (0x0    <<   4) |           // [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
//        (0x1    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
        (0x0    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
        (0x0    <<   2) |           // [    2] Reserved - SBZ
        (0x0    <<   1));           // [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1

#if 1
    WriteIO32( &pReg_Drex->CONCONTROL,
        (0x0    <<  28) |           // [   28] dfi_init_start
        (0xFFF  <<  16) |           // [27:16] timeout_level0
//        (0x3    <<  12) |           // [14:12] rd_fetch
        (0x1    <<  12) |           // [14:12] rd_fetch
        (0x1    <<   8) |           // [    8] empty
//        (0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
        (0x0    <<   3) |           // [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
        (0x0    <<   1));           // [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved
#else

    temp = (U32)(
        (0x0    <<  28) |           // [   28] dfi_init_start
        (0xFFF  <<  16) |           // [27:16] timeout_level0
        (0x3    <<  12) |           // [14:12] rd_fetch
        (0x1    <<   8) |           // [    8] empty
        (0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
        (0x0    <<   3) |           // [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
        (0x0    <<   1));           // [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved

    if (isResume)
        temp &= ~(0x1    <<   5);

    WriteIO32( &pReg_Drex->CONCONTROL,  temp );
#endif

#if 0
    // Step 5. dfi_init_start : High
    SetIO32( &pReg_Drex->CONCONTROL,    (0x1    <<  28) );          // DFI PHY initialization start
    while( (ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1<<3) ) == 0);   // wait for DFI PHY initialization complete
    ClearIO32( &pReg_Drex->CONCONTROL,  (0x1    <<  28) );          // DFI PHY initialization clear
#endif

    WriteIO32( &pReg_DDRPHY->PHY_CON[12],
        (0x10   <<  24) |           // [30:24] ctrl_start_point
        (0x10   <<  16) |           // [22:16] ctrl_inc
        (0x0    <<   8) |           // [14: 8] ctrl_force
        (0x1    <<   6) |           // [    6] ctrl_start
        (0x1    <<   5) |           // [    5] ctrl_dll_on
//        (0xF    <<   1));           // [ 4: 1] ctrl_ref
        (0x8    <<   1));           // [ 4: 1] ctrl_ref
    DMC_Delay(1);


    // Step 8 : Update DLL information
    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization


    // Step 11. MemBaseConfig
    WriteIO32( &pReg_Drex->MEMBASECONFIG[0],
        (0x0        <<  16) |                   // chip_base[26:16]. AXI Base Address. if 0x20 ==> AXI base addr of memory : 0x2000_0000
#if (CFG_NSIH_EN == 0)
        (chip_mask  <<   0));                   // 256MB:0x7F0, 512MB: 0x7E0, 1GB:0x7C0, 2GB: 0x780, 4GB:0x700
#else
        (pSBI->DII.ChipMask <<   0));           // chip_mask[10:0]. 1GB:0x7C0, 2GB:0x780
#endif

#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    {
        WriteIO32( &pReg_Drex->MEMBASECONFIG[1],
            (chip_base1 <<  16) |               // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
//            (0x040      <<  16) |               // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
            (chip_mask  <<   0));               // chip_mask[10:0]. 2048 - chip size
    }
#endif
#else
    if(pSBI->DII.ChipNum > 1)
    {
        WriteIO32( &pReg_Drex->MEMBASECONFIG[1],
            (pSBI->DII.ChipBase <<  16) |       // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
            (pSBI->DII.ChipMask <<   0));       // chip_mask[10:0]. 2048 - chip size
    }
#endif

    // Step 12. MemConfig
    WriteIO32( &pReg_Drex->MEMCONFIG[0],
//        (0x0    <<  16) |           // [31:16] Reserved - SBZ
        (0x1    <<  12) |           // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0:Linear(Bank, Row, Column, Width), 1:Interleaved(Row, bank, column, width), other : reserved
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
        WriteIO32( &pReg_Drex->MEMCONFIG[1],
//            (0x0    <<  16) |       // [31:16] Reserved - SBZ
            (0x1    <<  12) |       // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
            (chip_col   <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
            (chip_row   <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
            (0x3    <<   0));       // [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
#endif
#else
    if(pSBI->DII.ChipNum > 1) {
        WriteIO32( &pReg_Drex->MEMCONFIG[1],
//            (0x0    <<  16) |       // [31:16] Reserved - SBZ
            (0x1    <<  12) |       // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
            (pSBI->DII.ChipCol  <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
            (pSBI->DII.ChipRow  <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
            (0x3    <<   0));       // [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
    }
#endif

    // Step 13. Precharge Configuration
//    WriteIO32( &pReg_Drex->PRECHCONFIG,     0xFF000000 );           //- precharge policy counter
//    WriteIO32( &pReg_Drex->PWRDNCONFIG,     0xFFFF00FF );           //- low power counter



    // Step 14.  AC Timing
#if (CFG_NSIH_EN == 0)
    WriteIO32( &pReg_Drex->TIMINGAREF,
        (tREFI      <<   0));       //- refresh counter, 800MHz : 0x618

    WriteIO32( &pReg_Drex->TIMINGROW,
        (tRFC       <<  24) |
        (tRRD       <<  20) |
        (tRP        <<  16) |
        (tRCD       <<  12) |
        (tRC        <<   6) |
        (tRAS       <<   0)) ;
    WriteIO32( &pReg_Drex->TIMINGDATA,
        (tWTR       <<  28) |
        (tWR        <<  24) |
        (tRTP       <<  20) |
        (W2W_C2C    <<  16) |
        (R2R_C2C    <<  15) |
        (tDQSCK     <<  12) |
        (nWL        <<   8) |
        (nRL        <<   0));

    WriteIO32( &pReg_Drex->TIMINGPOWER,
        (tFAW       <<  26) |
        (tXSR       <<  16) |
        (tXP        <<   8) |
        (tCKE       <<   4) |
        (tMRD       <<   0));

//    WriteIO32( &pReg_Drex->TIMINGPZQ,   0x00004084 );     //- average periodic ZQ interval. Max:0x4084
    WriteIO32( &pReg_Drex->TIMINGPZQ,   tPZQ );           //- average periodic ZQ interval. Max:0x4084

    WriteIO32( &pReg_Drex->WRLVL_CONFIG[0],     (tWLO   <<   4) );      // tWLO[7:4]
#else

    WriteIO32( &pReg_Drex->TIMINGAREF,      pSBI->DII.TIMINGAREF );     //- refresh counter, 800MHz : 0x618
    WriteIO32( &pReg_Drex->TIMINGROW,       pSBI->DII.TIMINGROW) ;
    WriteIO32( &pReg_Drex->TIMINGDATA,      pSBI->DII.TIMINGDATA );
    WriteIO32( &pReg_Drex->TIMINGPOWER,     pSBI->DII.TIMINGPOWER );

//    WriteIO32( &pReg_Drex->TIMINGPZQ,       0x00004084 );               //- average periodic ZQ interval. Max:0x4084
    WriteIO32( &pReg_Drex->TIMINGPZQ,       pSBI->DII.TIMINGPZQ );      //- average periodic ZQ interval. Max:0x4084

    WriteIO32( &pReg_Drex->WRLVL_CONFIG[0],     (tWLO   <<   4) );      // tWLO[7:4]
#endif


#if 1   // fix - active
    WriteIO32( &pReg_Drex->MEMCONTROL,
        (0x0    <<  25) |           // [26:25] mrr_byte     : Mode Register Read Byte lane location
        (0x0    <<  24) |           // [   24] pzq_en       : DDR3 periodic ZQ(ZQCS) enable
//        (0x0    <<  23) |           // [   23] reserved     : SBZ
        (0x3    <<  20) |           // [22:20] bl           : Memory Burst Length                       :: 3'h3  - 8
#if (CFG_NSIH_EN == 0)
        ((_DDR_CS_NUM-1)        <<  16) |   // [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
#else
        ((pSBI->DII.ChipNum-1)  <<  16) |   // [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
#endif
        (0x2    <<  12) |           // [15:12] mem_width    : Width of Memory Data Bus                  :: 4'h2  - 32bits
#if defined(MEM_TYPE_DDR3)
        (0x6    <<   8) |           // [11: 8] mem_type     : Type of Memory                            :: 4'h6  - ddr3
#endif
#if defined(MEM_TYPE_LPDDR23)
        (0x7    <<   8) |           // [11: 8] mem_type     : Type of Memory                            :: 4'h7  - lpddr3
#endif
        (0x0    <<   6) |           // [ 7: 6] add_lat_pall : Additional Latency for PALL in cclk cycle :: 2'b00 - 0 cycle
        (0x0    <<   5) |           // [    5] dsref_en     : Dynamic Self Refresh                      :: 1'b0  - Disable
        (0x0    <<   4) |           // [    4] tp_en        : Timeout Precharge                         :: 1'b0  - Disable
        (0x0    <<   2) |           // [ 3: 2] dpwrdn_type  : Type of Dynamic Power Down                :: 2'b00 - Active/precharge power down
        (0x0    <<   1) |           // [    1] dpwrdn_en    : Dynamic Power Down                        :: 1'b0  - Disable
        (0x0    <<   0));           // [    0] clk_stop_en  : Dynamic Clock Control                     :: 1'b0  - Always running
#endif


#if 0
#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
#if (CFG_NSIH_EN == 0)
    WriteIO32( &pReg_DDRPHY->PHY_CON[4],    READDELAY);
    WriteIO32( &pReg_DDRPHY->PHY_CON[6],    WRITEDELAY);
#else
    WriteIO32( &pReg_DDRPHY->PHY_CON[4],    pSBI->DII.READDELAY);
    WriteIO32( &pReg_DDRPHY->PHY_CON[6],    pSBI->DII.WRITEDELAY);
#endif
#endif
#if defined(ARCH_NXP5430)
#if (CFG_NSIH_EN == 0)
    WriteIO32( &pReg_DDRPHY->OFFSETR_CON[0], READDELAY);
    WriteIO32( &pReg_DDRPHY->OFFSETW_CON[0], WRITEDELAY);
#else
    WriteIO32( &pReg_DDRPHY->OFFSETR_CON[0], pSBI->DII.READDELAY);
    WriteIO32( &pReg_DDRPHY->OFFSETW_CON[0], pSBI->DII.WRITEDELAY);
#endif
#endif
#endif

#if 0
    WriteIO32( &pReg_DDRPHY->PHY_CON[4],    0x08080808 );
    WriteIO32( &pReg_DDRPHY->PHY_CON[6],    0x08080808 );
#endif

#if (CFG_DDR_LOW_FREQ == 1)
    // 5. set CA0 ~ CA9 deskew code to 7h'60
    WriteIO32( &pReg_DDRPHY->PHY_CON[31+1], 0x0C183060 );   // PHY_CON31 DeSkew Code for CA
    WriteIO32( &pReg_DDRPHY->PHY_CON[32+1], 0x60C18306 );   // PHY_CON32 DeSkew Code for CA
    WriteIO32( &pReg_DDRPHY->PHY_CON[33+1], 0x00000030 );   // PHY_CON33 DeSkew Code for CA

    // Step 16: ctrl_offsetr0~3 = 0x7F, ctrl_offsetw0~3 = 0x7F
    WriteIO32( &pReg_DDRPHY->PHY_CON[4],    0x7F7F7F7F );
    WriteIO32( &pReg_DDRPHY->PHY_CON[6],    0x7F7F7F7F );

    // Step 17: ctrl_offsetd[7:0] = 0x7F
    WriteIO32( &pReg_DDRPHY->PHY_CON[10],   (0x7F   <<  0) );

    // Step 18: ctrl_force[14:8] = 0x7F
    SetIO32  ( &pReg_DDRPHY->PHY_CON[12],   (0x7F   <<  8) );

    // Step 19: ctrl_dll_on[5] = 0
    ClearIO32( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<  5) );

    // Step 20: Wait 10MPCLK
    DMC_Delay(100);
#endif  // #if (CFG_DDR_LOW_FREQ == 1)

    // Step 21, 22: Update DLL information
    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );  // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );  // Force DLL Resyncronization

    DMC_Delay(10);

    // Step 5. dfi_init_start : High
    SetIO32( &pReg_Drex->CONCONTROL,    (0x1    <<  28) );          // DFI PHY initialization start
    while( (ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1<<3) ) == 0);   // wait for DFI PHY initialization complete
    ClearIO32( &pReg_Drex->CONCONTROL,  (0x1    <<  28) );          // DFI PHY initialization clear

    // Step 52  auto refresh start.
//    SetIO32( &pReg_Drex->CONCONTROL,        (0x1    <<   5));            // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1

    // Step 16. Confirm that after RESET# is de-asserted, 500 us have passed before CKE becomes active.
    // Step 17. Confirm that clocks(CK, CK#) need to be started and
    //     stabilized for at least 10 ns or 5 tCK (which is larger) before CKE goes active.


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
    // Step 25 : Wait for minimum 200us
    DMC_Delay(100);


    // Step 26 : Send MR63 (RESET) command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 63, 0);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 63, 0);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 63, 0);
#endif


    // Step 27 : Wait for minimum 10us
    for (i = 0; i < 20000; i++)
    {
        SendDirectCommand(SDRAM_CMD_MRR, 0, 0, 0);                  // 0x9 = MRR (mode register reading), MR0_Device Information
        if ( ReadIO32( &pReg_Drex->MRSTATUS ) & (1 << 0) )          // OP0 = DAI (Device Auto-Initialization Status)
        {
            break;
        }
    }

#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    for (i = 0; i < 20000; i++)
    {
        SendDirectCommand(SDRAM_CMD_MRR, 1, 0, 0);                  // 0x9 = MRR (mode register reading), MR0_Device Information
        if ( ReadIO32( &pReg_Drex->MRSTATUS ) & (1 << 0) )          // OP0 = DAI (Device Auto-Initialization Status)
        {
            break;
        }
    }
#endif
#else
    if(pSBI->DII.ChipNum > 1)
    {
        for (i = 0; i < 20000; i++)
        {
            SendDirectCommand(SDRAM_CMD_MRR, 1, 0, 0);              // 0x9 = MRR (mode register reading), MR0_Device Information
            if ( ReadIO32( &pReg_Drex->MRSTATUS ) & (1 << 0) )      // OP0 = DAI (Device Auto-Initialization Status)
            {
                break;
            }
        }
    }
#endif

    // 12. Send MR10 command - DRAM ZQ calibration
    SendDirectCommand(SDRAM_CMD_MRS, 0, 10, 0xFF);                  // 0x0 = MRS/EMRS (mode register setting), MR10_Calibration, 0xFF: Calibration command after initialization
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 10, 0xFF);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 10, 0xFF);
#endif

    // 13. Wait for minimum 1us (tZQINIT).
    DMC_Delay(267); // MIN 1us


    // Step 20 :  Send MR2 command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 2, MR2.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 2, MR2.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 2, MR2.Reg);
#endif


    // Step 21 :  Send MR1 command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 1, MR1.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 1, MR1.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 1, MR1.Reg);
#endif


    // Step 22 :  Send MR3 command - I/O Configuration :: Drive Strength
    SendDirectCommand(SDRAM_CMD_MRS, 0, 3, MR3.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 3, MR3.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 3, MR3.Reg);
#endif

    // 14. Send MR11 command - ODT control
    SendDirectCommand(SDRAM_CMD_MRS, 0, 11, MR11.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 11, MR11.Reg);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 11, MR11.Reg);
#endif

    // Send MR16 PASR_Bank command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 16, 0xFF);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 16, 0xFF);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 16, 0xFF);
#endif

    // Send MR17 PASR_Seg command.
    SendDirectCommand(SDRAM_CMD_MRS, 0, 17, 0xFF);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
    SendDirectCommand(SDRAM_CMD_MRS, 1, 17, 0xFF);
#endif
#else
    if(pSBI->DII.ChipNum > 1)
        SendDirectCommand(SDRAM_CMD_MRS, 1, 17, 0xFF);
#endif

    DMC_Delay(267); // MIN 1us


#if (CFG_DDR_LOW_FREQ == 1)

#if 0
    // set CA0 ~ CA9 deskew code to 7h'00
    WriteIO32( &pReg_DDRPHY->PHY_CON[31+1], 0x00000000 );   // PHY_CON31 DeSkew Code for CA
    WriteIO32( &pReg_DDRPHY->PHY_CON[32+1], 0x00000000 );   // PHY_CON32 DeSkew Code for CA
    WriteIO32( &pReg_DDRPHY->PHY_CON[33+1], 0x00000000 );   // PHY_CON33 DeSkew Code for CA

    // Step 30: ctrl_offsetr0~3 = 0x00, ctrl_offsetw0~3 = 0x00
    WriteIO32( &pReg_DDRPHY->PHY_CON[4],    0x00000000 );
    WriteIO32( &pReg_DDRPHY->PHY_CON[6],    0x00000000 );

    // Step 31: ctrl_offsetd[7:0] = 0x00
    WriteIO32( &pReg_DDRPHY->PHY_CON[10],   0x00000000 );
#else

    // set CA0 ~ CA9 deskew code to 7h'08
    WriteIO32( &pReg_DDRPHY->PHY_CON[31+1], 0x81020408 );   // PHY_CON31 DeSkew Code for CA
    WriteIO32( &pReg_DDRPHY->PHY_CON[32+1], 0x08102040 );   // PHY_CON32 DeSkew Code for CA
    WriteIO32( &pReg_DDRPHY->PHY_CON[33+1], 0x00000004 );   // PHY_CON33 DeSkew Code for CA

    // Step 30: ctrl_offsetr0~3 = 0x08, ctrl_offsetw0~3 = 0x08
    WriteIO32( &pReg_DDRPHY->PHY_CON[4],    0x08080808 );
    WriteIO32( &pReg_DDRPHY->PHY_CON[6],    0x08080808 );

    // Step 31: ctrl_offsetd[7:0] = 0x08
    WriteIO32( &pReg_DDRPHY->PHY_CON[10],   0x00000008 );
#endif

    // Step 33 : Wait 10MPCLK
    DMC_Delay(100);

    // Step 34: ctrl_start[6] = 0
    ClearIO32( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<  6) );

    // Step 35 : Wait 10MPCLK
    SetIO32  ( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<  6) );

    // Step 36 : Wait 10MPCLK
    DMC_Delay(100);

    // Step 21, 22: Update DLL information
    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization

    // Step 23 : Wait 100ns
    DMC_Delay(10);
#endif  // #if (CFG_DDR_LOW_FREQ == 1)

    // Set PLL3.
#if (CFG_DDR_LOW_FREQ == 1)
#if defined(MEM_TYPE_LPDDR23)
    while(DebugIsBusy());
    setMemPLL(1);
#endif
#endif


#if 1   //(CONFIG_ODTOFF_GATELEVELINGON)
    MEMMSG("\r\n########## READ/GATE Level ##########\r\n");

#if (DDR_NEW_LEVELING_TRAINING == 0)
    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<   6) );          // ctrl_atgate=1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<  14) );          // p0_cmd_en=1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    <<   6) );          // InitDeskewEn=1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<  13) );          // byte_rdlvl_en=1
#else
    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<   6) );          // ctrl_atgate=1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    <<   6) );          // InitDeskewEn=1
#endif

    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1]) & ~(0xF <<  16);            // rdlvl_pass_adj=4
    temp |= (0x4 <<  16);
    WriteIO32( &pReg_DDRPHY->PHY_CON[1],        temp);

    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[2]) & ~(0x7F << 16);            // rdlvl_incr_adj=1
    temp |= (0x1 <<  16);
    WriteIO32( &pReg_DDRPHY->PHY_CON[2],        temp);


    do {
        SetIO32  ( &pReg_DDRPHY->PHY_CON[12],       (0x1    <<   5) );      // ctrl_dll_on[5]=1

        do {
            temp = ReadIO32( &pReg_DDRPHY->PHY_CON[13] );                   // read lock value
        } while( (temp & 0x7) != 0x7 );

        ClearIO32( &pReg_DDRPHY->PHY_CON[12],       (0x1    <<   5) );      // ctrl_dll_on[5]=0

        temp = ReadIO32( &pReg_DDRPHY->PHY_CON[13] );                       // read lock value
    } while( (temp & 0x7) != 0x7 );

    ClearIO32( &pReg_DDRPHY->PHY_CON[12],       (0x1    <<   5) );          // ctrl_dll_on[5]=0

    g_DDRLock = (temp >> 8) & 0x1FF;
    lock_div4 = (g_DDRLock >> 2);

    temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[12] );
    temp &= ~(0x7F <<  8);
    temp |= (lock_div4 <<  8);                                              // ctrl_force[14:8]
    WriteIO32( &pReg_DDRPHY->PHY_CON[12],   temp );

#if (DDR_NEW_LEVELING_TRAINING == 0)
    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  24) );              // rdlvl_gate_en=1

    SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<   8) );              // ctrl_shgate=1
    ClearIO32( &pReg_DDRPHY->PHY_CON[1],    (0xF    <<  20) );              // ctrl_gateduradj=0

    WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0x00000001 );                   // ctrl_rdlvl_data_en[1]=1, Gate Traning : Enable
    while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & 0x4000 ) != 0x4000 );      // Rdlvl_complete_ch0[14]=1

//    WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0x00000000 );                   //- ctrl_rdlvl_data_en[1]=0, Gate Traning : Disable
    WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0x00000001 );                   // LINARO

#if defined(MEM_TYPE_DDR3)
    WriteIO32( &pReg_DDRPHY->PHY_CON[14],   0x00000000 );                   // ctrl_pulld_dq[11:8]=0x0, ctrl_pulld_dqs[3:0]=0x0
#endif

    SetIO32  ( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<   6) );              // ctrl_start[6]=1
#else   // #if (DDR_NEW_LEVELING_TRAINING == 1)

    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization

#if (CONFIG_SET_MEM_TRANING_FROM_NSIH == 0)
#if (DDR_WRITE_LEVELING_EN == 1)
    DDR_Write_Leveling();
#endif
#if (DDR_CA_CALIB_EN == 1)
    DDR_CA_Calibration();
#endif
#if (DDR_GATE_LEVELING_EN == 1)
    DDR_Gate_Leveling(isResume);
#endif
#if (DDR_READ_DQ_CALIB_EN == 1)
    DDR_Read_DQ_Calibration(isResume);
#endif
#if (DDR_WRITE_DQ_CALIB_EN == 1)
    DDR_Write_DQ_Calibration(isResume);
#endif
#else   // #if (CONFIG_SET_MEM_TRANING_FROM_NSIH == 0)

//    if (pSBI->LvlTr_Mode & LVLTR_WR_LVL)
//        DDR_Write_Leveling();

#if (DDR_CA_CALIB_EN == 1)
    if (pSBI->LvlTr_Mode & LVLTR_CA_CAL)
    {
        DDR_CA_Calibration();
    }
#endif

#if (DDR_GATE_LEVELING_EN == 1)
    if (pSBI->LvlTr_Mode & LVLTR_GT_LVL)
        DDR_Gate_Leveling(isResume);
#endif

#if (DDR_READ_DQ_CALIB_EN == 1)
    if (pSBI->LvlTr_Mode & LVLTR_RD_CAL)
        DDR_Read_DQ_Calibration(isResume);
#endif

#if (DDR_WRITE_DQ_CALIB_EN == 1)
    if (pSBI->LvlTr_Mode & LVLTR_WR_CAL)
        DDR_Write_DQ_Calibration(isResume);
#endif
#endif  //#if (CONFIG_SET_MEM_TRANING_FROM_NSIH == 0)

#if defined(MEM_TYPE_DDR3)
    WriteIO32( &pReg_DDRPHY->PHY_CON[14],   0x00000000 );               // ctrl_pulld_dq[11:8]=0x0, ctrl_pulld_dqs[3:0]=0x0
#endif
    ClearIO32( &pReg_DDRPHY->PHY_CON[0],    (0x3    <<  13) );          // p0_cmd_en[14]=0, byte_rdlvl_en[13]=0
#endif  // #if (DDR_NEW_LEVELING_TRAINING == 1)

    SetIO32  ( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<   5) );          // ctrl_dll_on[5]=1
    SetIO32  ( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  12));           // DLLDeskewEn[2]=1

#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
    SetIO32  ( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24));           // ctrl_resync=1
    ClearIO32( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24));           // ctrl_resync=0
#endif

#endif  // gate leveling



    SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3));           // Force DLL Resyncronization
    ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3));           // Force DLL Resyncronization

    temp = (U32)(
//        (0x0    <<  29) |           // [31:29] Reserved - SBZ.
        (0x17   <<  24) |           // [28:24] T_WrWrCmd.
//        (0x0    <<  22) |           // [23:22] Reserved - SBZ.
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
#if defined(MEM_TYPE_LPDDR23)
        (0x3    <<  11) |           // [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
#endif
//        (0x0    <<  10) |           // [   10] Reserved - SBZ.
        (0x1    <<   9) |           // [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
#if (DDR_GATE_LEVELING_EN == 1)
//        (0x1    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
#endif
#if defined(MEM_TYPE_LPDDR23)
//        (0x1    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
#endif
        (0x0    <<   7) |           // [    7] ctrl_ckdis. 0:Clock output Enable, 1:Disable
//        (0x1    <<   6) |           // [    6] ctrl_atgate.
#if defined(MEM_TYPE_DDR3)
        (0x1    <<   5) |           // [    5] ctrl_read_disable. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
#endif
        (0x0    <<   4) |           // [    4] ctrl_cmosrcv.
        (0x0    <<   3) |           // [    3] ctrl_read_width.
        (0x0    <<   0));           // [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

#if (CFG_NSIH_EN == 1)
    if ((pSBI->DII.TIMINGDATA >> 28) == 3)      // 6 cycles
        temp |= (0x7    <<  17);
    else if ((pSBI->DII.TIMINGDATA >> 28) == 2) // 4 cycles
        temp |= (0x6    <<  17);
#endif

    WriteIO32( &pReg_DDRPHY->PHY_CON[0],    temp );


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

    WriteIO32( &pReg_Drex->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
        (0x1    <<  31) |           // [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
        (0x1    <<  30) |           // [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
        (0x0    <<   0) |           // [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
#if defined(MEM_TYPE_DDR3)
        (0x1    <<  29) |           // [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
#endif
        (0x0    <<  24) |           // [28:24] ctrl_pd. Input Gate for Power Down.
//        (0x0    <<   7) |           // [23: 7] reserved - SBZ
        (0x0    <<   4) |           // [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
        (0x0    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
        (0x0    <<   1));           // [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1

    WriteIO32( &pReg_Drex->CONCONTROL,
        (0x0    <<  28) |           // [   28] dfi_init_start
        (0xFFF  <<  16) |           // [27:16] timeout_level0
//        (0x3    <<  12) |           // [14:12] rd_fetch
        (0x1    <<  12) |           // [14:12] rd_fetch
        (0x1    <<   8) |           // [    8] empty
        (0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
        (0x0    <<   3) |           // [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
        (0x0    <<   1));           // [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved


    printf("\r\n");

    printf("Lock value  = %d\r\n",      g_DDRLock );

    printf("CA CAL CODE = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[10] ) );

    printf("GATE CYC    = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[3] ) );
    printf("GATE CODE   = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[8] ) );

    printf("Read  DQ    = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[4] ) );
    printf("Write DQ    = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[6] ) );
}

