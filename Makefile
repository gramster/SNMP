# Uncomment these for UNIX

project_files := $(shell aegis -l pf -terse -p $(project) -c $(change))
change_files  := $(shell aegis -l cf -terse -p $(project) -c $(change))
source_files  := $(sort $(project_files) $(change_files))
ccobject_files:= $(patsubst %.cc,%.o,$(filter %.cc,$(source_files)))
cobject_files := $(patsubst %.c,%.o,$(filter %.c,$(source_files)))
object_files  := $(cobject_files) $(ccobject_files)

CC=gcc
DEBUG=-g
OPTS=$(DEBUG) -Wall
LOPTS=$(DEBUG)
LIBS=
XSUF=
OSUF=.o
LSUF=.a
DEFINES=-Dlinux
ASN1CCTARGET=-o asn1cc
SNMPGETTARGET=-o snmpget
BROWSETARGET=-o browse

# Uncomment these for DOS

#CC=bcc
#DEBUG=-v
#OPTS=-P -ml $(DEBUG)
#LOPTS=-ls -ml $(DEBUG)
#LIBS=
#XSUF=.exe
#OSUF=.obj
#LSUF=.lib
#DEFINES=-DDEBUG -DTEST
#ASN1CCTARGET=
#SNMPGETTARGET=
#BROWSETARGET=

#############################
# Don't change below here

# Library components

SCREENOBJ=screen$(OSUF) config$(OSUF) ed$(OSUF) hypertxt$(OSUF) ansicurs$(OSUF)
CLSRVOBJ=clsrv$(OSUF) connect$(OSUF)
ASN1OBJ=asn1pars$(OSUF) asn1lex$(OSUF) asn1err$(OSUF) asn1name$(OSUF) asn1stab$(OSUF) 
SNMPOBJ=mibtree$(OSUF) berobj$(OSUF) snmpman$(OSUF)

CFLAGS= $(OPTS) $(DEFINES)
LFLAGS= $(LOPTS)

all: libs asn1cc$(XSUF) browse$(XSUF)

libs: libscreen$(LSUF) libclsrv$(LSUF) libasn1$(LSUF) libsnmp$(LSUF)

libscreen$(LSUF): $(SCREENOBJ)
	rm -f libscreen$(LSUF)
	ar -r libscreen$(LSUF) $(SCREENOBJ)
	ranlib libscreen$(LSUF)
	
libclsrv$(LSUF): $(CLSRVOBJ)
	rm -f libclsrv$(LSUF)
	ar -r libclsrv$(LSUF) $(CLSRVOBJ)
	ranlib libclsrv$(LSUF)
	
libasn1$(LSUF): $(ASN1OBJ)
	rm -f libasn1$(LSUF)
	ar -r libasn1$(LSUF) $(ASN1OBJ)
	ranlib libasn1$(LSUF)
	
libsnmp$(LSUF): $(SNMPOBJ)
	rm -f libsnmp$(LSUF)
	ar -r libsnmp$(LSUF) $(SNMPOBJ)
	ranlib libsnmp$(LSUF)
	
# The link order here is important under Linux. ncurses must be linked
# before other libraries, or the getch routine doesn't work properly...

browse$(XSUF): browse$(OSUF) libscreen$(LSUF) libclsrv$(LSUF) libsnmp$(LSUF)
	$(CC) $(BROWSETARGET) $(LFLAGS) browse$(OSUF) -L. -lscreen -lncurses -lsnmp -lclsrv $(LIBS)

snmpget$(XSUF): snmpget$(OSUF) libsnmp$(LSUF)
	$(CC) $(SNMPGETTARGET) $(LFLAGS) snmpget$(OSUF) -L. -lsnmp

asn1cc$(XSUF): asn1cc$(OSUF) libasn1$(LSUF)
	$(CC) $(ASN1CCTARGET) $(LFLAGS) asn1cc$(OSUF) -L. -lasn1

%.o: %.cc
	rm -f $*.o
	$(CC) $(CFLAGS) -c $*.cc

%.o: %.c
	rm -f $*.o
	$(CC) $(CFLAGS) -c $*.c

%.d: %.cc
	rm -f $*.d
	$(CC) $(CFLAGS) -M $*.cc | sed 's/^\(.*\).o :/\1.o \1.d :/' > $*.d

%.d: %.c
	rm -f $*.d
	$(CC) $(CFLAGS) -M $*.c | sed 's/^\(.*\).o :/\1.o \1.d :/' > $*.d

include $(patsubst %.o,%.d,$(object_files))

