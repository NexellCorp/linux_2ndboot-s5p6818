###########################################################################
# Build Version info
###########################################################################
VERINFO				= V036

###########################################################################
# Build Environment
###########################################################################
DEBUG				= n

#OPMODE				= aarch64
OPMODE				= aarch32


MEMTYPE				= DDR3
#MEMTYPE			= LPDDR3

BUILTINALL			= n
#INITPMIC			= YES
INITPMIC			= NO

CHIPNAME			= S5P6818

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

#BOARD				= _pyxis
#BOARD				= _lynx
#BOARD				= _vtk
#BOARD				= _drone
#BOARD				= _svt

# cross-tool pre-header
ifeq ($(OPMODE), aarch32)
ifeq ($(OS),Windows_NT)
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-none-eabi-
else
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-none-eabi-
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
ifeq ($(BUILTINALL),n)
TARGET_NAME			= $(PROJECT_NAME)$(BOARD)_$(BOOTFROM)
else ifeq ($(BUILTINALL),y)
TARGET_NAME			= $(PROJECT_NAME)
endif
LDS_NAME			= peridot_2ndboot_$(OPMODE)


###########################################################################
# Directories
###########################################################################
DIR_PROJECT_TOP			=

DIR_OBJOUTPUT			= obj
ifeq ($(BUILTINALL),n)
DIR_TARGETOUTPUT		= build$(BOARD)_$(BOOTFROM)_$(OPMODE)
else ifeq ($(BUILTINALL),y)
DIR_TARGETOUTPUT		= build$(BOARD)_$(OPMODE)
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
					-D$(OPMODE)


ifeq ($(OPMODE) , aarch32)
CFLAGS				+=	-msoft-float				\
					-mstructure-size-boundary=32
endif

ifeq ($(OPMODE) , aarch64)
ASFLAG				+=	-march=$(ARCH) -mcpu=$(CPU)

CFLAGS				+=	-mcmodel=small				\
					-march=$(ARCH)
endif

