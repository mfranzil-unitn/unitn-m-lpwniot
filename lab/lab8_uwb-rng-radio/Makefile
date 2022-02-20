CONTIKI_PROJECT = rng-init rng-resp
all: $(CONTIKI_PROJECT)

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += rng-support.c

# Enable Rime stack
CONTIKI_WITH_IPV4 = 0
CONTIKI_WITH_IPV6 = 0
CONTIKI_WITH_RIME = 1

UWB_CONTIKI ?= ../..
include $(UWB_CONTIKI)/Makefile.uwb
