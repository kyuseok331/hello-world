// $Id$
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "msg_queue.h"
#include "channel_db.h"
#include "cmd_operation.h"

#define CHANNEL_MAX 250
#define PRESET_MAX 20
#define FAVOR_MAX 20

enum {
    false,
    true
};

// Band-3 frequency table
struct ChFreqTable b3_freq_tab[] = {
    { "5A", 2, 174928 },
    { "5B", 2, 176640 },
    { "5C", 2, 178352 },
    { "5D", 2, 180064 },
    { "6A", 2, 181936 },
    { "6B", 2, 183648 },
    { "6C", 2, 185360 },
    { "6D", 2, 187072 },
    { "7A", 2, 188928 },
    { "7B", 2, 190640 },
    { "7C", 2, 192352 },
    { "7D", 2, 194064 },
    { "8A", 2, 195936 },
    { "8B", 2, 197648 },
    { "8C", 2, 199360 },
    { "8D", 2, 201072 },
    { "9A", 2, 202928 },
    { "9B", 2, 204640 },
    { "9C", 2, 206352 },
    { "9D", 2, 208064 },
    { "10A", 3, 209936 },
    { "10B", 3, 211648 },
    { "10C", 3, 213360 },
    { "10D", 3, 215072 },
    { "11A", 3, 216928 },
    { "11B", 3, 218640 },
    { "11C", 3, 220352 },
    { "11D", 3, 222064 },
    { "12A", 3, 223936 },
    { "12B", 3, 225648 },
    { "12C", 3, 227360 },
    { "12D", 3, 229072 },
    { "13A", 3, 230748 },
    { "13B", 3, 232496 },
    { "13C", 3, 234208 },
    { "13D", 3, 235776 },
    { "13E", 3, 237448 },
    { "13F", 3, 239200 },
    { "end", 3, 0 }
};

// Channel Information
struct ChannelInfo {
    int  sid;           // service id
    char long_name[20]; // long name
};

struct ChannelInfo channel_tab[CHANNEL_MAX];
struct ChannelInfo preset_tab[PRESET_MAX];
struct ChannelInfo favor_tab[FAVOR_MAX];

struct ChannelInfo* pchannel_tab[CHANNEL_MAX];
int channel_tab_max;
int cur_ch_idx = 1;
int cur_qos = 5;

int ChToFreq(char* chan)
{
    struct ChFreqTable* pcf;
    pcf = &b3_freq_tab[0];
    while (pcf->frequency != 0)
    {
        if (strncmp(chan, pcf->channel, pcf->len) == 0)
        {
            break;
        }
        pcf++;
    }
    return pcf->frequency;
}

int ChTabInit(void)
{
    struct ChannelInfo* pci;
    FILE* ifp;
    int idx;
    int leng;

    ifp = fopen("channel_db.txt", "r");

    channel_tab_max = 0;
    pci = &channel_tab[0];
    while (fgets(pci->long_name, 20, ifp) != NULL)
    {
        leng = strlen(pci->long_name);
        pci->long_name[leng-1] = '\0';    // remove \n
        pci++;
        channel_tab_max++;
        if (channel_tab_max > CHANNEL_MAX)
            break;
    }

    fclose(ifp);

    for (idx=0; idx<CHANNEL_MAX; idx++)
    {
        pchannel_tab[idx] = &channel_tab[idx];
    }

    return 0;
}

// bubble sort
int ChTabSort(void)
{
    int ii, jj;
    int swapped;
    struct ChannelInfo* temp;

    for (ii = 0; ii < channel_tab_max - 1; ii++)
    {
        swapped = false;

        for (jj=0; jj<channel_tab_max - 1; jj++)
        {
            if (strncmp(pchannel_tab[jj]->long_name, pchannel_tab[jj+1]->long_name, 20) > 0)
            {   // swap
                temp = pchannel_tab[jj];
                pchannel_tab[jj] = pchannel_tab[jj+1];
                pchannel_tab[jj+1] = temp;
                swapped = true;
            }
        }

        if (swapped == false)
            break;
    }

    return 0;
}

int ChTabPrint(void)
{
    int ii;
    struct ChannelInfo* pci;

    for (ii=0; ii<channel_tab_max; )
    {
        pci = pchannel_tab[ii++];
        // if (pci->long_name[0] == NULL)
        //     break;

        printf("%d \"%s\" %d\n", ii, pci->long_name, pci->sid);
    }

    return 0;
}

int ChTabFind(struct ChannelInfo* pch)
{
    int ii;
    struct ChannelInfo* pci;

    for (ii=0; ii<channel_tab_max; )
    {
        pci = pchannel_tab[ii++];
        if (strncmp(pci->long_name, pch->long_name, 20) == 0 &&
                pci->sid == pch->sid)
        {
            return ii;
        }
    }

    return 0;
}

