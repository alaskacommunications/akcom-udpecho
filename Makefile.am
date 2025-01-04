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
#   @configure_input@
#
#   Makefile.am - automate compiling on a unix platform
#

# Sub directories
SUBDIRS					= $(subdirs)
DIST_SUBDIRS				= $(subdirs)


# GNU make options
V					= 0


# Global flags
AM_LIBS					=
AM_LDFLAGS				=
AM_CFLAGS				= -O2 $(CFLAGS_WARNINGS)
AM_CXXFLAGS				= -O2 @AM_CXXFLAGS@
AM_OBJCFLAGS				= -O2 @AM_OBJCFLAGS@
AM_CPPFLAGS				= -O2 \
					  -DHAVE_CONFIG_H=1 \
					  -I$(top_builddir)/include \
					  -I$(top_srcdir)/include \
					  -DSYSCONFDIR=\"$(sysconfdir)\"
ACLOCAL_AMFLAGS				= -I m4 -W all -W error
AM_MAKEINFOFLAGS                        = --no-split
DEFS					=


# automake targets
bin_PROGRAMS				= src/akcom-udpecho \
					  src/akcom-udpechod
check_PROGRAMS				=
doc_DATA				=
noinst_DATA				=
EXTRA_PROGRAMS				=
include_HEADERS				=
info_TEXINFOS				=
lib_LIBRARIES				=
lib_LTLIBRARIES				=
EXTRA_LIBRARIES				=
EXTRA_LTLIBRARIES			=
man_MANS				= docs/akcom-udpecho.1 \
					  docs/akcom-udpechod.8
noinst_HEADERS				=
noinst_PROGRAMS				=
noinst_LIBRARIES			=


# lists
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT	=
BUILT_SOURCES				=
TESTS					=
XFAIL_TESTS				=
EXTRA_MANS				=
EXTRA_DIST				= $(noinst_HEADERS) \
					  docs/akcom-udpecho.1.in \
					  docs/akcom-udpechod.8.in \
					  docs/TR-143.pdf \
					  docs/rfc862.txt \
					  ChangeLog.md \
					  NEWS.md \
					  README.md \
					  TODO.md
CLEANFILES				= $(builddir)/a.out   $(srcdir)/a.out \
					  $(builddir)/*/a.out $(srcdir)/*/a.out \
					  config.h.in~ $(srcdir)/config.h.in~ \
					  $(man_MANS) \
					  $(pkgdata_DATA) \
					  $(bin_SCRIPTS) \
					  $(sbin_SCRIPTS) \
					  @PACKAGE_TARNAME@-*.tar.* \
					  @PACKAGE_TARNAME@-*.txz \
					  @PACKAGE_TARNAME@-*.zip
DISTCHECK_CONFIGURE_FLAGS		= --enable-strictwarnings \
					  LDFLAGS="$(LDFLAGS)" \
					  CFLAGS="$(CFLAGS)" \
					  CPPFLAGS="$(CPPFLAGS)"


# macros for src/akcom-udpecho
src_akcom_udpecho_DEPENDENCIES		= Makefile
src_akcom_udpecho_CPPFLAGS		= -DPROGRAM_NAME="\"akcom-udpecho\"" $(AM_CPPFLAGS)
src_akcom_udpecho_SOURCES		= src/akcom-udpecho.c


# macros for src/akcom-udpechod
src_akcom_udpechod_DEPENDENCIES		= Makefile
src_akcom_udpechod_CPPFLAGS		= -DPROGRAM_NAME="\"akcom-udpechod\"" $(AM_CPPFLAGS)
src_akcom_udpechod_SOURCES		= src/akcom-udpechod.c


# Makefile includes
GIT_PACKAGE_VERSION_DIR=include
SUBST_EXPRESSIONS =
include contrib/bindletools/build-aux/makefile-subst.am
include contrib/bindletools/build-aux/makefile-version.am


# custom targets
.PHONY:

docs/akcom-udpecho.1: Makefile $(srcdir)/docs/akcom-udpecho.1.in
	@$(do_subst_dt)


docs/akcom-udpechod.8: Makefile $(srcdir)/docs/akcom-udpechod.8.in
	@$(do_subst_dt)


# local targets
install-exec-local:

install-data-local:

install-exec-hook:

install-data-hook:

uninstall-local:

clean-local:

distclean-local:
	rm -fR $(srcdir)/autom4te.cache

maintainer-clean-local:

# end of automake file
