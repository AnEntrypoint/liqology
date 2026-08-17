#include "dispatch.hpp"

#include <nlohmann/json.hpp>
#include <optional>

#include "abi.hpp"
#include "liqology/memory_store.hpp"
#include "liqology/ownership.hpp"

namespace liqology::plugin {

namespace {

using json = nlohmann::json;

constexpr const char* kPluginName = "liqology";

std::optional<MemoryStore>& store_slot() {
    static std::optional<MemoryStore> instance;
    return instance;
}

json error_response(const std::string& code) {
    return json{{"ok", false}, {"error", code}, {"plugin", kPluginName}};
}

json memory_store_error_response(MemoryStoreError err) {
    switch (err) {
        case MemoryStoreError::DimensionMismatch:
            return error_response("dimension_mismatch");
        case MemoryStoreError::NotFound:
            return error_response("not_found");
        case MemoryStoreError::IndexBuildFailed:
            return error_response("index_build_failed");
    }
    return error_response("unknown_memory_store_error");
}

CostBalancePolicy default_policy() {
    return CostBalancePolicy{
        .reinform_cost_per_unit_weight = 5.0f,
        .retain_cost = 1.0f,
        .prune_retention_floor = 0.3f,
        .reinforce_gain_per_similarity_unit = 0.5f,
        .decay_per_tick = 0.15f,
        .minimum_similarity_to_reinforce = 0.9f,
    };
}

json handle_capabilities() {
    return json{
        {"ok", true},
        {"plugin", kPluginName},
        {"verbs",
         json::array({"record", "query_relevance", "prune_report", "tune_policy", "suggest_fsm_update",
                       "capabilities"})},
        {"payload_field",
         json{{"record", "evicted_ids"},
              {"query_relevance", "candidates"},
              {"prune_report", "would_evict_ids"},
              {"tune_policy", "policy"},
              {"suggest_fsm_update", "proposal"}}},
    };
}

json handle_record(const json& body) {
    if (!body.contains("input_embedding") || !body.contains("output_embedding")) {
        return error_response("missing_embedding_fields");
    }
    const auto input = body.at("input_embedding").get<std::vector<float>>();
    const auto output = body.at("output_embedding").get<std::vector<float>>();

    auto& slot = store_slot();
    if (!slot.has_value()) {
        auto created = MemoryStore::Create(output.size(), default_policy());
        if (!created.has_value()) {
            return memory_store_error_response(created.error());
        }
        slot = std::move(created.value());
    }

    auto result = slot->record(Span<const float>(input.data(), static_cast<std::ptrdiff_t>(input.size())),
                                Span<const float>(output.data(), static_cast<std::ptrdiff_t>(output.size())));
    if (!result.has_value()) {
        return memory_store_error_response(result.error());
    }

    json evicted = json::array();
    for (const auto id : result.value()) {
        evicted.push_back(static_cast<std::int64_t>(id));
    }
    return json{{"ok", true}, {"evicted_ids", evicted}, {"retained_count", slot->entries().size()}};
}

json handle_query_relevance() {
    auto& slot = store_slot();
    if (!slot.has_value()) {
        return json{{"ok", true}, {"candidates", json::array()}};
    }
    json candidates = json::array();
    for (const auto& entry : slot->entries()) {
        candidates.push_back(json{
            {"id", static_cast<std::int64_t>(entry.id)},
            {"retention_weight", entry.retention_weight},
        });
    }
    return json{{"ok", true}, {"candidates", candidates}};
}

json handle_prune_report() {
    auto& slot = store_slot();
    if (!slot.has_value()) {
        return json{{"ok", true}, {"would_evict_ids", json::array()}};
    }
    json would_evict = json::array();
    for (const auto id : slot->preview_prune()) {
        would_evict.push_back(static_cast<std::int64_t>(id));
    }
    return json{{"ok", true}, {"would_evict_ids", would_evict}, {"retained_count", slot->entries().size()}};
}

json policy_to_json(const CostBalancePolicy& policy) {
    return json{
        {"reinform_cost_per_unit_weight", policy.reinform_cost_per_unit_weight},
        {"retain_cost", policy.retain_cost},
        {"prune_retention_floor", policy.prune_retention_floor},
        {"reinforce_gain_per_similarity_unit", policy.reinforce_gain_per_similarity_unit},
        {"decay_per_tick", policy.decay_per_tick},
        {"minimum_similarity_to_reinforce", policy.minimum_similarity_to_reinforce},
    };
}

json handle_suggest_fsm_update(const json& body) {
    if (!body.contains("phase") || !body.contains("gate") || !body.contains("recurrence_count")) {
        return error_response("missing_pattern_fields");
    }
    const auto phase = body.at("phase").get<std::string>();
    const auto gate = body.at("gate").get<std::string>();
    const auto recurrence_count = body.at("recurrence_count").get<std::int64_t>();
    const auto correction = body.value("correction_summary", std::string{"unspecified"});
    if (recurrence_count < 2) {
        return error_response("recurrence_count_too_low_for_a_real_pattern");
    }

    auto& slot = store_slot();
    std::size_t unused_entry_count = 0;
    if (slot.has_value()) {
        for (const auto& entry : slot->entries()) {
            if (entry.retention_weight < slot->policy().prune_retention_floor) {
                unused_entry_count += 1;
            }
        }
    }

    json proposal = json{
        {"kind", "prose"},
        {"target_phase", phase},
        {"target_gate", gate},
        {"evidence",
         json{
             {"recurrence_count", recurrence_count},
             {"correction_summary", correction},
             {"surfaced_but_unused_memory_entries", unused_entry_count},
         }},
        {"rationale",
         "Same correction (" + correction + ") recurred " + std::to_string(recurrence_count) +
             " times at " + phase + "/" + gate +
             " while relevant memory entries were repeatedly surfaced but never reflected in the "
             "resulting diff (retention_weight decayed below prune_retention_floor without "
             "reinforcement) -- suggests the phase's served prose does not state this correction "
             "clearly enough for it to stick without re-deriving it each time."},
        {"applied", false},
        {"apply_instructions",
         "Human or separate session review required -- pass this proposal's evidence/rationale to "
         "the real fsm-propose-override verb, expressed as attributed anchors per gm-config's "
         "self-reconfiguration content-shape rule. This verb only shapes the proposal; it never "
         "calls fsm-propose-override itself (least-privilege, human-in-the-loop)."},
    };
    return json{{"ok", true}, {"proposal", proposal}};
}

json handle_tune_policy(const json& body) {
    auto& slot = store_slot();
    if (!slot.has_value()) {
        return error_response("store_not_initialized");
    }
    CostBalancePolicy updated = slot->policy();
    if (body.contains("reinform_cost_per_unit_weight")) {
        updated.reinform_cost_per_unit_weight = body.at("reinform_cost_per_unit_weight").get<float>();
    }
    if (body.contains("retain_cost")) {
        updated.retain_cost = body.at("retain_cost").get<float>();
    }
    if (body.contains("prune_retention_floor")) {
        updated.prune_retention_floor = body.at("prune_retention_floor").get<float>();
    }
    if (body.contains("reinforce_gain_per_similarity_unit")) {
        updated.reinforce_gain_per_similarity_unit = body.at("reinforce_gain_per_similarity_unit").get<float>();
    }
    if (body.contains("decay_per_tick")) {
        updated.decay_per_tick = body.at("decay_per_tick").get<float>();
    }
    if (body.contains("minimum_similarity_to_reinforce")) {
        updated.minimum_similarity_to_reinforce = body.at("minimum_similarity_to_reinforce").get<float>();
    }
    slot->set_policy(updated);
    return json{{"ok", true}, {"policy", policy_to_json(updated)}};
}

}  // namespace

std::uint64_t dispatch_verb(const std::string& verb, const std::string& body_json) {
    json body = json::parse(body_json, nullptr, false);
    if (body.is_discarded()) {
        body = json::object();
    }

    json response;
    if (verb == "record") {
        response = handle_record(body);
    } else if (verb == "query_relevance") {
        response = handle_query_relevance();
    } else if (verb == "prune_report") {
        response = handle_prune_report();
    } else if (verb == "tune_policy") {
        response = handle_tune_policy(body);
    } else if (verb == "suggest_fsm_update") {
        response = handle_suggest_fsm_update(body);
    } else if (verb == "capabilities") {
        response = handle_capabilities();
    } else {
        response = json{{"ok", false}, {"error", "unknown_verb"}, {"verb", verb}, {"plugin", kPluginName}};
    }
    return return_json(response.dump());
}

}  // namespace liqology::plugin
