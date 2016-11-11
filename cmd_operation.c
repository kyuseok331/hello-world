// $Id$
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "msg_queue.h"
#include "channel_db.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

char version_str[32] = "GL3140-2MB-v3.1.0.6519\n";
char prompt_str[32] = "DR> ";
char mode_str[4] = "DAB";
char sort_str[10] = "alpha";
int cur_volume = 64;

int ErrorOutput(struct MsgQueue* q_out, char* msg)
{
    char str[80];
    snprintf(str, 80, "Error: %s\n", msg);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

int WaitOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) prompt_str);
    return 0;
}

int PromptOutput(struct MsgQueue* q_out)
{
    char str[80];
    strncpy(str, prompt_str, 32);
    strncat(str, "\n", 2);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

int PromptSet(struct MsgQueue* q_out, char* new_prompt)
{
    int len;

    len = strlen(new_prompt);
    DEBUG_PRINT("new_prompt=\"%s\" %d\n", new_prompt, len);

    if (len < 32)
    {
        strncpy(prompt_str, new_prompt+1, len-2);  // remove "
        prompt_str[len-2] = '\0';

        MsgQueuePut(q_out, (void*) "OK\n");
    }
    else
    {
        ErrorOutput(q_out, "length of string is too long.");
    }
    return 0;
}

int VersionOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) version_str);
    return 0;
}

int VolumeOutput(struct MsgQueue* q_out)
{
    char str[80];
    snprintf(str, 80, "%d\n", cur_volume);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

int VolumeSet(struct MsgQueue* q_out, int value)
{
    if (value >= 0 && value <= 100)
    {
        cur_volume = value;
        MsgQueuePut(q_out, (void*) "OK\n");
    }
    else
    {
        ErrorOutput(q_out, "olume value must be 0 ~ 100\n");
    }
    return 0;
}

int VolumeInc(struct MsgQueue* q_out, int value)
{
    cur_volume += value;
    if (cur_volume > 100)
        cur_volume = 100;
    else if (cur_volume < 0)
        cur_volume = 0;

    MsgQueuePut(q_out, (void*) "OK\n");
    return 0;
}

int TimeOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) "dd-mm-yy HH:MM:SS\n");
    return 0;
}

int ResetOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) "reset all properties as default\n");
    return 0;
}

int ModeOutput(struct MsgQueue* q_out)
{
    char str[80];
    strncpy(str, mode_str, 4);
    strncat(str, "\n", 2);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

int ModeSet(struct MsgQueue* q_out, char* new_mode)
{
    strncpy(mode_str, new_mode, 3);
    mode_str[3] = '\0';
    MsgQueuePut(q_out, (void*) "OK\n");
    return 0;
}

int ScanOutput(struct MsgQueue* q_out)
{
    struct ChFreqTable* pcf;
    char str[80];

    pcf = &b3_freq_tab[0];
    while (pcf->frequency != 0)
    {
        snprintf(str, 80, "tuning %s %d kHz ...\n",
                pcf->channel, pcf->frequency);
        usleep(20000);
        MsgQueuePut(q_out, (void*) str);
        pcf++;
    }

    ChTabInit();

    MsgQueuePut(q_out, (void*) "OK\n");
    return 0;
}

int TuneSet(struct MsgQueue* q_out, int freq)
{
    char str[80];

    snprintf(str, 80, "tuning %d kHz ...\n", freq);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

int SortOutput(struct MsgQueue* q_out)
{
    char str[80];
    strncpy(str, sort_str, 10);
    strncat(str, "\n", 2);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

int SortSet(struct MsgQueue* q_out, char* new_sort)
{
    strncpy(sort_str, new_sort, 9);
    sort_str[9] = '\0';

    ChTabSort();

    MsgQueuePut(q_out, (void*) "OK\n");
    return 0;
}

int ExitOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) "exit");
    return 0;
}


