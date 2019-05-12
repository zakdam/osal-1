###############################################################################
# File: osal.mak
#
# Purpose:
#   Compile the OS Abstraction layer library.
#
###############################################################################

# Subsystem produced by this makefile.
TARGET = osal.o

#==============================================================================
# Object files required to build subsystem.

OBJS = osapi.o osfileapi.o osfilesys.o  osnetwork.o osloader.o ostimer.o

#==============================================================================
# FreeRTOS object files

# Source VPATHS
VPATH += $(FREERTOS_ROOT)/Source
VPATH += $(FREERTOS_ROOT)/Source/portable/MemMang
VPATH += $(FREERTOS_ROOT)/Source/portable/GCC/POSIX
VPATH += $(FREERTOS_ROOT)/Demo/Common
VPATH += $(FREERTOS_ROOT)/Project/FileIO
VPATH += $(FREERTOS_ROOT)/Project

# FreeRTOS Objects
OBJS += croutine.o
OBJS += event_groups.o
OBJS += list.o
OBJS += queue.o
OBJS += tasks.o
OBJS += timers.o

# portable Objects
OBJS += heap_3.o
OBJS += port.o

# Include Paths
INCLUDES += -I$(FREERTOS_ROOT)/Source/include
INCLUDES += -I$(FREERTOS_ROOT)/Source/portable/GCC/POSIX/
INCLUDES += -I$(FREERTOS_ROOT)/Demo/Common/include
INCLUDES += -I$(FREERTOS_ROOT)/Project


#==============================================================================
# Source files required to build subsystem; used to generate dependencies.

SOURCES = $(OBJS:.o=.c)

