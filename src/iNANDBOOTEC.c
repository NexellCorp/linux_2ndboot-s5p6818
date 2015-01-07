//------------------------------------------------------------------------------
//
//	Copyright (C) 2009 Nexell Co. All Rights Reserved
//	NEXELL Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//	AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//	BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//	FOR A PARTICULAR PURPOSE.
//
//	Module		: iROMBOOT
//	File		: iNANDBOOT.c
//	Description	: Bootloader for NAND flash memory with H/W BCH error detection.
//	Author		: Goofy
//	History		:
//		2010-12-08	matin	simulation
//		2010-10-15	Hans	Add 24bit Error Correction.
//		2008-10-23	Goofy	Get reset configuration from option argument
//		2008-08-20	Goofy	Create
//------------------------------------------------------------------------------
//#undef NX_DEBUG

#include "sysHeader.h"

#include <nx_mcus.h>


//------------------------------------------------------------------------------
#ifdef DEBUG
#define dprintf(x, ...)	printf(x, ...)
#else
#define dprintf(x, ...)
#endif

#define ROW_START				(0)

#define PENDDELAY				0x10000000

//------------------------------------------------------------------------------
// Registers
struct  NX_NAND_RegisterSet
{
	volatile U32 NFDATA			;	///< 00h
	volatile U32 __Reserved0[3]	;	///< 04h ~ 0Ch		: Reserved for future use.
	volatile U8  NFCMD			;	///< 10h
	volatile U8  __Reserved1[7]	;	///< 11h ~ 17h		: Reserved for future use.
	volatile U8  NFADDR			;	///< 18h
	volatile U8  __Reserved2[7]	;	///< 19h ~ 1Fh		: Reserved for future use.
};

//------------------------------------------------------------------------------
// Register Bits
#define NX_NFCTRL_NCSENB			(1U<<31)
#define NX_NFCTRL_AUTORESET			(1U<<30)
#define NX_NFCTRL_ECCMODE			(7U<<27)
#define NX_NFCTRL_ECCMODE_S4		(0U<<27)
#define NX_NFCTRL_ECCMODE_S8		(1U<<27)
#define NX_NFCTRL_ECCMODE_S12		(2U<<27)
#define NX_NFCTRL_ECCMODE_S16		(3U<<27)
#define NX_NFCTRL_ECCMODE_S24		(5U<<27)
#define NX_NFCTRL_ECCMODE_60		(7U<<27)
#define NX_NFCTRL_ECCMODE_40		(6U<<27)
#define NX_NFCTRL_ECCMODE_24		(4U<<27)
#define NX_NFCTRL_IRQPEND			(1U<<15)
#define NX_NFCTRL_ECCRST			(1U<<11)
#define NX_NFCTRL_RNB				(1U<< 9)
#define NX_NFCTRL_IRQENB			(1U<< 8)
#define NX_NFCTRL_HWBOOT_W			(1U<< 6)
#define NX_NFCTRL_EXSEL_R			(1U<< 6)
#define NX_NFCTRL_EXSEL_W			(1U<< 5)
#define NX_NFCTRL_NFTYPE			(3U<< 3)
#define NX_NFCTRL_BANK				(3U<< 0)

#define NX_NFACTRL_SYN				(1U<< 1)		// 0: auto mode 1: cpu mode
#define NX_NFACTRL_ELP				(1U<< 0)		// 0: auto mode 1: cpu mode

#define NX_NFECCSTATUS_ELPERR	 (0x3FU<< 16)		// 6bit (16, 17, 18, 19, 20, 21)
#define NX_NFECCSTATUS_CHIENERR	 (0x7FU<<  4)		// 7bit (4, 5, 6, 7, 8, 9, 10)
#define NX_NFECCSTATUS_ERROR		(1U<< 3)
#define NX_NFECCSTATUS_BUSY			(1U<< 2)
#define NX_NFECCSTATUS_DECDONE		(1U<< 1)
#define NX_NFECCSTATUS_ENCDONE		(1U<< 1)

