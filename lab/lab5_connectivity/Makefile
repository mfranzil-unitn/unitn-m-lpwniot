# For Zoulertia Firefly (testbed) use the following target and board
TARGET	?= zoul
BOARD	?= firefly

# For Zolertia Firefly only
LDFLAGS += -specs=nosys.specs

DEFINES=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = connectivity

all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RIME = 1
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
