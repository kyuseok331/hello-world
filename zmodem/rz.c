#define PUBDIR "/usr/spool/uucppublic"

/*
 * rz.c By Chuck Forsberg
 *
 *      define CRCTABLE to use table driven CRC
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <utime.h>

#include "rz.h"
#include "rbsb.h"
#include "zm.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

// #include "rbsb.c"   /* most of the system dependent stuff here */

FILE *fout;

/*
 * Routine to calculate the free bytes on the current file system
 *  ~0 means many free bytes (unknown)
 */
long getfree(void)
{
    return(~0L);    /* many free bytes ... */
}

int Lastrx;
int Crcflg;
int Firstsec;
int Eofseen;        /* indicates cpm eof (^Z) has been received */
int errors;

#define DEFBYTL 2000000000L /* default rx file size */
long Bytesleft;     /* number of bytes of incoming file left */
long Modtime;       /* Unix style mod time for incoming file */
short Filemode;     /* Unix style mode for incoming file */
char Pathname[PATHLEN];

int Batch=0;
int MakeLCPathname=TRUE;    /* make received pathname lower case */
int Quiet=0;        /* overrides logic that would otherwise set verbose */
int Nflag = 0;      /* Don't really transfer files */
int Rxbinary=FALSE; /* receive all files in bin mode */
int Rxascii=FALSE;  /* receive files in ascii (translate) mode */
int Thisbinary;     /* current file is to be received in bin mode */
int Blklen;     /* record length of received packets */
char secbuf[KSIZE];
// time_t timep[2];
struct utimbuf timep;
char Lzmanag;       /* Local file management request */
char zconv;     /* ZMODEM file conversion request */
char zmanag;        /* ZMODEM file management request */
char ztrans;        /* ZMODEM file transport request */

// #include "zm.c"

int tryzhdrtype=ZRINIT; /* Header type to send corresponding to Last rx close */

/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
void zmputs(char* s)
{
    int c;

    while (*s) {
        switch (c = *s++) {
        case '\336':
            sleep(1); continue;
        case '\335':
            sendbrk(); continue;
        default:
            sendline(c);
        }
    }
}

/* called by signal interrupt or terminate to clean things up */
static void bibi(int n)
{
    if (Zmodem)
        zmputs(Attn);
    canit(); mode(0);
    fprintf(stderr, "rz: caught signal %d; exiting", n);
    exit(128+n);
}


/* make string s lower case */
static void uncaps(char* s)
{
    for ( ; *s; ++s)
        if (isupper(*s))
            *s = tolower(*s);
}
/*
 * IsAnyLower returns TRUE if string s has lower case letters.
 */
static int IsAnyLower(char* s)
{
    for ( ; *s; ++s)
        if (islower(*s))
            return TRUE;
    return FALSE;
}

/*
 * Totalitarian Communist pathname processing
 */
void checkpath(char* name)
{
    if (Restricted) {
        if (fopen(name, "r") != NULL) {
            canit();
            Lleft=0;    /* Do read next time ... */
            fprintf(stderr, "\r\nrz: %s exists\n", name);
            bibi(0);
        }
        /* restrict pathnames to current tree or uucppublic */
        if ( substr(name, "../")
         || (name[0]== '/' && strncmp(name, PUBDIR, strlen(PUBDIR))) ) {
            canit();
            Lleft=0;    /* Do read next time ... */
            fprintf(stderr,"\r\nrz:\tSecurity Violation\r\n");
            bibi(0);
        }
    }
}

/*
 * Strip leading ! if present, do shell escape.
 */
int sys2(char* s)
{
    if (*s == '!')
        ++s;
    return system(s);
}
/*
 * Strip leading ! if present, do exec.
 */
void exec2(char* s)
{
    if (*s == '!')
        ++s;
    mode(0);
    execl("/bin/sh", "sh", "-c", s);
}



/*
 * Wcgetsec fetches a Ward Christensen type sector.
 * Returns sector number encountered or ERROR if valid sector not received,
 * or CAN CAN received
 * or WCEOT if eot sector
 * time is timeout for first char, set to 4 seconds thereafter
 ***************** NO ACK IS SENT IF SECTOR IS RECEIVED OK **************
 *    (Caller must do that when he is good and ready to get next sector)
 */

