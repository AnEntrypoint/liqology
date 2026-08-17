#pragma once

#include <cstdint>
#include <string>
#include <vector>

extern "C" {

__attribute__((export_name("plugkit_alloc"))) std::uint32_t plugkit_alloc(std::uint32_t len);
__attribute__((export_name("plugkit_free"))) void plugkit_free(std::uint32_t ptr, std::uint32_t len);
__attribute__((export_name("plugin_call"))) std::uint64_t plugin_call(std::uint32_t verb_ptr, std::uint32_t verb_len,
                                                                       std::uint32_t body_ptr,
                                                                       std::uint32_t body_len);

__attribute__((import_module("env"), import_name("host_log"))) std::uint32_t host_log(std::uint32_t level,
                                                                                        const char* msg_ptr,
                                                                                        std::uint32_t msg_len);

__attribute__((import_module("env"), import_name("host_plugin_call"))) std::uint64_t host_plugin_call(
    const char* plugin_ptr, std::uint32_t plugin_len, const char* verb_ptr, std::uint32_t verb_len,
    const char* body_ptr, std::uint32_t body_len);
}

namespace liqology::plugin {

std::string read_str_from_host_written_linear_memory(std::uint32_t ptr, std::uint32_t len);
std::uint64_t pack_ptr_len_into_u64_result(std::uint32_t ptr, std::size_t len);
std::uint64_t return_bytes(std::vector<std::uint8_t> bytes);
std::uint64_t return_json(const std::string& json_text);
void elog(const std::string& msg);
std::string call_host_plugin(const std::string& plugin, const std::string& verb, const std::string& body);

}  // namespace liqology::plugin
