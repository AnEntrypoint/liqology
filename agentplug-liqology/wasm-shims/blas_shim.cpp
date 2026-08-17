#include <cctype>
#include <cstdint>

using FINTEGER = long;

namespace {

bool is_transposed(const char* trans) {
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(*trans)));
    return c == 'T' || c == 'C';
}

}  // namespace

extern "C" {

// Standard Fortran BLAS column-major convention: an M-by-N logical matrix
// stored column-major with leading dimension LD is element(row,col) =
// data[row + col*LD]. "Transpose" means the LOGICAL op(A) is A^T -- op(A) is
// M-by-K, but the stored A is K-by-M column-major, i.e. element(row,col) of
// op(A) = A_stored[col + row*LDA].
int sgemm_(const char* transa, const char* transb, FINTEGER* m, FINTEGER* n, FINTEGER* k, const float* alpha,
           const float* a, FINTEGER* lda, const float* b, FINTEGER* ldb, float* beta, float* c, FINTEGER* ldc) {
    const bool trans_a = is_transposed(transa);
    const bool trans_b = is_transposed(transb);
    const FINTEGER mm = *m;
    const FINTEGER nn = *n;
    const FINTEGER kk = *k;
    const float al = *alpha;
    const float be = *beta;

    // op(A) is M-by-K, op(B) is K-by-N, C is M-by-N.
    auto a_at = [&](FINTEGER row, FINTEGER col) -> float {
        return trans_a ? a[col + row * (*lda)] : a[row + col * (*lda)];
    };
    auto b_at = [&](FINTEGER row, FINTEGER col) -> float {
        return trans_b ? b[col + row * (*ldb)] : b[row + col * (*ldb)];
    };

    for (FINTEGER col = 0; col < nn; ++col) {
        for (FINTEGER row = 0; row < mm; ++row) {
            float sum = 0.0f;
            for (FINTEGER p = 0; p < kk; ++p) {
                sum += a_at(row, p) * b_at(p, col);
            }
            float* dst = &c[row + col * (*ldc)];
            *dst = al * sum + be * (*dst);
        }
    }
    return 0;
}
}
