/*
 *  rz_main.c
 *
 */

#define VERSION "1.13 01-01-87"

#include <stdio.h>
#include <stdlib.h>
#include "rz.h"
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

void usage(void)
{
    fprintf(stderr,"%s %s for %s by Chuck Forsberg\n",
            Progname, VERSION, OS);
    fprintf(stderr,"Usage:  rz [-1abuv]     (ZMODEM Batch)\n");
    fprintf(stderr,"or  rb [-1abuv]     (YMODEM Batch)\n");
    fprintf(stderr,"or  rz [-1abcv] file    (XMODEM or XMODEM-1k)\n");
    fprintf(stderr,"      -1 For cu(1): Use fd 1 for input\n");
    fprintf(stderr,"      -a ASCII transfer (strip CR)\n");
    fprintf(stderr,"      -b Binary transfer for all files\n");
    fprintf(stderr,"      -v Verbose more v's give more info\n");
    fprintf(stderr,"      -c Use 16 bit CRC (XMODEM)\n");
    exit(1);
}

/*
 * Return 1 iff stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
int fromcu(void)
{
    struct stat a, b;
    fstat(1, &a); fstat(2, &b);
    return (a.st_rdev != b.st_rdev);
}

/*
 * Local console output simulation
 */
void bttyout(int c)
{
    if (Verbose || fromcu())
        putc(c, stderr);
}

int main(int argc, char* argv[])
{
    char *cp;
    int npats;
    // char *virgin;
    char **patts;
    int exitcode;

    //k if ((cp=getenv("SHELL")) && (substr(cp, "rsh") || substr(cp, "rksh")))
    //k     Restricted=TRUE;
    cp = "";

    //k chkinvok(virgin=argv[0]);   /* if called as [-]rzCOMMAND set flag */

    Rxtimeout = 100;
    npats = 0;
    setbuf(stderr, NULL);

    // Verbose = 11;
    Verbose = 3;
    while (--argc) {
        cp = *++argv;
        if ( !npats && argc>0) {
            if (argv[0][0]) {
                npats=argc;
                patts=argv;
            }
        }
    }
    if (npats > 1)
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
        // fprintf(stderr, "argv[0]=%s Progname=%s\n", virgin, Progname);
        fprintf(stderr, "Progname=%s\n", Progname);
    }
    if (fromcu() && !Quiet) {
        if (Verbose == 0)
            Verbose = 2;
    }

    //k mode(1);

    //k if (signal(SIGINT, bibi) == SIG_IGN) {
    //k     signal(SIGINT, SIG_IGN); signal(SIGKILL, SIG_IGN);
    //k }
    //k else {
    //k     signal(SIGINT, bibi); signal(SIGKILL, bibi);
    //k }

    DEBUG_PRINT("---\n");

    if (wcreceive(npats, patts)==ERROR) {
        exitcode=0200;
        canit();
    }
    mode(0);
    if (exitcode && !Zmodem)    /* bellow again with all thy might. */
        canit();
    serialPuts(fd_uart, "Transfer completed.\n");
    printf("exit\n");
    exit(exitcode);
}


