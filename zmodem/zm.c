/*
 *   Z M . C
 *    ZMODEM protocol primitives
 *    8-16-86  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *  zsbhdr(type, hdr) send binary header
 *  zshhdr(type, hdr) send hex header
 *  zgethdr(hdr, eflag) receive header - binary or hex
 *  zsdata(buf, len, frameend) send data
 *  zrdata(buf, len) receive data
 *  stohdr(pos) store position data in Txhdr
 *  long rclhdr(hdr) recover position offset from header
 */

#include <stdio.h>

#include "sz.h"
#include "rbsb.h"
#include "zm.h"

/* Globals used by ZMODEM functions */
int Rxframeind;		/* ZBIN or ZHEX indicates type of frame received */
int Rxtype;		/* Type of header received */
int Rxcount;		/* Count of data bytes received */
int Rxtimeout = 100;	/* Tenths of seconds to wait for something */
char Rxhdr[4];		/* Received header */
char Txhdr[4];		/* Transmitted header */
long Rxpos;		/* Received file position */
long Txpos;		/* Transmitted file position */
int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */
char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */

static char *frametypes[] = {
    "Carrier Lost",     /* -3 */
    "TIMEOUT",      /* -2 */
    "ERROR",        /* -1 */
#define FTOFFSET 3
    "ZRQINIT",
    "ZRINIT",
    "ZSINIT",
    "ZACK",
    "ZFILE",
    "ZSKIP",
    "ZNAK",
    "ZABORT",
    "ZFIN",
    "ZRPOS",
    "ZDATA",
    "ZEOF",
    "ZFERR",
    "ZCRC",
    "ZCHALLENGE",
    "ZCOMPL",
    "ZCAN",
    "ZFREECNT",
    "ZCOMMAND",
    "ZSTDERR",
    "xxxxx"
#define FRTYPES 22  /* Total number of frame types in this array */
            /*  not including psuedo negative entries */
};

/* Send ZMODEM binary header hdr of type type */
void zsbhdr(int type, register char* hdr)
{
    register int n;
    register unsigned short crc;

    vfile("zsbhdr: %s %lx", frametypes[type+FTOFFSET], rclhdr(hdr));
    if (type == ZDATA)
        for (n = Znulls; --n >=0; )
            zsendline(0);

    xsendline(ZPAD); xsendline(ZDLE); xsendline(ZBIN);

    zsendline(type); crc = updcrc(type, 0);

    for (n=4; --n >= 0;) {
        zsendline(*hdr);
        crc = updcrc((0377& *hdr++), crc);
    }
    crc = updcrc(0,updcrc(0,crc));
    zsendline(crc>>8);
    zsendline(crc);
    if (type != ZDATA)
        flushmo();
}

/* Send a byte as two hex digits */
void zputhex(int c)
{
    static char digits[]    = "0123456789abcdef";

    if (Verbose>4)
        vfile("zputhex: %x", c);
    sendline(digits[(c&0xF0)>>4]);
    sendline(digits[(c)&0xF]);
}

/* Send ZMODEM HEX header hdr of type type */
void zshhdr(int type, char* hdr)
{
    int n;
    unsigned short crc;

    vfile("zshhdr: %s %lx", frametypes[type+FTOFFSET], rclhdr(hdr));
    sendline(ZPAD); sendline(ZPAD); sendline(ZDLE); sendline(ZHEX);
    zputhex(type);

    crc = updcrc(type, 0);
    for (n=4; --n >= 0;) {
        zputhex(*hdr); crc = updcrc((0377& *hdr++), crc);
    }
    crc = updcrc(0,updcrc(0,crc));
    zputhex(crc>>8); zputhex(crc);

    /* Make it printable on remote machine */
    sendline(015); sendline(012);
    /*
     * Uncork the remote in case a fake XOFF has stopped data flow
     */
    if (type != ZFIN)
        sendline(021);
    flushmo();
}

/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
void zsdata(char* buf, int length, int frameend)
{
    unsigned short crc;

    vfile("zsdata: length=%d end=%x", length, frameend);
    crc = 0;
    for (;--length >= 0;) {
        zsendline(*buf);
        crc = updcrc((0377& *buf++), crc);
    }
    xsendline(ZDLE); xsendline(frameend);
    crc = updcrc(frameend, crc);

    crc = updcrc(0,updcrc(0,crc));
    zsendline(crc>>8);
    zsendline(crc);
    if (frameend == ZCRCW) {
        xsendline(XON);  flushmo();
    }
}

