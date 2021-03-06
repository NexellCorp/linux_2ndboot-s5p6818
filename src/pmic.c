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
//  File            : pmic.c
//  Description     :
//  Author          : Hans
//  History         :
//          2015-05-21  Hans
////////////////////////////////////////////////////////////////////////////////

#include "sysHeader.h"
void DMC_Delay(int milisecond);

#define SET_AUTO_VOLTAGE_CONTROL_FOR_CORE   (1)
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))


//------------------------------------------------------------------------------
#if defined( INITPMIC_YES )
#define MP8845C_REG_VSEL                0x00
#define MP8845C_REG_SYSCNTL1            0x01
#define MP8845C_REG_SYSCNTL2            0x02
#define MP8845C_REG_ID1                 0x03
#define MP8845C_REG_ID2                 0x04
#define MP8845C_REG_STATUS              0x05

#define AXP228_DEF_DDC1_VOL_MIN         1600000 /* UINT = 1uV, 1.6V */
#define AXP228_DEF_DDC1_VOL_MAX         3400000 /* UINT = 1uV, 3.4V */

#define AXP228_DEF_DDC234_VOL_MIN       660000  /* UINT = 1uV, 0.66V */
#define AXP228_DEF_DDC24_VOL_MAX        1540000 /* UINT = 1uV, 1.54V */
#define AXP228_DEF_DDC3_VOL_MAX         1860000 /* UINT = 1uV, 1.86V */
#define AXP228_DEF_DDC5_VOL_MIN         1050000 /* UINT = 1uV, 1.05V */
#define AXP228_DEF_DDC5_VOL_MAX         2600000 /* UINT = 1uV, 2.60V */

#define AXP228_DEF_DDC1_VOL_STEP        100000  /* UINT = 1uV, 100.0mV */
#define AXP228_DEF_DDC234_VOL_STEP      20000   /* UINT = 1uV, 20.0mV */
#define AXP228_DEF_DDC5_VOL_STEP        50000   /* UINT = 1uV, 50.0mV */

//#define AXP228_DEF_DDC2_VOL             1200000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 20 mV, default(OTP) = 1.1V */
#define AXP228_DEF_DDC2_VOL             1300000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 20 mV, default(OTP) = 1.1V */
#define AXP228_DEF_DDC4_VOL             1350000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 20 mV, default(OTP) = 1.5V */
#define AXP228_DEF_DDC5_VOL             1350000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 50 mV, default(OTP) = 1.5V */

#define AXP228_REG_DC1VOL               0x21
#define AXP228_REG_DC2VOL               0x22
#define AXP228_REG_DC3VOL               0x23
#define AXP228_REG_DC4VOL               0x24
#define AXP228_REG_DC5VOL               0x25


#define NXE2000_DEF_DDCx_VOL_MIN        600000  /* UINT = 1uV, 0.6V */
#define NXE2000_DEF_DDCx_VOL_MAX        3500000 /* UINT = 1uV, 3.5V */
#define NXE2000_DEF_DDCx_VOL_STEP       12500   /* UINT = 1uV, 12.5mV */

#define NXE2000_DEF_DDC1_VOL            1100000 /* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.3V */
#define NXE2000_DEF_DDC2_VOL            1100000 /* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.1V */
#define NXE2000_DEF_DDC3_VOL            3300000 /* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 3.3V */
#define NXE2000_DEF_DDC4_VOL            1500000 /* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.6V */
#define NXE2000_DEF_DDC5_VOL            1500000 /* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.6V */

#define NXE2000_REG_DC1VOL              0x36
#define NXE2000_REG_DC2VOL              0x37
#define NXE2000_REG_DC3VOL              0x38
#define NXE2000_REG_DC4VOL              0x39
#define NXE2000_REG_DC5VOL              0x3A


#define I2C_ADDR_NXE2000   (0x64 >> 1)  // SVT & ASB
#define I2C_ADDR_MP8845    (0x38 >> 1)  // SVT & ASB
#define I2C_ADDR_AXP228    (0x68 >> 1)  // DroneL

#define DRONE_PMIC_INIT
//#define BF700_PMIC_INIT
//#define SVT_PMIC_INIT
//#define ASB_PMIC_INIT

#define AXP_I2C_GPIO_GRP            (-1)
#define NXE2000_I2C_GPIO_GRP        (-1)

#ifdef DRONE_PMIC_INIT
#undef  AXP_I2C_GPIO_GRP
#define AXP_I2C_GPIO_GRP            3   // D group
#define AXP_I2C_SCL                 20
#define AXP_I2C_SDA                 16
#endif

