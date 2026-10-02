#include <cctype>
#include <cstdint>

using FINTEGER = long;

namespace {

bool is_transposed(const char* trans) {
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(*trans)));
    return c == 'T' || c == 'C';
}

}

extern "C" {

int sgemm_(const char* transa, const char* transb, FINTEGER* m, FINTEGER* n, FINTEGER* k, const float* alpha,
           const float* a, FINTEGER* lda, const float* b, FINTEGER* ldb, float* beta, float* c, FINTEGER* ldc) {
    const bool trans_a = is_transposed(transa);
    const bool trans_b = is_transposed(transb);
    const FINTEGER mm = *m;
    const FINTEGER nn = *n;
    const FINTEGER kk = *k;
    const float al = *alpha;
    const float be = *beta;

    auto op_a_column_major_element = [&](FINTEGER row, FINTEGER col) -> float {
        return trans_a ? a[col + row * (*lda)] : a[row + col * (*lda)];
    };
    auto op_b_column_major_element = [&](FINTEGER row, FINTEGER col) -> float {
        return trans_b ? b[col + row * (*ldb)] : b[row + col * (*ldb)];
    };

    for (FINTEGER col = 0; col < nn; ++col) {
        for (FINTEGER row = 0; row < mm; ++row) {
            float sum = 0.0f;
            for (FINTEGER p = 0; p < kk; ++p) {
                sum += op_a_column_major_element(row, p) * op_b_column_major_element(p, col);
            }
            float* dst = &c[row + col * (*ldc)];
            *dst = al * sum + be * (*dst);
        }
    }
    return 0;
}
}
