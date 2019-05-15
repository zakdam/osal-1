######## Build options ########

verbose = 0

######## Build setup ########

# SRCROOT should always be the current directory
SRCROOT         = $(FREERTOS_ROOT)

# .o directory
ODIR            = $(CURDIR)/obj

# Source VPATHS
VPATH           += $(SRCROOT)/Source
VPATH	        += $(SRCROOT)/Source/portable/MemMang
VPATH	        += $(SRCROOT)/Source/portable/GCC/POSIX
VPATH           += $(SRCROOT)/Demo/Common
VPATH			+= $(SRCROOT)/Project/FileIO
VPATH			+= $(SRCROOT)/Project

# FreeRTOS Objects
LIB_C_FILES			+= croutine.c
LIB_C_FILES			+= event_groups.c
LIB_C_FILES			+= list.c
LIB_C_FILES			+= queue.c
LIB_C_FILES			+= tasks.c
LIB_C_FILES			+= timers.c

# portable Objects
LIB_C_FILES			+= heap_3.c
LIB_C_FILES			+= port.c

# Demo Objects
BIN_C_FILES			+= Minimal/blocktim.c
BIN_C_FILES			+= Minimal/countsem.c
BIN_C_FILES			+= Minimal/GenQTest.c
BIN_C_FILES			+= Minimal/QPeek.c
BIN_C_FILES			+= Minimal/recmutex.c
BIN_C_FILES			+= Full/BlockQ.c
BIN_C_FILES			+= Full/death.c
BIN_C_FILES			+= Full/dynamic.c
BIN_C_FILES			+= Full/flop.c
BIN_C_FILES			+= Full/integer.c
BIN_C_FILES			+= Full/PollQ.c
BIN_C_FILES			+= Full/semtest.c
BIN_C_FILES			+= Full/print.c

BIN_C_FILES			+= Minimal/AbortDelay.c
BIN_C_FILES			+= Minimal/EventGroupsDemo.c
BIN_C_FILES			+= Minimal/IntSemTest.c
BIN_C_FILES			+= Minimal/QueueSet.c
BIN_C_FILES			+= Minimal/QueueSetPolling.c
BIN_C_FILES			+= Minimal/QueueOverwrite.c
BIN_C_FILES			+= Minimal/TaskNotify.c
BIN_C_FILES			+= Minimal/TimerDemo.c

# Main Object
BIN_C_FILES			+= main.c

# Include Paths
INCLUDES        += -I$(SRCROOT)/Source/include
INCLUDES        += -I$(SRCROOT)/Source/portable/GCC/POSIX/
INCLUDES        += -I$(SRCROOT)/Demo/Common/include
INCLUDES        += -I$(CURDIR)  # local FreeRTOSConfig.h substitution

# Generate LIB_OBJS names
LIB_OBJS = $(patsubst %.c,%.o,$(LIB_C_FILES))

# Generate BIN_OBJS names
BIN_OBJS = $(patsubst %.c,%.o,$(BIN_C_FILES))

######## C Flags ########

# Warnings
CWARNS += -W
CWARNS += -Wall
CWARNS += -Werror
CWARNS += -Wextra
CWARNS += -Wformat
CWARNS += -Wmissing-braces
CWARNS += -Wno-cast-align
CWARNS += -Wparentheses
CWARNS += -Wshadow
CWARNS += -Wno-sign-compare
CWARNS += -Wswitch
CWARNS += -Wuninitialized
CWARNS += -Wunknown-pragmas
CWARNS += -Wunused-function
CWARNS += -Wunused-label
CWARNS += -Wunused-parameter
CWARNS += -Wunused-value
CWARNS += -Wunused-variable
CWARNS += -Wmissing-prototypes

#CWARNS += -Wno-unused-function

CFLAGS += -m32
CFLAGS += -DDEBUG=1
#CFLAGS += -g -DUSE_STDIO=1 -D__GCC_POSIX__=1
CFLAGS += -g -UUSE_STDIO -D__GCC_POSIX__=1
ifneq ($(shell uname), Darwin)
CFLAGS += -pthread
endif

# MAX_NUMBER_OF_TASKS = max pthreads used in the POSIX port. 
# Default value is 64 (_POSIX_THREAD_THREADS_MAX), the minimum number required by POSIX.
CFLAGS += -DMAX_NUMBER_OF_TASKS=300

CFLAGS += $(INCLUDES) $(CWARNS) -O2

LIBS += FreeRTOS_Sim.a

######## Makefile targets ########

# Rules
.PHONY : all lib bin
all: bin


# Fix to place .o files in ODIR
_LIB_OBJS = $(patsubst %,$(ODIR)/%,$(LIB_OBJS))

_BIN_OBJS = $(patsubst %,$(ODIR)/%,$(BIN_OBJS))

$(ODIR)/%.o: %.c
	@mkdir -p $(dir $@)
# If verbose, print gcc execution, else hide
ifeq ($(verbose),1)
	@echo ">> Compiling $<"
	$(CC) $(CFLAGS) -c -o $@ $<
else
	@echo ">> Compiling $(notdir $<)"
	@$(CC) $(CFLAGS) -c -o $@ $<
endif

FreeRTOS_Sim.a: $(_LIB_OBJS)
	@echo ">> Linking $@..."
ifeq ($(verbose),1)
	$(AR) -rcs $@ $^
else
	@$(AR) -rcs $@ $^
endif

	@echo "-------------------------"
	@echo "BUILD COMPLETE: $@"
	@echo "-------------------------"

lib: FreeRTOS_Sim.a

bin: lib FreeRTOS_Sim

FreeRTOS_Sim: $(_BIN_OBJS)
	@echo ">> Linking $@..."
ifeq ($(verbose),1)
	$(CC) $(CFLAGS) $^ $(LINKFLAGS) $(LIBS) -o $@
else
	@$(CC) $(CFLAGS) $^ $(LINKFLAGS) $(LIBS) -o $@
endif

	@echo "-------------------------"
	@echo "BUILD COMPLETE: $@"
	@echo "-------------------------"


.PHONY : clean
clean:
	@-rm -rf $(ODIR) FreeRTOS_Sim.a FreeRTOS_Sim
	@echo "--------------"
	@echo "CLEAN COMPLETE"
	@echo "--------------"
