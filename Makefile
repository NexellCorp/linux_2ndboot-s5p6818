#	Copyright (C) 2009 Nexell Co., All Rights Reserved
#	Nexell Co. Proprietary & Confidential
#
#	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
#	AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
#	BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
#	FOR A PARTICULAR PURPOSE.
#
#	Module		:
#	File		:
#	Description	:
#	Author		:
#	History		:
#------------------------------------------------------------------------------

include config.mak

LDFLAGS		=	-Bstatic							\
			-Wl,-Map=$(DIR_TARGETOUTPUT)/$(TARGET_NAME).map,--cref		\
			-T$(LDS_NAME).lds						\
			-Wl,--start-group						\
			-Lsrc/$(DIR_OBJOUTPUT)						\
			-Wl,--end-group							\
			-nostdlib

SYS_OBJS	=	startup.o secondboot.o				\
			resetcon.o GPIO.o CRC32.o			\
			clockinit.o debug.o lib2ndboot.o buildinfo.o	\
			sysbus.o					\
			printf.o
SYS_OBJS	+=	MemoryInit.o
#SYS_OBJS	+=	CRYPTO.o
#SYS_OBJS	+=	nx_tieoff.o

ifeq ($(BOOTFROM),USB)
SYS_OBJS	+=	iUSBBOOT.o
endif
ifeq ($(BOOTFROM),SPI)
SYS_OBJS	+=	iSPIBOOT.o CRYPTO.o
endif
ifeq ($(BOOTFROM),SDMMC)
SYS_OBJS	+=	iSDHCBOOT.o
endif
ifeq ($(BOOTFROM),SDFS)
SYS_OBJS	+=	iSDHCBOOT.o diskio.o fatfs.o iSDHCFSBOOT.o
endif
ifeq ($(BOOTFROM),NAND)
SYS_OBJS	+=	iNANDBOOTEC.o
endif
ifeq ($(BOOTFROM),UART)
SYS_OBJS	+=	iUARTBOOT.o
endif


SYS_OBJS_LIST	=	$(addprefix $(DIR_OBJOUTPUT)/,$(SYS_OBJS))

SYS_INCLUDES	=	-I src				\
			-I prototype/base 		\
			-I prototype/module

###################################################################################################
$(DIR_OBJOUTPUT)/%.o: src/%.c
	@echo [compile....$<]
	$(Q)$(CC) -MMD $< -c -o $@ $(CFLAGS) $(SYS_INCLUDES)
###################################################################################################
$(DIR_OBJOUTPUT)/%.o: src/%.S
	@echo [compile....$<]
	$(Q)$(CC) -MMD $< -c -o $@ $(ASFLAG) $(CFLAGS) $(SYS_INCLUDES)
###################################################################################################


all: mkobjdir $(SYS_OBJS_LIST) link bin

mkobjdir:
ifeq ($(OS),Windows_NT)
	@if not exist $(DIR_OBJOUTPUT)			\
		@$(MKDIR) $(DIR_OBJOUTPUT)
	@if not exist $(DIR_TARGETOUTPUT)		\
		@$(MKDIR) $(DIR_TARGETOUTPUT)
else
	@if	[ ! -e $(DIR_OBJOUTPUT) ]; then 	\
		$(MKDIR) $(DIR_OBJOUTPUT);		\
	fi;
	@if	[ ! -e $(DIR_TARGETOUTPUT) ]; then 	\
		$(MKDIR) $(DIR_TARGETOUTPUT);		\
	fi;
endif

link:
	@echo [link.... $(DIR_TARGETOUTPUT)/$(TARGET_NAME).elf]

	$(Q)$(CC) $(SYS_OBJS_LIST) $(LDFLAGS) -o $(DIR_TARGETOUTPUT)/$(TARGET_NAME).elf

bin:
	@echo [binary.... $(DIR_TARGETOUTPUT)/$(TARGET_NAME).bin]
	$(Q)$(MAKEBIN) -O binary $(DIR_TARGETOUTPUT)/$(TARGET_NAME).elf $(DIR_TARGETOUTPUT)/$(TARGET_NAME).bin
ifeq ($(OS),Windows_NT)
	@if exist $(DIR_OBJOUTPUT)			\
		@$(RM) $(DIR_OBJOUTPUT)\buildinfo.o
else
	@if	[ -e $(DIR_OBJOUTPUT) ]; then 		\
		$(RM) $(DIR_OBJOUTPUT)/buildinfo.o;	\
	fi;
endif

###################################################################################################
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(DIR_OBJOUTPUT)			\
		@$(RMDIR) $(DIR_OBJOUTPUT)
	@if exist $(DIR_TARGETOUTPUT)			\
		@$(RMDIR) $(DIR_TARGETOUTPUT)
else
	@if	[ -e $(DIR_OBJOUTPUT) ]; then 		\
		$(RMDIR) $(DIR_OBJOUTPUT);		\
	fi;
	@if	[ -e $(DIR_TARGETOUTPUT) ]; then 	\
		$(RMDIR) $(DIR_TARGETOUTPUT);		\
	fi;
endif

-include $(SYS_OBJS_LIST:.o=.d)

