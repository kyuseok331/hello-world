/*
 *  zm.h
 */

#ifndef __ZM_H_
#define __ZM_H_

#include "zmodem.h"

extern void zsendline(int c);
extern void zsbhdr(int type, register char* hdr);
extern void zshhdr(int type, char* hdr);
extern int zgethdr(char* hdr, int eflag);
extern void zsdata(char* buf, int length, int frameend);
extern int zrdata(char* buf, int length);
extern void stohdr(long pos);
extern long rclhdr(char* hdr);

#endif // __ZM_H_
