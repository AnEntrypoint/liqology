#include "abi.hpp"

#include <cstdlib>
#include <cstring>
#include <new>

#include "dispatch.hpp"

extern "C" {

std::uint32_t plugkit_alloc(std::uint32_t len) {
    if (len == 0) {
        return 0;
    }
    void* ptr = std::malloc(len);
    if (ptr == nullptr) {
        std::abort();
    }
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(ptr));
}

void plugkit_free(std::uint32_t ptr, std::uint32_t len) {
    (void)len;
    if (ptr == 0) {
        return;
    }
    std::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(ptr)));
}

std::uint64_t plugin_call(std::uint32_t verb_ptr, std::uint32_t verb_len, std::uint32_t body_ptr,
                           std::uint32_t body_len) {
    const auto verb = liqology::plugin::read_str_from_host_written_linear_memory(verb_ptr, verb_len);
    const auto body = liqology::plugin::read_str_from_host_written_linear_memory(body_ptr, body_len);
    return liqology::plugin::dispatch_verb(verb, body);
}
}

namespace liqology::plugin {

std::string read_str_from_host_written_linear_memory(std::uint32_t ptr, std::uint32_t len) {
    if (len == 0) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(static_cast<std::uintptr_t>(ptr)), len);
}

std::uint64_t pack_ptr_len_into_u64_result(std::uint32_t ptr, std::size_t len) {
    return (static_cast<std::uint64_t>(ptr) & 0xffffffffULL) | (static_cast<std::uint64_t>(len) << 32);
}

std::uint64_t return_bytes(std::vector<std::uint8_t> bytes) {
    if (bytes.empty()) {
        return 0;
    }
    const auto len = bytes.size();
    const auto ptr = plugkit_alloc(static_cast<std::uint32_t>(len));
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(ptr)), bytes.data(), len);
    return pack_ptr_len_into_u64_result(ptr, len);
}

std::uint64_t return_json(const std::string& json_text) {
    return return_bytes(std::vector<std::uint8_t>(json_text.begin(), json_text.end()));
}

void elog(const std::string& msg) {
    host_log(2, msg.data(), static_cast<std::uint32_t>(msg.size()));
}

std::string call_host_plugin(const std::string& plugin, const std::string& verb, const std::string& body) {
    const std::uint64_t packed = host_plugin_call(plugin.data(), static_cast<std::uint32_t>(plugin.size()),
                                                    verb.data(), static_cast<std::uint32_t>(verb.size()),
                                                    body.data(), static_cast<std::uint32_t>(body.size()));
    if (packed == 0) {
        return "{}";
    }
    const auto ptr = static_cast<std::uint32_t>(packed & 0xffffffffULL);
    const auto len = static_cast<std::uint32_t>(packed >> 32);
    auto result = read_str_from_host_written_linear_memory(ptr, len);
    plugkit_free(ptr, len);
    return result;
}

}  // namespace liqology::plugin
