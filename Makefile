# THis should currently be compatible with both GNU's and Microsoft's make.

EDHEADERS= thoutext/edrows.h thoutext/edtools.h
EDSRC= thoutext/edrows.c thoutext/edtools.c

ROOTHEADERS= kilo.h appenbuf.h term.h syntax.h edfind.h edevents.h
ROOTSRC= kilo.c appenbuf.c term.c syntax.c edfind.c edevents.c

all: kilo

headers: $(ROOTHEADERS) $(EDHEADERS)
sources: $(ROOTSRC) $(EDSRC)
kilo: headers sources Makefile
	$(CC) -o kilo $(ROOTSRC) $(EDSRC) -Wall -W -pedantic -std=c99

clean:
	rm kilo
