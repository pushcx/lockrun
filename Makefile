
SHELL = /bin/sh
PREFIX ?= /usr/local

prefix = $(PREFIX)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1


lockrun:  
	gcc $(CFLAGS) lockrun.c -o lockrun

install: build
	install -d $(DESTDIR)$(bindir) 
	install ./lockrun $(DESTDIR)$(bindir)
	
	install -d $(DESTDIR)$(man1dir)
	install lockrun.1 $(DESTDIR)$(man1dir)
	gzip $(DESTDIR)$(man1dir)/lockrun.1

build: lockrun

clean:
	rm -f lockrun
