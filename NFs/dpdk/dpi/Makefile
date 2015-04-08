ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET=x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = dpi

# all source are stored in SRCS-y
SRCS-y := main.c init.c runtime.c 

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

#CFLAGS += -g

#Uncomment the next line to enable the usage of the semaphore in the NF
#CFLAGS += -DENABLE_SEMAPHORE

#Uncomment the next line to enable the log in the NF
CFLAGS += -DENABLE_LOG

LDFLAGS += -lpcre

include $(RTE_SDK)/mk/rte.extapp.mk