int ChTabSearch(struct MsgQueue* q_out, char* string)
{
    int ii;
    struct ChannelInfo* pci;
    int len;
    char substr[20];
    char str[80];

    snprintf(str, 80, "searching %s in the audio list\n", string);
    MsgQueuePut(q_out, (void*) str);

    len = strlen(string);
    strncpy(substr, string+1, len-2);  // remove "
    substr[len-2] = '\0';

    for (ii=0; ii<channel_tab_max; )
    {
        pci = pchannel_tab[ii++];
        if (strstr(pci->long_name, substr) != NULL)
        {
            snprintf(str, 80, "%d: \"%s\" %d\n", ii, pci->long_name, pci->sid);
            MsgQueuePut(q_out, (void*) str);
        }
    }

    MsgQueuePut(q_out, (void*) "OK\n");

    return 0;
}

int ChTabList(struct MsgQueue* q_out, int mode)
{
    int ii;
    struct ChannelInfo* pci;
    char str[80];

    if (mode == 1)
        snprintf(str, 80, "A1: list audio\n");
    else if (mode == 2)
        snprintf(str, 80, "D1: list data\n");
    else // if (mode == 3)
        snprintf(str, 80, "A1: list audio and data\n");

    MsgQueuePut(q_out, (void*) str);

    for (ii=0; ii<channel_tab_max; )
    {
        pci = pchannel_tab[ii++];
        snprintf(str, 80, "%d: \"%s\" %d\n", ii, pci->long_name, pci->sid);
        MsgQueuePut(q_out, (void*) str);
    }

    MsgQueuePut(q_out, (void*) "OK\n");

    return 0;
}

int ChTabPlay(struct MsgQueue* q_out, int idx)
{
    char str[80];
    struct ChannelInfo* pci;

    if (idx == 0)
        idx = cur_ch_idx;
    else if (idx == -1) // prev
    {
        idx = cur_ch_idx - 1;
        if (idx <= 0)
            idx = channel_tab_max;
    }
    else if (idx == -2) // next
    {
        idx = cur_ch_idx + 1;
        if (idx > channel_tab_max)
            idx = 1;
    }
    else if (idx > channel_tab_max)
    {
        ErrorOutput(q_out, "invalid args");
        return 1;
    }

    cur_ch_idx = idx;
    pci = pchannel_tab[idx-1];
    snprintf(str, 80, "%d \"%s\" %d\n", idx, pci->long_name, pci->sid);
    MsgQueuePut(q_out, (void*) str);

    return 0;
}

int ChTabStop(struct MsgQueue* q_out)
{
    char str[80];

    snprintf(str, 80, "OK\n");
    MsgQueuePut(q_out, (void*) str);

    return 0;
}

int ChTabInfo(struct MsgQueue* q_out)
{
    char str[80];
    struct ChannelInfo* pci;

    pci = pchannel_tab[cur_ch_idx-1];
    snprintf(str, 80, "Service-name: %s\n", pci->long_name);
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Program-type: %s\n", "news");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Multiplex-name: %s\n", "BBC");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Frequency: %s\n", "174928 kHz");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Bitrate: %s\n", "128 kbps");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Time: %s\n", "HH:MM:SS");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Date: %s\n", "dd-mm-yy");
    MsgQueuePut(q_out, (void*) str);

    MsgQueuePut(q_out, (void*) "OK\n");

    return 0;
}

int ChTabStatus(struct MsgQueue* q_out)
{
    char str[80];
    struct ChannelInfo* pci;    // channel info

    pci = pchannel_tab[cur_ch_idx-1];

    snprintf(str, 80, "DLS: %s\n", "yes");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "SLS: %s\n", "yes");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Announcement: %s\n", "none");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Announcement: %s\n", "traffic in idx");
    MsgQueuePut(q_out, (void*) str);

    MsgQueuePut(q_out, (void*) "OK\n");

    return 0;
}

int PresetInit(void)
{
    struct ChannelInfo* ppi;    // preset channel info.
    FILE* ifp;
    int leng;

    ifp = fopen("preset.txt", "r");

    ppi = &preset_tab[0];
    while (fgets(ppi->long_name, 20, ifp) != NULL)
    {
        leng = strlen(ppi->long_name);
        ppi->long_name[leng-1] = '\0';    // remove \n
        ppi++;
    }

    fclose(ifp);

    return 0;
}

