////////////////////////////////////////////////////////////////////////////////
//
//	Copyright (C) 2009 Nexell Co., Ltd All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	Nexell informs that this code and information is provided "as is" base
//	and without warranty of any kind, either expressed or implied, including
//	but not limited to the implied warranties of merchantability and/or fitness
//	for a particular puporse.
//
//
//	Module		:
//	File		: libdiv.c
//	Description	:
//	Author		: Hans
//	History		:
//		2014-08-20	Hans create
////////////////////////////////////////////////////////////////////////////////
#include <nx_type.h>
#include "nx_peridot.h"

#ifdef aarch32
U32 getquotient(U32 dividend, U32 divisor)
{
	U32 quotient, remainder;
	U32 t, num_bits;
	U32 q, bit, d=0;
	U32 i;

	remainder = 0;
	quotient = 0;

	if (divisor == 0)
		return -1;

	if (divisor > dividend) {
		return 0;
	}

	if (divisor == dividend) {
		return 1;
	}

	num_bits = 32;

	while (remainder < divisor) {
		bit = (dividend & 0x80000000) >> 31;
		remainder = (remainder << 1) | bit;
		d = dividend;
		dividend = dividend << 1;
		num_bits--;
	}

	dividend = d;
	remainder = remainder >> 1;
	num_bits++;

	for (i = 0; i < num_bits; i++) {
		bit = (dividend & 0x80000000) >> 31;
		remainder = (remainder << 1) | bit;
		t = remainder - divisor;
		q = !((t & 0x80000000) >> 31);
		dividend = dividend << 1;
		quotient = (quotient << 1) | q;
		if (q) {
			remainder = t;
		}
	}
	return quotient;
}

U32 getremainder(U32 dividend, U32 divisor)
{
	U32 quotient, remainder;
	U32 t, num_bits;
	U32 q, bit, d=0;
	U32 i;

	remainder = 0;
	quotient = 0;

	if (divisor == 0)
		return -1;

	if (divisor > dividend) {
		return dividend;
	}

	if (divisor == dividend) {
		return 0;
	}

	num_bits = 32;

	while (remainder < divisor) {
		bit = (dividend & 0x80000000) >> 31;
		remainder = (remainder << 1) | bit;
		d = dividend;
		dividend = dividend << 1;
		num_bits--;
	}

	dividend = d;
	remainder = remainder >> 1;
	num_bits++;

	for (i = 0; i < num_bits; i++) {
		bit = (dividend & 0x80000000) >> 31;
		remainder = (remainder << 1) | bit;
		t = remainder - divisor;
		q = !((t & 0x80000000) >> 31);
		dividend = dividend << 1;
		quotient = (quotient << 1) | q;
		if (q) {
			remainder = t;
		}
	}
	return remainder;
}
#endif

#ifdef aarch64
U32 getquotient(U32 dividend, U32 divisor)
{
	return dividend/divisor;
}
U32 getremainder(U32 dividend, U32 divisor)
{
	return dividend%divisor;
}
#endif

inline void DMC_Delay(int milisecond)
{
	register volatile int   count, temp;

	for (count = 0; count < milisecond; count++)
	{
		temp ^= count;
	}
}

