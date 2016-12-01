/*
 *  rz.h
 *
 */

#ifndef __RZ_H_
#define __RZ_H_

#include <stdio.h>
#include "rzsz.h"

extern int Crcflg;
extern int MakeLCPathname;    /* make received pathname lower case */
extern int Quiet;        /* overrides logic that would otherwise set verbose */
extern int Nflag;      /* Don't really transfer files */
extern int Rxbinary; /* receive all files in bin mode */
extern int Rxascii;  /* receive files in ascii (translate) mode */
extern char Lzmanag;       /* Local file management request */


extern void canit(void);
extern int wcreceive(int argc, char** argp);

#endif // __RZ_H_
