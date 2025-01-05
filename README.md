
Alaska Communications UDP Echo Tools
====================================

Copyright (C) 2020 Alaska Communications
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Contents
--------

   * Overview
   * Software Requirements
   * Utilities
     - akcom-udpecho
     - akcom-udpechod
   * Building Package
   * Source Code
   * Package Maintence Notes


Overview
==========

This package contains a UDP echo server and client. Both the client and the
server support RFC 862 (default) and support TR-143 UDPEchoPlus if passed the
`--echoplus` flag.


Software Requirements
=====================

   * GNU GCC 4.2.1/Clang 13.0.0
   * GNU Make 3.81
   * GNU Libtool 2.4
   * GNU Autoconf 2.69
   * GNU Automake 1.16.5
   * Git 1.7.2.3


Utilities
=========

akcom-udpecho
-------------

_akcom-udpecho_ is a shell utility for testing UDP echo servers. _akcom-udpecho_
supports for RFC 862 compliant servers and TR-143 UDPEchoPlus compliant
servers.  If the UDP Echo variant is not specified, _akcom-udpecho_ will 
attempt to determine the variant by examining the __TestRespSN__, 
__TestRespRecvTimeStamp__, and __TestRespReplyTimeStamp__ fields of the
UDPEchoPlus packet for non-zero values.

_akcom-udpecho_ usage:

      Usage: akcom-udpecho [options] host [port]
      OPTIONS:
        -4                        connect via IPv4 only
        -6                        connect via IPv6 only
        -c count                  stop after sending count packets
        -d, --debug               print packet debugging information
        -e, --echoplus            expect echo plus response (default: auto detect)
        -h, --help                print this help and exit
        -i interval               interval between packet (default: 1 sec)
        -r, --rfc                 expect RFC compliant response (default: auto detect)
        -q, --quiet, --silent     do not print messages
        -s packetsize             size of data bytes to be sent. (default: 40 bytes)
        -t sec                    response timeout (default: 5 sec)
        -v, --verbose             enable verbose output
        -V, --version             print version number and exit

Example usage (RFC 862 compliant):

      $ akcom-udpecho -c 5 --rfc udpecho.example.com 30006
      UDPECHO udpecho.example.com:30006 (209.112.131.108:30006): 40 bytes
      udpecho_seq=1 time=17.3 ms
      udpecho_seq=2 time=13.7 ms
      udpecho_seq=3 time=19.0 ms
      udpecho_seq=4 time=18.3 ms
      udpecho_seq=5 time=12.6 ms
      
      --- udpecho.example.com udpecho statistics ---
      5 packets transmitted, 5 packets received, 0.0% packet loss
      round-trip min/avg/max = 12.6/16.1/19.0 ms
      $

Example usage (TR-143 UDPEchoPlus compliant) using server with 100000 us 
random delay:

      $ akcom-udpecho -c 5 --echoplus udpecho.example.com 30006
      UDPECHO udpecho.example.com:30006 (209.112.131.108:30006): 40 bytes
      udpecho_seq=1 failures=0 time=158.0 ms delay=77.9 ms adj_time=80.1 ms
      udpecho_seq=2 failures=0 time=109.1 ms delay=29.2 ms adj_time=79.9 ms
      udpecho_seq=3 failures=0 time=136.1 ms delay=56.2 ms adj_time=79.9 ms
      udpecho_seq=4 failures=0 time=83.1 ms delay=3.0 ms adj_time=80.0 ms
      udpecho_seq=5 failures=0 time=133.2 ms delay=53.1 ms adj_time=80.0 ms
      
      --- udpecho.example.com udpecho statistics ---
      5 packets transmitted, 5 packets received, 0.0% packet loss
      round-trip min/avg/max = 83.1/123.9/158.0 ms
      adjusted round-trip min/avg/max = 79.9/80.0/80.1 ms
      $

akcom-udpechod
--------------

