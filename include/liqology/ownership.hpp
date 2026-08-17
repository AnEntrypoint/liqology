#pragma once

#include <gsl/gsl>
#include <memory>

namespace liqology {

template <typename T>
using NotNull = gsl::not_null<T>;

template <typename T>
using Span = gsl::span<T>;

template <typename T>
using Owner = gsl::owner<T>;

template <typename T>
using Box = std::unique_ptr<T>;

template <typename T>
using Rc = std::shared_ptr<T>;

template <typename T>
using Weak = std::weak_ptr<T>;

template <typename T, typename... Args>
[[nodiscard]] auto make_box(Args&&... args) -> Box<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
[[nodiscard]] auto make_rc(Args&&... args) -> Rc<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

}  // namespace liqology
