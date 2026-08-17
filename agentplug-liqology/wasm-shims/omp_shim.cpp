#include "omp.h"

// wasm32-wasip1 (this plugin's reactor model) is single-threaded --
// these are the real single-thread-correct semantics for every omp_*
// FAISS calls, not placeholders: max_threads/num_threads=1, thread_num=0,
// in_parallel=false, locks degrade to a plain non-reentrant flag since
// there is never a second thread to contend with.

extern "C" {

int omp_get_max_threads(void) {
    return 1;
}

int omp_get_num_threads(void) {
    return 1;
}

int omp_get_thread_num(void) {
    return 0;
}

int omp_in_parallel(void) {
    return 0;
}

int omp_get_nested(void) {
    return 0;
}

void omp_set_nested(int) {}

void omp_set_num_threads(int) {}

void omp_init_lock(omp_lock_t* lock) {
    lock->locked = 0;
}

void omp_destroy_lock(omp_lock_t*) {}

void omp_set_lock(omp_lock_t* lock) {
    lock->locked = 1;
}

void omp_unset_lock(omp_lock_t* lock) {
    lock->locked = 0;
}

int omp_test_lock(omp_lock_t* lock) {
    if (lock->locked) {
        return 0;
    }
    lock->locked = 1;
    return 1;
}
}
