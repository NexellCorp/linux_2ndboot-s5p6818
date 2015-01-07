#include <nx_prototype.h>
#include "nx_type.h"
#include "printf.h"
#include "debug.h"
#include <nx_debug2.h>
#include "nx_peridot.h"
#include <nx_tieoff.h>

#include <nx_drex.h>
#include "secondboot.h"


extern struct NX_SecondBootInfo * const pSBI;
extern struct NX_TIEOFF_RegisterSet * const pTieoffreg;

void SetAXIBus(void)
{
//	U32 i_slot;//, lock0;
	// Test for AXI bus (PL301)
	/*
		DebugPutString("\r\n########## AXI BUS ##########");
		WriteIO32( NX_BASE_REG_AXI_BOT | 0x0428, 0x00000002);
		WriteIO32( NX_BASE_REG_AXI_BOT | 0x0428, 0x07000000);
		WriteIO32( NX_BASE_REG_AXI_BOT | 0x0428, 0x00000001);

		WriteIO32( NX_BASE_REG_AXI_BOT | 0x042C, 0x00000002);
		WriteIO32( NX_BASE_REG_AXI_BOT | 0x042C, 0x07000000);
		WriteIO32( NX_BASE_REG_AXI_BOT | 0x042C, 0x00000001);
	*/


#if (REG_MSG)
		printf("\r\n########## AXI BUS ##########\r\n");
#endif
#if 0
		WriteIO32( NX_BASE_REG_PL301_BOTT | 0x0428, 0x00000002);
		WriteIO32( NX_BASE_REG_PL301_BOTT | 0x0428, 0x07000000);
		WriteIO32( NX_BASE_REG_PL301_BOTT | 0x0428, 0x00000001);

		WriteIO32( NX_BASE_REG_PL301_BOTT | 0x042C, 0x00000002);
		WriteIO32( NX_BASE_REG_PL301_BOTT | 0x042C, 0x07000000);
		WriteIO32( NX_BASE_REG_PL301_BOTT | 0x042C, 0x00000001);
#endif

#if 0
		// SI=0
		WriteIO32( NX_BASE_REG_PL301_BOTT | (0x400	<< 0), (1 << 0));
		WriteIO32( NX_BASE_REG_PL301_BOTT | (0x404	<< 0), ~((1 << 6)|(1 << 7)));

		// cpu 6,7
	//	WriteIO32( NX_BASE_REG_PL301_BOTT | (0x408	<< 0), (6 << 24) | (0x0 << 0));
	//	WriteIO32( NX_BASE_REG_PL301_BOTT | (0x408	<< 0), (7 << 24) | (0x0 << 0));

		WriteIO32( NX_BASE_REG_PL301_DISP | (0x408	<< 0), (0 << 24) | (0x0 << 0));
		for(i_slot = 1; i_slot<32; i_slot++)
		{
			WriteIO32( NX_BASE_REG_PL301_DISP | (0x408	<< 0), (i_slot << 24) | (0x1 << 0));
	//		WriteIO32( NX_BASE_REG_PL301_DISP | (0x408	<< 0),		(0x1 << 24) | (0x0 << 8));
		}
#endif
}

void Adjust_DREX_QoS(void)
{
	struct NX_DREXSDRAM_RegisterSet *pDREX = (struct NX_DREXSDRAM_RegisterSet *)PHY_BASEADDR_DREX_MODULE_CH0_APB;
return;
		WriteIO32( &pDREX->BRBRSVCONFIG,
				0x8<<24 |		// enable write brb reservation for AXI port 2
				0x8<<20 |		// enable write brb reservation for AXI port 1
				0x8<<16 |		// enable write brb reservation for AXI port 0
				0x8<< 8 |		// enable read brb reservation for AXI port 2
				0x8<< 4 |		// enable read brb reservation for AXI port 1
				0x8<< 0			// enable read brb reservation for AXI port 0
				);
		WriteIO32( &pDREX->BRBRSVCONTROL,
				1<<6	|		// enable write brb reservation for AXI port 2
				1<<5	|		// enable write brb reservation for AXI port 1
				1<<4	|		// enable write brb reservation for AXI port 0
				1<<2	|		// enable read brb reservation for AXI port 2
				1<<1	|		// enable read brb reservation for AXI port 1
				1<<0			// enable read brb reservation for AXI port 0
				);
		WriteIO32( &pDREX->QOSCONTROL[0].QOSCONTROL,			0x00000100);
		WriteIO32( &pDREX->QOSCONTROL[1].QOSCONTROL,			0x00000100);
		WriteIO32( &pDREX->QOSCONTROL[2].QOSCONTROL,			0x00000100);
}