int wcgetsec(char* rxbuf, int maxtime)
{
    int checksum, wcj, firstch;
    unsigned short oldcrc;
    char *p;
    int sectcurr;

    for (Lastrx=errors=0; errors<RETRYMAX; errors++) {

        if ((firstch=readline(maxtime))==STX) {
            Blklen=KSIZE; goto get2;
        }
        if (firstch==SOH) {
            Blklen=SECSIZ;
get2:
            sectcurr=readline(1);
            if ((sectcurr+(oldcrc=readline(1)))==Wcsmask) {
                oldcrc=checksum=0;
                for (p=rxbuf,wcj=Blklen; --wcj>=0; ) {
                    if ((firstch=readline(1)) < 0)
                        goto bilge;
                    oldcrc=updcrc(firstch, oldcrc);
                    checksum += (*p++ = firstch);
                }
                if ((firstch=readline(1)) < 0)
                    goto bilge;
                if (Crcflg) {
                    oldcrc=updcrc(firstch, oldcrc);
                    if ((firstch=readline(1)) < 0)
                        goto bilge;
                    oldcrc=updcrc(firstch, oldcrc);
                    if (oldcrc & 0xFFFF)
                        DEBUG_PRINT("CRC=0%o\n", oldcrc);
                    else {
                        Firstsec=FALSE;
                        return sectcurr;
                    }
                }
                else if (((checksum-firstch)&Wcsmask)==0) {
                    Firstsec=FALSE;
                    return sectcurr;
                }
                else
                    DEBUG_PRINT( "Checksum Error\n");
            }
            else
                DEBUG_PRINT("Sector number garbled 0%o 0%o\n", sectcurr, oldcrc);
        }
        /* make sure eot really is eot and not just mixmash */
#ifdef NFGVMIN
        else if (firstch==EOT && readline(1)==TIMEOUT)
            return WCEOT;
#else
        else if (firstch==EOT && Lleft==0)
            return WCEOT;
#endif
        else if (firstch==CAN) {
            if (Lastrx==CAN) {
                DEBUG_PRINT( "Sender CANcelled\n");
                return ERROR;
            } else {
                Lastrx=CAN;
                continue;
            }
        }
        else if (firstch==TIMEOUT) {
            if (Firstsec)
                goto humbug;
bilge:
            DEBUG_PRINT( "Timeout\n");
        }
        else
            DEBUG_PRINT( "Got 0%o sector header\n", firstch);

humbug:
        Lastrx=0;
        while(readline(1)!=TIMEOUT)
            ;
        if (Firstsec) {
            sendline(Crcflg?WANTCRC:NAK);
            Lleft=0;    /* Do read next time ... */
        } else {
            maxtime=40; sendline(NAK);
            Lleft=0;    /* Do read next time ... */
        }
    }
    /* try to stop the bubble machine. */
    canit();
    Lleft=0;    /* Do read next time ... */
    return ERROR;
}

/*
 * Process incoming file information header
 */
