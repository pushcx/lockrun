
lockrun:  
	gcc $(CFLAGS) lockrun.c -o lockrun

install: build
	install -d $(bindir) 
	install ./lockrun $(bindir)
	install lockrun.1 $(man1dir)
	gzip $(man1dir)/lockrun.1

build: lockrun

clean:
	rm -f lockrun
