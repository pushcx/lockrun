
SHELL = /bin/sh
PREFIX ?= /usr/local

prefix = $(PREFIX)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1

all: build man

lockrun:  
	gcc $(CFLAGS) lockrun.c -o lockrun

lockrun.1: README.markdown
	ronn < README.markdown > lockrun.1

build: lockrun

man: lockrun.1

install: build
	install -d $(DESTDIR)$(bindir) 
	install ./lockrun $(DESTDIR)$(bindir)

install-man: man
	install -d $(DESTDIR)$(man1dir)
	install lockrun.1 $(DESTDIR)$(man1dir)
	gzip $(DESTDIR)$(man1dir)/lockrun.1

clean:
	rm -f lockrun lockrun.1 lockrun.1.gz
