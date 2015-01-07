###########################################################################
# Build Version info
###########################################################################
VERINFO				= V02

###########################################################################
# Build Environment
###########################################################################
DEBUG				= n

CHIPNAME			= S5P6818

BOOTFROM			= USB
#BOOTFROM			= SPI
#BOOTFROM			= SDMMC
#BOOTFROM			= SDFS
#BOOTFROM			= NAND
#BOOTFROM			= UART

#BOARD				= _pyxis
#BOARD				= _lynx
#BOARD				= _vtk
#BOARD				= _drone
#BOARD				= _svt

# cross-tool pre-header
ifeq ($(OS),Windows_NT)
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-none-eabi-
else
CROSS_TOOL_TOP			=
CROSS_TOOL			= $(CROSS_TOOL_TOP)arm-eabi-
endif

###########################################################################
# Top Names
###########################################################################
PROJECT_NAME			= $(CHIPNAME)_2ndboot
TARGET_NAME			= $(PROJECT_NAME)_$(VERINFO)$(BOARD)_$(BOOTFROM)
LDS_NAME			= peridot_2ndboot


###########################################################################
# Directories
###########################################################################
DIR_PROJECT_TOP			=

DIR_OBJOUTPUT			= obj
DIR_TARGETOUTPUT		= build$(BOARD)_$(BOOTFROM)

CODE_MAIN_INCLUDE		=

###########################################################################
# Build Environment
###########################################################################
CPU				= cortex-a9
CC				= $(CROSS_TOOL)gcc
LD 				= $(CROSS_TOOL)ld
AS 				= $(CROSS_TOOL)as
AR 				= $(CROSS_TOOL)ar
MAKEBIN				= $(CROSS_TOOL)objcopy
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

ASFLAG				= -D__ASSEMBLY__

CFLAGS				+=	-g -Wall				\
					-Wextra -ffreestanding -fno-builtin	\
					-msoft-float				\
					-mlittle-endian				\
					-mcpu=$(CPU)				\
					-mstructure-size-boundary=32		\
					$(CODE_MAIN_INCLUDE)			\
					-D__arm -DLOAD_FROM_$(BOOTFROM)
