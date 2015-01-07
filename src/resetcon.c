////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009 Nexell Co. All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  Nexell informs that this code and information is provided "as is" base
//  and without warranty of any kind, either expressed or implied, including
//  but not limited to the implied warranties of merchantability and/or fitness
//  for a particular puporse.
//
//
//  Module      : ResetCon.c
//  File        :
//  Description :
//  Author      : Hans
//  History     : 2013.01.10 First implementation
//
////////////////////////////////////////////////////////////////////////////////

#include "sysHeader.h"

void ResetCon(U32 devicenum, CBOOL en)
{
    if (en)
        ClearIO32( &pReg_RstCon->REGRST[(devicenum>>5)&0x3],    (0x1<<(devicenum&0x1F)) );  // reset
    else
        SetIO32  ( &pReg_RstCon->REGRST[(devicenum>>5)&0x3],    (0x1<<(devicenum&0x1F)) );  // reset negate
}

