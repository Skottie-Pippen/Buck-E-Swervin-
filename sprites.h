obj-m := video.o KEY_SW.o
GAME_NAME := bucke_swervin
C_SOURCES := main.c pushbutton.c audio.c

all: modules $(GAME_NAME)

modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules EXTRA_CFLAGS="-Wno-declaration-after-statement"

$(GAME_NAME): $(C_SOURCES) defines.h audio.h
	gcc -o $(GAME_NAME) $(C_SOURCES) -lrt -lm -lpthread -std=gnu99

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(GAME_NAME)
	rm -f *.o *~
	rm -f Module.symvers modules.order
	rm -f *.mod.c *.mod *.ko

.PHONY: all modules clean