/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
int zdlread(void)
{
    int c;

    if ((c = readline(Rxtimeout)) != ZDLE)
        return c;
    if ((c = readline(Rxtimeout)) < 0)
        return c;
    if (c == CAN && (c = readline(Rxtimeout)) < 0)
        return c;
    if (c == CAN && (c = readline(Rxtimeout)) < 0)
        return c;
    if (c == CAN && (c = readline(Rxtimeout)) < 0)
        return c;
    switch (c) {
    case CAN:
        return GOTCAN;
    case ZCRCE:
    case ZCRCG:
    case ZCRCQ:
    case ZCRCW:
        return (c | GOTOR);
    case ZRUB0:
        return 0177;
    case ZRUB1:
        return 0377;
    default:
        if ((c & 0140) ==  0100)
            return (c ^ 0100);
        break;
    }
    zperr("Got bad ZMODEM escape sequence %x", c);
    return ERROR;
}

/* Receive a binary style header (type and position) */
int zrbhdr(char* hdr)
{
    int c, n;
    unsigned short crc;

    if ((c = zdlread()) & ~0377)
        return c;
    Rxtype = c;
    crc = updcrc(c, 0);

    for (n=4; --n >= 0;) {
        if ((c = zdlread()) & ~0377)
            return c;
        crc = updcrc(c, crc);
        *hdr++ = c;
    }
    if ((c = zdlread()) & ~0377)
        return c;
    crc = updcrc(c, crc);
    if ((c = zdlread()) & ~0377)
        return c;
    crc = updcrc(c, crc);
    if (crc & 0xFFFF) {
        zperr("Bad Header CRC"); return ERROR;
    }
    Zmodem = 1;
    return Rxtype;
}

/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 */
int zrdata(char* buf, int length)
{
    int c;
    unsigned short crc;
    int d;

    crc = Rxcount = 0;
    for (;;) {
        if ((c = zdlread()) & ~0377) {
crcfoo:
            switch (c) {
            case GOTCRCE:
            case GOTCRCG:
            case GOTCRCQ:
            case GOTCRCW:
                crc = updcrc((d=c)&0377, crc);
                if ((c = zdlread()) & ~0377)
                    goto crcfoo;
                crc = updcrc(c, crc);
                if ((c = zdlread()) & ~0377)
                    goto crcfoo;
                crc = updcrc(c, crc);
                if (crc & 0xFFFF) {
                    zperr("Bad data CRC %x", crc);
                    return ERROR;
                }
                vfile("zrdata: cnt = %d ret = %x", Rxcount, d);
                return d;
            case GOTCAN:
                zperr("ZMODEM: Sender Canceled");
                return ZCAN;
            case TIMEOUT:
                zperr("ZMODEM data TIMEOUT");
                return c;
            default:
                zperr("ZMODEM bad data subpacket ret=%x", c);
                return c;
            }
        }
        if (--length < 0) {
            zperr("ZMODEM data subpacket too long");
            return ERROR;
        }
        ++Rxcount;
        *buf++ = c;
        crc = updcrc(c, crc);
        continue;
    }
}

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
int noxread7(void)
{
    int c;

    for (;;) {
        if ((c = readline(Rxtimeout)) < 0)
            return c;
        switch (c &= 0177) {
        case XON:
        case XOFF:
            continue;
        default:
            return c;
        }
    }
}

/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
void zsendline(int c)
{
    static int lastsent;

    switch (c & 0377) {
    case ZDLE:
        xsendline(ZDLE);
        xsendline (lastsent = (c ^= 0100));
        break;
    case 015:
    case 0215:
        if ((lastsent & 0177) != '@')
            goto sendit;
    /* **** FALL THRU TO **** */
    case 020:
    case 021:
    case 023:
    case 0220:
    case 0221:
    case 0223:
#ifdef ZKER
        if (Zctlesc<0)
            goto sendit;
#endif
        xsendline(ZDLE);
        c ^= 0100;
sendit:
        xsendline(lastsent = c);
        break;
    default:
#ifdef ZKER
        if (Zctlesc>0 && ! (c & 0140)) {
            xsendline(ZDLE);
            c ^= 0100;
        }
#endif
        xsendline(lastsent = c);
    }
}

int zgeth1(void)
{
    int c, n;

    if ((c = noxread7()) < 0)
        return c;
    n = c - '0';
    if (n > 9)
        n -= ('a' - ':');
    if (n & ~0xF)
        return ERROR;
    if ((c = noxread7()) < 0)
        return c;
    c -= '0';
    if (c > 9)
        c -= ('a' - ':');
    if (c & ~0xF)
        return ERROR;
    c += (n<<4);
    return c;
}

