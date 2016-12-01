/*
 * rzsz.c
 *
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rzsz.h"

int Zmodem=0;       /* ZMODEM protocol requested */
unsigned Baudrate;
int Restricted=0;   /* restricted; no /.. or ../ in filenames */
char *Progname = "sz";
int Nozmodem = 0;   /* If invoked as "rb" */
int Topipe=0;

/*
 * substr(string, token) searches for token in string s
 * returns pointer to token within string if found, NULL otherwise
 */
char *substr(char* s, char* t)
{
    char *ss,*tt;
    /* search for first char of token */
    for (ss=s; *s; s++)
        if (*s == *t)
            /* compare token with substring */
            for (ss=s,tt=t; ;) {
                if (*tt == 0)
                    return s;
                if (*ss++ != *tt++)
                    break;
            }
    return NULL;
}

/*
 * If called as [-][dir/../]vrzCOMMAND set Verbose to 1
 * If called as [-][dir/../]rzCOMMAND set the pipe flag
 * If called as rb use YMODEM protocol
 */
void chkinvok(char* s)
{
    char *p;

    p = s;
    while (*p == '-')
        s = ++p;
    while (*p)
        if (*p++ == '/')
            s = p;
    if (*s == 'v') {
        Verbose=1; ++s;
    }
    Progname = s;
    if (s[0]=='r' && s[1]=='b')
        Nozmodem = TRUE;
    if (s[2] && s[0]=='r' && s[1]=='b')
        Topipe=TRUE;
    if (s[2] && s[0]=='r' && s[1]=='z')
        Topipe=TRUE;
}

/* send cancel string to get the other end to shut up */
void canit(void)
{
    static char canistr[] = {
     24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
    };

    serialPrintf(fd_uart, canistr);   // printf(canistr);
    // Lleft=0;    /* Do read next time ... */
    flushmo(); // fflush(stdout);
}

void purgeline(void)
{
#ifdef USG
    ioctl(iofd, TCFLSH, 0);
#else
    lseek(iofd, 0L, 2);
#endif
}

jmp_buf tohere;     /* For the interrupt on RX timeout */

static void alrm(int n)
{
    longjmp(tohere, -1);
}

#ifdef ONEREAD
/* Sorry, Regulus and some others don't work right in raw mode! */
int Readnum = 1;    /* Number of bytes to ask for in read() from modem */
#else
int Readnum = HOWMANY;  /* Number of bytes to ask for in read() from modem */
#endif

int Wcsmask=0377;
int Verbose=0;
char linbuf[HOWMANY];
int Lleft=0;        /* number of characters in linbuf */

/*
 * This version of readline is reasoably well suited for
 * reading many characters.
 *  (except, currently, for the Regulus version!)
 *
 * timeout is in tenths of seconds
 */
int readline(int timeout)
{
    int n;
    static char *cdq;   /* pointer for removing chars from linbuf */

    if (--Lleft >= 0) {
        if (Verbose > 8) {
            fprintf(stderr, "%02x ", *cdq&0377);
        }
        return (*cdq++ & Wcsmask);
    }
    n = timeout/10;
    if (n < 2)
        n = 3;
    if (Verbose > 3)
        fprintf(stderr, "Calling read: n=%d ", n);
    if (setjmp(tohere)) {
#ifdef TIOCFLUSH
/*      ioctl(iofd, TIOCFLUSH, 0); */
#endif
        Lleft = 0;
        if (Verbose>1)
            fprintf(stderr, "Readline:TIMEOUT\n");
        return TIMEOUT;
    }
    signal(SIGALRM, alrm); alarm(n);
    // Lleft=read(iofd, cdq=linbuf, Readnum);
    Lleft=read(fd_uart, cdq=linbuf, Readnum);
    alarm(0);
    if (Verbose > 3) {
        fprintf(stderr, "Read returned %d bytes\n", Lleft);
    }
    if (Lleft < 1)
        return TIMEOUT;
    --Lleft;
    if (Verbose > 8) {
        fprintf(stderr, "%02x ", *cdq&0377);
    }
    return (*cdq++ & Wcsmask);
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
void ackbibi(void)
{
    int n;

    vfile("ackbibi:");
    Readnum = 1;
    stohdr(0L);
    for (n=4; --n>=0; ) {
        zshhdr(ZFIN, Txhdr);
        for (;;) {
            switch (readline(100)) {
            case 'O':
                readline(1);    /* Discard 2nd 'O' */
                /* ***** FALL THRU TO ***** */
            case TIMEOUT:
                vfile("ackbibi complete");
                return;
            default:
                break;
            }
        }
    }
}




