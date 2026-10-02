#include <cstdlib>

using FINTEGER = long;

namespace {

[[noreturn]] void abort_unsupported_faiss_quantizer_or_pca_path() {
    std::abort();
}

}

extern "C" {

int dgemm_(const char*, const char*, FINTEGER*, FINTEGER*, FINTEGER*, const double*, const double*, FINTEGER*,
           const double*, FINTEGER*, double*, double*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int ssyrk_(const char*, const char*, FINTEGER*, FINTEGER*, float*, float*, FINTEGER*, float*, float*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int ssyev_(const char*, const char*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int dsyev_(const char*, const char*, FINTEGER*, double*, FINTEGER*, double*, double*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int sgesvd_(const char*, const char*, FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, float*,
            FINTEGER*, float*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int dgesvd_(const char*, const char*, FINTEGER*, FINTEGER*, double*, FINTEGER*, double*, double*, FINTEGER*, double*,
            FINTEGER*, double*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int sgeqrf_(FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int sorgqr_(FINTEGER*, FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int sgemv_(const char*, FINTEGER*, FINTEGER*, float*, const float*, FINTEGER*, const float*, FINTEGER*, float*,
           float*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

void sgetrf_(FINTEGER*, FINTEGER*, float*, FINTEGER*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

void sgetri_(FINTEGER*, float*, FINTEGER*, FINTEGER*, float*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int dgetrf_(FINTEGER*, FINTEGER*, double*, FINTEGER*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int dgetri_(FINTEGER*, double*, FINTEGER*, FINTEGER*, double*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}

int sgelsd_(FINTEGER*, FINTEGER*, FINTEGER*, float*, FINTEGER*, float*, FINTEGER*, float*, float*, FINTEGER*,
            float*, FINTEGER*, FINTEGER*, FINTEGER*) {
    abort_unsupported_faiss_quantizer_or_pca_path();
}
}
