PACKAGES := libconfig
PACKAGES_CFLAGS := $(shell pkg-config --cflags $(PACKAGES))
PACKAGES_LIBS := $(shell pkg-config --libs $(PACKAGES))

test: ecd.o
	gcc -o ecd ecd.o -lbcm2835 -lwiringPi -lm -Wall $(PACKAGES_LIBS)
	@echo "Building complite...\n"
	@cp ecd /usr/bin/
	@cp conf/ecd.conf /etc/
	@ecd
ecd.o: ecd.c
	gcc $(PACKAGES_CFLAGS) -c -o ecd.o ecd.c 
