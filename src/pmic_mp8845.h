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
//  File            : pmic_mp8845.h
//  Description     :
//  Author          : Hans
//  History         :
//          2015-08-19  Hans Create
////////////////////////////////////////////////////////////////////////////////

#ifndef __PMIC_MP8845_H__
#define __PMIC_MP8845_H__

#define MP8845C_REG_VSEL                0x00
#define MP8845C_REG_SYSCNTL1            0x01
#define MP8845C_REG_SYSCNTL2            0x02
#define MP8845C_REG_ID1                 0x03
#define MP8845C_REG_ID2                 0x04
#define MP8845C_REG_STATUS              0x05

#define I2C_ADDR_MP8845                 (0x38 >> 1)  // SVT & ASB

#endif	// ifdef __PMIC_MP8845_H__