#define NX_NFECCCTRL_RUNECC_W		28	// ecc start
#define NX_NFECCCTRL_DECMODE_R		28
#define NX_NFECCCTRL_ELPLOAD		27	// set after elp registers
#define NX_NFECCCTRL_DECMODE_W		26	// 0: encoder 1: decoder
#define NX_NF_ENCODE			0
#define NX_NF_DECODE			1
#define NX_NFECCCTRL_ELPNUM			18	// number of elp (0x7F)
#define NX_NFECCCTRL_PDATACNT		10	// number of parity bit (0xFF)
#define NX_NFECCCTRL_DATACNT		0	// nand data count value(write) (0x3FF)

//------------------------------------------------------------------------------
// NAND Command
#define NAND_CMD_READ_1ST		(0x00)
#define NAND_CMD_READ_2ND		(0x30)
#define NAND_CMD_READ_ID		(0x90)
#define NAND_CMD_RESET			(0xFF)


//------------------------------------------------------------------------------
typedef struct tag_NANDBOOTECSTATUS
{
	U32		iNX_BCH_VAR_K;
	U32		iNX_BCH_VAR_M;

	U32		iNX_BCH_VAR_T;		// 4, 8, 12, 16, 24, 40, 60

	U32		iNX_BCH_VAR_TMAX;
	U32		iNX_BCH_VAR_RMAX;

	U32		iNX_BCH_VAR_RMAX32;

	U32		dwSectorSize;

	S32		iSectorsPerPage;
	U32		dwNFType;

	U32		dwRowCur;
	S32		iECCLeft;
	S32		iSectorLeft;
	U32 	pECCBASE[1024/4];
} NANDBOOTECSTATUS;

void GPIOSetAltFunction(U32 AltFunc);


static struct NX_MCUS_RegisterSet * const pNandControl = (struct NX_MCUS_RegisterSet *)PHY_BASEADDR_MCUSTOP_MODULE;
static struct NX_NAND_RegisterSet * const pNandAccess = (struct NX_NAND_RegisterSet *)BASEADDR_NFMEM;


//------------------------------------------------------------------------------
// BCH variables:
//------------------------------------------------------------------------------
//	k : number of information
//	m : dimension of Galois field.
//	t : number of error that can be corrected.
//	n : length of codeword = 2^m - 1
//	r : number of parity bit = m * t
//------------------------------------------------------------------------------
#define NX_BCH_VAR_K		(1024 * 8)	// 512*8, 1024*8
#define NX_BCH_VAR_M		(14)		// 13, 14

#define NX_BCH_VAR_T		(60)		// 4, 8, 12, 16, 24, 40, 60

#define NX_BCH_VAR_N		(((1<<NX_BCH_VAR_M)-1))
#define NX_BCH_VAR_R		(NX_BCH_VAR_M * NX_BCH_VAR_T)

#define NX_BCH_VAR_TMAX		(60)
#define NX_BCH_VAR_RMAX		(NX_BCH_VAR_M * NX_BCH_VAR_TMAX)

#define NX_BCH_VAR_R32		((NX_BCH_VAR_R		+31)/32)
#define NX_BCH_VAR_RMAX32	((NX_BCH_VAR_RMAX	+31)/32)


U32 divnx(U32 mk, U32 na)
{
	U32 i=0;
	while(mk>=na)
	{
		mk-=na;
		i++;
	}
	return i;
}

//------------------------------------------------------------------------------
static S32	NX_NAND_GetErrorLocation( NANDBOOTECSTATUS *pBootStatus, U32 *pLocation )
{
	U32 i;
	volatile U32 *pRegErrLoc = pNandControl->NFERRLOCATION;

	if(((pNandControl->NFECCSTATUS & NX_NFECCSTATUS_CHIENERR)>>4) != ((pNandControl->NFECCSTATUS & NX_NFECCSTATUS_ELPERR)>>16))
	{
		printf("chienerr: 0x%08X elperr: 0x%08X\r\n",
			((pNandControl->NFECCSTATUS & NX_NFECCSTATUS_CHIENERR)>>4), ((pNandControl->NFECCSTATUS & NX_NFECCSTATUS_ELPERR)>>16));
		return -1;
	}

	for(i=0; i<pBootStatus->iNX_BCH_VAR_TMAX/2; i++)
	{
		register U32 regvalue = *pRegErrLoc++;
		*pLocation++ = (regvalue>>0  & 0x3FFF)^0x7;
		*pLocation++ = (regvalue>>14 & 0x3FFF)^0x7;
	}

	return (pNandControl->NFECCSTATUS & NX_NFECCSTATUS_CHIENERR) >> 4;
}

