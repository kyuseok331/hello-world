// $Id$
//

#ifndef __CMD_OPERATION_H_
#define __CMD_OPERATION_H_

#include "msg_queue.h"

extern int ErrorOutput(struct MsgQueue* q_out, char* msg);
extern int WaitOutput(struct MsgQueue* q_out);

extern int PromptOutput(struct MsgQueue* q_out);
extern int PromptSet(struct MsgQueue* q_out, char* new_prompt);

extern int VersionOutput(struct MsgQueue* q_out);

extern int VolumeOutput(struct MsgQueue* q_out);
extern int VolumeSet(struct MsgQueue* q_out, int value);
extern int VolumeInc(struct MsgQueue* q_out, int value);

extern int TimeOutput(struct MsgQueue* q_out);

extern int ResetOutput(struct MsgQueue* q_out);

extern int ModeOutput(struct MsgQueue* q_out);
extern int ModeSet(struct MsgQueue* q_out, char* new_mode);

extern int ScanOutput(struct MsgQueue* q_out);
extern int TuneSet(struct MsgQueue* q_out, int freq);

extern int SortOutput(struct MsgQueue* q_out);
extern int SortSet(struct MsgQueue* q_out, char* new_sort);

extern int ExitOutput(struct MsgQueue* q_out);

#endif // __CMD_OPERATION_H_
