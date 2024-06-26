/**
 * Copyright (C) 2018 Cisco Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// @author Changxue Deng <chadeng@cisco.com>

#include <unistd.h>
#include <string.h>

#include "async_writer.h"
#include "error.h"
#include "logger.h"
#include "mb_data.h"
#include "mb_rc.h"
#include "integer_4b_5b.h"
#include "dict.h"
#include "./util/shm_mutex.h"

namespace mabain {

static void free_async_node(AsyncNode *node_ptr)
{
#ifndef __SHM_QUEUE__
    if(node_ptr->key != NULL)
    {
        free(node_ptr->key);
        node_ptr->key = NULL;
        node_ptr->key_len = 0;
    }

    if(node_ptr->data != NULL)
    {
        free(node_ptr->data);
        node_ptr->data = NULL;
        node_ptr->data_len = 0;
    }
#endif

    node_ptr->type = MABAIN_ASYNC_TYPE_NONE;
}

AsyncWriter::AsyncWriter(DB *db_ptr)
                       : db(db_ptr),
                         tid(0),
                         stop_processing(false),
                         queue(NULL),
#ifdef __SHM_QUEUE__
                         header(NULL)
#else
                         num_users(0),
                         queue_index(0),
                         writer_index(0)
#endif
{
    dict = NULL;
    if(!(db_ptr->GetDBOptions() & CONSTS::ACCESS_MODE_WRITER))
        throw (int) MBError::NOT_ALLOWED;
    if(db == NULL)
        throw (int) MBError::INVALID_ARG;
    dict = db->GetDictPtr();
    if(dict == NULL)
        throw (int) MBError::NOT_INITIALIZED;

#ifdef __SHM_QUEUE__
    // initialize shared memory queue pointer
    header = dict->GetHeaderPtr();
    if(header == NULL)
        throw (int) MBError::NOT_INITIALIZED;
    char *hdr_ptr = (char *) header;
    queue = reinterpret_cast<AsyncNode *>(hdr_ptr + RollableFile::page_size);
    header->rc_flag.store(0, std::memory_order_release);
#else
    queue = new AsyncNode[MB_MAX_NUM_SHM_QUEUE_NODE];
    memset(queue, 0, MB_MAX_NUM_SHM_QUEUE_NODE * sizeof(AsyncNode));
    for(int i = 0; i < MB_MAX_NUM_SHM_QUEUE_NODE; i++)
    {
        queue[i].in_use.store(false, std::memory_order_release);
        if(pthread_mutex_init(&queue[i].mutex, NULL) != 0)
        {
            Logger::Log(LOG_LEVEL_ERROR, "failed to init mutex");
            throw (int) MBError::MUTEX_ERROR;
        }
        if(pthread_cond_init(&queue[i].cond, NULL) != 0)
        {
            Logger::Log(LOG_LEVEL_ERROR, "failed to init conditional variable");
            throw (int) MBError::MUTEX_ERROR;
        }
    }
    is_rc_running = false;
#endif

    rc_backup_dir = NULL;
    // start the thread
    if(pthread_create(&tid, NULL, async_thread_wrapper, this) != 0)
    {
        Logger::Log(LOG_LEVEL_ERROR, "failed to create async thread");
        tid = 0;
        throw (int) MBError::THREAD_FAILED;
    }

}

AsyncWriter::~AsyncWriter()
{
}

#ifndef __SHM_QUEUE__
void AsyncWriter::UpdateNumUsers(int delta)
{
    if(delta > 0)
        num_users.fetch_add(1, std::memory_order_release);
    else if(delta < 0)
        num_users.fetch_sub(1, std::memory_order_release);
}
#endif

int AsyncWriter::StopAsyncThread()
{
#ifndef __SHM_QUEUE__
    if(num_users.load(std::memory_order_consume) > 0)
    {
        Logger::Log(LOG_LEVEL_ERROR, "still being used, cannot shutdown async thread");
    }
#endif

    stop_processing = true;

#ifndef __SHM_QUEUE__
    for(int i = 0; i < MB_MAX_NUM_SHM_QUEUE_NODE; i++)
    {
        pthread_cond_signal(&queue[i].cond);
    }
#endif

    if(tid != 0)
    {
        Logger::Log(LOG_LEVEL_INFO, "joining async writer thread");
        pthread_join(tid, NULL);
    }

#ifndef __SHM_QUEUE__
    for(int i = 0; i < MB_MAX_NUM_SHM_QUEUE_NODE; i++)
    {
        pthread_mutex_destroy(&queue[i].mutex);
        pthread_cond_destroy(&queue[i].cond);
    }

    if(queue != NULL)
        delete [] queue;
#endif

    return MBError::SUCCESS;
}

#ifndef __SHM_QUEUE__
// Check if async tasks are completed.
bool AsyncWriter::Busy() const
{
    uint32_t index = queue_index.load(std::memory_order_consume);
    return index != writer_index || is_rc_running;
}
#endif

#ifndef __SHM_QUEUE__
AsyncNode* AsyncWriter::AcquireSlot()
{
    uint32_t index = queue_index.fetch_add(1, std::memory_order_release);
    AsyncNode *node_ptr = queue + (index % MB_MAX_NUM_SHM_QUEUE_NODE);

    if(pthread_mutex_lock(&node_ptr->mutex) != 0)
    {
        Logger::Log(LOG_LEVEL_ERROR, "failed to lock mutex");
        return NULL;
    }

    while(node_ptr->in_use.load(std::memory_order_consume))
    {
        pthread_cond_wait(&node_ptr->cond, &node_ptr->mutex);
    }

    return node_ptr;
}
#endif

#ifndef __SHM_QUEUE__
int AsyncWriter::PrepareSlot(AsyncNode *node_ptr) const
{
    node_ptr->in_use.store(true, std::memory_order_release);
    pthread_cond_signal(&node_ptr->cond);
    if(pthread_mutex_unlock(&node_ptr->mutex) != 0)
        return MBError::MUTEX_ERROR;
    return MBError::SUCCESS;
}
#endif

#ifndef __SHM_QUEUE__
int AsyncWriter::Add(const char *key, int key_len, const char *data,
                     int data_len, bool overwrite)
{
    if(stop_processing)
        return MBError::DB_CLOSED;

    AsyncNode *node_ptr = AcquireSlot();
    if(node_ptr == NULL)
        return MBError::MUTEX_ERROR;

    node_ptr->key = (char *) malloc(key_len);
    node_ptr->data = (char *) malloc(data_len);
    if(node_ptr->key == NULL || node_ptr->data == NULL)
    {
        pthread_mutex_unlock(&node_ptr->mutex);
        free_async_node(node_ptr);
        return MBError::NO_MEMORY;
    }
    memcpy(node_ptr->key, key, key_len);
    memcpy(node_ptr->data, data, data_len);
    node_ptr->key_len = key_len;
    node_ptr->data_len = data_len;
    node_ptr->overwrite = overwrite;

    node_ptr->type = MABAIN_ASYNC_TYPE_ADD;


    return PrepareSlot(node_ptr);
}
#endif

#ifndef __SHM_QUEUE__
int AsyncWriter::Remove(const char *key, int len)
{
    if(stop_processing)
        return MBError::DB_CLOSED;

    AsyncNode *node_ptr = AcquireSlot();
    if(node_ptr == NULL)
        return MBError::MUTEX_ERROR;

    node_ptr->key = (char *) malloc(len);
    if(node_ptr->key == NULL)
    {
        pthread_mutex_unlock(&node_ptr->mutex);
        free_async_node(node_ptr);
        return MBError::NO_MEMORY;
    }
    memcpy(node_ptr->key, key, len);
    node_ptr->key_len = len;
    node_ptr->type = MABAIN_ASYNC_TYPE_REMOVE;

    return PrepareSlot(node_ptr);
}
#endif

#ifndef __SHM_QUEUE__
int AsyncWriter::Backup(const char *backup_dir)
{
    if(backup_dir == NULL)
        return MBError::INVALID_ARG;

    if(stop_processing)
        return MBError::DB_CLOSED;

    AsyncNode *node_ptr = AcquireSlot();
    if(node_ptr == NULL)
        return MBError::MUTEX_ERROR;

    node_ptr->data = (char *) strdup(backup_dir);
    if(node_ptr->data == NULL)
    {
        pthread_mutex_unlock(&node_ptr->mutex);
        free_async_node(node_ptr);
        return MBError::NO_MEMORY;
    }
    node_ptr->type = MABAIN_ASYNC_TYPE_BACKUP;
    return PrepareSlot(node_ptr);
}
#endif

#ifndef __SHM_QUEUE__
int AsyncWriter::RemoveAll()
{
    if(stop_processing)
        return MBError::DB_CLOSED;

    AsyncNode *node_ptr = AcquireSlot();
    if(node_ptr == NULL)
        return MBError::MUTEX_ERROR;

    node_ptr->type = MABAIN_ASYNC_TYPE_REMOVE_ALL;

    return PrepareSlot(node_ptr);
}
#endif

#ifndef __SHM_QUEUE__
int  AsyncWriter::CollectResource(int64_t m_index_rc_size, int64_t m_data_rc_size,
                                  int64_t max_dbsz, int64_t max_dbcnt)
{
    if(stop_processing)
        return MBError::DB_CLOSED;

    AsyncNode *node_ptr = AcquireSlot();
    if(node_ptr == NULL)
        return MBError::MUTEX_ERROR;

    int64_t *data_ptr = (int64_t *) calloc(4, sizeof(int64_t));
    if (data_ptr == NULL)
        return MBError::NO_MEMORY;

    node_ptr->data = (char *) data_ptr;
    node_ptr->data_len = sizeof(int64_t)*4;
    data_ptr[0] = m_index_rc_size;
    data_ptr[1] = m_data_rc_size;
    data_ptr[2] = max_dbsz;
    data_ptr[3] = max_dbcnt;
    node_ptr->type = MABAIN_ASYNC_TYPE_RC;

    return PrepareSlot(node_ptr);
}
#endif

// Run a given number of tasks if they are available.
// This function should only be called by rc or pruner.
int AsyncWriter::ProcessTask(int ntasks, bool rc_mode)
{
    AsyncNode *node_ptr;
    MBData mbd;
    int rval = MBError::SUCCESS;
    int count = 0;
#ifdef __SHM_QUEUE__
    struct timespec tm_exp;
#endif

    while(count < ntasks)
    {
#ifdef __SHM_QUEUE__
        node_ptr = &queue[header->writer_index % header->async_queue_size];
        tm_exp.tv_sec = time(NULL) + MB_ASYNC_SHM_LOCK_TMOUT;
        tm_exp.tv_nsec = 0;
        rval = pthread_mutex_timedlock(&node_ptr->mutex, &tm_exp);
        if(rval == ETIMEDOUT)
        {
            Logger::Log(LOG_LEVEL_WARN, "mutex lock timeout, need to re-intialize");
            InitShmMutex(&node_ptr->mutex);
            count = ntasks;
            continue;
        }
        else if(rval != 0)
#else
        node_ptr = &queue[writer_index % MB_MAX_NUM_SHM_QUEUE_NODE];
        if(pthread_mutex_lock(&node_ptr->mutex) != 0)
#endif
        {
            Logger::Log(LOG_LEVEL_ERROR, "failed to lock mutex");
            throw (int) MBError::MUTEX_ERROR;
        }

        if(node_ptr->in_use.load(std::memory_order_consume))
        {
            switch(node_ptr->type)
            {
                case MABAIN_ASYNC_TYPE_ADD:
                    if(rc_mode)
                        mbd.options = CONSTS::OPTION_RC_MODE;
                    mbd.buff = (uint8_t *) node_ptr->data;
                    mbd.data_len = node_ptr->data_len;
                    try {
                        rval = dict->Add((uint8_t *)node_ptr->key, node_ptr->key_len, mbd, node_ptr->overwrite);
                    } catch (int err) {
                        rval = err;
                        Logger::Log(LOG_LEVEL_ERROR, "dict->Add throws error %s",
                                MBError::get_error_str(err));
                    }
                    break;
                case MABAIN_ASYNC_TYPE_REMOVE:
                    // FIXME
                    // Removing entries during rc is currently not supported.
                    // The index or data index could have been reset in fucntion ResourceColletion::Finish.
                    // However, the deletion may be run for some entry which still exist in the rc root tree.
                    // This requires modifying buffer in high end, where the offset for writing is greather than
                    // header->m_index_offset. This causes exception thrown from DictMem::WriteData.
                    // Note this is not a problem for Dict::Add since Add does not modify buffers in high end.
                    // This problem will be fixed when resolving issue: https://github.com/chxdeng/mabain/issues/21
                    rval = MBError::SUCCESS;
                    break;
                case MABAIN_ASYNC_TYPE_REMOVE_ALL:
                    if(!rc_mode)
                    {
                        try {
                            rval = dict->RemoveAll();
                        } catch (int err) {
                            Logger::Log(LOG_LEVEL_ERROR, "dict->Add throws error %s",
                                        MBError::get_error_str(err));
                            rval = err;
                        }
                    }
                    else
                    {
                        rval = MBError::SUCCESS;
                    }
                    break;
                case MABAIN_ASYNC_TYPE_RC:
                    // ignore rc task since it is running already.
                    rval = MBError::RC_SKIPPED;
                    break;
                case MABAIN_ASYNC_TYPE_NONE:
                    rval = MBError::SUCCESS;
                    break;
                case MABAIN_ASYNC_TYPE_BACKUP:
                    // clean up existing backup dir varibale buffer.
                    if (rc_backup_dir != NULL)
                        free(rc_backup_dir);
#ifdef __SHM_QUEUE__
                    rc_backup_dir = (char *) malloc(node_ptr->data_len+1);
                    memcpy(rc_backup_dir, node_ptr->data, node_ptr->data_len);
                    rc_backup_dir[node_ptr->data_len] = '\0';
#else
                    rc_backup_dir = (char *) node_ptr->data;
                    node_ptr->data = NULL;
#endif
                    rval = MBError::SUCCESS;
                    break;
                default:
                    rval = MBError::INVALID_ARG;
                    break;
            }

#ifdef __SHM_QUEUE__
            header->writer_index++;
#else
            writer_index++;
#endif
            free_async_node(node_ptr);
            node_ptr->in_use.store(false, std::memory_order_release);
            pthread_cond_signal(&node_ptr->cond);
            mbd.Clear();
            count++;
        }
        else
        {
            // done processing
            count = ntasks;
        }

        if(pthread_mutex_unlock(&node_ptr->mutex) != 0)
        {
            Logger::Log(LOG_LEVEL_ERROR, "failed to unlock mutex");
            throw (int) MBError::MUTEX_ERROR;
        }

        if(rval != MBError::SUCCESS)
        {
            Logger::Log(LOG_LEVEL_DEBUG, "failed to run update %d: %s",
                        (int)node_ptr->type, MBError::get_error_str(rval));
        }
    }

    if(stop_processing)
        return MBError::RC_SKIPPED;
    return MBError::SUCCESS;
}

#ifdef __SHM_QUEUE__
uint32_t AsyncWriter::NextShmSlot(uint32_t windex, uint32_t qindex)
{
    int cnt = 0;
    while(windex != qindex)
    {
        if(queue[windex % header->async_queue_size].in_use.load(std::memory_order_consume))
            break;
        if(++cnt > header->async_queue_size)
        {
            windex = qindex;
            break;
        }

        windex++;
    }

    return windex;
}
#endif

void* AsyncWriter::async_writer_thread()
{
    AsyncNode *node_ptr;
    MBData mbd;
    int rval;
    int64_t min_index_size = 0;
    int64_t min_data_size = 0;
    int64_t max_dbsize = MAX_6B_OFFSET;
    int64_t max_dbcount = MAX_6B_OFFSET;
#ifdef __SHM_QUEUE__
    struct timespec tm_exp;
    bool skip;
#endif

    Logger::Log(LOG_LEVEL_INFO, "async writer started");
    while(true)
    {
#ifdef __SHM_QUEUE__
        node_ptr = &queue[header->writer_index % header->async_queue_size];
        tm_exp.tv_sec = time(NULL) + MB_ASYNC_SHM_LOCK_TMOUT;
        tm_exp.tv_nsec = 0;
        rval = pthread_mutex_timedlock(&node_ptr->mutex, &tm_exp);
        if(rval == ETIMEDOUT)
        {
            Logger::Log(LOG_LEVEL_WARN, "async writer shared memory mutex lock timeout, need to re-intialize");
            InitShmMutex(&node_ptr->mutex);
            header->writer_index++;
            continue;
        }
        else if(rval != 0)
#else
        node_ptr = &queue[writer_index % MB_MAX_NUM_SHM_QUEUE_NODE];
        if(pthread_mutex_lock(&node_ptr->mutex) != 0)
#endif
        {
            Logger::Log(LOG_LEVEL_ERROR, "async writer failed to lock shared memory mutex");
            throw (int) MBError::MUTEX_ERROR;
        }

#ifdef __SHM_QUEUE__
        skip = false;
#endif
        while(!node_ptr->in_use.load(std::memory_order_consume))
        {
            if(stop_processing)
                break;
#ifdef __SHM_QUEUE__
            tm_exp.tv_sec = time(NULL) + MB_ASYNC_SHM_LOCK_TMOUT;
            tm_exp.tv_nsec = 0;
            pthread_cond_timedwait(&node_ptr->cond, &node_ptr->mutex, &tm_exp);

            uint32_t windex = header->writer_index;
            uint32_t qindex = header->queue_index.load(std::memory_order_consume);
            if(windex != qindex)
            {
                // Reader process may have exited unexpectedly. Recover index.
                skip = true;
                header->writer_index = NextShmSlot(windex, qindex);
                break;
            }
            continue;
#else
            pthread_cond_wait(&node_ptr->cond, &node_ptr->mutex);
#endif
        }

#ifdef __SHM_QUEUE__
        if(skip || stop_processing)
        {
            if(pthread_mutex_unlock(&node_ptr->mutex) != 0)
            {
                Logger::Log(LOG_LEVEL_ERROR, "async writer failed to unlock shared memory mutex");
                throw (int) MBError::MUTEX_ERROR;
            }
            if(stop_processing)
                break;
            continue;
        }
#else
        if(stop_processing && !node_ptr->in_use.load(std::memory_order_consume))
        {
            pthread_mutex_unlock(&node_ptr->mutex);
            break;
        }
#endif
        // process the node
        switch(node_ptr->type)
        {
            case MABAIN_ASYNC_TYPE_ADD:
                mbd.buff = (uint8_t *) node_ptr->data;
                mbd.data_len = node_ptr->data_len;
                try {
                    rval = dict->Add((uint8_t *)node_ptr->key, node_ptr->key_len, mbd,
                                     node_ptr->overwrite);
                } catch (int err) {
                    Logger::Log(LOG_LEVEL_ERROR, "dict->Add throws error %s",
                                MBError::get_error_str(err));
                    rval = err;
                }
                break;
            case MABAIN_ASYNC_TYPE_REMOVE:
                mbd.options |= CONSTS::OPTION_FIND_AND_STORE_PARENT;
                try {
                    rval = dict->Remove((uint8_t *)node_ptr->key, node_ptr->key_len, mbd);
                } catch (int err) {
                    Logger::Log(LOG_LEVEL_ERROR, "dict->Remmove throws error %s",
                                MBError::get_error_str(err));
                    rval = err;
                }
                mbd.options &= ~CONSTS::OPTION_FIND_AND_STORE_PARENT;
                break;
            case MABAIN_ASYNC_TYPE_REMOVE_ALL:
                try {
                    rval = dict->RemoveAll();
                } catch (int err) {
                    Logger::Log(LOG_LEVEL_ERROR, "dict->RemoveAll throws error %s",
                                MBError::get_error_str(err));
                    rval = err;
                }
                break;
            case MABAIN_ASYNC_TYPE_RC:
                rval = MBError::SUCCESS;
#ifdef __SHM_QUEUE__
                header->rc_flag.store(1, std::memory_order_release);
#else
                is_rc_running = true;
#endif
                {
                    int64_t *data_ptr = reinterpret_cast<int64_t *>(node_ptr->data);
                    min_index_size = data_ptr[0];
                    min_data_size  = data_ptr[1];
                    max_dbsize = data_ptr[2];
                    max_dbcount = data_ptr[3];
                }
                break;
            case MABAIN_ASYNC_TYPE_NONE:
                rval = MBError::SUCCESS;
                break;
            case MABAIN_ASYNC_TYPE_BACKUP:
                try {
                    DBBackup mbbk(*db);
                    rval = mbbk.Backup((const char*) node_ptr->data);
                } catch (int error) {
                    rval = error;
                }
                    break;
            default:
                rval = MBError::INVALID_ARG;
                break;
        }

#ifdef __SHM_QUEUE__
        header->writer_index++;
#else
        writer_index++;
#endif
        free_async_node(node_ptr);
        node_ptr->in_use.store(false, std::memory_order_release);
        pthread_cond_signal(&node_ptr->cond);
        if(pthread_mutex_unlock(&node_ptr->mutex) != 0)
        {
            Logger::Log(LOG_LEVEL_ERROR, "failed to unlock mutex");
            throw (int) MBError::MUTEX_ERROR;
        }

        if(rval != MBError::SUCCESS)
        {
            Logger::Log(LOG_LEVEL_DEBUG, "failed to run update %d: %s",
                        (int)node_ptr->type, MBError::get_error_str(rval));
        }

        mbd.Clear();

#ifdef __SHM_QUEUE__
        if (header->rc_flag.load(std::memory_order_consume) == 1)
#else
        if(is_rc_running)
#endif
        {
            rval = MBError::SUCCESS;
            try {
                ResourceCollection rc = ResourceCollection(*db);
                rc.ReclaimResource(min_index_size, min_data_size, max_dbsize, max_dbcount, this);
            } catch (int error) {
                if(error != MBError::RC_SKIPPED)
                    Logger::Log(LOG_LEVEL_WARN, "rc failed :%s", MBError::get_error_str(error));
                else
                    rval = error;
            }

#ifdef __SHM_QUEUE__
            header->rc_flag.store(0, std::memory_order_release);
#else
            is_rc_running = false;
#endif
            if(rc_backup_dir != NULL)
            {
                if(rval == MBError::SUCCESS)
                {
#ifdef __SHM_QUEUE__
                    dict->SHMQ_Backup(rc_backup_dir);
#else
                    Backup(rc_backup_dir);
#endif
                }
                free(rc_backup_dir);
                rc_backup_dir = NULL;
            }
        }
    }

    mbd.buff = NULL;
    Logger::Log(LOG_LEVEL_INFO, "async writer exiting");
    return NULL;
}

void* AsyncWriter::async_thread_wrapper(void *context)
{
    AsyncWriter *instance_ptr = static_cast<AsyncWriter *>(context);
    return instance_ptr->async_writer_thread();
}

}
