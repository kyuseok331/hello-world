// $Id$
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "msg_queue.h"

#include <wiringPi.h>
#include <wiringSerial.h>

#define DEBUG 0
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

#define CMD_INPUT_BUFSIZE 80

#define EOT 4
#define BS 127
#define LF '\n' // 0x0A
#define CR '\r' // 0x0D

int fd_uart;
pthread_mutex_t uart_mutex; // mutex for access control between threads
char cmd_input_buf[CMD_INPUT_BUFSIZE];

void *InputHandler(void *arg)
{
    int ch, wp;
    struct MsgQueue* q_in;

    if ((fd_uart = serialOpen ("/dev/serial0", 115200)) < 0)
    {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
        pthread_exit((void *) 1);
    }

    if (wiringPiSetup () == -1)
    {
        fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
        pthread_exit((void *) 1);
    }

    pthread_mutex_init(&uart_mutex, NULL);

    q_in = (struct MsgQueue*) arg;

    wp = 0;

    // while ((ch=getch()) != EOF)
    while (1)
    {
        pthread_mutex_lock(&uart_mutex);
        while (serialDataAvail(fd_uart))
        {
            ch = serialGetchar(fd_uart);
            if (ch == EOT) // 
                break;
            if (ch == CR)
            {
                serialPutchar(fd_uart, ch);
                // skip this char.
            }
            else if (ch == LF)
            {
                //serialPutchar(fd_uart, ch);
                serialPutchar(fd_uart, ch);
                cmd_input_buf[wp++] = ch;
                cmd_input_buf[wp] = '\0';
                // {
                //     int i;
                //     char str[80], str2[4];
                //     memset(str, 0, 80);
                //     for (i=0; i<wp; i++)
                //     {
                //         sprintf(str2, " %02x", cmd_input_buf[i]);
                //         strcat(str, str2);
                //     }
                //     DEBUG_PRINT("put cmd_input_buf=%s, cmd_input_buf=\"%s\"\n", str, cmd_input_buf);
                // }
                MsgQueuePut(q_in, (void*) cmd_input_buf);
                wp = 0;
            }
            else if (ch == BS || ch == 8)
            {
                wp--;
                if (wp < 0)
                    wp = 0;
                else
                    serialPutchar(fd_uart, 8); // 
            }
            else
            {
                serialPutchar(fd_uart, ch);
                cmd_input_buf[wp++] = ch;
            }
        }
        pthread_mutex_unlock(&uart_mutex);
        delay(3);   // msec
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
        DEBUG_PRINT("waiting queue\n");
        MsgQueueGet(q_out, str);
        if (strncmp(str, "exit", 4) == 0)
            break;
        DEBUG_PRINT("get %d: \"%s\"\n", ++i, str);
        serialPuts(fd_uart, str);
    }

    DEBUG_PRINT("exit\n");
    pthread_exit((void *) 0);
}
