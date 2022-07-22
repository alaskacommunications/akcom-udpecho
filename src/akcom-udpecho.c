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

#define my_sec2msec( sec )   (  sec * 1000 )
#define my_sec2usec( sec )   (  sec * 1000000 )
#define my_sec2nsec( sec )   (  sec * 1000000000 )

#define my_msec2sec(  msec ) ( msec / 1000 )
#define my_msec2usec( msec ) ( msec * 1000 )
#define my_msec2nsec( msec ) ( msec * 1000000 )

#define my_usec2sec(  usec ) ( usec / 1000000 )
#define my_usec2msec( usec ) ( usec / 1000 )
#define my_usec2nsec( usec ) ( usec * 1000 )

#define my_nsec2sec(  nsec ) ( nsec / 1000000000 )
#define my_nsec2msec( nsec ) ( nsec / 1000000 )
#define my_nsec2usec( nsec ) ( nsec / 1000 )

#define my_usec2msec_tenths( usec ) ((usec%1000)/100)


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


typedef struct udp_echo_plus
{
   uint32_t  req_sn;
   uint32_t  res_sn;
   uint32_t  recv_time;
   uint32_t  reply_time;
   uint32_t  failures;
   uint32_t  iteration;
   struct timespec   send_time;
   uint8_t   bytes[];
} echoplus_t;


/////////////////
//             //
//  Variables  //
//             //
/////////////////
#pragma mark - Variables

static const char        * prog_name        = PROGRAM_NAME;
static uint32_t            cnf_count        = 0;
static int                 cnf_debug        = 0;
static int                 cnf_echoplus     = -1;
static int                 cnf_verbose      = 0;
static int                 cnf_silent       = 0;
static unsigned long       cnf_timeout      = 5;
static const char        * cnf_port         = "30006";
static const char        * cnf_host         = NULL;
static int                 cnf_ai_family    = PF_UNSPEC;
static unsigned long       cnf_interval     = 1;
static size_t              cnf_packetsize   = sizeof(echoplus_t);
static int                 should_stop      = 0;


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////
#pragma mark - Prototypes

// main statement
extern int
main(
         int                           argc,
         char *                        argv[] );


static int
my_loop(
         int                           s,
         echoplus_t *                  rcvbuff,
         echoplus_t *                  sndbuff );


int
my_socket(
         const char *                  host,
         const char *                  port,
         int                           family );


// signal system stop
static void
   my_stop(
         int                           signum );


extern uint64_t
my_timespec_delta(
         struct timespec *             ts1,
         struct timespec *             ts2,
         struct timespec *             delta );


// display program usage
static void
my_usage(
      void );


// display program usage error
static void
my_usage_error(
      const char *                     fmt,
      ... );


/////////////////
//             //
//  Functions  //
//             //
/////////////////
#pragma mark - Functions

