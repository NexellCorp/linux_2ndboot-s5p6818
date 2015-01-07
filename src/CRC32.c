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
//  Module      : CRC32.c
//  File        :
//  Description :
//  Author      : Hans
//  History     :
//                  2013.01.10 First implementation
//
////////////////////////////////////////////////////////////////////////////////
#include <nx_peridot.h>
#include <nx_type.h>

U32 get_fcs(U32 fcs, U8 data)
{
    int i;

    fcs ^= (U32)data;
    for(i=0; i<8; i++)
    {
        if(fcs & 0x01)
            fcs ^= POLY;
        fcs >>= 1;
    }

    return fcs;
}

U32 iget_fcs(U32 fcs, U32 data)
{
    int i;

    fcs ^= data;
    for(i = 0; i < 32; i++)
    {
        if(fcs & 0x01)
            fcs ^= POLY;
        fcs >>= 1;
    }

    return fcs;
}

#define CHKSTRIDE   8
U32 __calc_crc(void *addr, int len)
{
    U32 *c = (U32*)addr;
    U32 crc = 0, chkcnt = ((len+3)/4);
    U32 i;

    for (i = 0; chkcnt > i; i += CHKSTRIDE, c += CHKSTRIDE)
    {
        crc = iget_fcs(crc, *c);
    }

    return crc;
}


