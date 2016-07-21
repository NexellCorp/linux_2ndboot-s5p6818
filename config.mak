###########################################################################
# Build Version info
###########################################################################
VERINFO				= V036

###########################################################################
# Build Environment
###########################################################################

CHIPNAME			= S5P6818

DEBUG				= n

#OPMODE				= aarch64
OPMODE				= aarch32


MEMTYPE				= DDR3
#MEMTYPE			= LPDDR3

BUILTINALL			= y
INITPMIC			= YES
#INITPMIC			= NO

ifeq ($(BUILTINALL),n)
#BOOTFROM			= USB
#BOOTFROM			= SPI
BOOTFROM			= SDMMC
#BOOTFROM			= SDFS
#BOOTFROM			= NAND
#BOOTFROM			= UART
else ifeq ($(BUILTINALL),y)
BOOTFROM			= ALL
endif

#BOARD				= SVT
#BOARD				= ASB
BOARD				= DRONE
#BOARD				= AVN
#BOARD				= AVN_BT
#BOARD				= BF700

# cross-tool pre-header
ifeq ($(OPMODE), aarch32)
ifeq ($(OS),Windows_NT)
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-none-eabi-
else
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-eabi-
#CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-linux-gnueabihf-
endif
endif

ifeq ($(OPMODE), aarch64)
ifeq ($(OS),Windows_NT)
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)aarch64-none-elf-
else
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)aarch64-none-elf-
endif
endif

###########################################################################
# Top Names
###########################################################################
PROJECT_NAME			= $(CHIPNAME)_2ndboot_$(OPMODE)_$(MEMTYPE)_$(VERINFO)

ifeq ($(INITPMIC), YES)
TARGET_NAME			= $(PROJECT_NAME)_$(BOARD)_$(BOOTFROM)
endif
ifeq ($(INITPMIC), NO)
TARGET_NAME			= $(PROJECT_NAME)_$(BOOTFROM)
endif

LDS_NAME			= peridot_2ndboot_$(OPMODE)


###########################################################################
# Directories
###########################################################################
DIR_PROJECT_TOP			=

DIR_OBJOUTPUT			= obj
ifeq ($(BUILTINALL),n)
DIR_TARGETOUTPUT		= build_$(BOARD)_$(BOOTFROM)_$(OPMODE)
else ifeq ($(BUILTINALL),y)
DIR_TARGETOUTPUT		= build_$(BOARD)_$(OPMODE)
endif

CODE_MAIN_INCLUDE		=

###########################################################################
# Build Environment
###########################################################################
ifeq ($(OPMODE) , aarch32)
ARCH				= armv7-a
CPU				= cortex-a15
endif
ifeq ($(OPMODE) , aarch64)
ARCH				= armv8-a
CPU				= cortex-a53
endif


CC				= $(CROSS_TOOL)gcc
LD 				= $(CROSS_TOOL)ld
AS 				= $(CROSS_TOOL)as
AR 				= $(CROSS_TOOL)ar
MAKEBIN				= $(CROSS_TOOL)objcopy
OBJCOPY				= $(CROSS_TOOL)objcopy
RANLIB 				= $(CROSS_TOOL)ranlib

GCC_LIB				= $(shell $(CC) -print-libgcc-file-name)

ifeq ($(DEBUG), y)
CFLAGS				= -DNX_DEBUG -O0
Q				=
else
CFLAGS				= -DNX_RELEASE -Os
Q				= @
endif

###########################################################################
# MISC tools for MS-DOS
###########################################################################
ifeq ($(OS),Windows_NT)
MKDIR				= mkdir
RM				= del /q /F
MV				= move
CD				= cd
CP				= copy
ECHO				= echo
RMDIR				= rmdir /S /Q
else
MKDIR				= mkdir
RM				= rm -f
MV				= mv
CD				= cd
CP				= cp
ECHO				= echo
RMDIR				= rm -rf
endif
###########################################################################
# FLAGS
###########################################################################
ARFLAGS				= rcs
ARFLAGS_REMOVE			= -d
ARLIBFLAGS			= -v -s

ASFLAG				= -D__ASSEMBLY__ -D$(OPMODE)

CFLAGS				+=	-g -Wall				\
					-Wextra -ffreestanding -fno-builtin	\
					-mlittle-endian				\
					-mcpu=$(CPU)				\
					$(CODE_MAIN_INCLUDE)			\
					-D__arm -DLOAD_FROM_$(BOOTFROM)		\
					-DMEMTYPE_$(MEMTYPE)			\
					-DINITPMIC_$(INITPMIC)			\
					-D$(OPMODE) -D$(BOARD)


ifeq ($(OPMODE) , aarch32)
CFLAGS				+=	-msoft-float				\
					-mstructure-size-boundary=32
endif

ifeq ($(OPMODE) , aarch64)
ASFLAG				+=	-march=$(ARCH) -mcpu=$(CPU)

CFLAGS				+=	-mcmodel=small				\
					-march=$(ARCH)
endif

ifeq ($(INITPMIC), YES)
CFLAGS				+=	-D$(BOARD)_PMIC_INIT
endif

