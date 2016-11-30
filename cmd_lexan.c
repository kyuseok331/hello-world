// $Id$
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "msg_queue.h"
#include "cmd_lexan.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

char yyinbuf[80];
char* yyin = yyinbuf; // FILE* yyin= stdin;
//FILE* yyout= stdout;

char yytext[80];
int yyleng;
//int yylval;

#define LF '\n'
#define CR '\r'
#define SPACE ' '
#define TAB '\t'
#define PLUS '+'
#define MINUS '-'
#define DQUOTE '\"'

enum LexanState
{
    kLexanBegin = 0,
    kLexanP0,
    kLexanP1,
    kLexanF0,
    kLexanF1,
    kLexanId,
    kLexanNum,
    kLexanChannel0,
    kLexanChannel1,
    kLexanChannel2,
    kLexanString,
    kLexanError
} lex_fsm;

struct IdTable {
    int tk;
    char* str;
    int len;
};

struct IdTable id_table[] = {
    {kTokenHELP, "help", 4},
    {kTokenVERSION, "version", 7},
    {kTokenVOLUME, "volume", 6},
    {kTokenTIME, "time", 4},
    {kTokenPROMPT, "prompt", 6},
    {kTokenMODE, "mode", 4},
    {kTokenDAB, "DAB", 3},
    {kTokenFM, "FM", 2},
    {kTokenRESET, "reset", 5},
    {kTokenSCAN, "scan", 4},
    {kTokenTUNE, "tune", 4},
    {kTokenSORT, "sort", 4},
    {kTokenALPHA, "alpha", 5},
    {kTokenENSEMBLE, "ensemble", 8},
    {kTokenLIST, "list", 8},
    {kTokenAUDIO, "audio", 5},
    {kTokenDATA, "data", 4},
    {kTokenALL, "all", 3},
    {kTokenPRESET, "preset", 6},
    {kTokenFAVORITE, "favorite", 8},
    {kTokenSEARCH, "search", 6},
    {kTokenSTORE, "store", 5},
    {kTokenPLAY, "play", 4},
    {kTokenLAST, "last", 4},
    {kTokenPREV, "prev", 4},
    {kTokenNEXT, "next", 4},
    {kTokenSTOP, "stop", 4},
    {kTokenINFO, "info", 4},
    {kTokenSIGNAL, "signal", 6},
    {kTokenSTATUS, "status", 6},
    {kTokenEXIT, "exit", 4},
    {kTokenUndef, "", 0}
};

int CmdLexan(struct MsgQueue* q_in)
{
    int token = kTokenUndef;
    yyleng = 0;

    while (token == kTokenUndef)
    {
        if (*yyin == '\0')
        {
            MsgQueueGet(q_in, yyinbuf);
            yyin = yyinbuf;
            // DEBUG_PRINT("get: yyin[0]=%d \"%s\"\n", yyin[0], yyin);
        }

        yytext[yyleng++] = *yyin;

        switch (lex_fsm)
        {
            case kLexanBegin:
                if (tolower(*yyin) == 'p')
                    lex_fsm = kLexanP0;
                else if (tolower(*yyin) == 'f')
                    lex_fsm = kLexanF0;
                else if (isalpha(*yyin))
                    lex_fsm = kLexanId;
                else if (isdigit(*yyin))
                    lex_fsm = kLexanChannel0;
                else if (*yyin == DQUOTE)
                    lex_fsm = kLexanString;
                else if (*yyin == PLUS)
                    token = kTokenPLUS;
                else if (*yyin == MINUS)
                    token = kTokenMINUS;
                else if (*yyin == LF)
                    token = kTokenNEWLINE;
                else if (*yyin == CR)
                    token = kTokenNEWLINE;
                else if (*yyin == SPACE)    // isblank
                    yyleng = 0;
                else if (*yyin == TAB)      // isblank
                    yyleng = 0;
                else
                {
                    token = kTokenError;
                    yyleng = 0;
                }
                break;

            case kLexanP0:
                if (isalpha(*yyin))
                    lex_fsm = kLexanId;
                else if (isdigit(*yyin))
                    lex_fsm = kLexanP1;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenID;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanP1:
                if (isalpha(*yyin))
                    lex_fsm = kLexanId;
                else if (isdigit(*yyin))
                    lex_fsm = kLexanP1;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenPSLOT;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanF0:
                if (isalpha(*yyin))
                    lex_fsm = kLexanId;
                else if (isdigit(*yyin))
                    lex_fsm = kLexanF1;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenID;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanF1:
                if (isalpha(*yyin))
                    lex_fsm = kLexanId;
                else if (isdigit(*yyin))
                    lex_fsm = kLexanF1;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenFSLOT;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanId:
                if (isalpha(*yyin))
                    lex_fsm = kLexanId;
                else if (isdigit(*yyin))
                    lex_fsm = kLexanId;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenID;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanChannel0:   // #
                if (isdigit(*yyin))
                    lex_fsm = kLexanChannel1;
                else if (isalpha(*yyin))
                    lex_fsm = kLexanChannel2;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenNUMBER;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanChannel1:   // ##
                if (isdigit(*yyin))
                    lex_fsm = kLexanNum;
                else if (isalpha(*yyin))
                    lex_fsm = kLexanChannel2;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenNUMBER;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanChannel2:   // ##A
                if (isdigit(*yyin)) // ##A#
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenError;
                }
                else if (isalpha(*yyin))
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenError;
                }
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenCHANNEL;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanNum:
                if (isdigit(*yyin))
                    lex_fsm = kLexanNum;
                else
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenNUMBER;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanString:
                if (*yyin == DQUOTE)
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenSTRING;
                }
                else if (*yyin == LF)
                {
                    // yyerror();
                    lex_fsm = kLexanBegin;
                    token = kTokenError;
                    yyin--;
                    yyleng--;
                }
                break;

            case kLexanError:
                if (*yyin == LF)
                {
                    lex_fsm = kLexanBegin;
                    token = kTokenError;
                    yyin--;
                    yyleng--;
                }
                break;

        }
        yyin++;
        yytext[yyleng] = '\0';

        if (token == kTokenID)
        {
            struct IdTable* id = &id_table[0];
            while (id->tk != kTokenUndef)
            {
                if (strncmp(yytext, id->str, id->len) == 0)
                {
                    token = id->tk;
                    break;
                }
                id++;
            }
        }
    }

    DEBUG_PRINT("token=%d, yytext=\"%s\"\n", token, yytext);
    return token;
}