/* Decode two lower case hex digits into an 8 bit byte value */
int zgethex(void)
{
    int c;

    c = zgeth1();
    if (Verbose>4)
        vfile("zgethex: %x", c);
    return c;
}

/* Store long integer pos in Txhdr */
void stohdr(long pos)
{
    Txhdr[ZP0] = pos;
    Txhdr[ZP1] = pos>>8;
    Txhdr[ZP2] = pos>>16;
    Txhdr[ZP3] = pos>>24;
}

/* Recover a long integer from a header */
long rclhdr(char* hdr)
{
    long l;

    l = (hdr[ZP3] & 0377);
    l = (l << 8) | (hdr[ZP2] & 0377);
    l = (l << 8) | (hdr[ZP1] & 0377);
    l = (l << 8) | (hdr[ZP0] & 0377);
    return l;
}

/* Receive a hex style header (type and position) */
int zrhhdr(char* hdr)
{
    int c;
    unsigned short crc;
    int n;

    if ((c = zgethex()) < 0)
        return c;
    Rxtype = c;
    crc = updcrc(c, 0);

    for (n=4; --n >= 0;) {
        if ((c = zgethex()) < 0)
            return c;
        crc = updcrc(c, crc);
        *hdr++ = c;
    }
    if ((c = zgethex()) < 0)
        return c;
    crc = updcrc(c, crc);
    if ((c = zgethex()) < 0)
        return c;
    crc = updcrc(c, crc);
    if (crc & 0xFFFF) {
        zperr("Bad Header CRC"); return ERROR;
    }
    if (readline(1) == '\r')    /* Throw away possible cr/lf */
        readline(1);
    Zmodem = 1; return Rxtype;
}

/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *  eflag controls local display of non zmodem characters:
 *  0:  no display
 *  1:  display printing characters only
 *  2:  display all non ZMODEM characters
 *  On success, set Zmodem to 1 and return type of header.
 *   Otherwise return negative on error
 */
int zgethdr(char* hdr, int eflag)
{
    int c, n, cancount;

    n = Baudrate;   /* Max characters before start of frame */
    cancount = 5;
again:
    Rxframeind = Rxtype = 0;
    switch (c = noxread7()) {
    case RCDO:
    case TIMEOUT:
        goto fifi;
    case CAN:
        if (--cancount <= 0) {
            c = ZCAN; goto fifi;
        }
    /* **** FALL THRU TO **** */
    default:
agn2:
        if ( --n == 0) {
            zperr("ZMODEM Garbage count exceeded");
            return(ERROR);
        }
        if (eflag && ((c &= 0177) & 0140))
            bttyout(c);
        else if (eflag > 1)
            bttyout(c);
        if (c != CAN)
            cancount = 5;
        goto again;
    case ZPAD:      /* This is what we want. */
        break;
    }
    cancount = 5;
splat:
    switch (c = noxread7()) {
    case ZPAD:
        goto splat;
    case RCDO:
    case TIMEOUT:
        goto fifi;
    default:
        goto agn2;
    case ZDLE:      /* This is what we want. */
        break;
    }

    switch (c = noxread7()) {
    case RCDO:
    case TIMEOUT:
        goto fifi;
    case ZBIN:
        Rxframeind = ZBIN;
        c =  zrbhdr(hdr);
        break;
    case ZHEX:
        Rxframeind = ZHEX;
        c =  zrhhdr(hdr);
        break;
    case CAN:
        if (--cancount <= 0) {
            c = ZCAN; goto fifi;
        }
        goto agn2;
    default:
        goto agn2;
    }
    Rxpos = hdr[ZP3] & 0377;
    Rxpos = (Rxpos<<8) + (hdr[ZP2] & 0377);
    Rxpos = (Rxpos<<8) + (hdr[ZP1] & 0377);
    Rxpos = (Rxpos<<8) + (hdr[ZP0] & 0377);
fifi:
    switch (c) {
    case GOTCAN:
        c = ZCAN;
    /* **** FALL THRU TO **** */
    case ZNAK:
    case ZCAN:
    case ERROR:
    case TIMEOUT:
    case RCDO:
        zperr("ZMODEM: Got %s %s", frametypes[c+FTOFFSET],
          (c >= 0) ? "header" : "error");
    /* **** FALL THRU TO **** */
    default:
        if (c >= -3 && c <= FRTYPES)
            vfile("zgethdr: %s %lx", frametypes[c+FTOFFSET], Rxpos);
        else
            vfile("zgethdr: %d %lx", c, Rxpos);
    }
    return c;
}

