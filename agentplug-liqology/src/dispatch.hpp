#pragma once

#include <cstdint>
#include <string>

namespace liqology::plugin {

std::uint64_t dispatch_verb(const std::string& verb, const std::string& body_json);

}  // namespace liqology::plugin
