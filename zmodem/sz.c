#define PUBDIR "/usr/spool/uucppublic"

/*
 * sz.c By Chuck Forsberg
 *
 *      define CRCTABLE to use table driven CRC
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "sz.h"
#include "rbsb.h"
#include "zm.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

// int Fromcu = 0;     /* Were called from cu or yam */
// #include "rbsb.c"   /* most of the system dependent stuff here */

/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */
#ifdef READCHECK
char Myattn[] = { 0 };
#else
#ifdef USG
char Myattn[] = { 03, 0336, 0 };
#else
char Myattn[] = { 0 };
#endif
#endif

FILE *in;

char Lastrx;
char Crcflg;
// int Modem=0;        /* MODEM - don't send pathnames */
//int Quiet=0;        /* overrides logic that would otherwise set verbose */
//int Ascii=0;        /* Add CR's for brain damaged programs */
int Fullname=0;     /* transmit full pathname */
int Unlinkafter=0;  /* Unlink file after it is sent */
int Dottoslash=0;   /* Change foo.bar.baz to foo/bar/baz */
int firstsec;
int errcnt=0;       /* number of files unreadable */
int blklen=SECSIZ;      /* length of transmitted records */
int Optiong;        /* Let it rip no wait for sector ACK's */
int Noeofseen;
int Totsecs;        /* total number of sectors this file */
char txbuf[KSIZE];
int Filcnt=0;       /* count of number of files opened */
int Lfseen=0;
int Rxbuflen = 16384;  /* Receiver's max buffer length */
int Tframlen = 0;   /* Override for tx frame length */
int blkopt=0;       /* Override value for zmodem blklen */
int Rxflags = 0;
char Lzconv;    /* Local ZMODEM file conversion request */
char Lzmanag;   /* Local ZMODEM file management request */
char Lztrans;
char zconv;     /* ZMODEM file conversion request */
char zmanag;        /* ZMODEM file management request */
char ztrans;        /* ZMODEM file transport request */
char *Cmdstr;       /* Pointer to the command string */
int Cmdtries = 11;
int Cmdack1;        /* Rx ACKs command, then do it */
int Exitcode;
int Testattn;       /* Force receiver to send Attn, etc with qbf. */
char *qbf="The quick brown fox jumped over the lazy dog's back 1234567890\r\n";
long Lastread;      /* Beginning offset of last buffer read */
int Lastc;      /* Count of last buffer read or -1 */
int Dontread;       /* Don't read the buffer, it's still there */

jmp_buf tohere;     /* For the interrupt on RX timeout */
jmp_buf intrjmp;    /* For the interrupt on RX CAN */

/* Called when Zmodem gets an interrupt (^X) */
//  void onintr(int n)
//  {
//      signal(SIGINT, SIG_IGN);
//      longjmp(intrjmp, -1);
//  }


static void alrm(int n)
{
    longjmp(tohere, -1);
}

#define ZKER
int Zctlesc;    /* Encode control characters */
// #include "zm.c"

/* called by signal interrupt or terminate to clean things up */
//  static void bibi(int n)
//  {
//      canit(); flushmo(); // mode(0);
//      fprintf(stderr, "sz: caught signal %d; exiting\n", n);
//      if (n == SIGQUIT)
//          abort();
//      exit(128+n);
//  }

/*
 * readock(timeout, count) reads character(s) from file descriptor 0
 *  (1 <= count <= 3)
 * it attempts to read count characters. If it gets more than one,
 * it is an error unless all are CAN
 * (otherwise, only normal response is ACK, CAN, or C)
 *  Only looks for one if Optiong, which signifies cbreak, not raw input
 *
 * timeout is in tenths of seconds
 */
int readock(int timeout, int count)
{
    int c;
    static char byt[5];

    if (Optiong)
        count = 1;  /* Special hack for cbreak */

    flushmo();
    if (setjmp(tohere)) {
        DEBUG_PRINT("TIMEOUT\n");
        return TIMEOUT;
    }
    c = timeout/10;
    if (c<2)
        c=2;
    if (Verbose>3) {
        fprintf(stderr, "Timeout=%d Calling alarm(%d) count=%d ", timeout, c, count);
        byt[1] = 0;
    }
    signal(SIGALRM, alrm); alarm(c);
#ifdef ONEREAD
    c=read(fd_uart, byt, 1);       /* regulus raw read is unique */
#else
    c=read(fd_uart, byt, count);
#endif
    alarm(0);
    if (Verbose>5)
        fprintf(stderr, "ret cnt=%d %x %x\n", c, byt[0], byt[1]);
    if (c<1)
        return TIMEOUT;
    if (c==1)
        return (byt[0]&0377);
    else
        while (c)
            if (byt[--c] != CAN)
                return ERROR;
    return CAN;
}

