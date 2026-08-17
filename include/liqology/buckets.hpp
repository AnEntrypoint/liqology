#pragma once

#include <cstdint>
#include <vector>

#include "liqology/result.hpp"

#if __has_include("liqology/disambiguation.hpp")
#include "liqology/disambiguation.hpp"
#define LIQOLOGY_HAS_DISAMBIGUATION_HPP 1  // NOLINT(cppcoreguidelines-macro-usage): preprocessor conditional compilation, not a constant -- constexpr cannot gate #ifndef
#endif

namespace liqology {

#ifndef LIQOLOGY_HAS_DISAMBIGUATION_HPP
enum class EntityId : std::int64_t {};
#endif

struct BucketId {
    std::uint64_t value;
};

enum class BucketError : std::uint8_t { InvalidTau };

class Bucket {
public:
    [[nodiscard]] static Result<Bucket, BucketError> Create(BucketId id, std::size_t hidden_dim, float tau) {
        if (!(tau > 0.0f)) {
            return Fail<BucketError, Bucket>(BucketError::InvalidTau);
        }
        return Ok<Bucket, BucketError>(Bucket{id, hidden_dim, tau});
    }

    [[nodiscard]] BucketId id() const { return id_; }
    [[nodiscard]] float tau() const { return tau_; }

    [[nodiscard]] std::vector<EntityId>& members() { return members_; }
    [[nodiscard]] const std::vector<EntityId>& members() const { return members_; }

    [[nodiscard]] std::vector<float>& hidden_state() { return hidden_state_; }
    [[nodiscard]] const std::vector<float>& hidden_state() const { return hidden_state_; }

private:
    Bucket(BucketId id, std::size_t hidden_dim, float tau)  // NOLINT(bugprone-easily-swappable-parameters): hidden_dim and tau have no realistic call-site overlap (one is a count, one is a positive time constant)
        : id_(id), tau_(tau), hidden_state_(hidden_dim, 0.0f) {}

    BucketId id_;
    float tau_;
    std::vector<EntityId> members_;
    std::vector<float> hidden_state_;
};

}  // namespace liqology
