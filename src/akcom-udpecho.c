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
#include <string.h>
#include <strings.h>


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

union my_sa
{
   struct sockaddr         sa;
   struct sockaddr_in      sin;
   struct sockaddr_in6     sin6;
   struct sockaddr_storage ss;
};


struct udp_echo_plus
{
   uint32_t  req_sn;
   uint32_t  res_sn;
   uint32_t  recv_time;
   uint32_t  reply_time;
   uint32_t  failures;
};


union udp_buffer
{
   uint8_t              * data;
   struct udp_echo_plus * echoplus;
};


/////////////////
//             //
//  Variables  //
//             //
/////////////////
#pragma mark - Variables

static const char        * prog_name        = PROGRAM_NAME;
static uint32_t            cnf_count        = 0;
static int                 cnf_echoplus     = 0;
static int                 cnf_verbose      = 0;
static int                 cnf_silent       = 0;
static unsigned long       cnf_timeout      = 5;
static const char        * cnf_port         = "30006";
static const char        * cnf_host         = NULL;
static unsigned long       cnf_interval     = 1;
static size_t              cnf_packetsize   = sizeof(struct udp_echo_plus);
static int                 should_stop      = 0;


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////
#pragma mark - Prototypes

// main statement
int main(int argc, char * argv[]);

