#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <faiss/IndexFlat.h>

#include "liqology/ownership.hpp"
#include "liqology/result.hpp"

namespace liqology {

enum class EntityId : std::int64_t {};

enum class DisambiguationError : std::uint8_t {
    DimensionMismatch,
    EmptyIndex,
    InvalidK,
    IndexBuildFailed,
};

struct ScoredCandidate {
    EntityId id;
    float score;
};

class DisambiguationIndex {
public:
    explicit DisambiguationIndex(std::size_t dim)
        : dim_(dim), index_(make_box<faiss::IndexFlatIP>(static_cast<int>(dim))) {}

    [[nodiscard]] auto add(EntityId id, Span<const float> embedding) -> Result<void, DisambiguationError> {
        if (embedding.size() != dim_) {
            return Fail<DisambiguationError>(DisambiguationError::DimensionMismatch);
        }
#if __cpp_exceptions
        try {
            index_->add(1, embedding.data());
        } catch (...) {
            return Fail<DisambiguationError>(DisambiguationError::IndexBuildFailed);
        }
#else
        index_->add(1, embedding.data());
#endif
        ids_by_insertion_order_.push_back(id);
        return Ok<void, DisambiguationError>();
    }

    [[nodiscard]] auto query(Span<const float> query_embedding, std::size_t k) const
        -> Result<std::vector<ScoredCandidate>, DisambiguationError> {
        if (query_embedding.size() != dim_) {
            return Fail<DisambiguationError, std::vector<ScoredCandidate>>(DisambiguationError::DimensionMismatch);
        }
        if (k == 0) {
            return Fail<DisambiguationError, std::vector<ScoredCandidate>>(DisambiguationError::InvalidK);
        }
        if (ids_by_insertion_order_.empty()) {
            return Fail<DisambiguationError, std::vector<ScoredCandidate>>(DisambiguationError::EmptyIndex);
        }

        const auto n = static_cast<faiss::idx_t>(std::min(k, ids_by_insertion_order_.size()));
        std::vector<float> distances(static_cast<std::size_t>(n));
        std::vector<faiss::idx_t> labels(static_cast<std::size_t>(n));

#if __cpp_exceptions
        try {
            index_->search(1, query_embedding.data(), n, distances.data(), labels.data());
        } catch (...) {
            return Fail<DisambiguationError, std::vector<ScoredCandidate>>(DisambiguationError::IndexBuildFailed);
        }
#else
        index_->search(1, query_embedding.data(), n, distances.data(), labels.data());
#endif

        std::vector<ScoredCandidate> results;
        results.reserve(static_cast<std::size_t>(n));
        for (faiss::idx_t i = 0; i < n; ++i) {
            const auto label = labels.at(static_cast<std::size_t>(i));
            if (label < 0) {
                continue;
            }
            results.push_back(ScoredCandidate{
                .id = ids_by_insertion_order_.at(static_cast<std::size_t>(label)),
                .score = distances.at(static_cast<std::size_t>(i)),
            });
        }
        return Ok<std::vector<ScoredCandidate>, DisambiguationError>(std::move(results));
    }

    [[nodiscard]] std::size_t size() const { return ids_by_insertion_order_.size(); }

private:
    std::size_t dim_;
    Box<faiss::IndexFlatIP> index_;
    std::vector<EntityId> ids_by_insertion_order_;
};

}  // namespace liqology
