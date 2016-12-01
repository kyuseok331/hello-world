/*
 *  sz_main.c
 *
 */

#define VERSION "sz 1.22 01-01-87"

#include <stdio.h>
#include <stdlib.h>
#include "sz.h"
#include "rbsb.h"
#include "zm.h"

#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

int fd_uart;

char *babble[] = {
    "Send file(s) with ZMODEM/YMODEM/XMODEM Protocol",
    "   (Y) = Option applies to YMODEM only",
    "   (Z) = Option applies to ZMODEM only",
    "Usage: sz [-12+adefkLlNnquvXy] [-] file ...",
    "   sz [-1eqv] -c COMMAND",
    "   1 Use stdout for modem input",
#ifdef CSTOPB
    "   2 Use 2 stop bits",
#endif
    "   + Append to existing destination file (Z)",
    "   a (ASCII) change NL to CR/LF",
    "   c send COMMAND (Z)",
    "   d Change '.' to '/' in pathnames (Y/Z)",
    "   e Escape all control characters (Z)",
    "   f send Full pathname (Y/Z)",
    "   i send COMMAND, ack Immediately (Z)",
    "   k Send 1024 byte packets (Y)",
    "   L N Limit subpacket length to N bytes (Z)",
    "   l N Limit frame length to N bytes (l>=L) (Z)",
    "   n send file if source Newer or longer (Z)",
    "   N send file if source different length or date (Z)",
    "   p Protect existing destination file (Z)",
    "   r Resume/Recover interrupted file transfer (Z)",
    "   q Quiet (no progress reports)",
    "   u Unlink file after transmission",
    "   v Verbose - debugging information",
    "   X XMODEM protocol - send no pathnames",
    "   y Yes, overwrite existing file (Z)",
    "- as pathname sends standard input as sPID.sz or environment ONAME",
    ""
};

void usage(void)
{
    char **pp;

    for (pp=babble; **pp; ++pp)
        fprintf(stderr, "%s\n", *pp);
    fprintf(stderr, "%s for %s by Chuck Forsberg  ", VERSION, OS);
    exit(1);
}

/*
 * return 1 iff stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
int from_cu(void)
{
    struct stat a, b;
    fstat(1, &a); fstat(2, &b);
    return (a.st_rdev != b.st_rdev);
}


int main(int argc, char* argv[])
{
    char *cp;
    int npats;
    int agcnt; char **agcv;
    char **patts;
    //k static char xXbuf[BUFSIZ];

    //k if ((cp = getenv("ZNULLS")) && *cp)
    //k     Znulls = atoi(cp);
    //k if ((cp=getenv("SHELL")) && (substr(cp, "rsh") || substr(cp, "rksh")))
    //k     Restricted=TRUE;
    //k chkinvok(argv[0]);

    printf("BUFSIZ = %d\n", BUFSIZ);

    Rxtimeout = 600;
    npats=0;
    if (argc<2)
        usage();
    //k setbuf(stdout, xXbuf);
    Verbose = 11;
    while (--argc) {
        cp = *++argv;
        if ( !npats && argc>0) {
            if (argv[0][0]) {
                npats=argc;
                patts=argv;
            }
        }
    }
    if (npats < 1)
        usage();

    if ((fd_uart = serialOpen ("/dev/serial0", 115200)) < 0)
    {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
        exit(1);
    }

    if (wiringPiSetup () == -1)
    {
        fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
        exit(1);
    }

            serialPuts(fd_uart, "start sz 01234567890123456789 123456789 123456789\n");
            {
                int i;
                for (i=0; i<200; i++)
                {
                    serialPuts(fd_uart, "0123456789 123456789 123456789 123456789\n");
                }
                    serialPuts(fd_uart, "0123456789 123456789 end.\n");
            }

    if (Verbose) {
        if (freopen(LOGFILE, "a", stderr)==NULL) {
            printf("Can't open log file %s\n",LOGFILE);
            exit(0200);
        }
        setbuf(stderr, NULL);
    }
    //k if ((Fromcu=from_cu()) && !Quiet) {
    //k     if (Verbose == 0)
    //k         Verbose = 2;
    //k }

    //k mode(1);

    //k if (signal(SIGINT, bibi) == SIG_IGN) {
    //k     signal(SIGINT, SIG_IGN); signal(SIGKILL, SIG_IGN);
    //k } else {
    //k     signal(SIGINT, bibi); signal(SIGKILL, bibi);
    //k }
    //k if ( !Fromcu)
    //k     signal(SIGQUIT, SIG_IGN);

    DEBUG_PRINT("---\n");

    serialPuts(fd_uart, "rz\n"); // serialFlush(fd_uart);
    serialPrintf(fd_uart, "%s: %d file%s requested:\r\n",
            Progname, npats, npats>1?"s":"");
    fprintf(stderr, "%s: %d file%s requested:\r\n",
            Progname, npats, npats>1?"s":"");
    for ( agcnt=npats, agcv=patts; --agcnt>=0; ) {
        fprintf(stderr, "%s ", *agcv++);
    }
    fprintf(stderr, "\r\n");
    // printf("\r\n\bSending in Batch Mode\r\n");
    serialPuts(fd_uart, "\r\n\bSending in Batch Mode\r\n");

    if (wcsend(npats, patts)==ERROR) {
        Exitcode=0200;
        canit();
    }

    serialPuts(fd_uart, "Transfer completed.\n");
    printf("exit\n");
    exit((errcnt != 0) | Exitcode);
    /*NOTREACHED*/
}