int procheader(char* name)
{
    char *openmode, *p;

    /* set default parameters and overrides */
    openmode = "w";
    Thisbinary = Rxbinary || !Rxascii;
    if (Lzmanag)
        zmanag = Lzmanag;

    /*
     *  Process ZMODEM remote file management requests
     */
    if (!Rxbinary && zconv == ZCNL) /* Remote ASCII override */
        Thisbinary = 0;
    if (zconv == ZCBIN) /* Remote Binary override */
        ++Thisbinary;
    else if (zmanag == ZMAPND)
        openmode = "a";
    /* ZMPROT check for existing file */
    if (zmanag == ZMPROT && (fout=fopen(name, "r"))) {
        fclose(fout);  return ERROR;
    }

    Bytesleft = DEFBYTL; Filemode = 0; Modtime = 0L;

    p = name + 1 + strlen(name);
    if (*p) {   /* file coming from Unix or DOS system */
        sscanf(p, "%ld%lo%o", &Bytesleft, &Modtime, &Filemode);
        if (Filemode & UNIXFILE)
            ++Thisbinary;
        if (Verbose) {
            fprintf(stderr,  "Incoming: %s %ld %lo %o\n",
              name, Bytesleft, Modtime, Filemode);
        }
    }
    else {      /* File coming from CP/M system */
        for (p=name; *p; ++p)       /* change / to _ */
            if ( *p == '/')
                *p = '_';

        if ( *--p == '.')       /* zap trailing period */
            *p = 0;
    }

    if (!Zmodem && MakeLCPathname && !IsAnyLower(name))
        uncaps(name);
    if (Topipe) {
        sprintf(Pathname, "%s %s", Progname+2, name);
        if (Verbose)
            fprintf(stderr,  "Topipe: %s %s\n",
              Pathname, Thisbinary?"BIN":"ASCII");
        if ((fout=popen(Pathname, "w")) == NULL)
            return ERROR;
    } else {
        strcpy(Pathname, name);
        if (Verbose) {
            fprintf(stderr,  "Receiving %s %s %s\n",
              name, Thisbinary?"BIN":"ASCII", openmode);
        }
        checkpath(name);
        if (Nflag)
            name = "/dev/null";
        if ((fout=fopen(name, openmode)) == NULL)
            return ERROR;
    }
    return OK;
}

/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
int putsec(char* buf, int n)
{
    char *p;

    if (Thisbinary) {
        for (p=buf; --n>=0; )
            putc( *p++, fout);
    }
    else {
        if (Eofseen)
            return OK;
        for (p=buf; --n>=0; ++p ) {
            if ( *p == '\r')
                continue;
            if (*p == CPMEOF) {
                Eofseen=TRUE; return OK;
            }
            putc(*p ,fout);
        }
    }
    return OK;
}


/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
int tryz(void)
{
    int n;
    int cmdzack1flg;

    DEBUG_PRINT("tryz()\n");

    if (Nozmodem)       /* Check for "rb" program name */
        return 0;


    for (n=Zmodem?10:5; --n>=0; ) {
        /* Set buffer length (0) and capability flags */
        stohdr(0L);
#ifdef CANBREAK
        Txhdr[ZF0] = CANFDX|CANOVIO|CANBRK;
#else
        Txhdr[ZF0] = CANFDX|CANOVIO;
#endif
        zshhdr(tryzhdrtype, Txhdr);
again:
        switch (zgethdr(Rxhdr, 0)) {
        case ZRQINIT:
            continue;
        case ZEOF:
            continue;
        case TIMEOUT:
            continue;
        case ZFILE:
            zconv = Rxhdr[ZF0];
            zmanag = Rxhdr[ZF1];
            ztrans = Rxhdr[ZF2];
            tryzhdrtype = ZRINIT;
            if (zrdata(secbuf, KSIZE) == GOTCRCW)
                return ZFILE;
            zshhdr(ZNAK, Txhdr);
            goto again;
        case ZSINIT:
            if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
                zshhdr(ZACK, Txhdr);
                goto again;
            }
            zshhdr(ZNAK, Txhdr);
            goto again;
        case ZFREECNT:
            stohdr(getfree());
            zshhdr(ZACK, Txhdr);
            goto again;
        case ZCOMMAND:
            cmdzack1flg = Rxhdr[ZF0];
            if (zrdata(secbuf, KSIZE) == GOTCRCW) {
                if (cmdzack1flg & ZCACK1)
                    stohdr(0L);
                else
                    stohdr((long)sys2(secbuf));
                Lleft = 0;
                purgeline();    /* dump impatient questions */
                do {
                    zshhdr(ZCOMPL, Txhdr);
                }
                while (++errors<10 && zgethdr(Rxhdr,1) != ZFIN);
                ackbibi();
                if (cmdzack1flg & ZCACK1)
                    exec2(secbuf);
                return ZCOMPL;
            }
            zshhdr(ZNAK, Txhdr); goto again;
        case ZCOMPL:
            goto again;
        default:
            continue;
        case ZFIN:
            ackbibi(); return ZCOMPL;
        case ZCAN:
            return ERROR;
        }
    }
    return 0;
}

