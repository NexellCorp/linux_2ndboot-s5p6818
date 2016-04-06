/*
 *      Copyright (C) 2012 Nexell Co., All Rights Reserved
 *      Nexell Co. Proprietary & Confidential
 *
 *      NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *      AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *      BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
 *FITNESS
 *      FOR A PARTICULAR PURPOSE.
 *
 *      Module          : PMIC AXP228
 *      File            : pmic_axp228.h
 *      Description     :
 *      Author          :
 *      History         :
 */

#ifndef __PMIC_AXP228_H__
#define __PMIC_AXP228_H__

#define AXP228_DEF_DDC1_VOL_MIN 1600000 /* UINT = 1uV, 1.6V */
#define AXP228_DEF_DDC1_VOL_MAX 3400000 /* UINT = 1uV, 3.4V */

#define AXP228_DEF_DDC234_VOL_MIN 600000 /* UINT = 1uV, 0.60V */
#define AXP228_DEF_DDC24_VOL_MAX 1540000 /* UINT = 1uV, 1.54V */
#define AXP228_DEF_DDC3_VOL_MAX 1860000  /* UINT = 1uV, 1.86V */
#define AXP228_DEF_DDC5_VOL_MIN 1050000  /* UINT = 1uV, 1.05V */
#define AXP228_DEF_DDC5_VOL_MAX 2600000  /* UINT = 1uV, 2.60V */

#define AXP228_DEF_DDC1_VOL_STEP 100000  /* UINT = 1uV, 100.0mV */
#define AXP228_DEF_DDC234_VOL_STEP 20000 /* UINT = 1uV, 20.0mV */
#define AXP228_DEF_DDC5_VOL_STEP 50000   /* UINT = 1uV, 50.0mV */

//#define AXP228_DEF_DDC2_VOL             1200000 /* VAL(uV) = 0: 0.60 ~ 1.54V,
//Step 20 mV, default(OTP) = 1.1V */
#define AXP228_DEF_DDC2_VOL                                                    \
	1250000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 20 mV, default(OTP) = 1.1V  \
		   */
#define AXP228_DEF_DDC4_VOL                                                    \
	1500000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 20 mV, default(OTP) = 1.5V  \
		   */
#define AXP228_DEF_DDC5_VOL                                                    \
	1500000 /* VAL(uV) = 0: 0.60 ~ 1.54V, Step 50 mV, default(OTP) = 1.5V  \
		   */

#define AXP228_REG_DC1VOL 0x21
#define AXP228_REG_DC2VOL 0x22
#define AXP228_REG_DC3VOL 0x23
#define AXP228_REG_DC4VOL 0x24
#define AXP228_REG_DC5VOL 0x25

#define DCDC_SYS (1 << 3) // VCC1P5_SYS
#define DCDC_DDR (1 << 4) // VCC1P5_DDR

#define I2C_ADDR_AXP228 (0x68 >> 1) // DroneL

#endif // ifdef __PMIC_AXP228_H__