// main statement
int
main(
         int                           argc,
         char *                        argv[] )
{
   int                       c;
   int                       s;
   int                       fd;
   int                       opt_index;
   char                    * ptr;
   echoplus_t *              sndbuff;
   echoplus_t *              rcvbuff;

   // getopt options
   static char   short_opt[] = "46c:dehi:qrs:t:vV";
   static struct option long_opt[] =
   {
      {"debug",         no_argument,       0, 'd'},
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

   // process arguments
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case '4':
         cnf_ai_family = PF_INET;
         break;

         case '6':
         cnf_ai_family = PF_INET6;
         break;

         case 'c':
         cnf_count = (uint32_t)strtoul(optarg, NULL, 10);
         break;

         case 'd':
         cnf_debug = 1;
         break;

         case 'e':
         cnf_echoplus = 1;
         break;

         case 'r':
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
         cnf_timeout  = (uint64_t)strtoull(optarg, NULL, 10);
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

   // adjust defaults
   if (cnf_packetsize < sizeof(echoplus_t))
      cnf_packetsize = sizeof(echoplus_t);

   // allocate receive buffer
   if ((rcvbuff = malloc(cnf_packetsize)) == NULL)
   {
      fprintf(stderr, "%s: out of virtual memory\n", prog_name);
      return(1);
   };

   // allocate send buffer
   if ((sndbuff = malloc(cnf_packetsize)) == NULL)
   {
      fprintf(stderr, "%s: out of virtual memory\n", prog_name);
      free(rcvbuff);
      return(1);
   };
   memset(sndbuff, 0, sizeof(echoplus_t));
   sndbuff->iteration  = htonl(1);
   if (cnf_packetsize > sizeof(echoplus_t))
   {
      if ((fd = open("/dev/urandom", O_RDONLY)) == -1)
      {
         fprintf(stderr, "%s: open(/dev/urandom): %s\n", prog_name, strerror(errno));
         free(rcvbuff);
         free(sndbuff);
         return(1);
      };
      if (read(fd, sndbuff->bytes, cnf_packetsize-sizeof(echoplus_t)) == -1)
      {
         fprintf(stderr, "%s: read(): %s\n", prog_name, strerror(errno));
         close(fd);
         free(sndbuff);
         free(rcvbuff);
         return(1);
      };
      close(fd);
   };

   // configure signals
   signal(SIGPIPE, SIG_IGN);
   signal(SIGUSR1, SIG_IGN);
   signal(SIGUSR2, SIG_IGN);
   signal(SIGHUP,  my_stop);
   signal(SIGINT,  my_stop);
   signal(SIGQUIT, my_stop);
   signal(SIGALRM, my_stop);
   signal(SIGTERM, my_stop);

   // resolve host
   if ((s = my_socket(cnf_host, cnf_port, cnf_ai_family)) == -1)
   {
      free(sndbuff);
      free(rcvbuff);
      return(1);
   };

   if ((my_loop(s, rcvbuff, sndbuff)))
   {
      free(sndbuff);
      free(rcvbuff);
      close(s);
      return(1);
   };

   // free resources
   free(sndbuff);
   free(rcvbuff);
   close(s);


   return(0);
}


int
my_loop(
         int                           s,
         echoplus_t *                  rcvbuff,
         echoplus_t *                  sndbuff )
{
   uint64_t                pckt_time;
   uint64_t                pckt_time_adj;
   uint64_t                epoch_usec;
   uint64_t                pckt_delay;
   uint32_t                stats_sent;
   uint32_t                stats_rcvd;
   uint64_t                stats_max;
   uint64_t                stats_max_adj;
   uint64_t                stats_min;
   uint64_t                stats_min_adj;
   uint64_t                stats_avg;
   uint64_t                stats_avg_adj;
   struct pollfd           fds[2];
   struct timespec         ts_sent;
   struct timespec         start;
   struct timespec         now;

   // initialize poller data
   fds[0].fd      = s;
   fds[0].events  = POLLIN;
   fds[0].revents = 0;
   fds[1].fd      = -1;
   fds[1].events  = 0;
   fds[1].revents = 0;

   // initialize statistics
   stats_sent     = 0;
   stats_rcvd     = 0;
   stats_min      = 0;
   stats_min_adj  = 0;
   stats_max      = 0;
   stats_max_adj  = 0;
   stats_avg      = 0;
   stats_avg_adj  = 0;

   // initialize timers
   clock_gettime(CLOCK_MONOTONIC_RAW, &start);
   start.tv_sec -= cnf_interval;
   ts_sent.tv_sec  = start.tv_sec;
   ts_sent.tv_nsec = start.tv_nsec;

   // master loop
   while (!(should_stop))
   {
      // calculate loop epoch
      clock_gettime(CLOCK_MONOTONIC_RAW, &now);
      epoch_usec  = my_sec2usec((uint64_t)now.tv_sec)   + my_nsec2usec((uint64_t)now.tv_nsec);
      epoch_usec -= my_sec2usec((uint64_t)start.tv_sec) + my_nsec2usec((uint64_t)start.tv_nsec);

      // trigger stop
      if ( ((cnf_count)) && (stats_sent >= cnf_count) )
      {
         if (stats_rcvd == stats_sent)
         {
            should_stop = 1;
            continue;
         };
         if (my_timespec_delta(&now, &ts_sent, NULL) > my_sec2usec((uint64_t)cnf_timeout))
         {
            should_stop = 1;
            continue;
         };
      };

      // send UDP echo request
      if ((stats_sent < (epoch_usec/my_sec2usec(cnf_interval))) && ((stats_sent < cnf_count) || (!(cnf_count))) )
      {
         stats_sent++;
         sndbuff->req_sn            = htonl((uint32_t)stats_sent);
         sndbuff->send_time.tv_sec  = now.tv_sec;
         sndbuff->send_time.tv_nsec = now.tv_nsec;
         ts_sent.tv_sec             = now.tv_sec;
         ts_sent.tv_nsec            = now.tv_nsec;
         send(s, sndbuff, cnf_packetsize, 0);
      };

      // receive UDP echo response
      if (poll(fds, 1, 0) <= 0)
      {
         usleep(50);
         continue;
      };

      // reads packet
      if (recv(s, rcvbuff, cnf_packetsize, 0) != (ssize_t)cnf_packetsize)
         continue;
      rcvbuff->req_sn      = ntohl(rcvbuff->req_sn);
      rcvbuff->res_sn      = ntohl(rcvbuff->res_sn);
      rcvbuff->recv_time   = ntohl(rcvbuff->recv_time);
      rcvbuff->reply_time  = ntohl(rcvbuff->reply_time);
      rcvbuff->failures    = ntohl(rcvbuff->failures);
      rcvbuff->iteration   = ntohl(1);
      if ((cnf_debug))
      {
         printf("   packet: GenSN/REspSN:    %08" PRIx32 " %08" PRIx32 "\n", rcvbuff->req_sn, rcvbuff->res_sn);
         printf("           Recv/Reply Time: %08" PRIx32 " %08" PRIx32 "\n", rcvbuff->recv_time, rcvbuff->reply_time);
         printf("           Failures:        %08" PRIx32 "\n",               rcvbuff->failures);
         printf("           Iteration:       %08" PRIx32 "\n",               rcvbuff->iteration);
      };
      stats_rcvd++;

      // performs stats on packet
      pckt_time      = my_timespec_delta(&now, &rcvbuff->send_time, NULL);
      pckt_delay     = rcvbuff->reply_time - rcvbuff->recv_time;
      pckt_time_adj  = pckt_time - pckt_delay;
      stats_avg      += pckt_time;
      stats_avg_adj  += pckt_time_adj;
      stats_min      = ( (!(stats_min)) || (pckt_time < stats_min) ) ? pckt_time : stats_min;
      stats_max      = ( (!(stats_max)) || (pckt_time > stats_max) ) ? pckt_time : stats_max;
      stats_min_adj  = ( (!(stats_min_adj)) || (pckt_time_adj < stats_min_adj) ) ? pckt_time_adj : stats_min_adj;
      stats_max_adj  = ( (!(stats_max_adj)) || (pckt_time_adj > stats_max_adj) ) ? pckt_time_adj : stats_max_adj;

      // print packet
      if (cnf_echoplus == -1)
         cnf_echoplus = (((rcvbuff->res_sn)) || ((rcvbuff->recv_time)) || ((rcvbuff->reply_time))) ? 1 : 0;
      if (!(cnf_echoplus))
      {
         printf("udpecho_seq=%u time=%" PRIu64 ".%" PRIu64 " ms\n",
               rcvbuff->req_sn,
               my_usec2msec(pckt_time), ((pckt_time%1000)/100)
         );
         continue;
      };
      printf("udpecho_seq=%u failures=%" PRIu32 " time=%" PRIu64 ".%" PRIu64 " ms delay=%" PRIu64 ".%" PRIu64 " ms adj_time=%" PRIu64 ".%" PRIu64 " ms\n",
               rcvbuff->req_sn,
               rcvbuff->failures,
               my_usec2msec(pckt_time),     my_usec2msec_tenths(pckt_time),
               my_usec2msec(pckt_delay),    my_usec2msec_tenths(pckt_delay),
               my_usec2msec(pckt_time_adj), my_usec2msec_tenths(pckt_time_adj)
      );
   };

   if (!(cnf_silent))
   {
      printf("\n");
      printf("--- %s udpecho statistics ---\n", cnf_host);
      if ((stats_sent))
      {
         printf("%i packets transmitted, %i packets received, %i.%i%% packet loss\n",
                stats_sent,
                stats_rcvd,
                ((stats_sent - stats_rcvd) * 100) / stats_sent,
                (((stats_sent - stats_rcvd) * 1000) / stats_sent) % 10
         );
      };
      if ((stats_rcvd))
      {
         printf("round-trip min/avg/max = %" PRIu64 ".%" PRIu64 "/%" PRIu64 ".%" PRIu64 "/%" PRIu64 ".%" PRIu64 " ms\n",
                my_usec2msec(stats_min),            my_usec2msec_tenths(stats_min),
                my_usec2msec(stats_avg/stats_rcvd), my_usec2msec_tenths(stats_avg/stats_rcvd),
                my_usec2msec(stats_max),            my_usec2msec_tenths(stats_max)
         );
         if ((cnf_echoplus))
         {
            printf("adjusted round-trip min/avg/max = %" PRIu64 ".%" PRIu64 "/%" PRIu64 ".%" PRIu64 "/%" PRIu64 ".%" PRIu64 " ms\n",
                   my_usec2msec(stats_min_adj),            my_usec2msec_tenths(stats_min_adj),
                   my_usec2msec(stats_avg_adj/stats_rcvd), my_usec2msec_tenths(stats_avg_adj/stats_rcvd),
                   my_usec2msec(stats_max_adj),            my_usec2msec_tenths(stats_max_adj)
            );
         };
      };
   };

   return(0);
}


int
my_socket(
         const char *                  host,
         const char *                  service,
         int                           family )
{
   int                        rc;
   int                        s;
   unsigned short             port;
   socklen_t                  socklen;
   struct addrinfo *          res;
   struct addrinfo *          info;
   struct addrinfo            hints;
   union my_sa                sa;
   char                       addrstr[INET6_ADDRSTRLEN];
   char                       logmsg[256];

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags     = AI_ADDRCONFIG | AI_V4MAPPED | AI_ALL;
   hints.ai_family    = family;
   hints.ai_socktype  = SOCK_DGRAM;
   hints.ai_protocol  = IPPROTO_UDP;

   // resolve host
   memset(&sa,     0, sizeof(sa));
   memset(addrstr, 0, sizeof(addrstr));
   socklen = 0;
   if ((rc = getaddrinfo(host, service, &hints, &res)) != 0)
   {
      fprintf(stderr, "%s: getaddrinfo(): %s\n", prog_name, gai_strerror(rc));
      return(-1);
   };
   // use first IPv6 address
   for(info = res; ((info != NULL) && (!(socklen)) ); info = info->ai_next)
   {
      if (info->ai_addr->sa_family != AF_INET6)
         continue;
      socklen = info->ai_addrlen;
      memcpy(&sa, info->ai_addr, socklen);
      inet_ntop(AF_INET6, &sa.sin6.sin6_addr, addrstr, sizeof(addrstr));
      port = ntohs(sa.sin6.sin6_port);
      snprintf(logmsg, sizeof(logmsg), "UDPECHO %s:%hu ([%s]:%hu): %zu bytes\n", cnf_host, port, addrstr, port, cnf_packetsize);
   };
   // use first IPv4 address, if no IPv6 address
   for(info = res; ((info != NULL) && (!(socklen)) ); info = info->ai_next)
   {
      if (info->ai_addr->sa_family != AF_INET)
         continue;
      socklen = res->ai_addrlen;
      memcpy(&sa.sa, res->ai_addr, socklen);
      inet_ntop(AF_INET, &sa.sin.sin_addr, addrstr, sizeof(addrstr));
      port = ntohs(sa.sin.sin_port);
      snprintf(logmsg, sizeof(logmsg), "UDPECHO %s:%hu (%s:%hu): %zu bytes\n", cnf_host, port, addrstr, port, cnf_packetsize);
   };
   freeaddrinfo(res);
   if (!(socklen))
   {
      fprintf(stderr, "%s: unable to resolve host\n", prog_name);
      return(-1);
   };
   if (!(cnf_silent))
      printf("%s", logmsg);


   // open and connect socket
   if ((s = socket(sa.sa.sa_family, SOCK_DGRAM, 0)) == -1)
   {
      fprintf(stderr, "%s: socket(): %s\n", prog_name, strerror(errno));
      return(-1);
   };
   if (connect(s, &sa.sa, socklen) == -1)
   {
      fprintf(stderr, "%s: connect(): %s\n", prog_name, strerror(errno));
      return(-1);
   };

   // configure socket
   fcntl(s, F_SETFL, O_NONBLOCK);

   return(s);
}


// signal system stop
void
my_stop(
         int                           signum )
{
   should_stop = 1;
   signal(signum, my_stop);
   return;
}


// calculates delta between timespecs and returns result as microscond
uint64_t
my_timespec_delta(
         struct timespec *             ts1,
         struct timespec *             ts2,
         struct timespec *             delta )
{
   struct timespec ts;
   uint64_t        usec;

   assert(ts1   != NULL);
   assert(ts2   != NULL);

   if (!(delta))
      delta = &ts;

   delta->tv_sec  = ts1->tv_sec;
   delta->tv_nsec = ts1->tv_nsec;
   if (delta->tv_nsec < ts2->tv_nsec)
   {
      delta->tv_sec--;
      delta->tv_nsec += my_sec2nsec(1);
   };
   delta->tv_sec  -= ts2->tv_sec;
   delta->tv_nsec -= ts2->tv_nsec;

   usec  = my_sec2usec((uint64_t)delta->tv_sec);
   usec += my_nsec2usec((uint64_t)delta->tv_nsec);

   return(usec);
}


// display program usage
void
my_usage(
         void )
{
   printf("Usage: %s [options] host [port]\n", prog_name);
   printf("OPTIONS:\n");
   printf("  -4                        connect via IPv4 only\n");
   printf("  -6                        connect via IPv6 only\n");
   printf("  -c count                  stop after sending count packets\n");
   printf("  -d, --debug               print packet debugging information\n");
   printf("  -e, --echoplus            expect echo plus response%s\n", (cnf_echoplus == 1) ? " (default)" : "");
   printf("  -h, --help                print this help and exit\n");
   printf("  -i interval               interval between packet (default: %lu sec)\n", cnf_interval);
   printf("  -r, --rfc                 expect RFC compliant echo response%s\n", (cnf_echoplus == 0) ? " (default)" : "");
   printf("  -q, --quiet, --silent     do not print messages\n");
   printf("  -s packetsize             size of data bytes to be sent. (default: %zu bytes)\n", cnf_packetsize);
   printf("  -t sec                    response timeout (default: %lu sec)\n", cnf_timeout);
   printf("  -v, --verbose             enable verbose output\n");
   printf("  -V, --version             print version number and exit\n");
   printf("\n");
   return;
}


// display program usage error
void
my_usage_error(
         const char *                  fmt,
         ... )
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
