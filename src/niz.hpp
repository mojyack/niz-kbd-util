#pragma once
#include <optional>
#include <vector>

#include "util/variant.hpp"

namespace niz {
namespace func {
struct KeysFunction {
    std::vector<uint8_t> keycodes;
};

struct EmulateKeyFunction {
    uint16_t             delay;
    std::vector<uint8_t> keycodes;
};

enum class MacroRepeat : uint8_t {
    Count,
    Hold,
    Toggle,
};

struct AutoDelayMacroSequence {
    uint16_t             delay;
    std::vector<uint8_t> keycodes;
};

struct RecordedDelayMacroSequence {
    struct Event {
        uint8_t  keycode;
        uint16_t delay;
    };
    std::vector<Event> events;
};

using MacroSequence = Variant<AutoDelayMacroSequence, RecordedDelayMacroSequence>;

struct MacroKeyFunction {
    MacroRepeat   repeat;
    uint8_t       repeat_count;
    MacroSequence sequence;
};

using KeyFunction = Variant<KeysFunction, EmulateKeyFunction, MacroKeyFunction>;
} // namespace func

auto get_version(int fd) -> std::optional<std::string>;
auto read_counts(int fd) -> std::optional<std::vector<uint32_t>>;
auto flush_firmware(int fd, std::string_view firmware_path) -> bool;
auto enable_keypress(int fd, bool flag) -> bool;
auto do_initial_calibration(int fd) -> bool;
auto do_press_calibration(int fd) -> bool;

struct KeyMap {
    std::array<std::vector<func::KeyFunction>, 3> functions;

    auto write_to_keyboard(int fd) const -> bool;
    auto to_string() const -> std::string;
    auto debug_print() const -> void;

    static auto from_keyboard(int fd) -> std::optional<KeyMap>;
    static auto from_string(std::string_view str) -> std::optional<KeyMap>;
};
} // namespace niz
