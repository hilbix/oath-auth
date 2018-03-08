#

TARG=google-auth
GOOGLE_PAM_LIB=$(wildcard /lib/security/pam_google_authenticator.so /lib/*/security/pam_google_authenticator.so)
CFLAGS=-Wall -O3 -g -D'GOOGLE_PAM_LIB="$(GOOGLE_PAM_LIB)"'
LDLIBS=-ldl -lpam
PREFIX=/usr/local
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