#ifdef BF700_PMIC_INIT
#define MP8845_CORE_I2C_GPIO_GRP    4   // E group, FineDigital VDDA_1.2V (core)
#define MP8845_CORE_I2C_SCL         9
#define MP8845_CORE_I2C_SDA         8

#define MP8845_ARM_I2C_GPIO_GRP     4   // E group, FineDigital VDDB_1.2V (arm)
#define MP8845_ARM_I2C_SCL          11
#define MP8845_ARM_I2C_SDA          10

#define MP8845_PMIC_INIT            (1)
#endif

#ifdef SVT_PMIC_INIT
#undef  NXE2000_I2C_GPIO_GRP
#define NXE2000_I2C_GPIO_GRP        3   // D group, VCC1P0_CORE, NXE2000, MP8845
#define NXE2000_I2C_SCL             6
#define NXE2000_I2C_SDA             7

#define MP8845_I2C_GPIO_GRP         3   // D group , VCC1P0_ARM, MP8845
#define MP8845_I2C_SCL              2
#define MP8845_I2C_SDA              3

#define MP8845_PMIC_INIT            (1)
#endif

#ifdef ASB_PMIC_INIT
#undef  NXE2000_I2C_GPIO_GRP
#define NXE2000_I2C_GPIO_GRP        3   // D group, VCC1P0_ARM, NXE2000, MP8845
#define NXE2000_I2C_SCL             2
#define NXE2000_I2C_SDA             3

#define MP8845_I2C_GPIO_GRP         3   // D group , VCC1P0_CORE, MP8845
#define MP8845_I2C_SCL              6
#define MP8845_I2C_SDA              7

#define MP8845_PMIC_INIT            (1)
#endif

extern void  I2C_Init( U8 gpioGRP, U8 gpioSCL, U8 gpioSDA );
//extern void  I2C_Deinit( void );
extern CBOOL I2C_Read(U8 DeviceAddress, U8 RegisterAddress, U8* pData, U32 Length);
extern CBOOL I2C_Write(U8 DeviceAddress, U8 RegisterAddress, U8* pData, U32 Length);


#if (MP8845_PMIC_INIT == 1)
static const U8 MP8845_mV_list[] = {
    90, // 12021
    86, // 11753
    83, // 11553
    75, // 11018
    68  // 10549
};

#define MP8845_VOUT_ARRAY_SIZE  (int)(sizeof(MP8845_mV_list)/sizeof(MP8845_mV_list[0]))
#endif  // #if (MP8845_PMIC_INIT == 1)

#if (SET_AUTO_VOLTAGE_CONTROL_FOR_CORE == 1)
struct vdd_core_tb_info {
    int ids;
    int ro;
    int mV;
};

static const struct vdd_core_tb_info vdd_core_tables[] = {
    [0] = { .ids = 6,  .ro = 70,  .mV = 1200 },
    [1] = { .ids = 12, .ro = 110, .mV = 1175 },
    [2] = { .ids = 30, .ro = 150, .mV = 1150 },
    [3] = { .ids = 60, .ro = 180, .mV = 1100 },
    [4] = { .ids = 60, .ro = 180, .mV = 1050 }
};

#define VDD_CORE_ARRAY_SIZE     (int)(sizeof(vdd_core_tables)/sizeof(vdd_core_tables[0]))

static inline U32 MtoL(U32 data, int bits)
{
    U32 result = 0, mask = 1;
    int i = 0;

    for (i = 0; i < bits ; i++) {
        if (data & (1 << i))
            result |= mask << (bits-i-1);
    }

    return result;
}

int getASVIndex(U32 ecid_1)
{
    const struct vdd_core_tb_info *tb = &vdd_core_tables[0];
    int field = 0;
    int ids;
    int ro;
    int ids_L, ro_L;
    int i = 0;

    ids = MtoL((ecid_1>>16) & 0xFF, 8);
    ro  = MtoL((ecid_1>>24) & 0xFF, 8);

    /* find ids Level */
    for (i = 0; i < VDD_CORE_ARRAY_SIZE; i++) {
        tb = &vdd_core_tables[i];
        if (ids <= tb->ids)
            break;
    }
    ids_L = i < VDD_CORE_ARRAY_SIZE ? i : (VDD_CORE_ARRAY_SIZE-1);

    /* find ro Level */
    for (i = 0; i < VDD_CORE_ARRAY_SIZE; i++) {
        tb = &vdd_core_tables[i];
        if (ro <= tb->ro)
            break;
    }
    ro_L = i < VDD_CORE_ARRAY_SIZE ? i : (VDD_CORE_ARRAY_SIZE-1);

    /* find Lowest ASV Level */
    field = ids_L > ro_L ? ro_L : ids_L;

    return field;
}
#endif  // #if (SET_AUTO_VOLTAGE_CONTROL_FOR_CORE == 1)

