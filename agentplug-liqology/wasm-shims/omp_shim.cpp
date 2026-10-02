#include "omp.h"

namespace {

constexpr int single_thread_count = 1;
constexpr int primary_thread_number = 0;

}

extern "C" {

int omp_get_max_threads(void) {
    return single_thread_count;
}

int omp_get_num_threads(void) {
    return single_thread_count;
}

int omp_get_thread_num(void) {
    return primary_thread_number;
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
