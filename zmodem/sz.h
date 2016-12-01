/*
 *  sz.h
 *
 */

#ifndef __SZ_H_
#define __SZ_H_

#include <stdio.h>
#include "rzsz.h"

//0 extern int Modem;        /* MODEM - don't send pathnames */
//extern int Quiet;        /* overrides logic that would otherwise set verbose */
extern int errcnt;       /* number of files unreadable */
extern int Exitcode;
//1 extern int Fromcu;     /* Were called from cu or yam */

extern void canit(void);
extern int readline(int n);
extern void bttyout(int c);
extern void usage(void);
extern int wcsend(int argc, char* argp[]);

#endif // __SZ_H_