//  int readline(int n)
//  {
//      return (readock(n, 1));
//  }


/* Send send-init information */
int sendzsinit(void)
{
    int c;
    int errors;

    if (Myattn[0] == '\0')
        return OK;
    errors = 0;
    for (;;) {
        stohdr(0L);
        zsbhdr(ZSINIT, Txhdr);
        zsdata(Myattn, 1+strlen(Myattn), ZCRCW);
        c = zgethdr(Rxhdr, 1);
        switch (c) {
        case ZCAN:
            return ERROR;
        case ZACK:
            return OK;
        default:
            if (++errors > 9)
                return ERROR;
            continue;
        }
    }
}

/*
 * Get the receiver's init parameters
 */
int getzrxinit(void)
{
    int n;
    struct stat f;

    for (n=10; --n>=0; ) {

        switch (zgethdr(Rxhdr, 1)) {
        case ZCHALLENGE:    /* Echo receiver's challenge numbr */
            stohdr(Rxpos);
            zshhdr(ZACK, Txhdr);
            continue;
        case ZCOMMAND:      /* They didn't see out ZRQINIT */
            stohdr(0L);
            zshhdr(ZRQINIT, Txhdr);
            continue;
        case ZRINIT:
            Rxflags = 0377 & Rxhdr[ZF0];
            Rxbuflen = (0377 & Rxhdr[ZP0])+((0377 & Rxhdr[ZP1])<<8);
            vfile("Rxbuflen=%d Tframlen=%d", Rxbuflen, Tframlen);
            //k if ( !Fromcu)
            //k     signal(SIGINT, SIG_IGN);
#ifndef READCHECK
#ifdef USG
            // mode(2);    /* Set cbreak, XON/XOFF, etc. */
#else
            /* Use 1024 byte frames if no sample/interrupt */
            if (Rxbuflen < 32 || Rxbuflen > 1024) {
                Rxbuflen = 1024;
                vfile("Rxbuflen=%d", Rxbuflen);
            }
#endif
#endif
            /* Override to force shorter frame length */
            if (Rxbuflen && (Rxbuflen>Tframlen) && (Tframlen>=32))
                Rxbuflen = Tframlen;
            if ( !Rxbuflen && (Tframlen>=32) && (Tframlen<=1024))
                Rxbuflen = Tframlen;
            vfile("Rxbuflen=%d", Rxbuflen);

            /* If using a pipe for testing set lower buf len */
            fstat(iofd, &f);
            if ((f.st_mode & S_IFMT) != S_IFCHR
              && (Rxbuflen == 0 || Rxbuflen > 4096))
                Rxbuflen = 4096;
            /*
             * If input is not a regular file, force ACK's each 1024
             *  (A smarter strategey could be used here ...)
             */
            fstat(fileno(in), &f);
            if (((f.st_mode & S_IFMT) != S_IFREG)
              && (Rxbuflen == 0 || Rxbuflen > 1024))
                Rxbuflen = 1024;
            vfile("Rxbuflen=%d", Rxbuflen);

            return (sendzsinit());
        case ZCAN:
        case TIMEOUT:
            return ERROR;
        case ZRQINIT:
            if (Rxhdr[ZF0] == ZCOMMAND)
                continue;
        default:
            zshhdr(ZNAK, Txhdr);
            continue;
        }
    }
    return ERROR;
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int getinsync(void)
{
    int c;

    for (;;) {
        if (Testattn) {
            printf("\r\n\n\n***** Signal Caught *****\r\n");
            Rxpos = 0; c = ZRPOS;
        } else
            c = zgethdr(Rxhdr, 0);
        switch (c) {
        case ZCAN:
        case ZABORT:
        case ZFIN:
        case TIMEOUT:
            return ERROR;
        case ZRPOS:
            if (Lastc >= 0 && Lastread == Rxpos) {
                Dontread = TRUE;
            } else {
                clearerr(in);   /* In case file EOF seen */
                fseek(in, Rxpos, 0);
            }
            Txpos = Rxpos;
            return c;
        case ZACK:
            return c;
        case ZRINIT:
        case ZSKIP:
            fclose(in);
            return c;
        case ERROR:
        default:
            zsbhdr(ZNAK, Txhdr);
            continue;
        }
    }
}
/* Say "bibi" to the receiver, try to do it cleanly */
void saybibi(void)
{
    for (;;) {
        stohdr(0L);
        zsbhdr(ZFIN, Txhdr);
        switch (zgethdr(Rxhdr, 0)) {
        case ZFIN:
            sendline('O'); sendline('O'); flushmo();
        case ZCAN:
        case TIMEOUT:
            return;
        }
    }
}

/* fill buf with count chars */
int zfilbuf(char* buf, int count)
{
    int c, m;

    //DEBUG_PRINT("zfilbuf: count=%d\n", count);
    m=count;
    while ((c=getc(in))!=EOF) {
        *buf++ =c;
        if (--m == 0)
            break;
    }
    return (count - m);
}

/* Send command and related info */
int zsendcmd(char* buf, int blen)
{
    int c, errors;
    long cmdnum;

    cmdnum = getpid();
    errors = 0;
    for (;;) {
        stohdr(cmdnum);
        Txhdr[ZF0] = Cmdack1;
        zsbhdr(ZCOMMAND, Txhdr);
        zsdata(buf, blen, ZCRCW);
listen:
        Rxtimeout = 100;        /* Ten second wait for resp. */
        c = zgethdr(Rxhdr, 1);

        switch (c) {
        case ZRINIT:
            continue;
        case ERROR:
        case TIMEOUT:
            if (++errors > Cmdtries)
                return ERROR;
            continue;
        case ZCAN:
        case ZABORT:
        case ZFIN:
        case ZSKIP:
        case ZRPOS:
            return ERROR;
        default:
            if (++errors > 10)
                return ERROR;
            continue;
        case ZCOMPL:
            Exitcode = Rxpos;
            saybibi();
            return OK;
        case ZRQINIT:
            vfile("******** RZ *******");
            system("rz");
            vfile("******** SZ *******");
            goto listen;
        }
    }
}

/* Send the data in the file */
int zsendfdata(void)
{
    int c, e;
    int newcnt;
    long tcount = 0;
    static int tleft = 6;   /* Counter for test mode */

    if (Baudrate > 300)
        blklen = 256;
    if (Baudrate > 2400)
        blklen = KSIZE;
    if (Rxbuflen && blklen>Rxbuflen)
        blklen = Rxbuflen;
    if (blkopt && blklen > blkopt)
        blklen = blkopt;
    vfile("Rxbuflen=%d blklen=%d", Rxbuflen, blklen);
somemore:
    if (setjmp(intrjmp)) {
waitack:
        c = getinsync();
        switch (c) {
        default:
        case ZCAN:
            fclose(in);
            return ERROR;
        case ZSKIP:
            fclose(in);
            return c;
        case ZACK:
        case ZRPOS:
            break;
        case ZRINIT:
            return OK;
        }
#ifdef READCHECK
        /*
         * If the reverse channel can be tested for data,
         *  this logic may be used to detect error packets
         *  sent by the receiver, in place of setjmp/longjmp
         *  rdchk(fdes) returns non 0 if a character is available
         */
        while (rdchk(iofd)) {
#ifdef SVR2
            switch (checked)
#else
            switch (readline(1))
#endif
            {
            case CAN:
            case ZPAD:
                goto waitack;
            }
        }
#endif
    }

    //k if ( !Fromcu)
    //k     signal(SIGINT, onintr);
    newcnt = Rxbuflen;
    stohdr(Txpos);
    zsbhdr(ZDATA, Txhdr);

    /*
     * Special testing mode.  This should force receiver to Attn,ZRPOS
     *  many times.  Each time the signal should be caught, causing the
     *  file to be started over from the beginning.
     */
    if (Testattn) {
        if ( --tleft)
            while (tcount < 20000) {
                serialPrintf(fd_uart, qbf); // printf(qbf);
                flushmo();
                tcount += strlen(qbf);
#ifdef READCHECK
                while (rdchk(iofd)) {
#ifdef SVR2
                    switch (checked)
#else
                    switch (readline(1))
#endif
                    {
                    case CAN:
                    case ZPAD:
#ifdef TCFLSH
                        ioctl(iofd, TCFLSH, 1);
#endif
                        goto waitack;
                    }
                }
#endif
            }
        signal(SIGINT, SIG_IGN); canit();
        sleep(3); purgeline(); // mode(0);
        printf("\nsz: Tcount = %ld\n", tcount);
        if (tleft) {
            printf("ERROR: Interrupts Not Caught\n");
            exit(1);
        }
        exit(0);
    }

    do {
        if (Dontread) {
            c = Lastc;
        } else {
            c = zfilbuf(txbuf, blklen);
            Lastread = Txpos;  Lastc = c;
        }
        if (Verbose > 10)
            vfile("Dontread=%d c=%d", Dontread, c);
        Dontread = FALSE;
        if (c < blklen)
            e = ZCRCE;
        else if (Rxbuflen && (newcnt -= c) <= 0)
            e = ZCRCW;
        else
            e = ZCRCG;
        zsdata(txbuf, c, e);
        Txpos += c;
        if (e == ZCRCW)
            goto waitack;
#ifdef READCHECK
        /*
         * If the reverse channel can be tested for data,
         *  this logic may be used to detect error packets
         *  sent by the receiver, in place of setjmp/longjmp
         *  rdchk(fdes) returns non 0 if a character is available
         */
        flushmo();
        while (rdchk(iofd)) {
#ifdef SVR2
            switch (checked)
#else
            switch (readline(1))
#endif
            {
            case CAN:
            case ZPAD:
#ifdef TCFLSH
                ioctl(iofd, TCFLSH, 1);
#endif
                /* zcrce - dinna wanna start a ping-pong game */
                zsdata(txbuf, 0, ZCRCE);
                goto waitack;
            }
        }
#endif
    } while (c == blklen);
    //k if ( !Fromcu)
    //k     signal(SIGINT, SIG_IGN);

    for (;;) {
        stohdr(Txpos);
        zsbhdr(ZEOF, Txhdr);
        switch (getinsync()) {
        case ZACK:
            continue;
        case ZRPOS:
            goto somemore;
        case ZRINIT:
            return OK;
        case ZSKIP:
            fclose(in);
            return c;
        default:
            fclose(in);
            return ERROR;
        }
    }
}

/* Send file name and related info */
int zsendfile(char* buf, int blen)
{
    int c;

    DEBUG_PRINT("zsendfile: blen=%d\n", blen);
    for (;;) {
        Txhdr[ZF0] = Lzconv;    /* file conversion request */
        Txhdr[ZF1] = Lzmanag;   /* file management request */
        Txhdr[ZF2] = Lztrans;   /* file transport request */
        Txhdr[ZF3] = 0;
        zsbhdr(ZFILE, Txhdr);
        zsdata(buf, blen, ZCRCW);
again:
        c = zgethdr(Rxhdr, 1);
        switch (c) {
        case ZRINIT:
            goto again;
        case ZCAN:
        case TIMEOUT:
        case ZABORT:
        case ZFIN:
            return ERROR;
        case ZSKIP:
            fclose(in); return c;
        case ZRPOS:
            fseek(in, Rxpos, 0);
            Txpos = Rxpos; Lastc = -1; Dontread = FALSE;
            return zsendfdata();
        case ERROR:
        default:
            continue;
        }
    }
}

/* Local screen character display function */
void bttyout(int c)
{
    if (Verbose)
        putc(c, stderr);
}

/* fill buf with count chars padding with ^Z for CPM */
int filbuf(char* buf, int count)
{
    int c, m;

    // if ( !Ascii) {
        m = read(fileno(in), buf, count);
        if (m <= 0)
            return 0;
        while (m < count)
            buf[m++] = 032;
        return count;
    // }
    m=count;
    if (Lfseen) {
        *buf++ = 012; --m; Lfseen = 0;
    }
    while ((c=getc(in))!=EOF) {
        if (c == 012) {
            *buf++ = 015;
            if (--m == 0) {
                Lfseen = TRUE; break;
            }
        }
        *buf++ =c;
        if (--m == 0)
            break;
    }
    if (m==count)
        return 0;
    else
        while (--m>=0)
            *buf++ = CPMEOF;
    return count;
}

/* int cseclen;    data length of this sector to send */
int wcputsec(char* buf, int sectnum, int cseclen)
{
    int checksum, wcj;
    char *cp;
    unsigned oldcrc;
    int firstch;
    int attempts;

    firstch=0;  /* part of logic to detect CAN CAN */

    if (Verbose>1)
        fprintf(stderr, "\rSector %3d %2dk ", Totsecs, Totsecs/8 );
    for (attempts=0; attempts <= RETRYMAX; attempts++) {
        Lastrx= firstch;
        sendline(cseclen==KSIZE?STX:SOH);
        sendline(sectnum);
        sendline((-1) - sectnum);
        oldcrc=checksum=0;
        for (wcj=cseclen,cp=buf; --wcj>=0; ) {
            sendline(*cp);
            oldcrc=updcrc((0377& *cp), oldcrc);
            checksum += *cp++;
        }
        if (Crcflg) {
            oldcrc=updcrc(0,updcrc(0,oldcrc));
            sendline((int)oldcrc>>8);
            sendline((int)oldcrc);
        }
        else
            sendline(checksum);

        if (Optiong) {
            firstsec = FALSE; return OK;
        }
        firstch = readock(Rxtimeout, (Noeofseen&sectnum) ? 2:1);
gotnak:
        switch (firstch) {
        case CAN:
            if(Lastrx == CAN) {
cancan:
                DEBUG_PRINT("Cancelled\n");  return ERROR;
            }
            break;
        case TIMEOUT:
            DEBUG_PRINT("Timeout on sector ACK\n"); continue;
        case WANTCRC:
            if (firstsec)
                Crcflg = TRUE;
        case NAK:
            DEBUG_PRINT("NAK on sector\n"); continue;
        case ACK:
            firstsec=FALSE;
            Totsecs += (cseclen>>7);
            return OK;
        case ERROR:
            DEBUG_PRINT("Got burst for sector ACK\n"); break;
        default:
            DEBUG_PRINT("Got %02x for sector ACK\n", firstch); break;
        }
        for (;;) {
            Lastrx = firstch;
            if ((firstch = readock(Rxtimeout, 2)) == TIMEOUT)
                break;
            if (firstch == NAK || firstch == WANTCRC)
                goto gotnak;
            if (firstch == CAN && Lastrx == CAN)
                goto cancan;
        }
    }
    DEBUG_PRINT("Retry Count Exceeded\n");
    return ERROR;
}

int wctx(void)
{
    register int sectnum, attempts, firstch;

    firstsec=TRUE;

    while ((firstch=readock(Rxtimeout, 2))!=NAK && firstch != WANTCRC
      && firstch != WANTG && firstch!=TIMEOUT && firstch!=CAN)
        ;
    if (firstch==CAN) {
        DEBUG_PRINT("Receiver CANcelled\n");
        return ERROR;
    }
    if (firstch==WANTCRC)
        Crcflg=TRUE;
    if (firstch==WANTG)
        Crcflg=TRUE;
    sectnum=1;
    while (filbuf(txbuf, blklen)) {
        if (wcputsec(txbuf, sectnum, blklen)==ERROR) {
            return ERROR;
        } else
            sectnum++;
    }
    if (Verbose>1)
        fprintf(stderr, " Closing ");
    fclose(in);
    attempts=0;
    do {
        DEBUG_PRINT(" EOT ");
        purgeline();
        sendline(EOT);
        flushmo();
        ++attempts;
    }
        while ((firstch=(readock(Rxtimeout, 1)) != ACK) && attempts < RETRYMAX);
    if (attempts == RETRYMAX) {
        DEBUG_PRINT("No ACK on EOT\n");
        return ERROR;
    }
    else
        return OK;
}

int getnak(void)
{
    int firstch;

    Lastrx = 0;
    for (;;) {
        switch (firstch = readock(800,1)) {
        case ZPAD:
            if (getzrxinit())
                return ERROR;
            // Ascii = 0;
            return FALSE;
        case TIMEOUT:
            DEBUG_PRINT("Timeout on pathname\n");
            return TRUE;
        case WANTG:
#ifdef USG
            // mode(2);    /* Set cbreak, XON/XOFF, etc. */
#endif
            Optiong = TRUE;
            blklen=KSIZE;
        case WANTCRC:
            Crcflg = TRUE;
        case NAK:
            return FALSE;
        case CAN:
            if ((firstch = readock(20,1)) == CAN && Lastrx == CAN)
                return TRUE;
        default:
            break;
        }
        Lastrx = firstch;
    }
}

/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
int wctxpn(char* name)
{
    register char *p, *q;
    char name2[PATHLEN];
    struct stat f;

    //k if (Modem) {
    //k     if ((in!=stdin) && *name && fstat(fileno(in), &f)!= -1) {
    //k         fprintf(stderr, "Sending %s, %ld blocks: ",
    //k           name, f.st_size>>7);
    //k     }
    //k     fprintf(stderr, "Give your local XMODEM receive command now.\r\n");
    //k     return OK;
    //k }
    DEBUG_PRINT("\r\nAwaiting pathname nak for %s\r\n", *name?name:"<END>");
    // DEBUG_PRINT("Zmodem=%d\n",Zmodem);
    if ( !Zmodem)
        if (getnak())
            return ERROR;

    // DEBUG_PRINT("Zmodem=%d\n",Zmodem);   // Zmodem=1
    q = (char *) 0;
    if (Dottoslash) {       /* change . to . */
        for (p=name; *p; ++p) {
            if (*p == '/')
                q = p;
            else if (*p == '.')
                *(q=p) = '/';
        }
        if (q && strlen(++q) > 8) {  /* If name>8 chars */
            q += 8;         /*   make it .ext */
            strcpy(name2, q);   /* save excess of name */
            *q = '.';
            strcpy(++q, name2); /* add it back */
        }
    }

    DEBUG_PRINT("name=%s %d\n", name, strlen(name));
    for (p=name, q=txbuf ; *p; )
        if ((*q++ = *p++) == '/' && !Fullname)
            q = txbuf;
    *q++ = 0;
    p=q;
    while (q < (txbuf + KSIZE))
        *q++ = 0;
    // if (!Ascii && (in!=stdin) && *name && fstat(fileno(in), &f)!= -1)
    if ((in!=stdin) && *name && fstat(fileno(in), &f)!= -1)
        sprintf(p, "%lu %lo %o", f.st_size, f.st_mtime, f.st_mode);
    // DEBUG_PRINT("txbuf=%s %s\n", txbuf, p);
    // {
    //     struct tm time_str;
    //     time_t time_sec;
    //     time_str.tm_year = 2016 - 1900;
    //     time_str.tm_mon = 11 - 1;
    //     time_str.tm_mday = 21;
    //     time_str.tm_hour = 15;
    //     time_str.tm_min = 10;
    //     time_str.tm_sec = 0;
    //     time_str.tm_isdst = 0;
    //     time_sec = mktime(&time_str);
    //     DEBUG_PRINT("time=%lo\n", time_sec);
    // }
    /* force 1k blocks if name won't fit in 128 byte block */
    if (txbuf[125])
        blklen=KSIZE;
    else {      /* A little goodie for IMP/KMD */
        if (Zmodem)
            blklen = SECSIZ;
        txbuf[127] = (f.st_size + 127) >>7;
        txbuf[126] = (f.st_size + 127) >>15;
    }
    if (Zmodem)
        return zsendfile(txbuf, 1+strlen(p)+(p-txbuf));
    if (wcputsec(txbuf, 0, SECSIZ)==ERROR)
        return ERROR;
    return OK;
}

int wcs(char* oname)
{
    int c;
    // char *p;
    struct stat f;
    char name[PATHLEN];

    strcpy(name, oname);

    if ((in=fopen(oname, "r"))==NULL) {
        ++errcnt;
        return OK;  /* pass over it, there may be others */
    }
    ++Noeofseen;  Lastread = 0;  Lastc = -1; Dontread = FALSE;
    /* Check for directory or block special files */
    fstat(fileno(in), &f);
    c = f.st_mode & S_IFMT;
    if (c == S_IFDIR || c == S_IFBLK) {
        fclose(in);
        return OK;
    }

    ++Filcnt;
    switch (wctxpn(name)) {
    case ERROR:
        return ERROR;
    case ZSKIP:
        return OK;
    }
    if (!Zmodem && wctx()==ERROR)
        return ERROR;
    if (Unlinkafter)
        unlink(oname);
    return 0;
}

int wcsend(int argc, char* argp[])
{
    int n;

    stohdr(0L);
    zshhdr(ZRQINIT, Txhdr);

    Crcflg=FALSE;
    firstsec=TRUE;
    for (n=0; n<argc; ++n) {
        DEBUG_PRINT("wcsend %d: %s\n", n, argp[n]);
        Totsecs = 0;
        if (wcs(argp[n])==ERROR)
            return ERROR;
    }

    Totsecs = 0;
    if (Zmodem)
        saybibi();
    else
        wctxpn("");
    return OK;
}

