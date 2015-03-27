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
//  File            : buildinfo.c
//  Description     :
//  Author          : Hans
//  History         :
//          2015-02-03  Hans
////////////////////////////////////////////////////////////////////////////////

#include "sysHeader.h"
//------------------------------------------------------------------------------
CBOOL buildinfo(void)
{
    CBOOL   ret = CTRUE;
    U32     *pNsih_BuildInfo = (U32 *)(0xFFFF0000 + 0x1F8);
    U32     *p2nd_BuildInfo  = (U32 *)(0xFFFF0200 + 0x024);
    U32     tmp;

    // Read Build Infomation.
    tmp = ReadIO32( p2nd_BuildInfo ) & 0xFFFF;

    printf( "\r\n" );
    printf( "--------------------------------------------------------------------------------\r\n" );
    printf( " Second Boot by Nexell Co. : Ver%d.%d.%d - Built on %s %s\r\n", ((tmp >> 12) & 0xF), ((tmp >> 8) & 0xF), (tmp & 0xFF), __DATE__, __TIME__ );
    printf( "--------------------------------------------------------------------------------\r\n" );

    tmp = ReadIO32( pNsih_BuildInfo ) & 0xFFFFFF00;
    if ( (ReadIO32( p2nd_BuildInfo )  & 0xFFFFFF00) != tmp)
    {
        printf( " HSIN : Ver%d.%d.xx\r\n", ((tmp >> 12) & 0xF), ((tmp >> 8) & 0xF) );
        ret = CFALSE;
    }

    return ret;
}
