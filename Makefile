# This should currently be compatible with both GNU's and Microsoft's make.

COMPOPTS= -Wall -W -pedantic -std=c99 \
	-Wmultistatement-macros -Wparentheses -Wswitch-default -Wswitch-enum \
	-Wunknown-pragmas -Wbidi-chars=any,ucn -Wduplicated-cond -Wcast-align \
	-Wconversion -Wdangling-else -Wsign-compare \
	-Wsizeof-array-div -Wsizeof-pointer-div -Wmemset-elt-size \
	-Wstrict-prototypes -Wold-style-declaration \
	-Wmissing-parameter-type -Wmissing-prototypes -Wmissing-declarations \
	-Wmissing-declarations -Wno-odr \
	-Wredundant-decls
# -Wno-discarded-qualifiers
# -Wno-error=incompatible-pointer-types
# -Wtrailing-whitespace=any
# -Wleading-whitespace=tabs
# -ftabstop=4
# -Wtrampolines
# -Wfloat-equal
# -Wshadow
# -Wshadow=compatible-local
# -Wframe-larger-than=byte-size
# -Wstack-usage=byte-size
# -Wtype-limits
# -Wundef
# -Wcast-function-type
# -Wnormalized=[none|id|nfc|nfkc]
# -Wpacked-not-aligned
# -Wpadded
# -Wsign-conversion
#			Are these real?
# -Wenum-int-mismatch
# -Wdeprecated-non-prototype
# -Wmissing-variable-declarations
# -Winvalid-utf8

EDHEADERS= thoutext/edrows.h thoutext/edtools.h
UTILHEADERS= coroutine/coro.h
ROOTHEADERS= kilo.h appenbuf.h term.h syntax.h edfind.h edevents.h

EDSRC= thoutext/edrows.c thoutext/edtools.c
UTILSRC= coroutine/coro.c
ROOTSRC= kilo.c appenbuf.c term.c syntax.c edfind.c edevents.c

all: kilo

headers: $(ROOTHEADERS) $(EDHEADERS) $(UTILHEADERS)
sources: $(ROOTSRC) $(EDSRC) $(UTILSRC)
kilo: headers sources Makefile
	$(CC) -o kilo $(ROOTSRC) $(EDSRC) $(UTILSRC) $(COMPOPTS)

clean:
	rm kilo
