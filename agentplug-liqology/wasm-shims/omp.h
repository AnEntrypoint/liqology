#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int locked;
} omp_lock_t;

int omp_get_max_threads(void);
int omp_get_num_threads(void);
int omp_get_thread_num(void);
int omp_in_parallel(void);
int omp_get_nested(void);
void omp_set_nested(int nested);
void omp_set_num_threads(int num_threads);
void omp_init_lock(omp_lock_t* lock);
void omp_destroy_lock(omp_lock_t* lock);
void omp_set_lock(omp_lock_t* lock);
void omp_unset_lock(omp_lock_t* lock);
int omp_test_lock(omp_lock_t* lock);

#ifdef __cplusplus
}
#endif
