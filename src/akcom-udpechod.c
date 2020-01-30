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
 *  @file akcom-udpechod.c UDP echo server
 */
/*
 *  Simple Build:
 *     export CFLAGS='-Wall -Wno-unknown-pragmas'
 *     gcc ${CFLAGS} -c akcom-udpechod.c
 *     gcc ${CFLAGS} -o akcom-udpechod akcom-udpechod.o
 *
 *  Libtool Build:
 *     export CFLAGS='-Wall -Wno-unknown-pragmas'
 *     libtool --mode=compile --tag=CC gcc ${CFLAGS} -c akcom-udpechod.c
 *     libtool --mode=link    --tag=CC gcc ${CFLAGS} -o akcom-udpechod \
 *             akcom-udpechod.lo
 *
 *  Libtool Clean:
 *     libtool --mode=clean rm -f akcom-udpechod.lo akcom-udpechod
 */
#define _AKCOM_UDP_ECHO_SERVER_C 1

///////////////
//           //
//  Headers  //
//           //
///////////////
#pragma mark - Headers

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


///////////////////
//               //
//  Definitions  //
//               //
///////////////////
#pragma mark - Definitions

#define MY_BUFF_SIZE             4096    // default buffer size


#ifndef PROGRAM_NAME
#define PROGRAM_NAME "akcom-udpechod"
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

struct udp_echo_plus
{
   uint32_t  req_sn;
   uint32_t  res_sn;
   uint32_t  recv_time;
   uint32_t  reply_time;
   uint32_t  failures;
};


/////////////////
//             //
//  Variables  //
//             //
/////////////////
#pragma mark - Variables

static int should_stop = 0;

struct
{
   const char  * prog_name;
   const char  * pidfile;
   uint16_t      port;         // UDP port number
   uint16_t      echoplus;     // enable echo plus
   int           drop_perct;   // drop percentage
   int32_t       delay;        // Delay range in microseconds
   int32_t       verbose;      // runtime verbosity
   int           facility;     // syslog facility
   int           dont_fork;
}
cnf =
{
   .prog_name    = "a.out",
   .pidfile      = "/var/run/" PROGRAM_NAME ".pid",
   .port         = 30006,
   .echoplus     = 1,
   .drop_perct   = 10,
   .delay        = 10000,
   .verbose      = 0,
   .facility     = LOG_DAEMON,
   .dont_fork    = 0,
};


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////
#pragma mark - Prototypes

// main statement
int main(int argc, char * argv[]);

// daemonize process
int my_daemonize(void);

// main loop
int my_loop(int s);

// signal handler
void my_sighandler(int signum);

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
   char                    * ptr;
   int                       c;
   int                       opts;
   int                       s;
   unsigned                  seed;
   struct timespec           ts;

   // determines program name
   cnf.prog_name = argv[0];
   if ((ptr = rindex(argv[0], '/')) != NULL)
      cnf.prog_name = &ptr[1];

   // process arguments
   while ((c = getopt(argc, argv, "d:D:eEhnp:P:vV")) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case 'd':
         cnf.drop_perct = atol(optarg);
         if ((cnf.drop_perct < 0) || (cnf.drop_perct > 99))
         {
            my_usage_error("invalid value for `-d'");
            return(1);
         };
         break;

         case 'e':
         cnf.echoplus = 1;
         break;

         case 'E':
         cnf.echoplus = 0;
         break;

         case 'f':
         if      (!(strcasecmp(optarg, "daemon"))) { cnf.facility = LOG_DAEMON; }
         else if (!(strcasecmp(optarg, "local0"))) { cnf.facility = LOG_LOCAL0; }
         else if (!(strcasecmp(optarg, "local1"))) { cnf.facility = LOG_LOCAL1; }
         else if (!(strcasecmp(optarg, "local2"))) { cnf.facility = LOG_LOCAL2; }
         else if (!(strcasecmp(optarg, "local3"))) { cnf.facility = LOG_LOCAL3; }
         else if (!(strcasecmp(optarg, "local4"))) { cnf.facility = LOG_LOCAL4; }
         else if (!(strcasecmp(optarg, "local5"))) { cnf.facility = LOG_LOCAL5; }
         else if (!(strcasecmp(optarg, "local6"))) { cnf.facility = LOG_LOCAL6; }
         else if (!(strcasecmp(optarg, "local7"))) { cnf.facility = LOG_LOCAL7; }
         else if (!(strcasecmp(optarg, "user")))   { cnf.facility = LOG_USER; }
         else
         {
            my_usage_error("invalid or unsupported syslog facility -- `%s'", optarg);
            return(1);
         };
         break;

         case 'h':
         my_usage();
         return(0);

         case 'n':
         cnf.dont_fork = 1;
         break;

         case 'p':
         cnf.port = (uint16_t)(atoi(optarg) & 0xffff);
         break;

         case 'P':
         cnf.pidfile = optarg;
         break;

         case 'v':
         cnf.verbose++;
         break;

         case 'V':
         printf("%s (%s) %s\n", cnf.prog_name, PACKAGE_NAME, PACKAGE_VERSION);
         return(0);

         case '?':
         fprintf(stderr, "Try `%s --help' for more information.\n", cnf.prog_name);
         return(1);

         default:
         my_usage_error("unrecognized option `--%c'", c);
         return(1);
      };
   };

   // configure signals
   signal(SIGPIPE, SIG_IGN);
   signal(SIGINT,  my_sighandler);
   signal(SIGQUIT, my_sighandler);
   signal(SIGTERM, my_sighandler);

   // seed psuedo random number generator
   clock_gettime(CLOCK_REALTIME, &ts);
   seed  = (int)ts.tv_sec;
   seed += (int)ts.tv_nsec;
   seed += (int)getpid();
   seed += (int)getppid();
   srand(seed);

   // open syslog
   opts = LOG_PID;
   if ((cnf.verbose))
      opts |= LOG_PERROR;
   openlog(cnf.prog_name, opts, cnf.facility);
   syslog(LOG_NOTICE, "daemon starting");


   // starts daemon functions
   switch(s = my_daemonize())
   {
      case -1:
      syslog(LOG_NOTICE, "daemon stopping");
      closelog();
      return(1);
      break;

      case 0:
      return(0);

      default:
      break;
   };

   // loops
   while(!(should_stop))
      my_loop(s);

   // close syslog
   syslog(LOG_NOTICE, "daemon stopping");
   close(s);
   unlink(cnf.pidfile);
   closelog();
 
   return(0);
}


