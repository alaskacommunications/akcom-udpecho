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

// defined in the Single UNIX Specification
#ifndef _XOPEN_SOURCE
#   define _XOPEN_SOURCE 600
#endif

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
#define MY_INVAL 3


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
   uint32_t  iteration;
};


/////////////////
//             //
//  Variables  //
//             //
/////////////////
#pragma mark - Variables

static int           should_stop     = 0;

static const char  * prog_name       = "a.out";

static const char  * cnf_pidfile     = "/var/run/" PROGRAM_NAME ".pid";
static uint16_t      cnf_port        = 30006;                            // UDP port number
static int           cnf_echoplus    = 0;                                // enable echo plus
static int           cnf_drop_perct  = 0;                                // drop percentage
static useconds_t    cnf_delay       = 0;                                // Delay range in microseconds
static int32_t       cnf_verbose     = 0;                                // runtime verbosity
static int           cnf_facility    = LOG_DAEMON;                       // syslog facility
static int           cnf_dont_fork   = 0;
static const char  * cnf_listen      = NULL;                             // IP address to listen for requests
static uid_t         cnf_uid         = 0;                                // setuid
static gid_t         cnf_gid         = 0;                                // setgid

static struct udp_echo_plus state =
{
   .req_sn         = 0,
   .res_sn         = 0,
   .recv_time      = 0,
   .reply_time     = 0,
   .failures       = 0
};


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


// daemonize process
static int
my_daemonize(
         void );


// display debug message
static void
my_debug(
         const char *                  fmt,
         ... );


// display error message
static void
my_error(
         const char *                  fmt,
         ... );


// log connection
static int
my_log_conn(
         int                           mode,
         size_t *                      connp,
         union my_sa *                 sap,
         struct udp_echo_plus *        msgp,
         ssize_t                       ssize,
         struct timespec *             tsp,
         useconds_t                    delay );


// main loop
static int
my_loop(
         int                           s,
         size_t *                      connp );


// signal handler
static void
my_sighandler(
         int                           signum );


// display program usage
static void
my_usage(
         void );


// display program usage error
static void
my_usage_error(
         const char *                  fmt,
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
   prog_name = argv[0];
   if ((ptr = strrchr(argv[0], '/')) != NULL)
      prog_name = &ptr[1];

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
         cnf_drop_perct = atoi(optarg);
         if ((cnf_drop_perct < 0) || (cnf_drop_perct > 99))
         {
            my_usage_error("invalid value for `-d'");
            return(1);
         };
         break;

         case 'D':
         cnf_delay = (useconds_t)atoll(optarg);
         break;

         case 'e':
         cnf_echoplus = 1;
         break;

         case 'f':
         if      (!(strcasecmp(optarg, "auth")))   { cnf_facility = LOG_AUTH; }
         else if (!(strcasecmp(optarg, "cron")))   { cnf_facility = LOG_CRON; }
         else if (!(strcasecmp(optarg, "daemon"))) { cnf_facility = LOG_DAEMON; }
         else if (!(strcasecmp(optarg, "ftp")))    { cnf_facility = LOG_FTP; }
         else if (!(strcasecmp(optarg, "local0"))) { cnf_facility = LOG_LOCAL0; }
         else if (!(strcasecmp(optarg, "local1"))) { cnf_facility = LOG_LOCAL1; }
         else if (!(strcasecmp(optarg, "local2"))) { cnf_facility = LOG_LOCAL2; }
         else if (!(strcasecmp(optarg, "local3"))) { cnf_facility = LOG_LOCAL3; }
         else if (!(strcasecmp(optarg, "local4"))) { cnf_facility = LOG_LOCAL4; }
         else if (!(strcasecmp(optarg, "local5"))) { cnf_facility = LOG_LOCAL5; }
         else if (!(strcasecmp(optarg, "local6"))) { cnf_facility = LOG_LOCAL6; }
         else if (!(strcasecmp(optarg, "local7"))) { cnf_facility = LOG_LOCAL7; }
         else if (!(strcasecmp(optarg, "lpr")))    { cnf_facility = LOG_LPR; }
         else if (!(strcasecmp(optarg, "mail")))   { cnf_facility = LOG_MAIL; }
         else if (!(strcasecmp(optarg, "news")))   { cnf_facility = LOG_NEWS; }
         else if (!(strcasecmp(optarg, "uucp")))   { cnf_facility = LOG_UUCP; }
         else if (!(strcasecmp(optarg, "user")))   { cnf_facility = LOG_USER; }
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
               fprintf(stderr, "%s: invalid group specified\n", prog_name);
            else
               fprintf(stderr, "%s: getgrnam(): %s\n", prog_name, strerror(errno));
            return(1);
         };
         cnf_gid = gr->gr_gid;
         break;

         case 'h':
         my_usage();
         return(0);

         case 'l':
         cnf_listen = optarg;
         break;

         case 'n':
         cnf_dont_fork = 1;
         break;

         case 'p':
         cnf_port = (uint16_t)(atoi(optarg) & 0xffff);
         break;

         case 'P':
         cnf_pidfile = optarg;
         break;

         case 'r':
         cnf_echoplus = 0;
         break;

         case 'u':
         errno = 0;
         if ((pw = getpwnam(optarg)) == NULL)
         {
            if (errno == 0)
               fprintf(stderr, "%s: invalid user specified\n", prog_name);
            else
               fprintf(stderr, "%s: getpwnam(): %s\n", prog_name, strerror(errno));
            return(1);
         };
         cnf_uid = pw->pw_uid;
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

   // set defaults for setuid/setgid
   cnf_gid = (cnf_gid == 0) ? getgid() : cnf_gid;
   cnf_uid = (cnf_uid == 0) ? getuid() : cnf_uid;
   if ( (cnf_gid != getgid()) && (getuid() != 0) )
   {
      fprintf(stderr, "%s: setgid requires root access\n", prog_name);
      return(1);
   };
   if ( (cnf_uid != getuid()) && (getuid() != 0) )
   {
      fprintf(stderr, "%s: setuid requires root access\n", prog_name);
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
   unlink(cnf_pidfile);
   closelog();

   return(0);
}


