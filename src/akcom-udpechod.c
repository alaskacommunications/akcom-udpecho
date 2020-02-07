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
#include <pwd.h>
#include <grp.h>

#include "akcom-udpecho.h"


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

#define MY_SENT 0
#define MY_RECV 1
#define MY_DROP 2


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

static int should_stop = 0;

struct
{
   const char  * prog_name;
   const char  * pidfile;
   uint16_t      port;         // UDP port number
   uint16_t      echoplus;     // enable echo plus
   int           drop_perct;   // drop percentage
   useconds_t    delay;        // Delay range in microseconds
   int32_t       verbose;      // runtime verbosity
   int           facility;     // syslog facility
   int           dont_fork;
   const char  * listen;       // IP address to listen for requests
   uid_t         uid;          // setuid
   gid_t         gid;          // setgid
}
static cnf =
{
   .prog_name    = "a.out",
   .pidfile      = "/var/run/" PROGRAM_NAME ".pid",
   .port         = 30006,
   .echoplus     = 0,
   .drop_perct   = 0,
   .delay        = 0,
   .verbose      = 0,
   .facility     = LOG_DAEMON,
   .dont_fork    = 0,
   .listen       = NULL,
   .uid          = 0,
   .gid          = 0,
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

// display debug message
void my_debug(const char * fmt, ...);

// display error message
void my_error(const char * fmt, ...);

// log connection
int my_log_conn(int mode, size_t * connp, union my_sa * sap,
   struct udp_echo_plus * msgp, ssize_t ssize, struct timespec * tsp,
   useconds_t delay);

// main loop
int my_loop(int s, size_t * connp);

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
   int                       s;
   unsigned                  seed;
   struct timespec           ts;
   size_t                    conn;
   int                       opt_index;
   struct passwd           * pw;
   struct group            * gr;

   // getopt options
   static char   short_opt[] = "d:D:efg:hl:np:P:ru:vV";
   static struct option long_opt[] =
   {
      {"drop",          required_argument, 0, 'd'},
      {"delay",         required_argument, 0, 'D'},
      {"echoplus",      no_argument,       0, 'e'},
      {"facility",      required_argument, 0, 'f'},
      {"group",         required_argument, 0, 'g'},
      {"help",          no_argument,       0, 'h'},
      {"listen",        required_argument, 0, 'l'},
      {"foreground",    no_argument,       0, 'n'},
      {"port",          required_argument, 0, 'p'},
      {"pidfile",       required_argument, 0, 'P'},
      {"rfc",           no_argument,       0, 'r'},
      {"user",          required_argument, 0, 'u'},
      {"verbose",       no_argument,       0, 'v'},
      {"version",       no_argument,       0, 'V'},
      {NULL,            0,                 0, 0  }
   };

   // determines program name
   cnf.prog_name = argv[0];
   if ((ptr = rindex(argv[0], '/')) != NULL)
      cnf.prog_name = &ptr[1];

   // process arguments
   //while ((c = getopt(argc, argv, "d:D:eEhnp:P:vV")) != -1)
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case 'd':
         cnf.drop_perct = atoi(optarg);
         if ((cnf.drop_perct < 0) || (cnf.drop_perct > 99))
         {
            my_usage_error("invalid value for `-d'");
            return(1);
         };
         break;

         case 'D':
         cnf.delay = (useconds_t)atoll(optarg);
         break;

         case 'e':
         cnf.echoplus = 1;
         break;

         case 'f':
         if      (!(strcasecmp(optarg, "auth")))   { cnf.facility = LOG_AUTH; }
         else if (!(strcasecmp(optarg, "cron")))   { cnf.facility = LOG_CRON; }
         else if (!(strcasecmp(optarg, "daemon"))) { cnf.facility = LOG_DAEMON; }
         else if (!(strcasecmp(optarg, "ftp")))    { cnf.facility = LOG_FTP; }
         else if (!(strcasecmp(optarg, "local0"))) { cnf.facility = LOG_LOCAL0; }
         else if (!(strcasecmp(optarg, "local1"))) { cnf.facility = LOG_LOCAL1; }
         else if (!(strcasecmp(optarg, "local2"))) { cnf.facility = LOG_LOCAL2; }
         else if (!(strcasecmp(optarg, "local3"))) { cnf.facility = LOG_LOCAL3; }
         else if (!(strcasecmp(optarg, "local4"))) { cnf.facility = LOG_LOCAL4; }
         else if (!(strcasecmp(optarg, "local5"))) { cnf.facility = LOG_LOCAL5; }
         else if (!(strcasecmp(optarg, "local6"))) { cnf.facility = LOG_LOCAL6; }
         else if (!(strcasecmp(optarg, "local7"))) { cnf.facility = LOG_LOCAL7; }
         else if (!(strcasecmp(optarg, "lpr")))    { cnf.facility = LOG_LPR; }
         else if (!(strcasecmp(optarg, "mail")))   { cnf.facility = LOG_MAIL; }
         else if (!(strcasecmp(optarg, "news")))   { cnf.facility = LOG_NEWS; }
         else if (!(strcasecmp(optarg, "uucp")))   { cnf.facility = LOG_UUCP; }
         else if (!(strcasecmp(optarg, "user")))   { cnf.facility = LOG_USER; }
         else
         {
            my_usage_error("invalid or unsupported syslog facility -- `%s'", optarg);
            return(1);
         };
         break;

         case 'g':
         errno = 0;
         if ((gr = getgrnam(optarg)) == NULL)
         {
            if (errno == 0)
               fprintf(stderr, "%s: invalid group specified\n", cnf.prog_name);
            else
               fprintf(stderr, "%s: getgrnam(): %s\n", cnf.prog_name, strerror(errno));
            return(1);
         };
         cnf.gid = gr->gr_gid;
         break;

         case 'h':
         my_usage();
         return(0);

         case 'l':
         cnf.listen = optarg;
         break;

         case 'n':
         cnf.dont_fork = 1;
         break;

         case 'p':
         cnf.port = (uint16_t)(atoi(optarg) & 0xffff);
         break;

         case 'P':
         cnf.pidfile = optarg;
         break;

         case 'r':
         cnf.echoplus = 0;
         break;

         case 'u':
         errno = 0;
         if ((pw = getpwnam(optarg)) == NULL)
         {
            if (errno == 0)
               fprintf(stderr, "%s: invalid user specified\n", cnf.prog_name);
            else
               fprintf(stderr, "%s: getpwnam(): %s\n", cnf.prog_name, strerror(errno));
            return(1);
         };
         cnf.uid = pw->pw_uid;
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

   // set defaults for setuid/setgid
   cnf.gid = (cnf.gid == 0) ? getgid() : cnf.gid;
   cnf.uid = (cnf.uid == 0) ? getuid() : cnf.uid;
   if ( (cnf.gid != getgid()) && (getuid() != 0) )
   {
      fprintf(stderr, "%s: setgid requires root access\n", cnf.prog_name);
      return(1);
   };
   if ( (cnf.uid != getuid()) && (getuid() != 0) )
   {
      fprintf(stderr, "%s: setuid requires root access\n", cnf.prog_name);
      return(1);
   };

   // configure signals
   my_debug("configuring signal handling");
   signal(SIGHUP,  SIG_IGN);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGINT,  my_sighandler);
   signal(SIGQUIT, my_sighandler);
   signal(SIGTERM, my_sighandler);

   // seed psuedo random number generator
   my_debug("seeding psuedo random number generator");
   clock_gettime(CLOCK_REALTIME, &ts);
   seed  = (unsigned)ts.tv_sec;
   seed += (unsigned)ts.tv_nsec;
   seed += (unsigned)getpid();
   seed += (unsigned)getppid();
   srand(seed);

   // starts daemon functions
   switch(s = my_daemonize())
   {
      case -1:
      syslog(LOG_NOTICE, "daemon stopping");
      closelog();
      return(1);

      case 0:
      return(0);

      default:
      break;
   };

   // loops
   conn = 0;
   while(!(should_stop))
      my_loop(s, &conn);

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
   char                      buff[16];
   char                      addr_str[512];
   pid_t                     pid;
   unsigned short            port;
   FILE                    * fs;
   struct stat               sb;
   union
   {
      struct sockaddr         sa;
      struct sockaddr_in      sin;
      struct sockaddr_in6     sin6;
      struct sockaddr_storage ss;
   } sa;

   // check for existing instance
   fs = NULL;
   my_debug("checking for existing PID file (%s)", cnf.pidfile);
   if ((rc = stat(cnf.pidfile, &sb)) == -1)
   {
      if (errno != ENOENT)
      {
         my_error("stat(): %s", strerror(errno));
         return(-1);
      };
   } else if ((fs = fopen(cnf.pidfile, "r")) == NULL)
   {
      if (errno != ENOENT)
      {
         my_error("fopen(): %s", strerror(errno));
         return(-1);
      };
   } else
   {
      fscanf(fs, "%u", &pid);
      fclose(fs);
      if ((rc = kill(pid, 0)) == -1)
      {
         if (errno == ESRCH)
         {
            my_debug("removing stale PID file");
            unlink(cnf.pidfile);
         } else
         {
            my_error("kill(): %s", strerror(errno));
            return(-1);
         };
      } else
      {
         my_error("daemon already running");
         return(-1);
      };
   };

   // create PID file
   my_debug("creating PID file");
   strncpy(pidfile, cnf.pidfile, sizeof(pidfile)-7);
   strcat(pidfile, "XXXXXX");
   if ((fd = mkstemp(pidfile)) == -1)
   {
      my_error("mkstemp(): %s", strerror(errno));
      return(-1);
   };
   my_debug("temp pidfile: %s", pidfile);
   if ((rc = link(pidfile, cnf.pidfile)) == -1)
   {
      my_error("mkstemp(): %s", strerror(errno));
      close(fd);
      unlink(pidfile);
      return(-1);
   };
   unlink(pidfile);
   if ((rc = fchmod(fd, 0644)) == -1)
   {
      my_error("fchmod(): %s", strerror(errno));
      close(fd);
      return(-1);
   };
   if ( (cnf.uid != getuid()) ||
        (cnf.gid != getgid()) )
   {
      if ((rc = fchown(fd, cnf.uid, cnf.gid)) == -1)
      {
         my_error("fchown(): %s", strerror(errno));
         close(fd);
         return(-1);
      };
   };
   my_debug("pidfile: %s", cnf.pidfile);

   // determines interface on which to listen
   bzero(&sa, sizeof(sa));
   if (!(cnf.listen))
   {
      sa.sin6.sin6_family = AF_INET6;
      sa.sin6.sin6_addr   = in6addr_any;
      sa.sin6.sin6_port   = htons(cnf.port);
      socklen             = sizeof(struct sockaddr_in6);
   } else
   {
      if ((rc = inet_pton(AF_INET, cnf.listen, &sa.sin.sin_addr)) == 1)
      {
         sa.sin.sin_family   = AF_INET;
         sa.sin.sin_port     = htons(cnf.port);
         socklen             = sizeof(struct sockaddr_in);
      } else if ((rc = inet_pton(AF_INET6, cnf.listen, &sa.sin6.sin6_addr)) == 1)
      {
         sa.sin6.sin6_family = AF_INET6;
         sa.sin6.sin6_port   = htons(cnf.port);
         socklen             = sizeof(struct sockaddr_in6);
      } else
      {
         if (rc == -1)
            my_error("inet_pton(): %s", strerror(errno));
         else
            my_error("invalid address specified with `-l'");
         close(fd);
         unlink(cnf.pidfile);
         return(-1);
      };
   };

   // creates socket
   my_debug("creating UDP socket");
   if ((s = socket(sa.sa.sa_family, SOCK_DGRAM, 0)) == -1)
   {
      my_error("socket(): %s", strerror(errno));
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // set socket options
   my_debug("setting socket options");
   opt = 1;
   if ((rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(int))) == -1)
   {
      my_error("setsockopt(SO_REUSEADDR): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // bind socket to interface
   my_debug("binding socket");
   if ((rc = bind(s, &sa.sa, socklen)) == -1)
   {
      my_error("bind(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // log socket address
   socklen = sizeof(struct sockaddr_storage);
   if ((rc = getsockname(s, &sa.sa, &socklen)) == -1)
   {
      my_error("getsockname(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };
   switch(sa.ss.ss_family)
   {
      case AF_INET:
      inet_ntop(sa.sin.sin_family, &sa.sin.sin_addr,  addr_str, sizeof(addr_str));
      port = ntohs(sa.sin.sin_port);
      break;

      case AF_INET6:
      inet_ntop(sa.sin6.sin6_family, &sa.sin6.sin6_addr, addr_str, sizeof(addr_str));
      port = ntohs(sa.sin6.sin6_port);
      break;

      default:
      my_error("listening socket has invalid address family: %i\n", sa.sa.sa_family);
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // change ownership
   if ( (getgid() != cnf.gid) && ((rc = setregid(cnf.gid, cnf.gid)) == -1) )
   {
      my_error("getgid(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };
   if ( (getuid() != cnf.uid) && ((rc = setreuid(cnf.uid, cnf.uid)) == -1) )
   {
      my_error("getuid(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf.pidfile);
      return(-1);
   };

   // fork process
   if (!(cnf.dont_fork))
   {
      my_debug("forking process");
      switch(pid = fork())
      {
         case -1:
         my_error("fork(): %s", strerror(errno));
         break;

         case 0:
         break;

         default:
         my_debug("forking to %i", pid);
         closelog();
         close(fd);
         close(s);
         return(0);
      };
   };

   // record PID in pidfile
   pid = getpid();
   snprintf(buff, sizeof(buff), "%i", pid);
   write(fd, buff, strlen(buff));
   close(fd);

   // opens syslog
   openlog(cnf.prog_name, LOG_PID | (((cnf.dont_fork)) ? LOG_PERROR : 0), cnf.facility);
   syslog(LOG_NOTICE, "%s v%s", PROGRAM_NAME, PACKAGE_VERSION);
   syslog(LOG_NOTICE, "echo plus enabled: %s", ((cnf.echoplus)) ? "yes" : "no");
   syslog(LOG_NOTICE, "random delay: %u ms", cnf.delay);
   syslog(LOG_NOTICE, "drop probability: %u%%", cnf.drop_perct);
   syslog(LOG_NOTICE, "running as UID: %u", getuid());
   syslog(LOG_NOTICE, "running as GID: %u", getgid());
   syslog(LOG_NOTICE, "listening on [%s]:%hu", addr_str, port);

   return(s);
}


// display debug message
void my_debug(const char * fmt, ...)
{
   va_list args;

   if (!(cnf.verbose))
      return;

   printf("%s: ", cnf.prog_name);

   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);

   printf("\n");

   return;
}


// display error message
void my_error(const char * fmt, ...)
{
   va_list args;

   fprintf(stderr, "%s: error: ", cnf.prog_name);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   fprintf(stderr, "\n");

   return;
}


// log connection
int my_log_conn(int mode, size_t * connp, union my_sa * sap,
   struct udp_echo_plus * msgp, ssize_t ssize, struct timespec * tsp,
   useconds_t delay)
{
   const char               * mode_name;
   char                       addr_str[INET6_ADDRSTRLEN];
   unsigned short             port;

   // determine log entry type
   switch(mode)
   {
      case MY_SENT: mode_name = "sent"; break;
      case MY_RECV: mode_name = "recv"; break;
      case MY_DROP: mode_name = "drop"; break;
      default: return(-1);
   };

   // convert address to presentation format
   switch(sap->ss.ss_family)
   {
      case AF_INET:
      inet_ntop(AF_INET, &sap->sin.sin_addr, addr_str, sizeof(addr_str));
      port =        ntohs(sap->sin.sin_port);
      break;

      case AF_INET6:
      inet_ntop(AF_INET6, &sap->sin6.sin6_addr, addr_str, sizeof(addr_str));
      port =         ntohs(sap->sin6.sin6_port);
      break;

      default:
      syslog(LOG_DEBUG, "conn %zu: ignoring request from unknown address family: %i", *connp, sap->ss.ss_family);
      return(-1);
   };

   // log connection
   if ((cnf.echoplus))
   {
      syslog(LOG_INFO,
         "conn %zu: client: [%s]:%hu; %s bytes: %zi; timestamp: %lu.%09lu; seq: %u; delta: %u us; delay: %u ms;",
         *connp,
         addr_str,
         port,
         mode_name,
         ssize,
         tsp->tv_sec,
         tsp->tv_nsec,
         ntohl(msgp->req_sn),
         ntohl(msgp->reply_time) - ntohl(msgp->recv_time),
         delay
      );
   } else
   {
      syslog(LOG_INFO,
         "conn %zu: client: [%s]:%hu; %s bytes: %zi; timestamp: %lu.%09lu;",
         *connp,
         addr_str,
         port,
         mode_name,
         ssize,
         tsp->tv_sec,
         tsp->tv_nsec
      );
   };

   // log if IPv4 address mapped to IPv6
   if (sap->ss.ss_family == AF_INET6)
      if (mode == MY_RECV)
         if (IN6_IS_ADDR_V4MAPPED(&sap->sin6.sin6_addr) != 0)
            syslog(LOG_INFO, "conn %zu: client: [%s]:%hu; IPv4 mapped address", *connp, addr_str, port);

   return(0);
}


// main loop
int my_loop(int s, size_t * connp)
{
   socklen_t                  sinlen;
   ssize_t                    ssize;
   useconds_t                 delay;
   struct timespec            ts;
   uint64_t                   us;
   struct pollfd              fds[2];
   union my_sa                sa;
   union
   {
      char                    bytes[MY_BUFF_SIZE];
      struct udp_echo_plus    msg;
   } udpbuff;

   // setup poller
   fds[0].fd      = s;
   fds[0].events  = POLLIN;
   fds[0].revents = 0;
   if (cnf.verbose > 1)
      syslog(LOG_DEBUG, "waiting for echo request");
   if ((poll(fds, 1, 5000)) < 1)
      return(0);

   // increment connection counter
   (*connp)++;

   // read data
   syslog(LOG_DEBUG, "conn %zu: reading data", *connp);
   sinlen = sizeof(struct sockaddr_storage);
   if ((ssize = recvfrom(s, udpbuff.bytes, sizeof(udpbuff), 0, &sa.sa, &sinlen)) == -1)
      return(-1);

   // grab timestamp
   syslog(LOG_DEBUG, "conn %zu: calculating recv timestamp", *connp);
   clock_gettime(CLOCK_REALTIME, &ts);
   us  = (uint64_t)(ts.tv_sec * 1000000);
   us += (uint64_t)ts.tv_nsec / 1000;

   // log connection
   my_log_conn(MY_RECV, connp, &sa, &udpbuff.msg, ssize, &ts, 0);

   // process echo+ packet
   if ((cnf.echoplus))
   {
      syslog(LOG_DEBUG, "conn %zu: intializing echo plus header", *connp);
      udpbuff.msg.res_sn    = udpbuff.msg.req_sn;
      udpbuff.msg.recv_time = htonl(us & 0xFFFFFFFFLL);
      udpbuff.msg.failures  = 0;
   };

   // randomly drop packets
   if (cnf.drop_perct > 0)
   {
      if ( (rand() % 100) < cnf.drop_perct)
      {
         my_log_conn(MY_DROP, connp, &sa, &udpbuff.msg, ssize, &ts, 0);
         return(0);
      };
   };

   // insert random delay
   delay = 0;
   if (cnf.delay > 0)
   {
      delay = (useconds_t)rand() % cnf.delay;
      syslog(LOG_DEBUG, "conn %zu: delaying response for %i ms", *connp, delay);
      usleep(delay);
   };

   // grab timestamp
   syslog(LOG_DEBUG, "conn %zu: calculating sent timestamp", *connp);
   clock_gettime(CLOCK_REALTIME, &ts);
   ts.tv_nsec++;
   us  = (uint64_t)(ts.tv_sec * 1000000);
   us += (uint64_t)ts.tv_nsec / 1000;

   // send response
   if ((cnf.echoplus))
   {
      syslog(LOG_DEBUG, "conn %zu: updating echo plus header", *connp);
      udpbuff.msg.reply_time = htonl(us & 0xFFFFFFFFLL);
   };
   sendto(s, udpbuff.bytes, (size_t)ssize, 0, &sa.sa, sinlen);

   // log response
   my_log_conn(MY_SENT, connp, &sa, &udpbuff.msg, ssize, &ts, delay);

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
   printf("  -d num,  --drop=num       set packet drop probability [0-99] (default: %u)\n", cnf.drop_perct);
   printf("  -D ms,   --delay=ms       set echo delay range to microseconds (default: %u)\n", cnf.delay);
   printf("  -e,      --echoplus       enable echo plus, not RFC compliant%s\n", ((cnf.echoplus)) ? " (default)" : "");
   printf("  -f str,  --facility=str   set syslog facility (default: daemon)\n");
   printf("  -g gid,  --group=gid      setgid to gid (default: none)\n");
   printf("  -h,      --help           print this help and exit\n");
   printf("  -l addr, --listen=addr    bind to IP address (default: all)\n");
   printf("  -n,      --foreground     do not fork\n");
   printf("  -p port, --port=port      list on port number (default: %u)\n", cnf.port);
   printf("  -P file, --pidfile=file   PID file (default: %s)\n", cnf.pidfile);
   printf("  -r,      --rfc            RFC compliant echo protocol%s\n", (!(cnf.echoplus)) ? " (default)" : "");
   printf("  -u uid,  --user=uid       setuid to uid (default: none)\n");
   printf("  -v,      --verbose        enable verbose output\n");
   printf("  -V,      --version        print version number and exit\n");
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