//------------------------------------------------------------------------------
static CBOOL	CorrectErrors( NANDBOOTECSTATUS *pBootStatus, U32 *pData )
{
	U32 Loc[NX_BCH_VAR_T];
	S32 errors, i;

	errors = NX_NAND_GetErrorLocation( pBootStatus, &Loc[0] );
	if( 0 > errors )
	{
		printf( "\t\t ERROR -> Failed to correct errors.\r\n\n" );
		return CFALSE;
	}

	//printf( "\t%d error(s) found : ", errors);

	for( i=0 ; i<errors ; i++ )
	{
		//printf( "\t ERROR %d : %d\r\n", i, Loc[i] );

		if( pBootStatus->iNX_BCH_VAR_K > Loc[i] )
		{
			pData[ Loc[i] / 32 ] ^= 1U<<(Loc[i] % 32);
		}
	}

//	printf( "\r\n" );
//	printf( "-> Successful to correct errors.\n" );

	return CTRUE;
}

//------------------------------------------------------------------------------
static void		NANDFlash_SetOriECC( NANDBOOTECSTATUS *pBootStatus, U32 *pECC )
{
	register U32 i;
	volatile U32 *pRegOrgECC = pNandControl->NFORGECC;
	dprintf( "org ecc:0x%08X", pECC );
	for(i=0; i<pBootStatus->iNX_BCH_VAR_RMAX32; i++ )
	{
#if 0
		if(i%4 == 0)
			dprintf( "\r\n" );
		*pRegOrgECC = *pECC;
		dprintf("0x%08X ", *pECC);
		pRegOrgECC++;
		pECC++;
#else
		*pRegOrgECC++ = *pECC++;
#endif
	}
	dprintf( "\r\n" );
}

