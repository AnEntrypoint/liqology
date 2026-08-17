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
        {"verbs", json::array({"record", "query_relevance", "prune_report", "tune_policy", "capabilities"})},
        {"payload_field",
         json{{"record", "evicted_ids"},
              {"query_relevance", "candidates"},
              {"prune_report", "would_evict_ids"},
              {"tune_policy", "policy"}}},
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
    return handle_query_relevance();
}

json handle_tune_policy(const json&) {
    return error_response("not_yet_implemented");
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
    } else if (verb == "capabilities") {
        response = handle_capabilities();
    } else {
        response = json{{"ok", false}, {"error", "unknown_verb"}, {"verb", verb}, {"plugin", kPluginName}};
    }
    return return_json(response.dump());
}

}  // namespace liqology::plugin
