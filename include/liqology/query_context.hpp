#pragma once

#include <cstdint>
#include <vector>

#if __has_include(<liqology/buckets.hpp>)
#include <liqology/buckets.hpp>
#else
namespace liqology {
struct BucketId {
    std::uint64_t value;
};
}  // namespace liqology
#endif

namespace liqology {

class DissolvingQueryContext {
   public:
    DissolvingQueryContext() = default;

    std::vector<BucketId> active_buckets;
    std::vector<float> scratch;
};

}  // namespace liqology