//------------------------------------------------------------------------------
static CBOOL	NANDFlash_Open( NANDBOOTECSTATUS *pBootStatus )
{
	register U32 pagesize;
	register U32 temp;

	temp = pNandControl->NFCONTROL;
	temp &= ~ (NX_NFCTRL_ECCMODE | NX_NFCTRL_EXSEL_W);
	temp |= ((temp & NX_NFCTRL_EXSEL_R)>>1);
	temp &= ~ (NX_NFCTRL_HWBOOT_W);

	pagesize = pSBI->DBI.NANDBI.PageSize;

	if( pagesize > 512  )
	{
		pBootStatus->dwSectorSize		= 1024;
		pBootStatus->iNX_BCH_VAR_K		= 1024*8;
		pBootStatus->iNX_BCH_VAR_M		= 14;
		pBootStatus->iNX_BCH_VAR_T		= 60;
		pBootStatus->iNX_BCH_VAR_TMAX	= 60;
		pBootStatus->iSectorsPerPage 	= pagesize>>10;
		temp |=	NX_NFCTRL_NCSENB | NX_NFCTRL_ECCMODE_60 | NX_NFCTRL_IRQENB;
	}else
	{
		pBootStatus->dwSectorSize		= 512;
		pBootStatus->iNX_BCH_VAR_K		= 512*8;
		pBootStatus->iNX_BCH_VAR_M		= 13;
		pBootStatus->iNX_BCH_VAR_T		= 24;
		pBootStatus->iNX_BCH_VAR_TMAX	= 24;
		pBootStatus->iSectorsPerPage 	= 1;
		temp |=	NX_NFCTRL_NCSENB | NX_NFCTRL_ECCMODE_S24 | NX_NFCTRL_IRQENB;
	}
	pBootStatus->iNX_BCH_VAR_RMAX	= (pBootStatus->iNX_BCH_VAR_M * pBootStatus->iNX_BCH_VAR_TMAX);
	pBootStatus->iNX_BCH_VAR_RMAX32	= ((pBootStatus->iNX_BCH_VAR_RMAX	+31)/32);


	printf("Device Address:0x%08X, Page Size:0x%08X\r\n", pSBI->DEVICEADDR, pagesize);
	//--------------------------------------------------------------------------
	pBootStatus->dwRowCur 			= divnx(pSBI->DEVICEADDR, pagesize);			// base page address of NAND flash memory

	printf("Start Page Address:0x%08X\r\n", pBootStatus->dwRowCur);
	pBootStatus->iSectorLeft		= 0;
	pBootStatus->iECCLeft			= 0;

	//--------------------------------------------------------------------------
	// Set NAND flash memory controller
	//--------------------------------------------------------------------------
	// 1) Get NFTYPE from CfgNFType
	// 2) Set NX_NFCTRL_BANK	as '0'
	// 3) Enable an interrupt to use NX_NFCTRL_IRQPEND for checking ready status of NAND flash memory.
	// 4) Set 16-bit ECC mode
	//--------------------------------------------------------------------------
	pNandControl->NFCONTROL = temp;

	// Wait until RnB is 1
	temp = PENDDELAY;
	while( 0==(pNandControl->NFCONTROL & NX_NFCTRL_RNB) && temp-- );
	if(temp == (U32)-1)
		return CFALSE;

	//--------------------------------------------------------------------------
	// Reset NAND flash memory for Micron device.
	//--------------------------------------------------------------------------
	{
		temp = pNandControl->NFCONTROL;
		temp &= ~ (NX_NFCTRL_EXSEL_W);
		temp |= (((temp & NX_NFCTRL_EXSEL_R)>>1) | NX_NFCTRL_IRQPEND);		// Clear NX_NFCTRL_IRQPEND
		temp &= ~ (NX_NFCTRL_HWBOOT_W);
		pNandControl->NFCONTROL = temp;

		pNandAccess->NFCMD = NAND_CMD_RESET;

		temp = PENDDELAY;
		// Wait until ready by using NX_NFCTRL_IRQPEND which is set at rising edge of RnB.
		while( 0==(pNandControl->NFCONTROL & NX_NFCTRL_IRQPEND) && temp-- );
		if(temp == (U32)-1)
			return CFALSE;
	}
	return CTRUE;
}

//------------------------------------------------------------------------------
static void		NANDFlash_Close( void )
{
	register U32 temp;

	temp = pNandControl->NFCONTROL;
	temp &= ~(NX_NFCTRL_IRQENB | NX_NFCTRL_NCSENB | NX_NFCTRL_EXSEL_W);
	temp |= (((temp & NX_NFCTRL_EXSEL_R)>>1) | NX_NFCTRL_IRQPEND);
	temp &= ~ (NX_NFCTRL_HWBOOT_W);
	pNandControl->NFCONTROL = temp;
}

