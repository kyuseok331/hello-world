// $Id$
//

#include <stdlib.h>
#include "msg_queue.h"
#include "cmd_lexan.h"
#include "cmd_operation.h"
#include "channel_db.h"

enum ParserState
{
    kParserBegin = 0,
    kParserVolumeArg,
    kParserVolumeArgP,
    kParserVolumeArgM,
    kParserPromptArg,
    kParserModeArg,
    kParserTuneArg,
    kParserSortArg,
    kParserListArg,
    kParserSearchArg,
    kParserStoreArg,
    kParserPlayArg,
    kParserEND
} parser_fsm;


int CmdParser(struct MsgQueue* q_in, struct MsgQueue* q_out)
{
    int token;
    int value;

    while (1)
    {
        token = CmdLexan(q_in);
        switch (parser_fsm)
        {
            case kParserBegin:
                if (token == kTokenNEWLINE)
                {
                    WaitOutput(q_out);
                }
                else if (token == kTokenVERSION)
                {
                    VersionOutput(q_out);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenVOLUME)
                {
                    parser_fsm = kParserVolumeArg;
                }
                else if (token == kTokenTIME)
                {
                    TimeOutput(q_out);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenPROMPT)
                {
                    parser_fsm = kParserPromptArg;
                }
                else if (token == kTokenMODE)
                {
                    parser_fsm = kParserModeArg;
                }
                else if (token == kTokenRESET)
                {
                    ResetOutput(q_out);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenSCAN)
                {
                    ScanOutput(q_out);
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenTUNE)
                    parser_fsm = kParserTuneArg;

                else if (token == kTokenSORT)
                    parser_fsm = kParserSortArg;

                else if (token == kTokenLIST)
                    parser_fsm = kParserListArg;

                else if (token == kTokenSEARCH)
                    parser_fsm = kParserSearchArg;

                else if (token == kTokenSTORE)
                    parser_fsm = kParserStoreArg;

                else if (token == kTokenPLAY)
                    parser_fsm = kParserPlayArg;

                else if (token == kTokenPREV)
                {
                    ChTabPlay(q_out, -1);   // prev
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenNEXT)
                {
                    ChTabPlay(q_out, -2);   // next
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenSTOP)
                {
                    ChTabStop(q_out);
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenINFO)
                {
                    ChTabInfo(q_out);
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenSIGNAL)
                {
                    QosOutput(q_out);
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenSTATUS)
                {
                    ChTabStatus(q_out);
                    parser_fsm = kParserEND;
                }

                else if (token == kTokenEXIT)
                {
                    ExitOutput(q_out);
                    return 0;
                }

                else
                {
                    ErrorOutput(q_out, "unknown command.");
                }
                break;

            case kParserVolumeArg:
                if (token == kTokenNEWLINE)
                {
                    VolumeOutput(q_out);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenNUMBER)
                {
                    value = atoi(yytext);
                    VolumeSet(q_out, value);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenPLUS)
                {
                    parser_fsm = kParserVolumeArgP;
                }
                else if (token == kTokenMINUS)
                {
                    parser_fsm = kParserVolumeArgM;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserVolumeArgP:
                if (token == kTokenNEWLINE)
                {
                    VolumeInc(q_out, +1);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenNUMBER)
                {
                    value = atoi(yytext);
                    VolumeInc(q_out, value);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserVolumeArgM:
                if (token == kTokenNEWLINE)
                {
                    VolumeInc(q_out, -1);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenNUMBER)
                {
                    value = atoi(yytext);
                    VolumeInc(q_out, -value);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserPromptArg:
                if (token == kTokenNEWLINE)
                {
                    PromptOutput(q_out);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenSTRING)
                {
                    PromptSet(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserModeArg:
                if (token == kTokenNEWLINE)
                {
                    ModeOutput(q_out);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenDAB)
                {
                    ModeSet(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenFM)
                {
                    ModeSet(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserTuneArg:
                if (token == kTokenNEWLINE)
                {
                    ErrorOutput(q_out, "freq. is required.");
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenNUMBER)
                {
                    value = atoi(yytext);
                    TuneSet(q_out, value);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenCHANNEL)
                {
                    TuneSet(q_out, ChToFreq(yytext));
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserSortArg:
                if (token == kTokenNEWLINE)
                {
                    SortOutput(q_out);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenALPHA)
                {
                    SortSet(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenENSEMBLE)
                {
                    SortSet(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args; alpha | ensemble");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserListArg:
                if (token == kTokenNEWLINE)
                {
                    ChTabList(q_out, 1);
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenAUDIO)
                {
                    ChTabList(q_out, 1);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenDATA)
                {
                    ChTabList(q_out, 2);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenALL)
                {
                    ChTabList(q_out, 3);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenPRESET)
                {
                    PresetList(q_out);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenFAVORITE)
                {
                    FavorList(q_out);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args; audio | data | all | preset | favorite");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserSearchArg:
                if (token == kTokenNEWLINE)
                {
                    ErrorOutput(q_out, "too few args: <string>");
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenSTRING)
                {
                    ChTabSearch(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args: <string>");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserStoreArg:
                if (token == kTokenNEWLINE)
                {
                    ErrorOutput(q_out, "too few args: <P#> | <F#>");
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenPSLOT)
                {
                    PresetStore(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenFSLOT)
                {
                    FavorStore(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args: <P#> | <F#>");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserPlayArg:
                if (token == kTokenNEWLINE)
                {
                    ChTabPlay(q_out, 0);   // last
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else if (token == kTokenNUMBER)
                {
                    value = atoi(yytext);
                    ChTabPlay(q_out, value);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenPSLOT)
                {
                    PresetRecall(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else if (token == kTokenFSLOT)
                {
                    FavorRecall(q_out, yytext);
                    parser_fsm = kParserEND;
                }
                else
                {
                    ErrorOutput(q_out, "unknown args: <P#> | <F#>");
                    parser_fsm = kParserEND;
                }
                break;

            case kParserEND:
                if (token == kTokenNEWLINE)
                {
                    WaitOutput(q_out);
                    parser_fsm = kParserBegin;
                }
                else
                {
                    ; // ignore others
                }
                break;

        }

    }
}