#if (AXP_I2C_GPIO_GRP > -1)
static U8 axp228_get_dcdc_step(int want_vol, int step, int min, int max)
{
    U32 vol_step = 0;

    if (want_vol < min)
    {
        want_vol = min;
    }
    else if (want_vol > max)
    {
        want_vol = max;
    }

    vol_step = (want_vol - min + step - 1) / step;

    return (U8)(vol_step & 0xFF);
}
#endif

#if (NXE2000_I2C_GPIO_GRP > -1)
static U8 nxe2000_get_dcdc_step(int want_vol)
{
    U32 vol_step = 0;

    if (want_vol < NXE2000_DEF_DDCx_VOL_MIN)
    {
        want_vol = NXE2000_DEF_DDCx_VOL_MIN;
    }
    else if (want_vol > NXE2000_DEF_DDCx_VOL_MAX)
    {
        want_vol = NXE2000_DEF_DDCx_VOL_MAX;
    }

    vol_step    = (want_vol - NXE2000_DEF_DDCx_VOL_MIN + NXE2000_DEF_DDCx_VOL_STEP - 1) / NXE2000_DEF_DDCx_VOL_STEP;

    return (U8)(vol_step & 0xFF);
}
#endif

#ifdef DRONE_PMIC_INIT
#if (SET_AUTO_VOLTAGE_CONTROL_FOR_CORE == 1)
inline void PMIC_Drone(void)
{
    U8 pData[4];
#define DCDC_SYS    (1<<3)    // VCC1P5_SYS
#define DCDC_DDR    (1<<4)    // VCC1P5_DDR

    I2C_Init(AXP_I2C_GPIO_GRP, AXP_I2C_SCL, AXP_I2C_SDA);

    I2C_Read(I2C_ADDR_AXP228, 0x80, pData, 1);
//      printf("I2C addr 80 %X\r\n", pData[0]);
    pData[0] = (pData[0] & 0x1F) | DCDC_SYS | DCDC_DDR;
    I2C_Write(I2C_ADDR_AXP228, 0x80, pData, 1);

    // Set bridge DCDC2 and DCDC3
    I2C_Read(I2C_ADDR_AXP228, 0x37, pData, 1);
    pData[0] |= 0x10;
    I2C_Write(I2C_ADDR_AXP228, 0x37, pData, 1);

    // Set voltage.
//    pData[0] = axp228_get_dcdc_step(AXP228_DEF_DDC2_VOL, AXP228_DEF_DDCx_VOL_STEP, AXP228_DEF_DDC2345_VOL_MIN, AXP228_DEF_DDC245_VOL_MAX);
//    I2C_Write(I2C_ADDR_AXP228, AXP228_REG_DC2VOL, pData, 1);

#if 1
    // Set voltage of DCDC4.
    pData[0] = axp228_get_dcdc_step(AXP228_DEF_DDC4_VOL, AXP228_DEF_DDC234_VOL_STEP, AXP228_DEF_DDC234_VOL_MIN, AXP228_DEF_DDC24_VOL_MAX);
    I2C_Write(I2C_ADDR_AXP228, AXP228_REG_DC4VOL, pData, 1);

    // Set voltage of DCDC5.
    pData[0] = axp228_get_dcdc_step(AXP228_DEF_DDC5_VOL, AXP228_DEF_DDC5_VOL_STEP, AXP228_DEF_DDC5_VOL_MIN, AXP228_DEF_DDC5_VOL_MAX);
    I2C_Write(I2C_ADDR_AXP228, AXP228_REG_DC5VOL, pData, 1);
#endif
}
#else
inline void PMIC_Drone(void)
{
    U8 pData[4];
#define DCDC_SYS    (1<<3)  // VCC1P5_SYS
#define DCDC_DDR    (1<<4)  // VCC1P5_DDR

    I2C_Init(AXP_I2C_GPIO_GRP, AXP_I2C_SCL, AXP_I2C_SDA);

    // PFM -> PWM mode
#if 0
    I2C_Read(I2C_ADDR_AXP228, 0x80, pData, 1);
//    printf("I2C addr 80 %X\r\n", pData[0]);
    pData[0] = (pData[0] & 0x1F) | DCDC_SYS | DCDC_DDR;
    I2C_Write(I2C_ADDR_AXP228, 0x80, pData, 1);
#endif

    //
    // ARM voltage change
    //

    // Set bridge DCDC2 and DCDC3
    I2C_Read(I2C_ADDR_AXP228, 0x37, pData, 1);
    pData[0] |= 0x10;
    I2C_Write(I2C_ADDR_AXP228, 0x37, pData, 1);

    // Set voltage.
    pData[0] = axp228_get_dcdc_step(AXP228_DEF_DDC2_VOL, AXP228_DEF_DDCx_VOL_STEP, AXP228_DEF_DDC2345_VOL_MIN, AXP228_DEF_DDC245_VOL_MAX);
    I2C_Write(I2C_ADDR_AXP228, AXP228_REG_DC2VOL, pData, 1);
}
#endif
#endif     // DRONE