// signal system stop
void my_stop(int signum);

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
   int                       s;
   int                       fd;
   int                       rc;
   int                       opt_index;
   uint32_t                  count;
   uint32_t                  rcvd;
   uint64_t                  epoch;
   uint64_t                  epoch_adj;
   uint64_t                  epoch_max;
   uint64_t                  delay;
   uint64_t                  max;
   uint64_t                  max_adj;
   uint64_t                  min;
   uint64_t                  min_adj;
   uint64_t                  avg;
   uint64_t                  avg_adj;
   ssize_t                   size;
   unsigned short            port;
   char                    * ptr;
   char                      addrstr[INET6_ADDRSTRLEN];
   char                      logmsg[256];
   union my_sa               sa;
   socklen_t                 socklen;
   struct addrinfo         * res;
   struct addrinfo         * info;
   struct addrinfo           hints;
   struct timespec           start;
   struct timespec           now;
   struct pollfd             fds[2];
   union udp_buffer          sndbuff;
   union udp_buffer          rcvbuff;

   // getopt options
   static char   short_opt[] = "46c:ehi:qrs:t:vV";
   static struct option long_opt[] =
   {
      {"echoplus",      no_argument,       0, 'e'},
      {"help",          no_argument,       0, 'h'},
      {"quiet",         no_argument,       0, 'q'},
      {"silent",        no_argument,       0, 'q'},
      {"rfc",           no_argument,       0, 'r'},
      {"verbose",       no_argument,       0, 'v'},
      {"version",       no_argument,       0, 'V'},
      {NULL,            0,                 0, 0  }
   };

   // determines program name
   prog_name = argv[0];
   if ((ptr = rindex(argv[0], '/')) != NULL)
      prog_name = &ptr[1];

   // initializes variables
   bzero(&hints,        sizeof(struct addrinfo));
   hints.ai_flags     = AI_ADDRCONFIG | AI_V4MAPPED | AI_ALL;
   hints.ai_family    = PF_UNSPEC;
   hints.ai_socktype  = SOCK_DGRAM;
   hints.ai_protocol  = IPPROTO_UDP;

   // process arguments
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case '4':
         hints.ai_family = PF_INET;
         break;

         case '6':
         hints.ai_family = PF_INET6;
         break;

         case 'c':
         cnf_count = (uint32_t)strtoul(optarg, NULL, 10);
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

         case 'i':
         cnf_interval = (unsigned)strtoul(optarg, NULL, 10);
         break;

         case 'q':
         cnf_silent = 1;
         break;

         case 's':
         cnf_packetsize = (size_t)strtoull(optarg, NULL, 10);
         break;

         case 't':
         cnf_interval = (unsigned)strtoul(optarg, NULL, 10);
         break;

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
      cnf_port = argv[optind++];
   };
   if (optind < argc)
   {
      my_usage_error("unknown argument `%s'", argv[optind++]);
      return(1);
   };


   // allocate buffer
   if ((fd = open("/dev/urandom", O_RDONLY)) == -1)
   {
      fprintf(stderr, "%s: open(/dev/urandom): %s\n", prog_name, strerror(errno));
      return(1);
   };
   if (cnf_packetsize < sizeof(struct udp_echo_plus))
      cnf_packetsize = sizeof(struct udp_echo_plus);
   if ((sndbuff.data = malloc(cnf_packetsize)) == NULL)
   {
      fprintf(stderr, "%s: out of virtual memory\n", prog_name);
      close(fd);
      return(1);
   };
   if ((rcvbuff.data = malloc(cnf_packetsize)) == NULL)
   {
      fprintf(stderr, "%s: out of virtual memory\n", prog_name);
      close(fd);
      return(1);
   };
   if ((size = read(fd, sndbuff.data, cnf_packetsize)) == -1)
   {
      fprintf(stderr, "%s: read(): %s\n", prog_name, strerror(errno));
      close(fd);
      free(sndbuff.data);
      free(rcvbuff.data);
      return(1);
   };
   close(fd);
   bzero(sndbuff.data, sizeof(struct udp_echo_plus));


   // resolve host
   bzero(&sa,     sizeof(sa));
   bzero(addrstr, sizeof(addrstr));
   socklen = 0;
   if ((rc = getaddrinfo(cnf_host, cnf_port, &hints, &res)) != 0)
   {
      fprintf(stderr, "%s: getaddrinfo(): %s\n", prog_name, gai_strerror(rc));
      free(sndbuff.data);
      free(rcvbuff.data);
      return(1);
   };
   for(info = res; (info != NULL); info = info->ai_next)
   {
      if (info->ai_addr->sa_family != AF_INET6)
         continue;
      socklen = info->ai_addrlen;
      memcpy(&sa, info->ai_addr, socklen);
      inet_ntop(AF_INET6, &sa.sin6.sin6_addr, addrstr, sizeof(addrstr));
      port = ntohs(sa.sin6.sin6_port);
      snprintf(logmsg, sizeof(logmsg), "UDPECHO %s:%hu ([%s]:%hu): %zu bytes\n", cnf_host, port, addrstr, port, cnf_packetsize);
   }
   if (sa.sa.sa_family != AF_INET6)
   {
      socklen = res->ai_addrlen;
      memcpy(&sa.sa, res->ai_addr, socklen);
      inet_ntop(AF_INET, &sa.sin.sin_addr, addrstr, sizeof(addrstr));
      port = ntohs(sa.sin.sin_port);
      snprintf(logmsg, sizeof(logmsg), "UDPECHO %s:%hu (%s:%hu): %zu bytes\n", cnf_host, port, addrstr, port, cnf_packetsize);
   };
   freeaddrinfo(res);
   if (!(cnf_silent))
      printf("%s", logmsg);


   // open and connect socket
   if ((s = socket(sa.sa.sa_family, SOCK_DGRAM, 0)) == -1)
   {
      fprintf(stderr, "%s: socket(): %s\n", prog_name, strerror(errno));
      free(sndbuff.data);
      free(rcvbuff.data);
      return(1);
   };
   if ((rc = connect(s, &sa.sa, socklen)) == -1)
   {
      fprintf(stderr, "%s: connect(): %s\n", prog_name, strerror(errno));
      free(sndbuff.data);
      free(rcvbuff.data);
      return(1);
   };
   fds[0].fd      = s;
   fds[0].events  = POLLIN;
   fds[0].revents = 0;
   fds[1].fd      = -1;
   fds[1].events  = 0;
   fds[1].revents = 0;


   // configure signals
   signal(SIGPIPE, SIG_IGN);
   signal(SIGINFO, SIG_IGN);
   signal(SIGUSR1, SIG_IGN);
   signal(SIGUSR2, SIG_IGN);
   signal(SIGHUP,  my_stop);
   signal(SIGINT,  my_stop);
   signal(SIGQUIT, my_stop);
   signal(SIGALRM, my_stop);
   signal(SIGTERM, my_stop);


   // initialize statistics
   count   = 0;
   rcvd    = 0;
   min     = 0;
   min_adj = 0;
   max     = 0;
   max_adj = 0;
   avg     = 0;
   avg_adj = 0;


   // start clock
   clock_gettime(CLOCK_MONOTONIC_RAW, &start);
   start.tv_sec -= cnf_interval;
   epoch         = 0;


   // master loop
   while (!(should_stop))
   {
      // calculate loop epoch
      clock_gettime(CLOCK_MONOTONIC_RAW, &now);
      epoch  = ((uint64_t)now.tv_sec   * 10000) + ((uint64_t)now.tv_nsec   / 100000);
      epoch -= ((uint64_t)start.tv_sec * 10000) + ((uint64_t)start.tv_nsec / 100000);

      // trigger stop
      if ( ((cnf_count)) && (count >= cnf_count) )
      {
         epoch_max  = cnf_count * (uint64_t)cnf_interval * 10000;
         epoch_max += (uint64_t)cnf_timeout * 10000;
         if (( epoch >= epoch_max) || (rcvd >= count))
            should_stop = 1;
      };

      // send UDP echo request
      if ((count < (epoch/(10000*cnf_interval))) && ((count < cnf_count) || (!(cnf_count))) )
      {
         count++;
         sndbuff.echoplus->req_sn = count;
         send(s, sndbuff.data, cnf_packetsize, 0);
      };

      // receive UDP echo response
      if ((rc = poll(fds, 1, 0)) > 0)
      {
         if ((size = recv(s, rcvbuff.data, cnf_packetsize, 0)) == (ssize_t)cnf_packetsize)
         {
            rcvd++;
            delay      = rcvbuff.echoplus->reply_time - rcvbuff.echoplus->recv_time;
            delay     /= 100000000;
            epoch     -= (rcvbuff.echoplus->req_sn * 10000);
            epoch_adj  = epoch - delay;
            avg       += epoch;
            avg_adj   += epoch_adj;
            if ( (!(min)) || (epoch < min) )
               min = epoch;
            if ( (!(max)) || (epoch > max) )
               max = epoch;
            if ( (!(min_adj)) || (epoch_adj < min_adj) )
               min_adj = epoch_adj;
            if ( (!(max_adj)) || (epoch_adj > max_adj) )
               max_adj = epoch_adj;
            if ((cnf_echoplus))
            {
               printf("udpecho_seq=%u time=%llu.%llu ms delay=%llu.%llu ms adj_time=%llu.%llu ms\n",
                      rcvbuff.echoplus->req_sn,
                      epoch/10, epoch%10,
                      delay/10, delay%10,
                      epoch_adj/10, epoch_adj%10
                     );
            } else
            {
               printf("udpecho_seq=%u time=%llu.%llu ms\n",
                      rcvbuff.echoplus->req_sn,
                      epoch/10, epoch%10
                     );
            };
         };
      } else
      {
         usleep(100);
      };
   };


   if (!(cnf_silent))
   {
      printf("\n");
      printf("--- %s udpecho statistics ---\n", cnf_host);
      printf("%i packets transmitted, %i packets received, %i.%i%% packet loss\n",
             count,
             rcvd,
             ((count - rcvd) * 100) / count,
             (((count - rcvd) * 1000) / count) % 10
            );
      printf("round-trip min/avg/max = %llu.%llu/%llu.%llu/%llu.%llu ms\n",
             min/10,        min%10,
             (avg/rcvd)/10, (avg/rcvd)%10,
             max/10,        max%10
            );
      if ((cnf_echoplus))
      {
         printf("adjusted round-trip min/avg/max = %llu.%llu/%llu.%llu/%llu.%llu ms\n",
                min_adj/10,        min_adj%10,
                (avg_adj/rcvd)/10, (avg_adj/rcvd)%10,
                max_adj/10,        max_adj%10
               );
      };
   };


   // free resources
   free(sndbuff.data);
   free(rcvbuff.data);
   close(s);


   return(0);
}


// signal system stop
void my_stop(int signum)
{
   should_stop = 1;
   signal(signum, my_stop);
   return;
}


// display program usage
void my_usage(void)
{
   printf("Usage: %s [options] host [port]\n", prog_name);
   printf("OPTIONS:\n");
   printf("  -4                        connect via IPv4 only\n");
   printf("  -6                        connect via IPv6 only\n");
   printf("  -c count                  stop after sending count packets\n");
   printf("  -e, --echoplus            expect echo plus response%s\n", ((cnf_echoplus)) ? " (default)" : "");
   printf("  -h, --help                print this help and exit\n");
   printf("  -i interval               interval between packet (default: %lu sec)\n", cnf_interval);
   printf("  -r, --rfc                 expect RFC compliant echo response%s\n", (!(cnf_echoplus)) ? " (default)" : "");
   printf("  -q, --quiet, --silent     do not print messages\n");
   printf("  -s packetsize             size of data bytes to be sent. (default: %zu bytes)\n", cnf_packetsize);
   printf("  -t sec                    response timeout (default: %lu sec)\n", cnf_timeout);
   printf("  -v, --verbose             enable verbose output\n");
   printf("  -V, --version             print version number and exit\n");
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