_akcom-udpechod_ is simple UDP echo server.  The __drop__ and __delay__ 
features should not be used concurrently by multiple clients. Currently 
_akcom-udpechod_ processes each packet sequentially  which causes all packets 
received before previous packets have been processed to be skewed when using 
the __drop__ and __delay__ features.

_akcom-udpechod_ usage:

      Usage: akcom-udpechod [options]
      OPTIONS:
        -d num,  --drop num       set packet drop probability [0-99] (default: 0%)
        -D usec, --delay usec     set echo delay range to microseconds (default: 0 us)
        -e,      --echoplus       enable echo plus, not RFC compliant
        -f str,  --facility str   set syslog facility (default: daemon)
        -g gid,  --group gid      setgid to gid (default: none)
        -h,      --help           print this help and exit
        -l addr, --listen addr    bind to IP address (default: all)
        -n,      --foreground     do not fork
        -p port, --port port      list on port number (default: 30006)
        -P file, --pidfile file   PID file (default: /var/run/akcom-udpechod.pid)
        -r,      --rfc            RFC compliant echo protocol (default)
        -u uid,  --user=uid       setuid to uid (default: none)
        -v,      --verbose        enable verbose output
        -V,      --version        print version number and exit

Example usage (RFC 862 compliant):

      akcom-udpechod \
         --port 30006 \
         --pidfile /var/run/akcom-udpecho.pid \
         --user nobody \
         --group nobody \
         --delay=10000 \
         --drop=10 \
         --rfc

Example usage (TR-143 UDPEchoPlus compliant):

      akcom-udpechod \
         --port 30006 \
         --pidfile /var/run/akcom-udpecho.pid \
         --user nobody \
         --group nobody \
         --delay=10000 \
         --drop=10 \
         --echoplus


Building Package
================

To build the package using a source tarball, run the following:

      tar -xvf akcom-udpecho-x.y.z.tar.xz
      mkdir -p akcom-udpecho-x.y.z/build
      cd akcom-udpecho-x.y.z/build
      ../configure
      make
      make install
   
For additional configuration options, run:

      ../configure --help


Source Code
===========

The source code for this project is maintained using git
(http://git-scm.com).  The following contains information to checkout the
source code from the git repository.

Browse Source:

   * https://github.com/alaskacommunications/akcom-udpecho/ (public repoisitory)
   * https://scm.prv.acsalaska.net/pub/scm/ipeng/akcom-udpecho.git/ (internal repository)

Git URLs:

   * https://github.com/alaskacommunications/akcom-udpecho.git (public repoisitory)
   * https://scm.prv.acsalaska.net/pub/scm/ipeng/akcom-udpecho.git (internal repository)
   * scm.prv.acsalaska.net:/pub/scm/ipeng/akcom-udpecho.git (internal repository)

Downloading Source:

      $ git clone https://github.com/alaskacommunications/akcom-udpecho/

Preparing and compiling source using GNU autotools method:

      $ cd akcom-udpecho/build
      $ ../autogen.sh
      $ ../configure
      $ make && make install

Compiling source using basic method:

      $ cd akcom-udpecho/src
      $ make && make install

For more information on building and installing using configure, please
read the INSTALL file.

Git Branches:

   * master - Current release of packages.
   * next   - changes staged for next release
   * pu     - proposed updates for next release
   * xx/yy+ - branch for testing new changes before merging to 'pu' branch


Package Maintence Notes
=======================

This is a collection of notes for developers to use when maintaining this
package.

New Release Checklist:

   - Switch to 'master' branch in Git repository.
   - Update version in configure.ac.
   - Update version in akcom-udpecho.spec
   - Update version in GNUmakefile.version.
   - Update date and version in ChangeLog.
   - Commit configure.ac and ChangeLog changes to repository.
   - Create tag in git repository:

           $ git tag -s v${MAJOR}.${MINOR}

   - Push repository to publishing server:

           $ git push --tags origin master:master next:next pu:pu

Creating Source Distribution Archives:

      $ ./configure
      $ make update
      $ make distcheck
      $ make dist-bzip2
      $ make dist-xz

