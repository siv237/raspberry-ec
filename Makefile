PACKAGES := libconfig
PACKAGES_CFLAGS := $(shell pkg-config --cflags $(PACKAGES))
PACKAGES_LIBS := $(shell pkg-config --libs $(PACKAGES))

ecd: ecd.o
	gcc -o ecd ecd.o -lbcm2835 -lwiringPi -lm -Wall $(PACKAGES_LIBS)
	@echo "Building complite...\n"

ecd.o: ecd.c
	gcc $(PACKAGES_CFLAGS) -c -o ecd.o ecd.c 
install:
	make ecd
	@echo "Building complite...\n"
	@cp ecd /usr/bin/
	@cp conf/ecd.conf /etc/
	@echo "Create init.d scripts..."
	@cp conf/ecd /etc/init.d/
	@chmod +x /etc/init.d/ecd
	@update-rc.d ecd defaults
reinstall:
	make ecd
	echo "Building complite...\n"
	service ecd stop
	echo "Reinstall service ecd...\n"
	cp ecd /usr/bin/
	cp conf/ecd.conf /etc/
	echo "Create init.d scripts..."
	cp conf/ecd /etc/init.d/
	chmod +x /etc/init.d/ecd
	update-rc.d ecd defaults
	service ecd start
