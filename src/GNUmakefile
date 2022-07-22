#
#   Alaska Communications UDP Echo Tools
#   Copyright (C) 2020 Alaska Communications
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#      1. Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#
#      2. Redistributions in binary form must reproduce the above copyright
#         notice, this list of conditions and the following disclaimer in the
#         documentation and/or other materials provided with the distribution.
#
#      3. Neither the name of the copyright holder nor the names of its
#         contributors may be used to endorse or promote products derived from
#         this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#


include ../GNUmakefile.version


GIT_VERSION				= $(shell git describe --long 2>/dev/null|sed -e s/-/./g -e s/^v//g)
ifeq ($(GIT_VERSION),) 
   PACKAGE_VERSION			= $(VERSION)
else
   PACKAGE_VERSION			= $(GIT_VERSION)
endif


CFLAGS					= -O2 \
					  -std=gnu11 \
					  -pedantic \
					   -W \
					  -Wall \
					  -Wno-unknown-pragmas \
					  -Wno-format-nonliteral \
					  -Wno-reserved-id-macro \
					  -Wno-unused-macros \
					  -DPACKAGE_VERSION='"$(PACKAGE_VERSION)"'
INSTALL					?= install
PREFIX					?= /usr/local
INSTALL_OPTS				?= --strip -D


PROGS					= akcom-udpecho \
					  akcom-udpechod


.PHONY: all all-progs install clean uninstall notice


all: notice


all-progs: $(PROGS)


akcom-udpecho.o: akcom-udpecho.c


akcom-udpecho: akcom-udpecho.o


akcom-udpechod.o: akcom-udpechod.c


akcom-udpechod: akcom-udpechod.o


install: notice


install-progs: $(PROGS)
	$(INSTALL) $(INSTALL_OPTS) akcom-udpecho  $(DESTDIR)$(PREFIX)/bin/akcom-udpecho
	$(INSTALL) $(INSTALL_OPTS) akcom-udpechod $(DESTDIR)$(PREFIX)/sbin/akcom-udpechod


notice:
	@echo ""
	@echo ""
	@echo "   ##########################################################"
	@echo "   #                                                        #"
	@echo "   #  The preferred method of compiling is to the GNU       #"
	@echo "   #  autotools:                                            #"
	@echo "   #                                                        #"
	@echo "   #     $$ mkdir build && cd build                          #"
	@echo "   #     $$ ../autogen.sh # (if obtaining source with GitI)  #"
	@echo "   #     $$ ../configure                                     #"
	@echo "   #     $$ make                                             #"
	@echo "   #     $$ make install                                     #"
	@echo "   #                                                        #"
	@echo "   #  To use make without autotools, run the following from #"
	@echo "   #  the source ('src/') directory:                        #"
	@echo "   #                                                        #"
	@echo "   #     $$ make all-progs                                   #"
	@echo "   #     $$ make install-progs                               #"
	@echo "   #                                                        #"
	@echo "   ##########################################################"
	@echo ""
	@echo ""


uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/akcom-udpecho
	rm -f $(DESTDIR)$(PREFIX)/sbin/akcom-udpechod


clean: notice
	rm -f *.o *.lo $(PROGS)


# end of Makefile