/*
 * Close the receive dataset, return OK or ERROR
 */
int closeit(void)
{
    if (Topipe) {
        if (pclose(fout)) {
            return ERROR;
        }
        return OK;
    }
    if (fclose(fout)==ERROR) {
        fprintf(stderr, "file close ERROR\n");
        return ERROR;
    }
    if (Modtime) {
        timep.actime = time(NULL);
        timep.modtime = Modtime;
        utime(Pathname, &timep);
    }
    if (Filemode)
        chmod(Pathname, (07777 & Filemode));
    return OK;
}

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in secbuf
 */
int rzfile(void)
{
    int c, n;
    long rxbytes;

    DEBUG_PRINT("rzfile()\n");
    Eofseen=FALSE;
    if (procheader(secbuf) == ERROR) {
        return (tryzhdrtype = ZSKIP);
    }

    n = 10; rxbytes = 0l;

    for (;;) {
        stohdr(rxbytes);
        zshhdr(ZRPOS, Txhdr);
nxthdr:
        switch (c = zgethdr(Rxhdr, 0)) {
        default:
            vfile("rzfile: zgethdr returned %d", c);
            return ERROR;
        case ZNAK:
        case TIMEOUT:
            if ( --n < 0) {
                vfile("rzfile: zgethdr returned %d", c);
                return ERROR;
            }
        case ZFILE:
            zrdata(secbuf, KSIZE);
            continue;
        case ZEOF:
            if (rclhdr(Rxhdr) != rxbytes) {
                continue;
            }
            if (closeit()) {
                tryzhdrtype = ZFERR;
                vfile("rzfile: closeit returned <> 0");
                return ERROR;
            }
            vfile("rzfile: normal EOF");
            return c;
        case ERROR: /* Too much garbage in header search error */
            if ( --n < 0) {
                vfile("rzfile: zgethdr returned %d", c);
                return ERROR;
            }
            zmputs(Attn);
            continue;
        case ZDATA:
            if (rclhdr(Rxhdr) != rxbytes) {
                if ( --n < 0) {
                    return ERROR;
                }
                zmputs(Attn);  continue;
            }
moredata:
            switch (c = zrdata(secbuf, KSIZE)) {
            case ZCAN:
                vfile("rzfile: zgethdr returned %d", c);
                return ERROR;
            case ERROR: /* CRC error */
                if ( --n < 0) {
                    vfile("rzfile: zgethdr returned %d", c);
                    return ERROR;
                }
                zmputs(Attn);
                continue;
            case TIMEOUT:
                if ( --n < 0) {
                    vfile("rzfile: zgethdr returned %d", c);
                    return ERROR;
                }
                continue;
            case GOTCRCW:
                n = 10;
                putsec(secbuf, Rxcount);
                rxbytes += Rxcount;
                stohdr(rxbytes);
                zshhdr(ZACK, Txhdr);
                goto nxthdr;
            case GOTCRCQ:
                n = 10;
                putsec(secbuf, Rxcount);
                rxbytes += Rxcount;
                stohdr(rxbytes);
                zshhdr(ZACK, Txhdr);
                goto moredata;
            case GOTCRCG:
                n = 10;
                putsec(secbuf, Rxcount);
                rxbytes += Rxcount;
                goto moredata;
            case GOTCRCE:
                n = 10;
                putsec(secbuf, Rxcount);
                rxbytes += Rxcount;
                goto nxthdr;
            }
        }
    }
}

/*
 * Receive 1 or more files with ZMODEM protocol
 */
int rzfiles(void)
{
    int c;

    for (;;) {
        switch (c = rzfile()) {
        case ZEOF:
        case ZSKIP:
            switch (tryz()) {
            case ZCOMPL:
                return OK;
            default:
                return ERROR;
            case ZFILE:
                break;
            }
            continue;
        default:
            return c;
        case ERROR:
            return ERROR;
        }
    }
}

