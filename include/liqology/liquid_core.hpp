#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "liqology/buckets.hpp"
#include "liqology/ownership.hpp"
#include "liqology/result.hpp"

namespace liqology {

enum class LiquidCoreError : std::uint8_t { DimensionMismatch, EmptyBucketSpan, InvalidTau };

class LiquidCore {
public:
    [[nodiscard]] static Result<LiquidCore, LiquidCoreError> Create(std::size_t hidden_dim, float dt) {
        if (hidden_dim == 0 || !(dt > 0.0f)) {
            return Fail<LiquidCoreError, LiquidCore>(LiquidCoreError::DimensionMismatch);
        }
        return Ok<LiquidCore, LiquidCoreError>(LiquidCore{hidden_dim, dt});
    }

    [[nodiscard]] Result<void, LiquidCoreError> refine(Span<Bucket> buckets, Span<const float> context_embedding) {
        if (buckets.empty()) {
            return Fail<LiquidCoreError>(LiquidCoreError::EmptyBucketSpan);
        }
        if (w_proj_.size() != hidden_dim_ * static_cast<std::size_t>(context_embedding.size())) {
            w_proj_.assign(hidden_dim_ * static_cast<std::size_t>(context_embedding.size()), 0.0f);
        }
        const auto context_dim = static_cast<std::size_t>(context_embedding.size());

        std::vector<float> u_x(hidden_dim_, 0.0f);
        for (std::size_t i = 0; i < hidden_dim_; ++i) {
            float sum = 0.0f;
            for (std::size_t j = 0; j < context_dim; ++j) {
                sum += w_proj_.at(i * context_dim + j) *
                       gsl::at(context_embedding, static_cast<std::ptrdiff_t>(j));
            }
            u_x.at(i) = sum;
        }

        for (auto& bucket : buckets) {
            if (bucket.hidden_state().size() != hidden_dim_) {
                return Fail<LiquidCoreError>(LiquidCoreError::DimensionMismatch);
            }
            if (!(bucket.tau() > 0.0f)) {
                return Fail<LiquidCoreError>(LiquidCoreError::InvalidTau);
            }
            update_bucket(bucket, u_x);
        }

        return Ok<void, LiquidCoreError>();
    }

private:
    LiquidCore(std::size_t hidden_dim, float dt)  // NOLINT(bugprone-easily-swappable-parameters): hidden_dim and dt have no realistic call-site overlap (one is a dimension count, one is a positive discretization step)
        : hidden_dim_(hidden_dim), dt_(dt), w_hidden_untrained_zero_init_(hidden_dim * hidden_dim, 0.0f) {}

    void update_bucket(Bucket& bucket, const std::vector<float>& u_x) const {
        const auto& h_hat = bucket.hidden_state();
        std::vector<float> new_state(hidden_dim_);
        for (std::size_t i = 0; i < hidden_dim_; ++i) {
            float sum = 0.0f;
            for (std::size_t j = 0; j < hidden_dim_; ++j) {
                sum += w_hidden_untrained_zero_init_.at(i * hidden_dim_ + j) * h_hat.at(j);
            }
            const float l_i = std::tanh(sum + u_x.at(i));
            new_state.at(i) = h_hat.at(i) + dt_ * (l_i - h_hat.at(i)) / bucket.tau();
        }
        bucket.hidden_state() = std::move(new_state);
    }

    std::size_t hidden_dim_;
    float dt_;
    std::vector<float> w_proj_;
    std::vector<float> w_hidden_untrained_zero_init_;
};

}  // namespace liqology
