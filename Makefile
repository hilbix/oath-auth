#

TARG=google-auth
CFLAGS=-Wall -O3 -g
LDLIBS=-ldl -lpam
PREFIX=$(HOME)
TESTARGS="`read KEY < "$$HOME/.google_authenticator" && oathtool -b --totp "$$KEY"`"

.PHONY: all install clean distclean gdb strace ltrace latrace

all:	$(TARG)

test:	all
	! './$(TARG)' mustfail
	'./$(TARG)' $(TESTARGS)

gdb:	all
	gdb -ex run --args './$(TARG)' $(TESTARGS)

strace:	all
	strace './$(TARG)' $(TESTARGS)

ltrace:	all
	ltrace './$(TARG)' $(TESTARGS)

latrace:	all
	latrace -A './$(TARG)' $(TESTARGS)

install:
	cp '$(TARG)' '$(PREFIX)/bin/$(TARG)'
	strip -s '$(PREFIX)/bin/$(TARG)'

clean:	distclean

distclean:
	rm -f '$(TARG)'

