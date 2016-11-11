// $Id$
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "msg_queue.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

#define CMD_INPUT_BUFSIZE 80

#define EOT 4
#define BS 127
#define CR '\r'
#define LF '\n'

char cmd_input_buf[CMD_INPUT_BUFSIZE];

void *InputHandler(void *arg)
{
    int ch, wp;
    struct MsgQueue* q_in;

    q_in = (struct MsgQueue*) arg;

    wp = 0;

    while ((ch=getch()) != EOF)
    {
        if (ch == EOT) // 
            break;
        if (ch == LF)
        {
            putchar(ch);
            cmd_input_buf[wp++] = ch;
            cmd_input_buf[wp] = '\0';
            {
                int i;
                char str[80], str2[4];
                memset(str, 0, 80);
                for (i=0; i<wp; i++)
                {
                    sprintf(str2, " %02x", cmd_input_buf[i]);
                    strcat(str, str2);
                }
                DEBUG_PRINT("put cmd_input_buf=%s, cmd_input_buf=\"%s\"\n", str, cmd_input_buf);
            }
            MsgQueuePut(q_in, (void*) cmd_input_buf);
            wp = 0;
        }
        else if (ch == BS || ch == 8)
        {
            putchar(8); // 
            wp--;
            if (wp < 0) wp = 0;
        }
        else
        {
            putchar(ch);
            cmd_input_buf[wp++] = ch;
        }
    }

    MsgQueuePut(q_in, (void*) "exit\n");
    DEBUG_PRINT("exit %d\n", ch);
    pthread_exit((void *) 0);
}

void *OutputHandler(void *arg)
{
    int i;
    char str[80];
    struct MsgQueue* q_out;

    q_out = (struct MsgQueue*) arg;

    i = 0;

    while (1)
    {
        MsgQueueGet(q_out, str);
        DEBUG_PRINT("get %d: \"%s\"\n", ++i, str);
        if (strncmp(str, "exit", 4) == 0)
            break;
    }

    DEBUG_PRINT("exit\n");
    pthread_exit((void *) 0);
}
