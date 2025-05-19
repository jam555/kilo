all: kilo

.PHONY: all kilo clean

headers: kilo.h appenbuf.h term.h syntax.h edrows.h edfind.h edevents.h thoutext/edtools.h
sources: kilo.c appenbuf.c term.c syntax.c edrows.c edfind.c edevents.c thoutext/edtools.c
kilo: headers sources Makefile
	$(CC) -o kilo kilo.c appenbuf.c term.c syntax.c edrows.c edfind.c edevents.c thoutext/edtools.c -Wall -W -pedantic -std=c99

clean:
	rm kilo
