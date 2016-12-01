/*
 *  rzsz.h
 *
 */

#ifndef __RZSZ_H_
#define __RZSZ_H_

#include "zmodem.h"
#include "zm.h"
#include "rbsb.h"

#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR (-1)

#include <wiringPi.h>
#include <wiringSerial.h>
extern int fd_uart;

// #define sendline(c) putchar((c) & Wcsmask)
#define sendline(c) serialPutchar(fd_uart, (c) & Wcsmask)

// #define xsendline(c) putchar(c)
#define xsendline(c) serialPutchar(fd_uart, c)

// #define flushmo() fflush(stdout)
// #define flushmo() serialFlush(fd_uart)
#define flushmo() do {} while (0)

/* VARARGS1 */
#define vfile(f, ...) \
    do { if (Verbose > 1) { \
            fprintf(stderr, f, ##__VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } } while (0)

#define LOGFILE "/tmp/rzszlog"
#define zperr vfile

/*
 * Max value for HOWMANY is 255.
 *   A larger value reduces system overhead but may evoke kernel bugs.
 *   133 corresponds to a XMODEM/CRC sector
 */
#ifndef HOWMANY
#define HOWMANY 133
#endif

/* Ward Christensen / CP/M parameters - Don't change these! */
#define ENQ 005
#define CAN ('X'&037)
#define XOFF ('s'&037)
#define XON ('q'&037)
#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define NAK 025
#define CPMEOF 032
#define WANTCRC 0103    /* send C not NAK to get crc not checksum */
#define WANTG 0107  /* Send G not NAK to get nonstop batch xmsn */
#define TIMEOUT (-2)
#define RCDO (-3)
#define ERRORMAX 5
//#define RETRYMAX 5
#define RETRYMAX 10
#define WCEOT (-10)
#define SECSIZ 128  /* cp/m's Magic Number record size */
#define PATHLEN 257 /* ready for 4.2 bsd ? */
#define KSIZE 1024  /* record size with k option */
#define UNIXFILE 0x8000 /* happens to the the S_IFREG file mask bit for stat */

extern int Wcsmask;
extern int Verbose;
extern int Lleft;        /* number of characters in linbuf */
extern int Zmodem;       /* ZMODEM protocol requested */
extern unsigned Baudrate;
extern int Restricted;   /* restricted; no /.. or ../ in filenames */
extern char *Progname;     /* the name by which we were called */
extern int Nozmodem;   /* If invoked as "rb" */
extern int Topipe;

extern char *substr(char* s, char* t);
extern void chkinvok(char* s);
extern void canit(void);
extern void purgeline(void);
extern int readline(int timeout);
extern void ackbibi(void);

#endif // __RZSZ_H_
