
lockrun:  
	gcc $(CFLAGS) lockrun.c -o lockrun

install: build
	install -d $(DESTDIR)/usr/bin/ 
	install ./lockrun $(DESTDIR)/usr/bin/

build: lockrun

clean:
	rm -f lockrun
