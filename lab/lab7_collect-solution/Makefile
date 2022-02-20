# TMote Sky (emulated in Cooja) is the default target
ifndef TARGET
	TARGET = sky
else
# To compile for Zolertia Firefly (testbed) use the following target and board
# Don't forget to make clean when you are changing the board
	ifeq ($(TARGET), zoul)
		BOARD	?= firefly
		LDFLAGS += -specs=nosys.specs # For Zolertia Firefly only
	endif
endif

DEFINES=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = app
# CONTIKI_PROJECT = contiki-collect/app-collect # To compile Contiki Collect 

PROJECT_SOURCEFILES += my_collect.c

all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RIME = 1
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
