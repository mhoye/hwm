CFLAGS=-w						#livin' dangerously.
COPTS=-lX11 -lXinerama
C=gcc

all:	hwm

hwm:
	$(CC) $(CFLAGS) hwm.c -o hwm $(COPTS)

install:
	if [ -e /usr/share/xesssions/ ] ; then cp hwm.desktop /usr/share/xessions/ ; fi
	chmod +x hwm
	cp hwm /usr/bin/hwm

uninstall: 
	if [ -e /usr/share/xesssions/hwm.desktop ] ; then rm /usr/share/xessions/hwm.dekstop ; fi
	if [ -e /usr/bin/hwm ] ; then rm /usr/bin/hwm ; fi

clean:
	if [ -e ./hwm ] ; then rm ./hwm ; fi