//------------------------------------------------------------------------------
static CBOOL	NANDFlash_SetAddr( NANDBOOTECSTATUS *pBootStatus )
{
	register U32 temp;
	// check for changing page
	dprintf("iSectorLeft:0x%08X\r\n", pBootStatus->iSectorLeft);
	if( pBootStatus->iSectorLeft == 0 )
	{
		U32	row			= pBootStatus->dwRowCur;

		dprintf("row:0x%08X\r\n", row);
		temp = pNandControl->NFCONTROL;
		temp &= ~ (NX_NFCTRL_EXSEL_W);
		temp |= (((temp & NX_NFCTRL_EXSEL_R)>>1) | NX_NFCTRL_IRQPEND);		// Clear NX_NFCTRL_IRQPEND
		temp &= ~ (NX_NFCTRL_HWBOOT_W);
		pNandControl->NFCONTROL = temp;

		pNandAccess->NFCMD = NAND_CMD_READ_1ST;

		pNandAccess->NFADDR = 0;				// COL 1st
		if(pSBI->DBI.NANDBI.AddrStep > 3 && pSBI->DBI.NANDBI.PageSize != 512)
		{
			dprintf("COL 2nd\r\n");
			pNandAccess->NFADDR = 0;				// COL 2nd
		}
			pNandAccess->NFADDR = (U8)(row >>  0);	// ROW 1st
			pNandAccess->NFADDR = (U8)(row >>  8);	// ROW 2nd
		if(( pSBI->DBI.NANDBI.AddrStep > 3 && pSBI->DBI.NANDBI.PageSize == 512 )||
			( pSBI->DBI.NANDBI.AddrStep > 4 && pSBI->DBI.NANDBI.PageSize > 512 ))
		{
			dprintf("ROW 3rd\r\n");
			pNandAccess->NFADDR = (U8)(row >> 16);	// ROW 3rd
		}

		if( pSBI->DBI.NANDBI.PageSize > 512 )	// Large block
		{
			dprintf("Large block cmd\r\n");
			pNandAccess->NFCMD = NAND_CMD_READ_2ND;
		}

		// Wait until ready by using NX_NFCTRL_IRQPEND which is set at rising edge of RnB.
		temp = PENDDELAY;
		while( 0==(pNandControl->NFCONTROL & NX_NFCTRL_IRQPEND) && temp-- );
		if(temp == (U32)-1)
			return CFALSE;

		pBootStatus->iSectorLeft = pBootStatus->iSectorsPerPage;
		pBootStatus->dwRowCur++;
	}
	return CTRUE;
}

//------------------------------------------------------------------------------
static void		NANDFlash_ReadData( NANDBOOTECSTATUS *pBootStatus, U32 *pData )
{
	register U32 temp;
//	U32 i, *psave = pData;

	// run ecc
	pNandControl->NFECCCTRL =											 1<<NX_NFECCCTRL_RUNECC_W   |	   // run ecc
																		 0<<NX_NFECCCTRL_ELPLOAD    |
															  NX_NF_DECODE<<NX_NFECCCTRL_DECMODE_W	|
										 (pBootStatus->iNX_BCH_VAR_T&0x7F)<<NX_NFECCCTRL_ELPNUM		|
		((pBootStatus->iNX_BCH_VAR_M*pBootStatus->iNX_BCH_VAR_T/8-1)&0xFF)<<NX_NFECCCTRL_PDATACNT	|
									 ((pBootStatus->dwSectorSize-1)&0x3FF)<<NX_NFECCCTRL_DATACNT;

	// Read data from NAND flash memory to SRAM
	temp = pBootStatus->dwSectorSize/(4*8);
	while( temp-- )
	{
		pData[ 0] = pNandAccess->NFDATA;
		pData[ 1] = pNandAccess->NFDATA;
		pData[ 2] = pNandAccess->NFDATA;
		pData[ 3] = pNandAccess->NFDATA;

		pData[ 4] = pNandAccess->NFDATA;
		pData[ 5] = pNandAccess->NFDATA;
		pData[ 6] = pNandAccess->NFDATA;
		pData[ 7] = pNandAccess->NFDATA;

		pData += 8;
	}
#if 0
	temp = pBootStatus->dwSectorSize/4;
	dprintf( "Read Data is:" );
	for( i=0; i<temp; i++ )
	{
		if(i%4 == 0)
			dprintf( "\r\n" );
		dprintf("0x%08X ", psave[i]);
	}
	dprintf( "\r\n" );
#endif
	pBootStatus->iSectorLeft--;
}

