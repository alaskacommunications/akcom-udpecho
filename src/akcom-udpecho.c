/*
 *  Alaska Communications UDP Echo Tools
 *  Copyright (C) 2020 Alaska Communications
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *     1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *     3. Neither the name of the copyright holder nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 *  @file akcom-udpecho.c UDP echo client
 */
/*
 *  Simple Build:
 *     export CFLAGS='-Wall -Wno-unknown-pragmas'
 *     gcc ${CFLAGS} -c akcom-udpecho.c
 *     gcc ${CFLAGS} -o akcom-udpecho akcom-udpecho.o
 *
 *  Libtool Build:
 *     export CFLAGS='-Wall -Wno-unknown-pragmas'
 *     libtool --mode=compile --tag=CC gcc ${CFLAGS} -c akcom-udpecho.c
 *     libtool --mode=link    --tag=CC gcc ${CFLAGS} -o akcom-udpecho \
 *             akcom-udpecho.lo
 *
 *  Libtool Clean:
 *     libtool --mode=clean rm -f akcom-udpecho.lo akcom-udpecho
 */
#define _AKCOM_UDP_ECHO_CLIENT_C 1

///////////////
//           //
//  Headers  //
//           //
///////////////
#pragma mark - Headers

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <getopt.h>
#include <assert.h>
#include <syslog.h>
#include <signal.h>
#include <poll.h>

#include "akcom-udpecho.h"


///////////////////
//               //
//  Definitions  //
//               //
///////////////////
#pragma mark - Definitions

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "akcom-udpecho"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "akcom-udpecho"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.0"
#endif


/////////////////
//             //
//  Datatypes  //
//             //
/////////////////
#pragma mark - Datatypes


/////////////////
//             //
//  Variables  //
//             //
/////////////////
#pragma mark - Variables

static const char    * prog_name    = PROGRAM_NAME;
static int             cnf_echoplus = 0;
static int             cnf_verbose  = 0;
static time_t          cnf_timeout  = 5;
static short           cnf_port     = 30006;
static const char    * cnf_host     = NULL;


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////
#pragma mark - Prototypes

// main statement
int main(int argc, char * argv[]);

// display program usage
void my_usage(void);

// display program usage error
void my_usage_error(const char * fmt, ...);


/////////////////
//             //
//  Functions  //
//             //
/////////////////
#pragma mark - Functions

// main statement
int main(int argc, char * argv[])
{
   int                       c;
   int                       opt_index;
   char                    * ptr;
   //union my_sa               sa;

   // getopt options
   static char   short_opt[] = "d:D:efg:hl:np:P:ru:vV";
   static struct option long_opt[] =
   {
      {"echoplus",      no_argument,       0, 'e'},
      {"help",          no_argument,       0, 'h'},
      {"rfc",           no_argument,       0, 'r'},
      {"verbose",       no_argument,       0, 'v'},
      {"version",       no_argument,       0, 'V'},
      {NULL,            0,                 0, 0  }
   };

   // determines program name
   prog_name = argv[0];
   if ((ptr = rindex(argv[0], '/')) != NULL)
      prog_name = &ptr[1];

   // process arguments
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case 'e':
         cnf_echoplus = 1;
         break;

         case 'E':
         cnf_echoplus = 0;
         break;

         case 'h':
         my_usage();
         return(0);

         case 'v':
         cnf_verbose++;
         break;

         case 'V':
         printf("%s (%s) %s\n", prog_name, PACKAGE_NAME, PACKAGE_VERSION);
         return(0);

         case '?':
         fprintf(stderr, "Try `%s --help' for more information.\n", prog_name);
         return(1);

         default:
         my_usage_error("unrecognized option `--%c'", c);
         return(1);
      };
   };
   if (optind >= argc)
   {
      my_usage_error("missing remote host");
      return(1);
   };
   cnf_host = argv[optind++];
   if (optind < argc)
   {
      cnf_port = (short)strtoul(argv[optind++], &ptr, 10);
      if ( (!(cnf_port)) || ((ptr[0])) )
      {
         my_usage_error("invalid port number");
         return(1);
      };
   };
   if (optind < argc)
   {
      my_usage_error("unknown argument `%s'", argv[optind++]);
      return(1);
   };

printf("connect to %s:%hu\n", cnf_host, cnf_port);

   return(0);
}


// display program usage
void my_usage(void)
{
   printf("Usage: %s [options] host [port]\n", prog_name);
   printf("OPTIONS:\n");
   printf("  -e,      --echoplus       enable echo plus, not RFC compliant%s\n", ((cnf_echoplus)) ? " (default)" : "");
   printf("  -h,      --help           print this help and exit\n");
   printf("  -r,      --rfc            RFC compliant echo protocol%s\n", (!(cnf_echoplus)) ? " (default)" : "");
   printf("  -t sec                    response timeout (default: %lu s)\n", cnf_timeout);
   printf("  -v,      --verbose        enable verbose output\n");
   printf("  -V,      --version        print version number and exit\n");
   printf("\n");
   return;
}


// display program usage error
void my_usage_error(const char * fmt, ...)
{
   va_list args;

   fprintf(stderr, "%s: ", prog_name);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   fprintf(stderr, "\nTry `%s --help' for more information.\n", prog_name);

   return;
}


/* end of source file */
