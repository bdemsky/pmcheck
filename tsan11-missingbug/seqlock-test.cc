#include <pthread.h>
#include <atomic>
#include <assert.h>
//#include <threads.h>

using namespace std;

typedef struct seqlock {
	// Sequence for reader consistency check
	atomic_int _seq;
	// It needs to be atomic to avoid data races
	atomic_int _data1;
	atomic_int _data2;

	seqlock() {
        _seq.store(0);
        _data1.store(0);
        _data2.store(0);
	}

	void read(int * d1, int *d2) {
		while (true) {
			int old_seq = _seq.load(memory_order_acquire);
			if (old_seq % 2 == 1) continue;

			*d1 = _data1.load(memory_order_acquire);
			*d2 = _data2.load(memory_order_acquire);
			if (_seq.load(memory_order_relaxed) == old_seq) {
				return;
			}
		}
	}

	void write(int new_data, int new_data2) {
		while (true) {
			int old_seq = _seq.load(memory_order_relaxed); // Injected bug: should be acquire
			if (old_seq % 2 == 1)
				continue; // Retry

			if (_seq.compare_exchange_strong(old_seq, old_seq + 1,
				memory_order_relaxed, memory_order_relaxed))
				break;
		}

		// Update the data
		_data1.store(new_data, memory_order_release);
		_data2.store(new_data, memory_order_release);

		_seq.fetch_add(1, memory_order_release);
	}

} seqlock_t;


seqlock_t *lock;

void * a(void *obj) {
	lock->write(3,3);
	return NULL;
}

void * b(void *obj) {
	lock->write(2,2);
	return NULL;
}

void * c(void *obj) {
	int r1, r2;
	lock->read(&r1, &r2);
	assert(r1 == r2);
	return NULL;
}

int main(int argc, char **argv) {
	lock = new seqlock_t();

	pthread_t t1, t2, t3;
	pthread_create(&t1, NULL, &a, NULL);
	pthread_create(&t2, NULL, &b, NULL);
	pthread_create(&t3, NULL, &c, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	return 0;
}
