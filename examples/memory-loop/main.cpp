#include <cmath>
#include <cstdio>
#include <vector>

#include "liqology/memory_store.hpp"
#include "liqology/result.hpp"

namespace {

constexpr std::size_t kDim = 4;

std::vector<float> deterministic_hash_embed_stand_in_for_real_embedder(const char* tag) {
    std::vector<float> v(kDim, 0.0f);
    unsigned h = 2166136261u;
    for (const char* p = tag; *p != '\0'; ++p) {
        h = (h ^ static_cast<unsigned>(*p)) * 16777619u;
    }
    for (std::size_t i = 0; i < kDim; ++i) {
        v[i] = static_cast<float>((h >> (i * 4)) & 0xF) - 8.0f;
    }
    float norm = 0.0f;
    for (float x : v) {
        norm += x * x;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float& x : v) {
            x /= norm;
        }
    }
    return v;
}

}  // namespace

auto main() -> int {
    auto policy = liqology::CostBalancePolicy{
        .reinform_cost_per_unit_weight = 5.0f,
        .retain_cost = 1.0f,
        .prune_retention_floor = 0.3f,
        .reinforce_gain_per_similarity_unit = 0.5f,
        .decay_per_tick = 0.15f,
        .minimum_similarity_to_reinforce = 0.9f,
    };

    auto store_result = liqology::MemoryStore::Create(kDim, policy);
    if (!store_result.has_value()) {
        std::fprintf(stderr, "store creation failed\n");
        return 1;
    }
    auto store = std::move(store_result.value());

    const char* repeatedly_reinforced_topic_survives_decay = "topic-a";
    const char* single_shot_topics_decay_past_prune_floor[] = {"topic-b", "topic-c"};
    const char* calls[] = {
        repeatedly_reinforced_topic_survives_decay,
        single_shot_topics_decay_past_prune_floor[0],
        repeatedly_reinforced_topic_survives_decay,
        single_shot_topics_decay_past_prune_floor[1],
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
        repeatedly_reinforced_topic_survives_decay,
    };

    for (const char* tag : calls) {
        auto input = deterministic_hash_embed_stand_in_for_real_embedder(tag);
        auto output = deterministic_hash_embed_stand_in_for_real_embedder(tag);
        auto record_result = store.record(input, output);
        if (!record_result.has_value()) {
            std::fprintf(stderr, "record failed for %s\n", tag);
            return 1;
        }
        std::printf("recorded %-10s entries=%zu evicted=%zu\n", tag, store.entries().size(),
                    record_result.value().size());
    }

    std::printf("final retained entries: %zu\n", store.entries().size());
    for (const auto& entry : store.entries()) {
        std::printf("  id=%lld weight=%.3f\n", static_cast<long long>(entry.id), entry.retention_weight);
    }
    return 0;
}
