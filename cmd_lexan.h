// $Id$
//

#ifndef __LEXAN_H_
#define __LEXAN_H_

#include "msg_queue.h"

extern char yytext[80];
extern int yyleng;

enum LexanToken
{
    kTokenUndef,
    kTokenNEWLINE,
    kTokenNUMBER,
    kTokenCHANNEL,
    kTokenPLUS,
    kTokenMINUS,
    kTokenSTRING,
    kTokenID,
    kTokenVERSION,
    kTokenVOLUME,
    kTokenTIME,
    kTokenPROMPT,
    kTokenMODE,
    kTokenDAB,
    kTokenFM,
    kTokenRESET,
    kTokenSCAN,
    kTokenTUNE,
    kTokenSORT,
    kTokenALPHA,
    kTokenENSEMBLE,
    kTokenLIST,
    kTokenAUDIO,
    kTokenDATA,
    kTokenALL,
    kTokenPRESET,
    kTokenFAVORITE,
    kTokenSEARCH,
    kTokenSTORE,
    kTokenPLAY,
    kTokenLAST,
    kTokenPREV,
    kTokenNEXT,
    kTokenSTOP,
    kTokenINFO,
    kTokenSIGNAL,
    kTokenSTATUS,
    kTokenPSLOT,
    kTokenFSLOT,
    kTokenEXIT,
    kTokenError,
};

extern int CmdLexan(struct MsgQueue* q_in);

#endif // __LEXAN_H_