//------------------------------------------------------------------------------
static CBOOL	NANDFlash_ReadSector( NANDBOOTECSTATUS *pBootStatus, U32 *pData )
{
	register U32 temp;
//	U16	syndrome[NX_BCH_VAR_T];
	//--------------------------------------------------------------------------
	// Copy ECCs from SRAM to H/W BCH decoder
	if( pBootStatus->iECCLeft == 0 )
	{
		if(CFALSE == NANDFlash_SetAddr(pBootStatus))
			return CFALSE;
		NANDFlash_ReadData( pBootStatus, pBootStatus->pECCBASE );
		pBootStatus->iECCLeft = 7;
	}

	//--------------------------------------------------------------------------
	// Reset H/W BCH decoder.
	temp = pNandControl->NFCONTROL;
	temp &= ~ (NX_NFCTRL_EXSEL_W);
	temp |= (((temp & NX_NFCTRL_EXSEL_R)>>1) | NX_NFCTRL_ECCRST);
	temp &= ~ (NX_NFCTRL_HWBOOT_W);
	pNandControl->NFCONTROL = temp;

	pNandControl->NFECCAUTOMODE = (pNandControl->NFECCAUTOMODE & ~(NX_NFACTRL_ELP)) | NX_NFACTRL_SYN;	// disconnect syndrome path

	// Read Data with ECC
	if(CFALSE == NANDFlash_SetAddr(pBootStatus))
		return CFALSE;

	// Set original ECCs.
	NANDFlash_SetOriECC( pBootStatus, (U32 *)((U8*)pBootStatus->pECCBASE + (8-pBootStatus->iECCLeft)*(pBootStatus->dwSectorSize/8)) );	// 56*(60/4)/8 = 105
	pBootStatus->iECCLeft--;


	NANDFlash_ReadData( pBootStatus, pData );

	//--------------------------------------------------------------------------
	// Determines whether or not there's an error in data by using H/W BCH decoder.
	// Wait until H/W BCH decoder has been finished.
	temp = PENDDELAY;
	while( 0==(pNandControl->NFECCSTATUS & NX_NFECCSTATUS_DECDONE) && temp-- );
	if(temp == (U32)-1)
		return CFALSE;
#if 0
	for(temp = 0; temp<NX_BCH_VAR_T/2; temp++)
	{
		syndrome[temp+0] = pNandControl->NFSYNDROME[temp] & 0x0000FFFF<< 0;
		syndrome[temp+1] = pNandControl->NFSYNDROME[temp] & 0xFFFF0000>>16;
	}
#endif
	pNandControl->NFECCAUTOMODE = (pNandControl->NFECCAUTOMODE & ~(NX_NFACTRL_ELP | NX_NFACTRL_SYN));	// connect syndrome path

	// Check an error status of H/W BCH decoder.
	if( pNandControl->NFECCSTATUS & NX_NFECCSTATUS_ERROR )
	{
#if 0
		dprintf( "-> %d page, %d sector :\r\nSyndrome: ",
			pBootStatus->dwRowCur-1, pBootStatus->iSectorsPerPage - pBootStatus->iSectorLeft - 1 );

		for(temp = 0; temp < pBootStatus->iNX_BCH_VAR_T; temp++)
		{
			if(temp%8 == 0)
				dprintf( "\r\n" );
			dprintf("%04X, ", syndrome[temp] );
		}
		dprintf( "\r\n" );
#endif
		// load elp
		pNandControl->NFECCCTRL =											 0<<NX_NFECCCTRL_RUNECC_W   |
																			 1<<NX_NFECCCTRL_ELPLOAD    |	   // load elp
																  NX_NF_DECODE<<NX_NFECCCTRL_DECMODE_W	|
											 (pBootStatus->iNX_BCH_VAR_T&0x7F)<<NX_NFECCCTRL_ELPNUM 	|
			((pBootStatus->iNX_BCH_VAR_M*pBootStatus->iNX_BCH_VAR_T/8-1)&0xFF)<<NX_NFECCCTRL_PDATACNT	|
										 ((pBootStatus->dwSectorSize-1)&0x3FF)<<NX_NFECCCTRL_DATACNT;

		temp = PENDDELAY;
		while(pNandControl->NFECCSTATUS & NX_NFECCSTATUS_BUSY);
		if(temp == (U32)-1)
			return CFALSE;

		if(CFALSE == CorrectErrors( pBootStatus, pData ))
		{
			dprintf( "Page:%d Sector: %d\r\n", pBootStatus->dwRowCur-1, pBootStatus->iSectorsPerPage - pBootStatus->iSectorLeft - 1 );
			return CFALSE;
		}
	}

	return CTRUE;
}
//------------------------------------------------------------------------------
CBOOL	iNANDBOOTEC( struct NX_SecondBootInfo * pTBI )
{
	CBOOL Result = CTRUE;
	U32 dwBinAddr;
	S32 iBinSecLeft;
	NANDBOOTECSTATUS BootStatus, *pBootStatus;
	U32 TBI[1024/4];;
	struct NX_SecondBootInfo * plTBI = (struct NX_SecondBootInfo *)TBI;
	pBootStatus = &BootStatus;

#if 0
	pSBI->DBI.NANDBI.AddrStep = 5;
	pSBI->DBI.NANDBI.tCOS = 0xE;
	pSBI->DBI.NANDBI.tACC = 0xE;
	pSBI->DBI.NANDBI.tOCH = 0xE;
//	pSBI->DBI.NANDBI.PageSize = 2048;
//	pSBI->DBI.NANDBI.PageSize = 4096;
	pSBI->DBI.NANDBI.PageSize = 8192;
//	pSBI->DBI.NANDBI.PageSize = 16384;
//	pSBI->DEVICEADDR = 0x20000;
#endif
	pNandControl->NFTACS = 0xF;
	pNandControl->NFTCOS = pSBI->DBI.NANDBI.tCOS;
	pNandControl->NFTACC = pSBI->DBI.NANDBI.tACC;
	pNandControl->NFTOCH = pSBI->DBI.NANDBI.tOCH;
	pNandControl->NFTCAH = 0xF;

//	pReg_GPIO[1]->GPIOxALTFN[1] = (pReg_GPIO[1]->GPIOxALTFN[1] & ~0x00000033) | 0x00000011;		// B 16, 18 ALT1
//	pNandControl->NFCONTROL = (pNandControl->NFCONTROL & ~(NX_NFCTRL_BANK | NX_NFCTRL_HWBOOT_W | NX_NFCTRL_EXSEL_W)) | 0x01;	// nNSCS1
	pReg_GPIO[1]->GPIOxALTFN[1] &= ~0x00000033;		// B 16, 18 ALT0
	pNandControl->NFCONTROL = (pNandControl->NFCONTROL & ~(NX_NFCTRL_BANK | NX_NFCTRL_HWBOOT_W | NX_NFCTRL_EXSEL_W)) | 0x00;	// nNSCS0

	pReg_GPIO[1]->GPIOxALTFN[0] &= ~0x0C000000;	// GPIO B[13]
	pReg_GPIO[1]->GPIOxALTFN[1] &= ~0x0003FF33;	// GPIO B[15, 17, 19, 20, 21, 22, 23]


	if(NANDFlash_Open(pBootStatus))
	{
		if( CTRUE == NANDFlash_ReadSector( pBootStatus, (U32 *)plTBI ) )
		{
			if(plTBI->SIGNATURE == HEADER_ID)
			{
				printf("Load Address:0x%08X\r\nLoad Size:0x%08X\r\n", plTBI->LOADADDR, plTBI->LOADSIZE);

				dwBinAddr	= plTBI->LOADADDR;
				iBinSecLeft	= divnx((plTBI->LOADSIZE + pBootStatus->dwSectorSize-1), pBootStatus->dwSectorSize);

				while( iBinSecLeft-- )
				{
					if( CFALSE == NANDFlash_ReadSector( pBootStatus, (U32 *)dwBinAddr ) )
					{
						Result = CFALSE;
						break;
					}
					dwBinAddr += pBootStatus->dwSectorSize;
				}
				pTBI->LAUNCHADDR = plTBI->LAUNCHADDR;
			}
			else
			{
				printf("3rd boot Sinature is wrong! nand boot failure\r\n");
				Result = CFALSE;
			}
		}
		else
		{
			printf("cannot read boot header! nand boot failure\r\n");
			Result = CFALSE;
		}
	}else
	{
		printf("nand open failure! nand is not work or not exist\r\n");
		Result = CFALSE;
	}
	NANDFlash_Close();

	pNandControl->NFCONTROL = (pNandControl->NFCONTROL & ~(NX_NFCTRL_BANK | NX_NFCTRL_HWBOOT_W | NX_NFCTRL_EXSEL_W)) | 0x00;	// nNSCS0, Select SD bus

	return Result;
}