#ifdef BF700_PMIC_INIT
#if (SET_AUTO_VOLTAGE_CONTROL_FOR_CORE == 1)
inline void PMIC_BF7000(void)
{
    U8 pData[4];
    U32 ecid_1 = ReadIO32(PHY_BASEADDR_ECID_MODULE + (1<<2));
    int asv_idx = getASVIndex(ecid_1);
    //
    // I2C init for CORE power.
    //
    I2C_Init(MP8845_CORE_I2C_GPIO_GRP, MP8845_CORE_I2C_SCL, MP8845_CORE_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // Core voltage change
    //
#if (MP8845_PMIC_INIT == 1)
    if (ecid_1)
    {
        U8 Data;
        printf("ecid:%x, asv index:%d\r\n", ecid_1, asv_idx);
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
        pData[0] |= 1<<5;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

        pData[0] = MP8845_mV_list[asv_idx] | 1<<7;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
        Data = pData[0];
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
        if(Data != pData[0])
        {
            printf("verify core voltage code write:%d, read:%d\r\n", Data, pData[0]);
        }
    }
#endif

    //
    // I2C init for ARM power.
    //
    I2C_Init(MP8845_ARM_I2C_GPIO_GRP, MP8845_ARM_I2C_SCL, MP8845_ARM_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // ARM voltage change
    //
#if (MP8845_PMIC_INIT == 1)
    if (ecid_1)
    {
        U8 Data;
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
        pData[0] |= 1<<5;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

        pData[0] = MP8845_mV_list[asv_idx] | 1<<7;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
        Data = pData[0];
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
        if(Data != pData[0])
        {
            printf("verify arm voltage code write:%d, read:%d\r\n", Data, pData[0]);
        }
    }
#endif
}
#else
inline void PMIC_BF7000(void)
{
    U8 pData[4];

    //
    // I2C init for CORE power.
    //
    I2C_Init(MP8845_CORE_I2C_GPIO_GRP, MP8845_CORE_I2C_SCL, MP8845_CORE_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // Core voltage change
    //
#if 0
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
    pData[0] |= 1<<5;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

    pData[0] = 90 | 1<<7;     // 90: 1.2V
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
#endif

    //
    // I2C init for ARM power.
    //
    I2C_Init(MP8845_ARM_I2C_GPIO_GRP, MP8845_ARM_I2C_SCL, MP8845_ARM_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // ARM voltage change
    //
#if 0
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
    pData[0] |= 1<<5;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

    pData[0] = 90 | 1<<7;     // 90: 1.2V
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
#endif
}
#endif
#endif     // BF700

#ifdef SVT_PMIC_INIT
#if (SET_AUTO_VOLTAGE_CONTROL_FOR_CORE == 1)
inline void PMIC_SVT(void)
{
    U8 pData[4];
    U32 ecid_1 = ReadIO32(PHY_BASEADDR_ECID_MODULE + (1<<2));
    int asv_idx = getASVIndex(ecid_1);
    const struct vdd_core_tb_info *vdd_tb = &vdd_core_tables[asv_idx];

    //
    // I2C init for CORE & NXE2000 power.
    //
    I2C_Init(NXE2000_I2C_GPIO_GRP, NXE2000_I2C_SCL, NXE2000_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // Core voltage change
    //
#if (MP8845_PMIC_INIT == 1)
    if (ecid_1)
    {
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
        pData[0] |= 1<<5;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

        pData[0] = MP8845_mV_list[asv_idx] | 1<<7;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
    }
#endif

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC1_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC1VOL, pData, 1);

    pData[0] = nxe2000_get_dcdc_step(vdd_tb->mV * 1000);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC2VOL, pData, 1);  // Core - second power

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC4_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC4VOL, pData, 1);

//    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC5_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC5VOL, pData, 1);
}
#else
inline void PMIC_SVT(void)
{
    U8 pData[4];

    //
    // I2C init for CORE & NXE2000 power.
    //
    I2C_Init(NXE2000_I2C_GPIO_GRP, NXE2000_I2C_SCL, NXE2000_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // Core voltage change
    //
#if 0
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
    pData[0] |= 1<<5;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

    pData[0] = 90 | 1<<7;     // 90: 1.2V
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
#endif

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC1_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC1VOL, pData, 1);

//      pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC2_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC2VOL, pData, 1);

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC4_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC4VOL, pData, 1);

//    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC5_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC5VOL, pData, 1);
}
#endif
#endif     // SVT

#ifdef ASB_PMIC_INIT
#if (SET_AUTO_VOLTAGE_CONTROL_FOR_CORE == 1)
inline void PMIC_ASB(void)
{
    U8 pData[4];
    U32 ecid_1 = ReadIO32(PHY_BASEADDR_ECID_MODULE + (1<<2));
    int asv_idx = getASVIndex(ecid_1);
    const struct vdd_core_tb_info *vdd_tb = &vdd_core_tables[asv_idx];

    //
    // I2C init for Core power.
    //
    I2C_Init(MP8845_I2C_GPIO_GRP, MP8845_I2C_SCL, MP8845_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // Core voltage change
    //
#if (MP8845_PMIC_INIT == 1)
    if (ecid_1)
    {
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
        pData[0] |= 1<<5;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

        pData[0] = MP8845_mV_list[asv_idx] | 1<<7;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
    }
#endif

    //
    // I2C init for ARM & NXE2000 power.
    //
    I2C_Init(NXE2000_I2C_GPIO_GRP, NXE2000_I2C_SCL, NXE2000_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // ARM voltage change
    //
#if 1
#if (MP8845_PMIC_INIT == 1)
    if (ecid_1)
    {
        I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
        pData[0] |= 1<<5;
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

//        pData[0] = 90 | 1<<7;    // 90: 1.2V
        pData[0] = 80 | 1<<7;    // 80: 1.135V
        I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
    }
#endif
#endif

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC1_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC1VOL, pData, 1);

    pData[0] = nxe2000_get_dcdc_step(vdd_tb->mV * 1000);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC2VOL, pData, 1);  // Core - second power

    //
    // ARM voltage change
    //
#if 1
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
    pData[0] |= 1<<5;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

    pData[0] = 90 | 1<<7;    // 90: 1.2V
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
#endif
}
#else
inline void PMIC_ASB(void)
{
    U8 pData[4];

    //
    // I2C init for CORE power.
    //
    I2C_Init(MP8845_I2C_GPIO_GRP, MP8845_I2C_SCL, MP8845_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // Core voltage change
    //
#if 0
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
    pData[0] |= 1<<5;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

    pData[0] = 90 | 1<<7;    // 90: 1.20V
//    pData[0] = 83 | 1<<7;    // 83: 1.15V
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
#endif

    //
    // I2C init for ARM & NXE2000 power.
    //
    I2C_Init(NXE2000_I2C_GPIO_GRP, NXE2000_I2C_SCL, NXE2000_I2C_SDA);

    // PFM -> PWM mode
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);
    pData[0] |= 1<<0;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL1, pData, 1);

    //
    // ARM voltage change
    //
#if 0
    I2C_Read(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);
    pData[0] |= 1<<5;
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_SYSCNTL2, pData, 1);

    pData[0] = 90 | 1<<7;    // 90: 1.20V
//    pData[0] = 83 | 1<<7;    // 83: 1.15V
    I2C_Write(I2C_ADDR_MP8845, MP8845C_REG_VSEL, pData, 1);
#endif

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC1_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC1VOL, pData, 1);

//    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC2_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC2VOL, pData, 1);

    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC4_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC4VOL, pData, 1);

//    pData[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC5_VOL);
    I2C_Write(I2C_ADDR_NXE2000, NXE2000_REG_DC5VOL, pData, 1);
}
#endif
#endif     // ASB

void initPMIC(void)
{
#ifdef DRONE_PMIC_INIT
    PMIC_Drone();
#endif   // DRONE

#ifdef BF700_PMIC_INIT
    PMIC_BF7000();
#endif   // BF700

#ifdef SVT_PMIC_INIT
    PMIC_SVT();
#endif   // SVT

#ifdef ASB_PMIC_INIT
    PMIC_ASB();
#endif   // ASB

    DMC_Delay(100 * 1000);
}
#endif  // #if defined( INITPMIC_YES )