int PresetList(struct MsgQueue* q_out)
{
    int ii;
    struct ChannelInfo* ppi;    // preset channel info.
    char str[80];

    for (ii=0; ii<PRESET_MAX; )
    {
        ppi = &preset_tab[ii++];
        if (ppi->long_name[0] == '\0')
            snprintf(str, 80, "P%d: <empty>\n", ii);
        else
            snprintf(str, 80, "P%d: \"%s\" %d\n", ii, ppi->long_name, ppi->sid);
        MsgQueuePut(q_out, (void*) str);
    }

    MsgQueuePut(q_out, (void*) "OK\n");
    return 0;
}

int PresetStore(struct MsgQueue* q_out, char* pslot)
{
    int num;
    char str[80];
    struct ChannelInfo* pci;    // current channel info.
    struct ChannelInfo* ppi;    // preset channel info.

    num = atoi(pslot+1);
    if (num > 0 && num <= PRESET_MAX)
    {
        ppi = &preset_tab[num-1];
        pci = pchannel_tab[cur_ch_idx-1];

        ppi->sid = pci->sid;
        strncpy(ppi->long_name, pci->long_name, 20);

        snprintf(str, 80, "store to P%d\n", num);
        MsgQueuePut(q_out, (void*) str);
    }
    else
    {
        ErrorOutput(q_out, "slot number is invalid");
    }

    return 0;
}

int PresetRecall(struct MsgQueue* q_out, char* pslot)
{
    int num;
    int idx;
    char str[80];
    struct ChannelInfo* ppi;

    num = atoi(pslot+1);
    if (num > 0 && num <= PRESET_MAX)
    {
        ppi = &preset_tab[num-1];
        if (ppi->long_name != NULL)
        {
            idx = ChTabFind(ppi);
            cur_ch_idx = idx;

            snprintf(str, 80, "recall P%d\n", num);
            MsgQueuePut(q_out, (void*) str);
        }
        else
        {
            snprintf(str, 80, "P%d is empty\n", num);
            MsgQueuePut(q_out, (void*) str);
        }

    }
    else
    {
        ErrorOutput(q_out, "slot number is invalid");
    }

    return 0;
}

int FavorInit(void)
{
    struct ChannelInfo* pci;
    FILE* ifp;
    int leng;

    ifp = fopen("favor.txt", "r");

    pci = &favor_tab[0];
    while (fgets(pci->long_name, 20, ifp) != NULL)
    {
        leng = strlen(pci->long_name);
        pci->long_name[leng-1] = '\0';    // remove \n
        pci++;
    }

    fclose(ifp);

    return 0;
}

int FavorList(struct MsgQueue* q_out)
{
    int ii;
    struct ChannelInfo* pci;
    char str[80];

    for (ii=0; ii<FAVOR_MAX; )
    {
        pci = &favor_tab[ii++];
        if (pci->long_name[0] == '\0')
            snprintf(str, 80, "F%d: <empty>\n", ii);
        else
            snprintf(str, 80, "F%d: \"%s\" %d\n", ii, pci->long_name, pci->sid);
        MsgQueuePut(q_out, (void*) str);
    }

    MsgQueuePut(q_out, (void*) "OK\n");
    return 0;
}

int FavorStore(struct MsgQueue* q_out, char* fslot)
{
    int num;
    char str[80];
    struct ChannelInfo* pci;
    struct ChannelInfo* pfi;

    num = atoi(fslot+1);
    if (num > 0 && num <= PRESET_MAX)
    {
        pfi = &favor_tab[num-1];
        pci = pchannel_tab[cur_ch_idx-1];

        pfi->sid = pci->sid;
        strncpy(pfi->long_name, pci->long_name, 20);

        snprintf(str, 80, "store to F%d\n", num);
        MsgQueuePut(q_out, (void*) str);
    }
    else
    {
        ErrorOutput(q_out, "slot number is invalid");
    }

    return 0;
}

int FavorRecall(struct MsgQueue* q_out, char* fslot)
{
    int num;
    int idx;
    char str[80];
    struct ChannelInfo* pci;

    num = atoi(fslot+1);
    if (num > 0 && num <= FAVOR_MAX)
    {
        pci = &favor_tab[num-1];
        if (pci->long_name != NULL)
        {
            idx = ChTabFind(pci);
            cur_ch_idx = idx;

            snprintf(str, 80, "recall F%d\n", num);
            MsgQueuePut(q_out, (void*) str);
        }
        else
        {
            snprintf(str, 80, "F%d is empty\n", num);
            MsgQueuePut(q_out, (void*) str);
        }

    }
    else
    {
        ErrorOutput(q_out, "slot number is invalid");
    }

    return 0;
}

int QosOutput(struct MsgQueue* q_out)
{
    char str[80];

    snprintf(str, 80, "%d\n", cur_qos);
    MsgQueuePut(q_out, (void*) str);
    return 0;
}