// daemonize process
int
my_daemonize(
         void )
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
   my_debug("checking for existing PID file (%s)", cnf_pidfile);
   if (stat(cnf_pidfile, &sb) == -1)
   {
      if (errno != ENOENT)
      {
         my_error("stat(): %s", strerror(errno));
         return(-1);
      };
   } else if ((fs = fopen(cnf_pidfile, "r")) == NULL)
   {
      if (errno != ENOENT)
      {
         my_error("fopen(): %s", strerror(errno));
         return(-1);
      };
   } else if (fscanf(fs, "%i", &pid) != 1)
   {
      fclose(fs);
   } else
   {
      fclose(fs);
      if (kill(pid, 0) == -1)
      {
         if (errno == ESRCH)
         {
            my_debug("removing stale PID file");
            unlink(cnf_pidfile);
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
   strncpy(pidfile, cnf_pidfile, sizeof(pidfile)-7);
   strcat(pidfile, "XXXXXX");
   if ((fd = mkstemp(pidfile)) == -1)
   {
      my_error("mkstemp(): %s", strerror(errno));
      return(-1);
   };
   my_debug("temp pidfile: %s", pidfile);
   if (link(pidfile, cnf_pidfile) == -1)
   {
      my_error("mkstemp(): %s", strerror(errno));
      close(fd);
      unlink(pidfile);
      return(-1);
   };
   unlink(pidfile);
   if (fchmod(fd, 0644) == -1)
   {
      my_error("fchmod(): %s", strerror(errno));
      close(fd);
      return(-1);
   };
   if ( (cnf_uid != getuid()) ||
        (cnf_gid != getgid()) )
   {
      if (fchown(fd, cnf_uid, cnf_gid) == -1)
      {
         my_error("fchown(): %s", strerror(errno));
         close(fd);
         return(-1);
      };
   };
   my_debug("pidfile: %s", cnf_pidfile);

   // determines interface on which to listen
   memset(&sa, 0, sizeof(sa));
   if (!(cnf_listen))
   {
      sa.sin6.sin6_family = AF_INET6;
      sa.sin6.sin6_addr   = in6addr_any;
      sa.sin6.sin6_port   = htons(cnf_port);
      socklen             = sizeof(struct sockaddr_in6);
   } else
   {
      if (inet_pton(AF_INET, cnf_listen, &sa.sin.sin_addr) == 1)
      {
         sa.sin.sin_family   = AF_INET;
         sa.sin.sin_port     = htons(cnf_port);
         socklen             = sizeof(struct sockaddr_in);
      } else if ((rc = inet_pton(AF_INET6, cnf_listen, &sa.sin6.sin6_addr)) == 1)
      {
         sa.sin6.sin6_family = AF_INET6;
         sa.sin6.sin6_port   = htons(cnf_port);
         socklen             = sizeof(struct sockaddr_in6);
      } else
      {
         if (rc == -1)
            my_error("inet_pton(): %s", strerror(errno));
         else
            my_error("invalid address specified with `-l'");
         close(fd);
         unlink(cnf_pidfile);
         return(-1);
      };
   };

   // creates socket
   my_debug("creating UDP socket");
   if ((s = socket(sa.sa.sa_family, SOCK_DGRAM, 0)) == -1)
   {
      my_error("socket(): %s", strerror(errno));
      close(fd);
      unlink(cnf_pidfile);
      return(-1);
   };

   // set socket options
   my_debug("setting socket options");
   opt = 1;
   if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(int)) == -1)
   {
      my_error("setsockopt(SO_REUSEADDR): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf_pidfile);
      return(-1);
   };

   // bind socket to interface
   my_debug("binding socket");
   if (bind(s, &sa.sa, socklen) == -1)
   {
      my_error("bind(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf_pidfile);
      return(-1);
   };

   // log socket address
   socklen = sizeof(struct sockaddr_storage);
   if (getsockname(s, &sa.sa, &socklen) == -1)
   {
      my_error("getsockname(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf_pidfile);
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
      unlink(cnf_pidfile);
      return(-1);
   };

   // change ownership
   if ( (getgid() != cnf_gid) && (setregid(cnf_gid, cnf_gid) == -1) )
   {
      my_error("getgid(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf_pidfile);
      return(-1);
   };
   if ( (getuid() != cnf_uid) && (setreuid(cnf_uid, cnf_uid) == -1) )
   {
      my_error("getuid(): %s", strerror(errno));
      close(s);
      close(fd);
      unlink(cnf_pidfile);
      return(-1);
   };

   // fork process
   if (!(cnf_dont_fork))
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
   if (write(fd, buff, strlen(buff)) == -1)
   {
      close(fd);
      unlink(cnf_pidfile);
      my_error("write(): %s: %s", cnf_pidfile, strerror(errno));
      return(-1);
   };
   close(fd);

   // opens syslog
   openlog(prog_name, LOG_PID | (((cnf_dont_fork)) ? LOG_PERROR : 0), cnf_facility);
   syslog(LOG_NOTICE, "%s v%s", PROGRAM_NAME, PACKAGE_VERSION);
   syslog(LOG_NOTICE, "echo plus enabled: %s", ((cnf_echoplus)) ? "yes" : "no");
   syslog(LOG_NOTICE, "random delay: %u us", cnf_delay);
   syslog(LOG_NOTICE, "drop probability: %u%%", cnf_drop_perct);
   syslog(LOG_NOTICE, "running as UID: %u", getuid());
   syslog(LOG_NOTICE, "running as GID: %u", getgid());
   syslog(LOG_NOTICE, "listening on [%s]:%hu", addr_str, port);

   return(s);
}


// display debug message
void
my_debug(
         const char *                  fmt,
         ... )
{
   va_list args;

   if (!(cnf_verbose))
      return;

   printf("%s: ", prog_name);

   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);

   printf("\n");

   return;
}


// display error message
void
my_error(
         const char *                  fmt,
         ... )
{
   va_list args;

   fprintf(stderr, "%s: error: ", prog_name);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   fprintf(stderr, "\n");

   return;
}


// log connection
int
my_log_conn(
      int                              mode,
      size_t *                         connp,
      union my_sa *                    sap,
      struct udp_echo_plus *           msgp,
      ssize_t                          ssize,
      struct timespec *                tsp,
      useconds_t                       delay )
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
      case MY_INVAL: mode_name = "invalid"; break;
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
   if ((cnf_echoplus))
   {
      if (mode == MY_SENT)
      {
         syslog(LOG_INFO,
            "conn %zu: client: [%s]:%hu; %s bytes: %zi; timestamp: %lu.%09lu; seq: %u; delay: %u.%03u ms; delta: %u.%03u ms;",
            *connp,
            addr_str,
            port,
            mode_name,
            ssize,
            tsp->tv_sec,
            tsp->tv_nsec,
            ntohl(msgp->req_sn),
            (delay/1000),
            (delay%1000),
            (ntohl(msgp->reply_time) - ntohl(msgp->recv_time))/1000,
            (ntohl(msgp->reply_time) - ntohl(msgp->recv_time))%1000
         );
      } else
      {
         syslog(LOG_INFO,
            "conn %zu: client: [%s]:%hu; %s bytes: %zi; timestamp: %lu.%09lu; seq: %u;",
            *connp,
            addr_str,
            port,
            mode_name,
            ssize,
            tsp->tv_sec,
            tsp->tv_nsec,
            ntohl(msgp->req_sn)
         );
      };
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

   return(0);
}


// main loop
int
my_loop(
         int                           s,
         size_t *                      connp )
{
   socklen_t                  sinlen;
   ssize_t                    ssize;
   useconds_t                 delay;
   struct timespec            ts;
   uint64_t                   us_recv;
   uint64_t                   us_reply;
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
   if (cnf_verbose > 1)
      syslog(LOG_DEBUG, "waiting for echo request");
   if ((poll(fds, 1, 5000)) < 1)
      return(0);

   // increment connection counter
   state.req_sn++;
   (*connp)++;

   // read data
   sinlen = sizeof(struct sockaddr_storage);
   if ((ssize = recvfrom(s, udpbuff.bytes, sizeof(udpbuff), 0, &sa.sa, &sinlen)) == -1)
      return(-1);

   // grab timestamp
   clock_gettime(CLOCK_REALTIME, &ts);
   us_recv  = (uint64_t)(ts.tv_sec * 1000000);
   us_recv += (uint64_t)ts.tv_nsec / 1000;

   // log connection
   my_log_conn(MY_RECV, connp, &sa, &udpbuff.msg, ssize, &ts, 0);
   if ( ((cnf_echoplus)) && (ssize < (ssize_t)sizeof(struct udp_echo_plus)) )
   {
      my_log_conn(MY_INVAL, connp, &sa, &udpbuff.msg, ssize, &ts, 0);
      return(0);
   };

   // randomly drop packets
   if (cnf_drop_perct > 0)
   {
      if ( (rand() % 100) < cnf_drop_perct)
      {
         state.failures++;
         my_log_conn(MY_DROP, connp, &sa, &udpbuff.msg, ssize, &ts, 0);
         return(0);
      };
   };
   state.res_sn++;

   // insert random delay
   delay = 0;
   if (cnf_delay > 0)
   {
      delay = (useconds_t)rand() % cnf_delay;
      usleep(delay);
   };

   // grab timestamp
   clock_gettime(CLOCK_REALTIME, &ts);
   ts.tv_nsec++;
   us_reply  = (uint64_t)(ts.tv_sec * 1000000);
   us_reply += (uint64_t)ts.tv_nsec / 1000;

   // send response
   if ((cnf_echoplus))
   {
      udpbuff.msg.res_sn    = htonl(state.res_sn);
      udpbuff.msg.recv_time = htonl(us_recv & 0xFFFFFFFFLL);
      udpbuff.msg.reply_time = htonl(us_reply & 0xFFFFFFFFLL);
      udpbuff.msg.failures  = htonl(state.failures);
   };
   sendto(s, udpbuff.bytes, (size_t)ssize, 0, &sa.sa, sinlen);

   // log response
   my_log_conn(MY_SENT, connp, &sa, &udpbuff.msg, ssize, &ts, delay);

   return(0);
}


// signal handler
void
my_sighandler(
         int                           signum )
{
   signal(signum, my_sighandler);
   should_stop = 1;
   return;
}


// display program usage
void
my_usage(
         void )
{
   printf("Usage: %s [options]\n", prog_name);
   printf("OPTIONS:\n");
   printf("  -d num,  --drop=num       set packet drop probability [0-99] (default: %u%%)\n", cnf_drop_perct);
   printf("  -D usec, --delay=usec     set echo delay range to microseconds (default: %u us)\n", cnf_delay);
   printf("  -e,      --echoplus       enable echo plus, not RFC compliant%s\n", ((cnf_echoplus)) ? " (default)" : "");
   printf("  -f str,  --facility=str   set syslog facility (default: daemon)\n");
   printf("  -g gid,  --group=gid      setgid to gid (default: none)\n");
   printf("  -h,      --help           print this help and exit\n");
   printf("  -l addr, --listen=addr    bind to IP address (default: all)\n");
   printf("  -n,      --foreground     do not fork\n");
   printf("  -p port, --port=port      list on port number (default: %u)\n", cnf_port);
   printf("  -P file, --pidfile=file   PID file (default: %s)\n", cnf_pidfile);
   printf("  -r,      --rfc            RFC compliant echo protocol%s\n", (!(cnf_echoplus)) ? " (default)" : "");
   printf("  -u uid,  --user=uid       setuid to uid (default: none)\n");
   printf("  -v,      --verbose        enable verbose output\n");
   printf("  -V,      --version        print version number and exit\n");
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
