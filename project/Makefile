ifndef TARGET
	TARGET = sky
else
	# For Zolertia Firefly (testbed) use the following target and board
	# Don't forget to make clean when you are changing the board
	ifeq ($(TARGET), zoul)
		BOARD	?= firefly
		LDFLAGS += -specs=nosys.specs # For Zolertia Firefly only
	endif
endif

ifdef CHECKS
# C version, enable asm and warnings
	ifeq ($(CHECKS), true)
		CFLAGS += -std=c89
		CFLAGS += -fasm
		CFLAGS += -Wextra -Wall -pedantic -Wundef \
				  -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes \
				  -Wstrict-overflow=5 -Wwrite-strings -Wwrite-strings -Waggregate-return \
				  -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion \
				  -Wunreachable-code -Wfloat-equal -Werror -Wunused-variable
	else
		CFLAGS += -std=c99
		CFLAGS += -fno-asm
	endif
endif

DEFINES=PROJECT_CONF_H=\"src/conf/project_conf.h\"
CONTIKI_PROJECT = src/app
PROJECTDIRS += src/tools src/etc src/timing src/log
PROJECT_SOURCEFILES += simple-energest.c etc.c timing.c log.c

all: $(CONTIKI_PROJECT)
	mv src/app.* .
	mv app.c src/app.c

docs: cleanall
	doxygen
	cd docs/latex && $(MAKE)

cleanall: distclean
	rm -f symbols.c symbols.h COOJA.log
	rm -rf obj_zoul
	rm -f COOJA.testlog
	rm -f app.sky app.elf app.zoul app.hex
	rm -f src/app-zoul.map


CONTIKI_WITH_RIME = 1
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
