/*
 *  Functions for parser.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include "msg_queue.h"
#include "channel_db.h"
#include "sz.h"
#include "rz.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

char version_str[32] = "GL3140-2MB-v3.1.0.6519\n";
char prompt_str[32] = "DR> ";
char mode_str[4] = "DAB";
char sort_str[10] = "alpha";
int cur_volume = 64;

char *help_str[] = {
    "help",
    "version",
    "volume [+-]<number>",
    "time",
    "prompt [\"<string>\"]",
    "mode [DAB|FM]",
    "reset",
    "scan",
    "tune <freq.>",
    "sort [alpha|ensemble]",
    "list [audio|data|all|preset|favorite]",
    "search \"<string>\"",
    "store <preset idx>|<favorite idx>",
    "play <idx>|<preset idx>|<favorite idx>|last",
    "prev",
    "next",
    "stop",
    "info",
    "signal",
    "status",
    "sz <filename>",
    "rz",
    ""
};

int HelpOutput(struct MsgQueue* q_out)
{
    char str[80];
    char **pp;

    for (pp=help_str; **pp; ++pp)
    {
        snprintf(str, 80, "%s\n", *pp);
        MsgQueuePut(q_out, (void*) str);
    }
    return 0;
}


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
        // usleep(20000);
        MsgQueuePut(q_out, (void*) str);
        pcf++;
    }

    {
        int i;
        for (i=0; i<200; i++)
            MsgQueuePut(q_out, (void*) str);
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

int StatusOutput(struct MsgQueue* q_out)
{
    char str[80];
    DIR*           d;
    struct dirent* dir;

    snprintf(str, 80, "DLS: %s\n", "yes");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "SLS: %s\n", "yes");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Announcement: %s\n", "none");
    MsgQueuePut(q_out, (void*) str);

    snprintf(str, 80, "Announcement: %s\n", "traffic in idx");
    MsgQueuePut(q_out, (void*) str);

    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_type == DT_REG)
            {
                struct stat f;
                char buff[20];
                if (stat(dir->d_name, &f) != -1)
                    // strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&f.st_mtime));
                    strftime(buff, 20, "%b %d %H:%M:%S", localtime(&f.st_mtime));
                else
                    buff[0] = '\0';

                snprintf(str, 80, "%s %7d %s\n", buff, f.st_size, dir->d_name);
                MsgQueuePut(q_out, (void*) str);
            }
        }

        closedir(d);
    }

    MsgQueuePut(q_out, (void*) "OK\n");

    return 0;
}

int SzOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) "Usage: sz <filename>\n");
    return 0;
}

extern pthread_mutex_t uart_mutex; // mutex for access control between threads
int SzSend(struct MsgQueue* q_out, char* filename)
{
    // char str[80];
    // snprintf(str, 80, "sz %s\n", filename);
    // MsgQueuePut(q_out, (void*) str);

    char **patts;
    struct stat f;

    if (stat(filename, &f) == -1)
    {
        ErrorOutput(q_out, "file does not exist.");
        return 1;
    }

    patts = &filename;

    pthread_mutex_lock(&uart_mutex);
    serialPuts(fd_uart, "\r\n\bSending in Batch Mode\r\n");

    if (wcsend(1, patts)==ERROR) {
        canit();
    }
    pthread_mutex_unlock(&uart_mutex);

    MsgQueuePut(q_out, "Transfer completed.\n");
    return 0;
}

int RzReceive(struct MsgQueue* q_out)
{
    pthread_mutex_lock(&uart_mutex);
    serialPuts(fd_uart, "\r\n\bReceiving in Batch Mode\r\n");
    if (wcreceive(0, NULL)==ERROR) {
        canit();
        canit();
    }
    pthread_mutex_unlock(&uart_mutex);

    MsgQueuePut(q_out, "Transfer completed.\n");
    return 0;   // (exitcode);
}


int ExitOutput(struct MsgQueue* q_out)
{
    MsgQueuePut(q_out, (void*) "exit");
    return 0;
}


