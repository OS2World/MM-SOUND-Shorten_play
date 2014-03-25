#
#  flacplay.dll makefile for gnu MAKE and g++
#

# build environment
#-----------------------
#  gcc -v
# 
# Reading specs from J:/usr/gcc/3.3.5-csd3/lib/gcc-lib/i386-pc-os2-emx/3.3.5/specs
# Configured with: D:/CODING/LIBC/SVN/BRANCHES/LIBC-0.6/src/gcc/configure 
#  --enable-clh --enable-threads=os2 --enable-shared=libgcc,bfd,opcodes --enable-nls 
#  --without-included-gettext --with-local-prefix=D:/CODING/LIBC/SVN/BRANCHES/LIBC-0.6/TOOLS/x86.os2/gcc/staged 
#  --prefix=/gcc --with-gnu-as --disable-libgcj --enable-languages=c,c++
# Thread model: os2
# gcc version 3.3.5 (Bird Build 2007-06-10 14:30)
# 
#  make -v
# GNU Make version 3.77, by Richard Stallman and Roland McGrath.
# Copyright (C) 1988, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98
# 	Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.
# There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.
# 
# Report bugs to <bug-make@gnu.org>.
# 
#

NAME = shn
LIBS = -L/usr/test/lib -lFLAC++ -lFLAC -logg -lm
INCLUDES  = -Iinclude -I/usr/test/include
DEPDIR = .deps
PRODVERSION :=$(shell cat Version)
PRODBUILD   :=$(shell cat Build)
PRODNAME     =Shorten
AUTHOR	     =ntim
CONTACTS     =<ntim@softhome.net>
BUILDHOST    =$(shell echo $(HOSTNAME) | awk '{printf "%13s",$$1}' )

#		(empty)- no debug, 1 - base, 2 - more, 3 - most verbose
DEBUG=-DDEBUG=	
DEFS=$(DEBUG) -DHAVE_CONFIG_H -DPRODNAME=\"$(PRODNAME)\" -DPRODVERSION=\"$(PRODVERSION)\" -DPRODBUILD=\"$(PRODBUILD)\" 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
ADDOBJ = src/shorten.o src/vario.o src/other.o 

CC = g++ -c
LD = g++ -s
RM = rm

CFLAGS   = -Zmts -Wall -O3
TARGET = $(NAME)play

all: Version Build $(TARGET).dll
test: all
	cp shnplay.dll J:/Progs/MM/PM123/
	cmd /C "start /C /B /MIN J:\\Progs\\MM\\pm123\\pm123add.cmd `pwd | sed -e 's/\\//\\\\/g'`\\test.shn"


DEF      = $(NAME)play.def
OBJECTS  = $(NAME)play.o id3tag.o decoder.o decoder++.o debug.o

objects:
	cd src && make objects DEBUG=$(DEBUG)

$(TARGET).dll : objects $(OBJECTS) $(ADDOBJ) $(DEF) Makefile
	$(LD) -Zdll -Zmts $(CFLAGS) -o $(TARGET).dll $(OBJECTS) $(ADDOBJ) $(LIBS) $(DEF)

clean:
	-@$(RM) ..\*.dll $(OBJECTS) *.bak $(ADDOBJ) $(DEF) ..\*.bak 2> nul

# bldlevel support:
$(NAME)play.def: $(NAME)play_form.def Version Build Makefile
	DATE=`date +"%d %b %Y %X"` ; \
	sed -e 's/^DESCRIPTION .*/DESCRIPTION "@#'$(AUTHOR)':'$(PRODVERSION)'.'$(PRODBUILD)'#@##1## '"$$DATE"'$(BUILDHOST)::en:ru:$(PRODBUILD)::@@$(PRODNAME) plug-in for PM123 (C)2008 $(AUTHOR) $(CONTACTS)"/g' \
	    $< > $@

-include ./$(DEPDIR)/*.Po

.cpp.o: Version Build Makefile
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

