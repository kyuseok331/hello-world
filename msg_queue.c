// $Id$
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msg_queue.h"

struct MsgQueue* MsgQueueCreate(int id, int len, int width)
{
    int i;
    struct MsgQueue* mq;
    mq = (struct MsgQueue*) malloc(sizeof(struct MsgQueue));
    mq->m_id = id;
    mq->m_len = len;
    mq->m_width = width;
    mq->m_write_index = 0;
    mq->m_read_index = 0;
    mq->m_is_empty = CD_TRUE;
    mq->m_pp_data = (void **) malloc(sizeof(void *) * mq->m_len);

    for (i=0; i < mq->m_len; i++)
    {
        mq->m_pp_data[i] = (void*) malloc(mq->m_width);
    }

    pthread_cond_init(&mq->m_put_cond, NULL);
    pthread_cond_init(&mq->m_get_cond, NULL);
    pthread_mutex_init(&mq->m_mutex, NULL);

    return mq;
}

int MsgQueueDelete(struct MsgQueue* mq)
{
    int i;
    for (i=0; i < mq->m_len; i++)
    {
        free(mq->m_pp_data[i]);
    }

    free(mq->m_pp_data);
    free(mq);

    pthread_cond_destroy(&mq->m_put_cond);
    pthread_cond_destroy(&mq->m_get_cond);
    pthread_mutex_destroy(&mq->m_mutex);

    return CD_SUCCESS;
}

int MsgQueuePut(struct MsgQueue* mq, void * p_data)
{
    pthread_mutex_lock(&mq->m_mutex);

    if( mq->m_is_empty )
    {
        // mq->m_pp_data[mq->m_write_index] = p_data;
        strncpy(mq->m_pp_data[mq->m_write_index], p_data, mq->m_width);
        mq->m_write_index = (mq->m_write_index+1) % mq->m_len; // update write_index
        mq->m_is_empty = CD_FALSE;
        pthread_cond_signal(&mq->m_get_cond);
    }
    else
    {
        if( mq->m_write_index == mq->m_read_index ) // if queue is full
        {
            pthread_cond_wait(&mq->m_put_cond, &mq->m_mutex);

            // if queue is still full, show error message
            if( !mq->m_is_empty && (mq->m_write_index==mq->m_read_index) )
            {
                printf("ERROR: MsgQueue(id:%d) put error\n", mq->m_id);
                pthread_mutex_unlock(&mq->m_mutex);
                return CD_ERROR;
            }
        }

        // mq->m_pp_data[mq->m_write_index] = p_data;
        strncpy(mq->m_pp_data[mq->m_write_index], p_data, mq->m_width);
        mq->m_write_index = (mq->m_write_index+1) % mq->m_len; // update write_index
        mq->m_is_empty = CD_FALSE;
    }

    pthread_mutex_unlock(&mq->m_mutex);
    return CD_SUCCESS;
}

void * MsgQueueGet(struct MsgQueue* mq, void* p_data)
{
    pthread_mutex_lock(&mq->m_mutex);

    if( mq->m_is_empty )
    {
        pthread_cond_wait(&mq->m_get_cond, &mq->m_mutex);
        if( mq->m_is_empty )
        {
            printf("ERROR: MsgQueue(id:%d) get error\n", mq->m_id);
            pthread_mutex_unlock(&mq->m_mutex);
            return NULL;
        }
    }

    // p_data = mq->m_pp_data[mq->m_read_index];
    strncpy(p_data, mq->m_pp_data[mq->m_read_index], mq->m_width);
    // mq->m_pp_data[mq->m_read_index] = NULL;
    mq->m_read_index = (mq->m_read_index+1) % mq->m_len; // update read_index

    if( mq->m_write_index == mq->m_read_index ) // if queue is empty
        mq->m_is_empty = CD_TRUE;

    pthread_cond_signal(&mq->m_put_cond);
    pthread_mutex_unlock(&mq->m_mutex);
    return p_data;
}

int MsgQueueIsFull(struct MsgQueue* mq)
{
    int rv;
    
    pthread_mutex_lock(&mq->m_mutex);
    rv = CD_FALSE;
    if( !mq->m_is_empty && (mq->m_write_index==mq->m_read_index) ) // if queue is full
    {
        rv = CD_TRUE;
    }
    pthread_mutex_unlock(&mq->m_mutex);
    return rv;
}

