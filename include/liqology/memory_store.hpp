#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "liqology/buckets.hpp"
#include "liqology/disambiguation.hpp"
#include "liqology/liquid_core.hpp"
#include "liqology/ownership.hpp"
#include "liqology/result.hpp"

namespace liqology {

enum class InteractionId : std::int64_t {};

enum class MemoryStoreError : std::uint8_t {
    DimensionMismatch,
    NotFound,
    IndexBuildFailed,
};

struct Interaction {
    InteractionId id;
    std::vector<float> input_embedding;
    std::vector<float> output_embedding;
    float retention_weight;
};

struct CostBalancePolicy {
    float reinform_cost_per_unit_weight;
    float retain_cost;
    float prune_retention_floor;
    float reinforce_gain_per_similarity_unit;
    float decay_per_tick;
    float minimum_similarity_to_reinforce;
};

class MemoryStore {
public:
    [[nodiscard]] static Result<MemoryStore, MemoryStoreError> Create(std::size_t embedding_dim,
                                                                        CostBalancePolicy policy) {
        if (embedding_dim == 0) {
            return Fail<MemoryStoreError, MemoryStore>(MemoryStoreError::DimensionMismatch);
        }
        return Ok<MemoryStore, MemoryStoreError>(MemoryStore{embedding_dim, policy});
    }

    [[nodiscard]] Result<std::vector<InteractionId>, MemoryStoreError> record(
        Span<const float> input_embedding, Span<const float> output_embedding) {
        if (input_embedding.size() != dim_ || output_embedding.size() != dim_) {
            return Fail<MemoryStoreError, std::vector<InteractionId>>(MemoryStoreError::DimensionMismatch);
        }

        decay_all_entries();
        auto reinforce_result = reinforce_entries_similar_to(output_embedding);
        if (!reinforce_result.has_value()) {
            return Fail<MemoryStoreError, std::vector<InteractionId>>(MemoryStoreError::IndexBuildFailed);
        }

        const auto new_id = static_cast<InteractionId>(next_id_++);
        std::vector<float> in_copy(input_embedding.begin(), input_embedding.end());
        std::vector<float> out_copy(output_embedding.begin(), output_embedding.end());
        entries_.push_back(Interaction{
            .id = new_id,
            .input_embedding = std::move(in_copy),
            .output_embedding = std::move(out_copy),
            .retention_weight = 1.0f,
        });

        auto add_result = relevance_index_.add(static_cast<EntityId>(static_cast<std::int64_t>(new_id)),
                                                 output_embedding);
        if (!add_result.has_value()) {
            return Fail<MemoryStoreError, std::vector<InteractionId>>(MemoryStoreError::IndexBuildFailed);
        }

        return Ok<std::vector<InteractionId>, MemoryStoreError>(prune());
    }

    [[nodiscard]] const std::vector<Interaction>& entries() const { return entries_; }

    [[nodiscard]] const CostBalancePolicy& policy() const { return policy_; }

    void set_policy(CostBalancePolicy new_policy) { policy_ = new_policy; }

    [[nodiscard]] std::vector<InteractionId> preview_prune() const {
        std::vector<InteractionId> would_evict;
        for (const auto& entry : entries_) {
            const bool below_retention_floor = entry.retention_weight < policy_.prune_retention_floor;
            if (below_retention_floor && !is_worth_reinforming_instead_of_retaining(entry)) {
                would_evict.push_back(entry.id);
            }
        }
        return would_evict;
    }

private:
    MemoryStore(std::size_t embedding_dim, CostBalancePolicy policy)
        : dim_(embedding_dim), policy_(policy), relevance_index_(embedding_dim) {}

    void decay_all_entries() {
        for (auto& entry : entries_) {
            entry.retention_weight -= policy_.decay_per_tick;
        }
    }

    [[nodiscard]] Result<void, DisambiguationError> reinforce_entries_similar_to(
        Span<const float> output_embedding) {
        if (relevance_index_.size() == 0) {
            return Ok<void, DisambiguationError>();
        }
        const auto similar = relevance_index_.query(output_embedding, relevance_index_.size());
        if (!similar.has_value()) {
            return Fail<DisambiguationError, void>(similar.error());
        }
        for (const auto& candidate : *similar) {
            if (candidate.score >= policy_.minimum_similarity_to_reinforce) {
                reinforce_entry_matching(candidate.id, candidate.score);
            }
        }
        return Ok<void, DisambiguationError>();
    }

    void reinforce_entry_matching(EntityId matched, float similarity) {
        const auto matched_id = static_cast<InteractionId>(static_cast<std::int64_t>(matched));
        for (auto& entry : entries_) {
            if (entry.id == matched_id) {
                entry.retention_weight += policy_.reinforce_gain_per_similarity_unit * similarity;
                return;
            }
        }
    }

    [[nodiscard]] bool is_worth_reinforming_instead_of_retaining(const Interaction& entry) const {
        const float estimated_reinform_value = policy_.reinform_cost_per_unit_weight * entry.retention_weight;
        return estimated_reinform_value >= policy_.retain_cost;
    }

    [[nodiscard]] std::vector<InteractionId> prune() {
        std::vector<InteractionId> evicted;
        std::vector<Interaction> survivors;
        survivors.reserve(entries_.size());
        for (auto& entry : entries_) {
            const bool below_retention_floor = entry.retention_weight < policy_.prune_retention_floor;
            if (below_retention_floor && !is_worth_reinforming_instead_of_retaining(entry)) {
                evicted.push_back(entry.id);
                continue;
            }
            survivors.push_back(std::move(entry));
        }
        entries_ = std::move(survivors);
        return evicted;
    }

    std::size_t dim_;
    CostBalancePolicy policy_;
    DisambiguationIndex relevance_index_;
    std::vector<Interaction> entries_;
    std::int64_t next_id_ = 0;
};

}  // namespace liqology
