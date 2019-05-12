###############################################################################
# File: link-rules.mak
#
# Purpose:
#   Makefile for linking code and producing an executable image.
#
# History:
#
###############################################################################


##
## Linker flags that are needed
##
LDFLAGS ?= $(OSAL_M32)

# FreeRTOS static library
LIBS = $(OSAL_BUILD)/freertos-target/FreeRTOS_Sim.a

##
## Libraries to link in
##
LIBS += -lpthread 


##
## OSAL Core Link Rule
## 
$(APPTARGET).$(APP_EXT): $(OBJS)
	@echo "Linking..."
	$(COMPILER) $(DEBUG_FLAGS) -o $(APPTARGET).$(APP_EXT) $(OBJS) $(CORE_OBJS) $(LDFLAGS) $(LIBS)
	

