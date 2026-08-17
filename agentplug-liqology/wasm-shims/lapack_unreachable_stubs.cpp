#include <cstdlib>

using FINTEGER = long;

// These LAPACK entry points are called only from FAISS's quantizer/PCA code
// paths (LocalSearchQuantizer.cpp, ResidualQuantizer.cpp, VectorTransform.cpp)
// -- confirmed unreachable from IndexFlat/IndexFlatIP's add/search path this
// plugin actually exercises. Trapping on call surfaces any future code path
// that starts depending on them instead of silently returning wrong results.

extern "C" {

int dgemm_(const char*, const char*, FINTEGER*, FINTEGER*, FINTEGER*, const double*, const double*, FINTEGER*,
           const double*, FINTEGER*, double*, double*, FINTEGER*) {
    std::abort();
}

int ssyrk_(const char*, const char*, FINTEGER*, FINTEGER*, float*, float*, FINTEGER*, float*, float*, FINTEGER*) {
    std::abort();
}

int ssyev_(const char*, const char*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int dsyev_(const char*, const char*, FINTEGER*, double*, FINTEGER*, double*, double*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int sgesvd_(const char*, const char*, FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, float*,
            FINTEGER*, float*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int dgesvd_(const char*, const char*, FINTEGER*, FINTEGER*, double*, FINTEGER*, double*, double*, FINTEGER*, double*,
            FINTEGER*, double*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int sgeqrf_(FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int sorgqr_(FINTEGER*, FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int sgemv_(const char*, FINTEGER*, FINTEGER*, float*, const float*, FINTEGER*, const float*, FINTEGER*, float*,
           float*, FINTEGER*) {
    std::abort();
}

void sgetrf_(FINTEGER*, FINTEGER*, float*, FINTEGER*, FINTEGER*, FINTEGER*) {
    std::abort();
}

void sgetri_(FINTEGER*, float*, FINTEGER*, FINTEGER*, float*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int dgetrf_(FINTEGER*, FINTEGER*, double*, FINTEGER*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int dgetri_(FINTEGER*, double*, FINTEGER*, FINTEGER*, double*, FINTEGER*, FINTEGER*) {
    std::abort();
}

int sgelsd_(FINTEGER*, FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*,
            float*, FINTEGER*, FINTEGER*, FINTEGER*) {
    std::abort();
}
}