/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 */
int wcrxpn(char* rpn)
{
    int c;

#ifdef NFGVMIN
    readline(1);
#else
    Lleft = 0;
    purgeline();
#endif

et_tu:
    Firstsec=TRUE;  Eofseen=FALSE;
    sendline(Crcflg?WANTCRC:NAK);
    Lleft=0;    /* Do read next time ... */
    while ((c = wcgetsec(rpn, 100)) != 0) {
        DEBUG_PRINT( "Pathname fetch returned %d\n", c);
        if (c == WCEOT) {
            sendline(ACK);
            Lleft=0;    /* Do read next time ... */
            readline(1);
            goto et_tu;
        }
        return ERROR;
    }
    sendline(ACK);
    return OK;
}

/*
 * Adapted from CMODEM13.C, written by
 * Jack M. Wierda and Roderick W. Hart
 */

int wcrx(void)
{
    int sectnum, sectcurr;
    char sendchar;
    int cblklen;            /* bytes to dump this block */

    Firstsec=TRUE;sectnum=0; Eofseen=FALSE;
    sendchar=Crcflg?WANTCRC:NAK;

    for (;;) {
        sendline(sendchar); /* send it now, we're ready! */
        Lleft=0;    /* Do read next time ... */
        sectcurr=wcgetsec(secbuf, (sectnum&0177)?50:130);
        if (Verbose>1)
            fprintf(stderr, "%03d%c", sectcurr, sectcurr%10 ? ' ' : '\r');
        if (sectcurr==((sectnum+1) &Wcsmask)) {
            sectnum++;
            cblklen = Bytesleft>Blklen ? Blklen:Bytesleft;
            if (putsec(secbuf, cblklen)==ERROR)
                return ERROR;
            if ((Bytesleft-=cblklen) < 0)
                Bytesleft = 0;
            sendchar=ACK;
        }
        else if (sectcurr==(sectnum&Wcsmask)) {
            DEBUG_PRINT("Received dup Sector\n");
            sendchar=ACK;
        }
        else if (sectcurr==WCEOT) {
            if (closeit())
                return ERROR;
            sendline(ACK);
            Lleft=0;    /* Do read next time ... */
            return OK;
        }
        else if (sectcurr==ERROR)
            return ERROR;
        else {
            DEBUG_PRINT( "Sync Error\n");
            return ERROR;
        }
    }
}

/*
 * Let's receive something already.
 */

char *rbmsg =
"%s ready. To begin transfer, type \"%s file ...\" to your modem program\r\n";

int wcreceive(int argc, char** argp)
{
    int c;

    if (Batch || argc==0) {
        Crcflg=(Wcsmask==0377);
        if ( !Quiet)
            fprintf(stderr, rbmsg, Progname, Nozmodem?"sb":"sz");
        if ((c=tryz())) {
            if (c == ZCOMPL)
                return OK;
            if (c == ERROR)
                goto fubar;
            c = rzfiles();
            if (c)
                goto fubar;
        } else {
            for (;;) {
                if (wcrxpn(secbuf)== ERROR)
                    goto fubar;
                if (secbuf[0]==0)
                    return OK;
                if (procheader(secbuf) == ERROR)
                    goto fubar;
                if (wcrx()==ERROR)
                    goto fubar;
            }
        }
    } else {
        Bytesleft = DEFBYTL; Filemode = 0; Modtime = 0L;

        strcpy(Pathname, *argp);
        checkpath(Pathname);
        fprintf(stderr, "\nrz: ready to receive %s\r\n", Pathname);
        if ((fout=fopen(Pathname, "w")) == NULL)
            return ERROR;
        if (wcrx()==ERROR)
            goto fubar;
    }
    return OK;
fubar:
    canit();
    Lleft=0;    /* Do read next time ... */
    if (Topipe && fout) {
        pclose(fout);  return ERROR;
    }
    if (fout)
        fclose(fout);
    if (Restricted) {
        unlink(Pathname);
        fprintf(stderr, "\r\nrz: %s removed.\r\n", Pathname);
    }
    return ERROR;
}

