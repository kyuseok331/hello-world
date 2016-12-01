/*
 *  rbsb.h
 */

#ifndef __RBSB_H_
#define __RBSB_H_

#ifdef V7
#error V7 defined
#include <sys/types.h>
#include <sys/stat.h>
#include <sgtty.h>
#define OS "V7/BSD"
#endif

#ifndef OS
#ifndef USG
#define USG
#endif
#endif

#ifdef USG
#include <sys/types.h>
#include <sys/stat.h>
#include <termio.h>
#include <sys/ioctl.h>
#define OS "SYS III/V"
#endif

#if HOWMANY  > 255
Howmany must be 255 or less
#endif

extern int iofd;       /* File descriptor for ioctls & reads */

extern unsigned short updcrc(int c, unsigned crc);
extern int mode(int n);
extern void sendbrk(void);

#endif // __RBSB_H_
