// $Id$
//

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "msg_queue.h"
#include "io_handler.h"
#include "cmd_parser.h"

#define DEBUG 1
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, ##__VA_ARGS__); } while (0)

pthread_t threads[5];

int main(void)
{
    int i;
    int rc;
    int status;

    struct MsgQueue* q_in;
    struct MsgQueue* q_out;

    q_in = MsgQueueCreate(1, 3, 80);
    q_out = MsgQueueCreate(2, 3, 80);

    pthread_create(&threads[0], NULL, &InputHandler, (void *)q_in);
    pthread_create(&threads[1], NULL, &OutputHandler, (void *)q_out);

    CmdParser(q_in, q_out);

    for (i = 1; i >= 0; i--)
    {
        rc = pthread_join(threads[i], (void **)&status);
        if (rc == 0)
        {
            printf("Completed join with thread %d status= %d\n",i, status);
        }
        else
        {
            printf("ERROR; return code from pthread_join() is %d, thread %d\n", rc, i);
            return -1;
        }
    }

    MsgQueueDelete(q_in);
    MsgQueueDelete(q_out);

    return 0;
}

