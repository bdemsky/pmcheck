/**
 * Copyright (C) 2017 Cisco Inc.
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

#ifndef __ASYNC_WRITER_H__
#define __ASYNC_WRITER_H__

#include <pthread.h>

#include "db.h"
//#include "mb_rc.h"
#include "dict.h"
#include "mb_backup.h"

namespace mabain {

#define MABAIN_ASYNC_TYPE_NONE       0
#define MABAIN_ASYNC_TYPE_ADD        1
#define MABAIN_ASYNC_TYPE_REMOVE     2
#define MABAIN_ASYNC_TYPE_REMOVE_ALL 3
#define MABAIN_ASYNC_TYPE_RC         4
#define MABAIN_ASYNC_TYPE_BACKUP     5

#define MB_ASYNC_SHM_KEY_SIZE      256
#define MB_ASYNC_SHM_DATA_SIZE     1024
#define MB_ASYNC_SHM_LOCK_TMOUT    5

    
typedef struct _AsyncNode
{
    std::atomic<bool> in_use;
    pthread_mutex_t   mutex;
    pthread_cond_t    cond;

#ifdef __SHM_QUEUE__
    char key[MB_ASYNC_SHM_KEY_SIZE];
    char data[MB_ASYNC_SHM_DATA_SIZE];
#else
    char *key;
    char *data;
#endif
    int key_len;
    int data_len;
    bool overwrite;
    char type;
} AsyncNode;

class AsyncWriter
{
public:

    AsyncWriter(DB *db_ptr);
    ~AsyncWriter();

#ifndef __SHM_QUEUE__
    void UpdateNumUsers(int delta);
    int  Add(const char *key, int key_len, const char *data, int data_len, bool overwrite);
    int  Remove(const char *key, int len);
    int  RemoveAll();
    int  Backup(const char *backup_dir);
    int  CollectResource(int64_t m_index_rc_size, int64_t m_data_rc_size, 
                         int64_t max_dbsz, int64_t max_dbcnt);
    bool Busy() const;
#endif

    int  StopAsyncThread();
    int  ProcessTask(int ntasks, bool rc_mode);

private:
    static void *async_thread_wrapper(void *context);
    AsyncNode* AcquireSlot();
    int PrepareSlot(AsyncNode *node_ptr) const;
    void* async_writer_thread();
#ifdef __SHM_QUEUE__
    uint32_t NextShmSlot(uint32_t windex, uint32_t qindex);
#endif

    // db pointer
    DB *db;
    Dict *dict;

    // thread id
    pthread_t tid;
    bool stop_processing;

    AsyncNode *queue;
#ifdef __SHM_QUEUE__
    IndexHeader *header;
#else
    std::atomic<int> num_users;
    std::atomic<uint32_t> queue_index;
    uint32_t writer_index;
#endif

    bool is_rc_running;
    char *rc_backup_dir;
};

}

#endif
