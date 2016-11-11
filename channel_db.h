// $Id$
//

#ifndef __CHANNEL_DB_H_
#define __CHANNEL_DB_H_

#include "msg_queue.h"

struct ChFreqTable {
    char* channel;
    int   len;
    int   frequency;
};

extern struct ChFreqTable b3_freq_tab[];

extern int ChToFreq(char* chan);

extern int ChTabInit(void);
extern int ChTabSort(void);
extern int ChTabPrint(void);
extern int ChTabSearch(struct MsgQueue* q_out, char* string);
extern int ChTabList(struct MsgQueue* q_out, int mode);
extern int ChTabPlay(struct MsgQueue* q_out, int idx);
extern int ChTabStop(struct MsgQueue* q_out);
extern int ChTabInfo(struct MsgQueue* q_out);
extern int ChTabStatus(struct MsgQueue* q_out);

extern int PresetList(struct MsgQueue* q_out);
extern int PresetStore(struct MsgQueue* q_out, char* pslot);
extern int PresetRecall(struct MsgQueue* q_out, char* pslot);

extern int FavorList(struct MsgQueue* q_out);
extern int FavorStore(struct MsgQueue* q_out, char* fslot);
extern int FavorRecall(struct MsgQueue* q_out, char* fslot);

extern int QosOutput(struct MsgQueue* q_out);

#endif // __CHANNEL_DB_H_
