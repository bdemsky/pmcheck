#include <stdio.h>
#include <atomic>
#include <pthread.h>
#include <assert.h>
// #include <stdatomic.h>

using namespace std;

#define RW_LOCK_BIAS	    0x00100000
#define WRITE_LOCK_CMP	  RW_LOCK_BIAS

/** Example implementation of linux rw lock along with 2 thread test
 *  driver... */

typedef union {
	atomic_int lock;
} rwlock_t;

static inline void read_lock(rwlock_t *rw)
{
	int priorvalue = rw->lock.fetch_sub(1, memory_order_acquire);
	while (priorvalue <= 0) {
		rw->lock.fetch_add(1, memory_order_relaxed);
		while (rw->lock.load(memory_order_relaxed) <= 0) {
			pthread_yield();
		}
		priorvalue = rw->lock.fetch_sub(1, memory_order_acquire);
	}
}

// Injected bug for the two fetch_sub ***
static inline void write_lock(rwlock_t *rw)
{
	int priorvalue = rw->lock.fetch_sub(RW_LOCK_BIAS, memory_order_relaxed);	// Should be acquire
	while (priorvalue != RW_LOCK_BIAS) {
		rw->lock.fetch_add(RW_LOCK_BIAS, memory_order_relaxed);
		while (rw->lock.load(memory_order_relaxed) != RW_LOCK_BIAS) {
			pthread_yield();
		}
		priorvalue = rw->lock.fetch_sub(RW_LOCK_BIAS, memory_order_relaxed);	// Should be acquire
	}
}

static inline void read_unlock(rwlock_t *rw)
{
	rw->lock.fetch_add(1, memory_order_release);
}

static inline void write_unlock(rwlock_t *rw)
{
	rw->lock.fetch_add(RW_LOCK_BIAS, memory_order_release);
}

rwlock_t mylock;
atomic_int data1, data2;

void * a(void *obj)
{
	int i;
	for(i = 0; i < 4; i++) {
		if ((i % 2) == 0) {
			read_lock(&mylock);
			int d1 = data1.load(memory_order_relaxed);
			int d2 = data2.load(memory_order_relaxed);
			assert(d1 == d2);	// Should fail on buggy executions
			read_unlock(&mylock);
		} else {
			write_lock(&mylock);
			data1.store(i, memory_order_relaxed);
			data2.store(i, memory_order_relaxed);
			write_unlock(&mylock);
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t t1, t2, t3;
	mylock.lock.store(RW_LOCK_BIAS);

	pthread_create(&t1, NULL, &a, NULL);
	pthread_create(&t2, NULL, &a, NULL);
	pthread_create(&t3, NULL, &a, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	return 0;
}