// daemonize process
int my_daemonize(void)
{
   int                       s;
   int                       rc;
   int                       fd;
   int                       opt;
   socklen_t                 socklen;
   char                      pidfile[512];
   char                      buff[512];
   pid_t                     pid;
   pid_t                     oldpid;
   union
   {
      struct sockaddr         sa;
      struct sockaddr_in6     s6;
      struct sockaddr_storage ss;
   } sin;

   // create lock file
   strncpy(pidfile, cnf.pidfile, sizeof(pidfile)-7);
   strcat(pidfile, "XXXXXX");
   if ((fd = mkstemp(pidfile)) == -1)
   {
      syslog(LOG_ERR, "error: mkstemp(): %s", strerror(errno));
      return(-1);
   };
   if ((rc = link(pidfile, cnf.pidfile)) == -1)
   {
      syslog(LOG_ERR, "error: mkstemp(): %s", strerror(errno));
      close(fd);
      unlink(pidfile);
      return(-1);
   };
   unlink(pidfile);
   syslog(LOG_DEBUG, "pidfile: %s", cnf.pidfile);

   // creates socket
   if ((s = socket(PF_INET6, SOCK_DGRAM, 0)) == -1)
   {
      syslog(LOG_ERR, "socket error: %s", strerror(errno));
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };
   if ((s = socket(PF_INET6, SOCK_DGRAM, 0)) == -1)
   {
      syslog(LOG_ERR, "error: socket(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // set socket options
   opt = 1;
   if ((rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(int))) == -1)
   {
      syslog(LOG_ERR, "error: setsockopt(SO_REUSEADDR): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // bind socket to interface
   bzero(&sin, sizeof(sin));
   sin.s6.sin6_family = AF_INET6;
   sin.s6.sin6_addr   = in6addr_any;
   sin.s6.sin6_port   = htons(cnf.port);
   socklen            = sizeof(struct sockaddr_in6);
   if ((rc = bind(s, (struct sockaddr *)&sin, socklen)) == -1)
   {
      syslog(LOG_ERR, "error: bind(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // log socket address
   socklen = sizeof(struct sockaddr_in6);
   if ((rc = getsockname(s, &sin.sa, &socklen)) == -1)
   {
      syslog(LOG_ERR, "error: getsockname(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };
   inet_ntop(sin.sa.sa_family, &sin, buff, sizeof(buff));
   syslog(LOG_INFO, "listening on [%s]:%i", buff, cnf.port);

   // fork process
   if ((cnf.dont_fork))
      return(s);
   oldpid = getpid();
   switch(pid = fork())
   {
      case -1:
      syslog(LOG_ERR, "error: fork(): %s", strerror(errno));
      break;

      case 0:
      closelog();
      openlog(cnf.prog_name, LOG_PID, cnf.facility);
      syslog(LOG_INFO, "forked from %i", oldpid);
      break;

      default:
      syslog(LOG_INFO, "forking to %i", pid);
      close(fd);
      close(s);
      return(0);
   };

   // record PID in pidfile
   snprintf(buff, sizeof(buff), "%i", pid);
   write(fd, buff, strlen(buff));
   close(fd);

   return(s);
}


// main loop
int my_loop(int s)
{
   socklen_t                  sinlen;
   ssize_t                    ssize;
   useconds_t                 delay;
   char                       addr_str[INET6_ADDRSTRLEN];
   short                      port;
   struct timespec            ts;
   uint64_t                   ms;
   struct pollfd              fds[2];
   union
   {
      struct sockaddr         sa;
      struct sockaddr_in      s4;
      struct sockaddr_in6     s6;
      struct sockaddr_storage ss;
   } sin;
   union
   {
      char                    bytes[MY_BUFF_SIZE];
      struct udp_echo_plus *  msg;
   } udpbuff;

   // setup poller
   fds[0].fd      = s;
   fds[0].events  = POLLIN;
   fds[0].revents = 0;
   if ((poll(fds, 1, 1)) < 1)
      return(0);

   // read data
   sinlen = sizeof(struct sockaddr_storage);
   if ((ssize = recvfrom(s, udpbuff.bytes, sizeof(udpbuff), 0, &sin.sa, &sinlen)) == -1)
      return(-1);

   // grab timestamp
   clock_gettime(CLOCK_REALTIME, &ts);
   ms  = (uint64_t)(ts.tv_sec * 1000000000);
   ms += (uint64_t)ts.tv_nsec;

   // parse UDP source address
   switch(sin.ss.ss_family)
   {
      case AF_INET:
      inet_ntop(AF_INET, &sin.s4, addr_str, sizeof(addr_str));
      port = ntohs(sin.s4.sin_port);
      break;

      case AF_INET6:
      inet_ntop(AF_INET6, &sin.s6, addr_str, sizeof(addr_str));
      port = ntohs(sin.s6.sin6_port);
      break;

      default:
      return(0);
   };

   // process echo+ packet
   if ((cnf.echoplus))
   {
      udpbuff.msg->res_sn    = udpbuff.msg->req_sn;
      udpbuff.msg->recv_time = htonl(ms & 0xFFFFFFFFLL);
      udpbuff.msg->failures  = 0;
   };
   syslog(LOG_INFO,
          "client: [%s]:%i; recv bytes: %zi; timestamp: %lu.%09lu; seq: %u;",
          addr_str,
          port,
          ssize,
          ts.tv_sec,
          ts.tv_nsec,
          (((cnf.echoplus)) ? ntohl(udpbuff.msg->req_sn) : 0)
          );

   // randomly drop packets
   if (cnf.drop_perct > 0)
   {
      if ( (rand() % 100) < cnf.drop_perct)
      {
         syslog(LOG_INFO, "dropping echo request");
         return(0);
      };
   };

   // insert random delay
   delay = 0;
   if (cnf.delay > 0)
   {
      delay = rand() % cnf.delay;
      usleep(delay);
   };

   // grab timestamp
   clock_gettime(CLOCK_REALTIME, &ts);
   ts.tv_nsec++;
   ms  = (uint64_t)(ts.tv_sec * 1000000000);
   ms += (uint64_t)ts.tv_nsec;

   // send response
   if ((cnf.echoplus))
      udpbuff.msg->reply_time = htonl(ms & 0xFFFFFFFFLL);
   sendto(s, udpbuff.bytes, ssize, 0, &sin.sa, sinlen);
   syslog(LOG_INFO,
          "client: [%s]:%i; sent bytes: %zi; timestamp: %lu.%09lu; seq: %u; delay: %u usec;",
          addr_str,
          port,
          ssize,
          ts.tv_sec,
          ts.tv_nsec,
          (((cnf.echoplus)) ? ntohl(udpbuff.msg->req_sn) : 0),
          delay
          );

   return(0);
}


// signal handler
void my_sighandler(int signum)
{
   signal(signum, my_sighandler);
   should_stop = 1;
   return;
}


// display program usage
void my_usage(void)
{

   printf("Usage: %s [options]\n", cnf.prog_name);
   printf("OPTIONS:\n");
   printf("  -d percent                set packet drop probability percentage\n");
   printf("  -D microseconds           set echo delay range to microseconds\n");
   printf("  -e                        enable echo plus\n");
   printf("  -E                        disable echo plus\n");
   printf("  -h                        display this message and exit\n");
   printf("  -n                        do not fork\n");
   printf("  -p port                   list on port number\n");
   printf("  -P file                   PID file\n");
   printf("  -v                        enable verbose output\n");
   printf("  -V                        display version and exit\n");
   printf("\n");
   return;
}


// display program usage error
void my_usage_error(const char * fmt, ...)
{
   va_list args;

   fprintf(stderr, "%s: ", cnf.prog_name);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   fprintf(stderr, "\nTry `%s --help' for more information.\n", cnf.prog_name);

   return;
}


/* end of source file */
