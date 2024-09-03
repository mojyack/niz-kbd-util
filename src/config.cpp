#include "common.hpp"
#include "macros/unwrap.hpp"
#include "niz.hpp"
#include "util/charconv.hpp"
#include "util/print.hpp"
#include "util/split.hpp"

namespace niz {
const auto layer_str = std::array{"normal", "rightfn", "leftfn"};

auto find_layer_by_str(std::string_view str) -> std::optional<uint8_t> {
    for(auto i = 0u; i < layer_str.size(); i += 1) {
        if(layer_str[i] == str) {
            return i;
        }
    }
    bail("invalid layer ", str);
}

auto find_keycode_by_str(std::string_view str) -> std::optional<uint8_t> {
    for(auto i = 0u; i < keycodes.size(); i += 1) {
        if(keycodes[i] == str) {
            return i;
        }
    }
    bail("invalid keycode ", str);
}

namespace {
struct FixedMacro {
    std::string_view     name;
    uint16_t             interval;
    std::vector<uint8_t> keycodes;
};

auto parse_fixed_macro(const std::span<const std::string_view> elms) -> std::optional<FixedMacro> {
    ensure(elms.size() >= 3);
    const auto name = elms[1];
    unwrap(interval, from_chars<uint16_t>(elms[2]));
    auto keycodes = std::vector<uint8_t>();
    for(auto i = 3u; i < elms.size(); i += 1) {
        unwrap(keycode, find_keycode_by_str(elms[i]));
        keycodes.emplace_back(keycode);
    }
    return FixedMacro{name, interval, std::move(keycodes)};
}

struct RecordMacro {
    std::string_view                                     name;
    std::vector<func::RecordedDelayMacroSequence::Event> events;
};

auto parse_record_macro(const std::span<const std::string_view> elms) -> std::optional<RecordMacro> {
    ensure(elms.size() >= 4);
    const auto name   = elms[1];
    auto       events = std::vector<func::RecordedDelayMacroSequence::Event>();
    for(auto i = 2u; i < elms.size(); i += 2) {
        ensure(i + 1 < elms.size());
        unwrap(keycode, find_keycode_by_str(elms[i]));
        unwrap(interval, from_chars<uint16_t>(elms[i + 1]));
        events.push_back({keycode, interval});
    }
    return RecordMacro{name, std::move(events)};
}
} // namespace

auto KeyMap::to_string() const -> std::string {
    auto str         = std::string();
    auto macro_count = 0;
    for(auto layer = 0; layer < 3; layer += 1) {
        auto& funcs = functions[layer];
        for(auto pos = 0u; pos < funcs.size(); pos += 1) {
            switch(funcs[pos].get_index()) {
            case func::KeyFunction::index_of<func::KeysFunction>: {
                const auto& func = funcs[pos].as<func::KeysFunction>();

                str += build_string("map-keys ", layer_str[layer], " ", pos);
                for(auto i = 0u; i < func.keycodes.size(); i += 1) {
                    str += " ";
                    str += keycodes[func.keycodes[i]];
                }
                str += "\n";
            } break;
            case func::KeyFunction::index_of<func::EmulateKeyFunction>: {
                const auto& func = funcs[pos].as<func::EmulateKeyFunction>();

                str += build_string("map-emu ", layer_str[layer], " ", pos, " ", func.delay);
                for(auto i = 0u; i < func.keycodes.size(); i += 1) {
                    str += " ";
                    str += keycodes[func.keycodes[i]];
                }
                str += "\n";
            } break;
            case func::KeyFunction::index_of<func::MacroKeyFunction>: {
                const auto& func = funcs[pos].as<func::MacroKeyFunction>();

                auto macro_name = build_string("macro", macro_count += 1);
                switch(func.sequence.get_index()) {
                case func::MacroSequence::index_of<func::AutoDelayMacroSequence>: {
                    auto& sequence = func.sequence.as<func::AutoDelayMacroSequence>();
                    str += build_string("fixed-macro ", macro_name, " ", sequence.delay);
                    for(const auto keycode : sequence.keycodes) {
                        str += " ";
                        str += keycodes[keycode];
                    }
                    str += "\n";
                } break;
                case func::MacroSequence::index_of<func::RecordedDelayMacroSequence>: {
                    auto& sequence = func.sequence.as<func::RecordedDelayMacroSequence>();
                    str += build_string("record-macro ", macro_name);
                    for(auto i = 0u; i < sequence.events.size(); i += 1) {
                        str += " ";
                        str += keycodes[sequence.events[i].keycode];
                        str += " ";
                        str += std::to_string(sequence.events[i].delay);
                    }
                    str += "\n";
                } break;
                }

                str += build_string("map-macro ", layer_str[layer], " ", pos, " ", macro_name, " ");
                switch(func.repeat) {
                case func::MacroRepeat::Count:
                    str += std::to_string(func.repeat_count);
                    break;
                case func::MacroRepeat::Hold:
                    str += "hold";
                    break;
                case func::MacroRepeat::Toggle:
                    str += "toggle";
                    break;
                }
                str += "\n";
            } break;
            }
        }
    }

    return str;
}

auto KeyMap::from_string(const std::string_view str) -> std::optional<KeyMap> {
    auto map           = KeyMap();
    auto fixed_macros  = std::vector<FixedMacro>();
    auto record_macros = std::vector<RecordMacro>();
    for(auto& line : split(str, "\n")) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        const auto elms = split(line, " ");
        if(elms[0] == "fixed-macro") {
            unwrap_mut(macro, parse_fixed_macro(elms));
            fixed_macros.emplace_back(std::move(macro));
        } else if(elms[0] == "record-macro") {
            unwrap_mut(macro, parse_record_macro(elms));
            record_macros.emplace_back(std::move(macro));
        } else if(elms[0] == "map-keys") {
            ensure(elms.size() >= 4);
            unwrap(layer, find_layer_by_str(elms[1]));
            unwrap(pos, from_chars<uint8_t>(elms[2]));
            auto keycodes = std::vector<uint8_t>();
            for(auto i = 3u; i < elms.size(); i += 1) {
                unwrap(keycode, find_keycode_by_str(elms[i]));
                keycodes.emplace_back(keycode);
            }
            may_enlarge(map.functions[layer], pos).emplace<func::KeysFunction>(std::move(keycodes));
        } else if(elms[0] == "map-emu") {
            ensure(elms.size() >= 5);
            unwrap(layer, find_layer_by_str(elms[1]));
            unwrap(pos, from_chars<uint8_t>(elms[2]));
            unwrap(interval, from_chars<uint16_t>(elms[3]));
            auto keycodes = std::vector<uint8_t>();
            for(auto i = 4u; i < elms.size(); i += 1) {
                unwrap(keycode, find_keycode_by_str(elms[i]));
                keycodes.emplace_back(keycode);
            }
            may_enlarge(map.functions[layer], pos).emplace<func::EmulateKeyFunction>(interval, std::move(keycodes));
        } else if(elms[0] == "map-macro") {
            ensure(elms.size() == 5);
            unwrap(layer, find_layer_by_str(elms[1]));
            unwrap(pos, from_chars<uint8_t>(elms[2]));
            const auto macro_name = elms[3];
            auto       macro      = func::MacroKeyFunction();
            if(elms[4] == "hold") {
                macro.repeat = func::MacroRepeat::Hold;
            } else if(elms[4] == "toggle") {
                macro.repeat = func::MacroRepeat::Toggle;
            } else {
                macro.repeat = func::MacroRepeat::Count;
                unwrap(count, from_chars<uint8_t>(elms[4]));
                macro.repeat_count = count;
            }
            for(const auto& fixed : fixed_macros) {
                if(fixed.name == macro_name) {
                    macro.sequence.emplace<func::AutoDelayMacroSequence>(fixed.interval, fixed.keycodes);
                    break;
                }
            }
            if(!macro.sequence.is_valid()) {
                for(const auto& recorded : record_macros) {
                    if(recorded.name == macro_name) {
                        macro.sequence.emplace<func::RecordedDelayMacroSequence>(recorded.events);
                        break;
                    }
                }
            }
            ensure(macro.sequence.is_valid());
            may_enlarge(map.functions[layer], pos).emplace<func::MacroKeyFunction>(std::move(macro));
        } else {
            bail("unknown statement ", elms[0]);
        }
    }

    return map;
}
} // namespace niz
