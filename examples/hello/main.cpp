#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

#include "liqology/ownership.hpp"
#include "liqology/result.hpp"

namespace {

enum class ParseError { kEmpty, kNotNumeric, kOverflow };

auto parse_positive_int(liqology::NotNull<const char*> text) -> liqology::Result<int, ParseError> {
    if (std::strlen(text.get()) == 0) {
        return liqology::Fail<ParseError, int>(ParseError::kEmpty);
    }
    int value = 0;
    for (const char* p = text.get(); *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return liqology::Fail<ParseError, int>(ParseError::kNotNumeric);
        }
        const int digit = *p - '0';
        if (value > (std::numeric_limits<int>::max() - digit) / 10) {
            return liqology::Fail<ParseError, int>(ParseError::kOverflow);
        }
        value = (value * 10) + digit;
    }
    return liqology::Ok<int, ParseError>(value);
}

auto describe_error(ParseError err) -> const char* {
    switch (err) {
        case ParseError::kEmpty:
            return "empty input";
        case ParseError::kNotNumeric:
            return "not numeric";
        case ParseError::kOverflow:
            return "value overflows int";
    }
    return "unknown";
}

void demonstrate_asan_out_of_bounds_write() {
    auto buffer = liqology::make_box<int[]>(4);
    const auto out_of_bounds_index = 4;
    buffer[out_of_bounds_index] = 1;
    std::printf("unreachable if ASan is active: %d\n", buffer[4]);
}

}

auto main(int argc, char** argv) -> int {
    if (argc > 1 && std::strcmp(argv[1], "--inject-bug") == 0) {
        demonstrate_asan_out_of_bounds_write();
        return 0;
    }

    const char* input = argc > 1 ? argv[1] : "42";
    auto result = parse_positive_int(liqology::NotNull<const char*>{input});

    if (!result.has_value()) {
        std::fprintf(stderr, "parse failed: %s\n", describe_error(result.error()));
        return 1;
    }

    auto owned = liqology::make_box<int>(result.value());
    std::printf("parsed value: %d\n", *owned);
    return 0;
}
