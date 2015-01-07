//-----------------------------------------------------------------------------
//	Copyright (C) 2012 Nexell Co., All Rights Reserved
//	Nexell Co. Proprietary < Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//	AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//	BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//	FOR A PARTICULAR PURPOSE.
//
//	Module		: second boot
//	File		: secondboot.h
//	Description	: This must be synchronized with NSIH.txt
//	Author		: Firmware Team
//	History		:
// 				Hans 2013-06-23 Create
//------------------------------------------------------------------------------
#ifndef __NX_SECONDBOOT_H__
#define __NX_SECONDBOOT_H__

#include "cfgBootDefine.h"


#define HEADER_ID       ((((U32)'N')<< 0) | (((U32)'S')<< 8) | (((U32)'I')<<16) | (((U32)'H')<<24))

enum
{
    BOOT_FROM_USB   = 0UL,
    BOOT_FROM_SPI   = 1UL,
    BOOT_FROM_NAND  = 2UL,
    BOOT_FROM_SDMMC = 3UL,
    BOOT_FROM_SDFS  = 4UL,
    BOOT_FROM_UART  = 5UL
};

struct NX_NANDBootInfo
{
    U8  AddrStep;
    U8  tCOS;
    U8  tACC;
    U8  tOCH;
    U32 PageSize    : 24;
    U32 LoadDevice  : 8;
    U32 CRC32;
};

struct NX_SPIBootInfo
{
    U8  AddrStep;
    U8  _Reserved0[3];
    U32 _Reserved1  : 24;
    U32 LoadDevice  : 8;
    U32 CRC32;
};

struct NX_UARTBootInfo
{
    U32 _Reserved0;
    U32 _Reserved1  : 24;
    U32 LoadDevice  : 8;
    U32 CRC32;
};

struct NX_SDMMCBootInfo
{
    U8  PortNumber;
    U8  _Reserved0[3];
    U32 _Reserved1  : 24;
    U32 LoadDevice  : 8;
    U32 CRC32;
};

struct NX_SDFSBootInfo
{
    char BootFileName[12];		// 8.3 format ex)"NXDATA.TBL"
};

union NX_DeviceBootInfo
{
    struct NX_NANDBootInfo  NANDBI;
    struct NX_SPIBootInfo   SPIBI;
    struct NX_SDMMCBootInfo SDMMCBI;
    struct NX_SDFSBootInfo  SDFSBI;
    struct NX_UARTBootInfo  UARTBI;
};

struct NX_DDRInitInfo
{
	U8  ChipNum;		// 0x88
	U8  ChipRow;		// 0x89
	U8  BusWidth;		// 0x8A
	U8  ChipCol;		// 0x8B

	U16 ChipMask;		// 0x8C
	U16 ChipBase;		// 0x8E

#if 0
	U8  CWL;			// 0x90
	U8  WL;				// 0x91
	U8  RL;				// 0x92
	U8  DDRRL;			// 0x93
#else
	U8  CWL;			// 0x90
	U8  CL;				// 0x91
	U8  MR1_AL;			// 0x92
	U8  MR0_WR;			// 0x93
#endif

	U32 READDELAY;		// 0x94
	U32 WRITEDELAY;		// 0x98

	U32 TIMINGPZQ;		// 0x9C
	U32 TIMINGAREF;		// 0xA0
	U32 TIMINGROW;		// 0xA4
	U32 TIMINGDATA;		// 0xA8
	U32 TIMINGPOWER;	// 0xAC
};
struct NX_SecondBootInfo
{
	U32 VECTOR[8];				// 0x000 ~ 0x01C
	U32 VECTOR_Rel[8];			// 0x020 ~ 0x03C

	U32 DEVICEADDR;				// 0x040

	U32 LOADSIZE;				// 0x044
	U32 LOADADDR;				// 0x048
	U32 LAUNCHADDR;				// 0x04C
	union NX_DeviceBootInfo DBI;	// 0x050~0x058

	U32 PLL[4];					// 0x05C ~ 0x068
	U32 PLLSPREAD[2];			// 0x06C ~ 0x070

#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
	U32 DVO[5];					// 0x074 ~ 0x084

	struct NX_DDRInitInfo DII;	// 0x088 ~ 0x0AC

	U32 Stub[(0x1F0-0x0B0)/4];	// 0x0B0 ~ 0x1EC
#endif
#if defined(ARCH_NXP5430)
	U32 DVO[9];					// 0x074 ~ 0x094

	struct NX_DDRInitInfo DII;	// 0x098 ~ 0x0BC

	U32 Stub[(0x1F0-0x0C0)/4];	// 0x0C0 ~ 0x1EC
#endif

	U32 MemTestAddr;			// 0x1F0
	U32 MemTestSize;			// 0x1F4
	U32 MemTestTryCount;		// 0x1F8

	U32 SIGNATURE;				// 0x1FC	"NSIH"
};

// [0] : Use ICache
// [1] : Change PLL
// [2] : Decrypt
// [3] : Suspend Check

#endif //__NX_SECONDBOOT_H__
