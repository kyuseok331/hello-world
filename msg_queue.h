// $Id$
//

#ifndef __MSG_QUEUE_H_
#define __MSG_QUEUE_H_

#include <pthread.h>

#define CD_TRUE     (1)
#define CD_FALSE    (0)
#define CD_ERROR    (-1)
#define CD_SUCCESS  (0)

struct MsgQueue
{
    int m_id;          // unique number for debug tracking
    int m_len;         // queue length (recommend 3)
    int m_width;       // data width

    void **m_pp_data;  // array to contains messages
    
    // read index points out the place to read when queue get
    // write index points out the place to write when queue put
    // If read index is same as write index, this means queue empty or full
    // So, is_empty flag is used to distinguish between queue empty and full
    int m_write_index; // index position for queue put
    int m_read_index;  // index position for queue get
    int m_is_empty;

    // condition variable to wait in put method when queue is full 
    pthread_cond_t m_put_cond;   
    // condition variable to wait int get method when queue is empty
    pthread_cond_t m_get_cond;   
    pthread_mutex_t m_mutex; // mutex for access control between threads
};

// prepares the resource and initialize member variables
struct MsgQueue* MsgQueueCreate(int id, int len, int width);

// releases the resource
int MsgQueueDelete(struct MsgQueue* mq); 

//returns whether siso queue is full or not (CD_TRUE, CD_FALSE).
int MsgQueueIsFull(struct MsgQueue* mq);

// sends the message into queue
//   if queue is full, this will wait until queue is available
int MsgQueuePut(struct MsgQueue* mq, void * p_data);

// receives the message from queue
//   if queue is empty, this will wait until the message comes
void * MsgQueueGet(struct MsgQueue* mq, void* p_data);

#endif // __MSG_QUEUE_H_
