#include <unistd.h>

#include "common.hpp"
#include "macros/assert.hpp"
#include "niz.hpp"

namespace niz {
namespace {
struct KeyFunctionType {
    enum : uint8_t {
        Keys        = 0x00, // press keys at the same time
        Emulate     = 0x01, // press keys in a row at constant intervals
        CountMacro  = 0x02, // run a recorded macro a specified number of times
        HoldMacro   = 0x03, // run a recorded macro while the key is pressed
        ToggleMacro = 0x04, // keep running the recorded macro until the same key is pressed again
    };
};

struct KeyFunctionPacket : Packet {
    uint8_t layer;
    uint8_t pos;
    uint8_t func_type;
} __attribute__((packed));

struct KeysKeyFunctionPacket : KeyFunctionPacket {
    uint8_t data_size;
    uint8_t keycodes[];
} __attribute__((packed));

struct EmulateKeyFunctionPacket : KeyFunctionPacket {
    uint8_t delay_upper;
    uint8_t delay_lower;
    uint8_t data_size;
    uint8_t keycodes[];
} __attribute__((packed));

struct MacroEvent {
    uint8_t keycode;
    uint8_t unknown1;
    uint8_t delay_upper;
    uint8_t delay_lower;
} __attribute__((packed));

struct MacroKeyFunctionPacket : KeyFunctionPacket {
    uint8_t repeat_count;
    uint8_t use_recorded_delay;
    uint8_t auto_delay_upper;
    uint8_t auto_delay_lower;
    uint8_t unknown2;
    uint8_t data_size;
} __attribute__((packed));

struct AutoDelayMacroKeyFunctionPacket : MacroKeyFunctionPacket {
    uint8_t keycodes[];
} __attribute__((packed));

struct RecordedDelayMacroKeyFunctionPacket : MacroKeyFunctionPacket {
    MacroEvent macro_events[]; // unaligned!
} __attribute__((packed));

auto print_keycodes(std::span<const uint8_t> codes) -> void {
    for(const auto code : codes) {
        std::print("{}({}),", code, keycodes[code]);
    }
    std::println();
}
} // namespace

auto KeyMap::write_to_keyboard(const int fd) const -> bool {
    send_packet(fd, PacketType::WriteAll, {});

    auto  buf = std::array<uint8_t, 65>();
    auto& key = *std::bit_cast<KeyFunctionPacket*>(buf.data() + 1);
    key.type  = PacketType::KeyData;

    for(auto i = 0; i < 3; i += 1) {
        key.layer = i + 1;

        auto& funcs = functions[i];
        for(auto i = 0uz; i < funcs.size(); i += 1) {
            key.pos = i + 1;
            switch(funcs[i].get_index()) {
            case func::KeyFunction::index_of<func::KeysFunction>: {
                const auto& func = funcs[i].as<func::KeysFunction>();

                auto& keys_key     = *std::bit_cast<KeysKeyFunctionPacket*>(&key);
                keys_key.func_type = 0x00;
                keys_key.data_size = func.keycodes.size();
                memcpy(keys_key.keycodes, func.keycodes.data(), func.keycodes.size());
                keys_key.keycodes[func.keycodes.size()] = 0;
            } break;
            case func::KeyFunction::index_of<func::EmulateKeyFunction>: {
                const auto& func = funcs[i].as<func::EmulateKeyFunction>();

                auto& emu_key       = *std::bit_cast<EmulateKeyFunctionPacket*>(&key);
                emu_key.func_type   = 0x01;
                emu_key.delay_upper = (func.delay & 0xff00) >> 8;
                emu_key.delay_lower = (func.delay & 0x00ff);
                emu_key.data_size   = func.keycodes.size();
                mempcpy(emu_key.keycodes, func.keycodes.data(), func.keycodes.size());
            } break;
            case func::KeyFunction::index_of<func::MacroKeyFunction>: {
                const auto& func = funcs[i].as<func::MacroKeyFunction>();

                auto& macro_key = *std::bit_cast<MacroKeyFunctionPacket*>(&key);
                switch(func.repeat) {
                case func::MacroRepeat::Count:
                    macro_key.func_type    = 0x02;
                    macro_key.repeat_count = func.repeat_count;
                    break;
                case func::MacroRepeat::Hold:
                    macro_key.func_type    = 0x03;
                    macro_key.repeat_count = 0;
                    break;
                case func::MacroRepeat::Toggle:
                    macro_key.func_type    = 0x04;
                    macro_key.repeat_count = 0;
                    break;
                }
                switch(func.sequence.get_index()) {
                case func::MacroSequence::index_of<func::AutoDelayMacroSequence>: {
                    auto& sequence                    = func.sequence.as<func::AutoDelayMacroSequence>();
                    auto& auto_macro_key              = *std::bit_cast<AutoDelayMacroKeyFunctionPacket*>(&key);
                    auto_macro_key.use_recorded_delay = 0;
                    auto_macro_key.auto_delay_upper   = (sequence.delay & 0xff00) >> 8;
                    auto_macro_key.auto_delay_lower   = (sequence.delay & 0x00ff);
                    auto_macro_key.unknown2           = 0;
                    auto_macro_key.data_size          = sequence.keycodes.size();
                    memcpy(auto_macro_key.keycodes, sequence.keycodes.data(), sequence.keycodes.size());
                } break;
                case func::MacroSequence::index_of<func::RecordedDelayMacroSequence>: {
                    auto& sequence                   = func.sequence.as<func::RecordedDelayMacroSequence>();
                    auto& rec_macro_key              = *std::bit_cast<RecordedDelayMacroKeyFunctionPacket*>(&key);
                    rec_macro_key.use_recorded_delay = 1;
                    rec_macro_key.auto_delay_upper   = 0;
                    rec_macro_key.auto_delay_lower   = 0;
                    rec_macro_key.unknown2           = 0;
                    rec_macro_key.data_size          = sequence.events.size() * sizeof(MacroEvent);
                    for(auto i = 0uz; i < sequence.events.size(); i += 1) {
                        auto& key_event       = rec_macro_key.macro_events[i];
                        auto& seq_event       = sequence.events[i];
                        key_event.keycode     = seq_event.keycode;
                        key_event.unknown1    = 0xc8;
                        key_event.delay_upper = (seq_event.delay & 0xff00) >> 8;
                        key_event.delay_lower = (seq_event.delay & 0x00ff);
                    }
                } break;
                }
            } break;
            default:
                continue;
            }
            ensure(write(fd, buf.data(), buf.size()) == buf.size());
        }
    }

    for(auto i = 1u; i < buf.size(); i += 1) {
        buf[i] = PacketType::DataEnd;
    }
    ensure(write(fd, buf.data(), buf.size()) == buf.size());

    return true;
}

auto KeyMap::debug_print() const -> void {
    for(auto i = 0; i < 3; i += 1) {
        std::println("==== layer {} ====", i);
        auto& funcs = functions[i];
        for(auto i = 0uz; i < funcs.size(); i += 1) {
            std::println("key {}", i);
            switch(funcs[i].get_index()) {
            case func::KeyFunction::index_of<func::KeysFunction>: {
                const auto& func = funcs[i].as<func::KeysFunction>();
                std::println("  func: keys");
                std::print("  codes: ");
                print_keycodes(func.keycodes);
            } break;
            case func::KeyFunction::index_of<func::EmulateKeyFunction>: {
                const auto& func = funcs[i].as<func::EmulateKeyFunction>();
                std::println("  func: emulate");
                std::println("  delay: {}", func.delay);
                std::print("  codes: ");
                print_keycodes(func.keycodes);
            } break;
            case func::KeyFunction::index_of<func::MacroKeyFunction>: {
                const auto& func = funcs[i].as<func::MacroKeyFunction>();
                switch(func.repeat) {
                case func::MacroRepeat::Count:
                    std::println("  repeat: {} times", func.repeat_count);
                    break;
                case func::MacroRepeat::Hold:
                    std::println("  repeat: hold");
                    break;
                case func::MacroRepeat::Toggle:
                    std::println("  repeat: toggle");
                    break;
                }
                switch(func.sequence.get_index()) {
                case func::MacroSequence::index_of<func::AutoDelayMacroSequence>: {
                    auto& sequence = func.sequence.as<func::AutoDelayMacroSequence>();
                    std::println("  auto delay: {}", sequence.delay);
                    std::print("  codes: ");
                    print_keycodes(sequence.keycodes);
                } break;
                case func::MacroSequence::index_of<func::RecordedDelayMacroSequence>: {
                    auto& sequence = func.sequence.as<func::RecordedDelayMacroSequence>();
                    std::println("  events:");
                    for(const auto& ev : sequence.events) {
                        std::print(" {} -> {}ms ->", keycodes[ev.keycode], ev.delay);
                    }
                    std::println();
                } break;
                }
            } break;
            }
        }
    }
}

auto KeyMap::from_keyboard(const int fd) -> std::optional<KeyMap> {
    auto keymap = KeyMap();
    auto buf    = std::array<uint8_t, 64>();

    ensure(send_packet(fd, PacketType::ReadAll, {}));
    while(true) {
        const auto len = read(fd, buf.data(), buf.size());
        ensure(len > 0);
        const auto& key = *std::bit_cast<KeyFunctionPacket*>(buf.data());
        if(key.type == PacketType::DataEnd) {
            break;
        }
        ensure(key.type == PacketType::KeyData);

        auto func = func::KeyFunction();
        switch(key.func_type) {
        case KeyFunctionType::Keys: {
            const auto& key = *std::bit_cast<KeysKeyFunctionPacket*>(buf.data());
            if(key.data_size == 0) {
                continue;
            }
            auto& keycodes = func.emplace<func::KeysFunction>().keycodes;
            keycodes.resize(key.data_size);
            memcpy(keycodes.data(), key.keycodes, key.data_size);
        } break;
        case KeyFunctionType::CountMacro:
        case KeyFunctionType::HoldMacro:
        case KeyFunctionType::ToggleMacro: {
            auto        macro_func = func::MacroKeyFunction();
            const auto& key        = *std::bit_cast<MacroKeyFunctionPacket*>(buf.data());
            switch(key.func_type) {
            case KeyFunctionType::CountMacro:
                macro_func.repeat       = func::MacroRepeat::Count;
                macro_func.repeat_count = key.repeat_count;
                break;
            case KeyFunctionType::HoldMacro:
                macro_func.repeat = func::MacroRepeat::Hold;
                break;
            case KeyFunctionType::ToggleMacro:
                macro_func.repeat = func::MacroRepeat::Toggle;
                break;
            default:
                continue;
            }
            if(key.use_recorded_delay) {
                auto& sequence = macro_func.sequence.emplace<func::RecordedDelayMacroSequence>();

                const auto& key = *std::bit_cast<RecordedDelayMacroKeyFunctionPacket*>(buf.data());
                for(auto i = 0uz; i < key.data_size / sizeof(MacroEvent); i += 1) {
                    const auto& s     = key.macro_events[i];
                    const auto  delay = uint16_t(s.delay_upper << 8 | s.delay_lower);
                    sequence.events.push_back(func::RecordedDelayMacroSequence::Event{s.keycode, delay});
                }
            } else {
                auto& sequence = macro_func.sequence.emplace<func::AutoDelayMacroSequence>();

                const auto& key = *std::bit_cast<AutoDelayMacroKeyFunctionPacket*>(buf.data());
                sequence.delay  = key.auto_delay_upper << 8 | key.auto_delay_lower;
                sequence.keycodes.resize(key.data_size);
                memcpy(sequence.keycodes.data(), key.keycodes, key.data_size);
            }
            func.emplace<func::MacroKeyFunction>(std::move(macro_func));
        } break;
        case KeyFunctionType::Emulate: {
            auto& emu_func = func.emplace<func::EmulateKeyFunction>();

            const auto& key = *std::bit_cast<EmulateKeyFunctionPacket*>(buf.data());
            emu_func.delay  = key.delay_upper << 8 | key.delay_lower;
            emu_func.keycodes.resize(key.data_size);
            memcpy(emu_func.keycodes.data(), key.keycodes, key.data_size);
        } break;
        default:
            WARN("unknown function type: ");
            dump_buffer(buf);
            continue;
        }

        may_enlarge(keymap.functions[key.layer - 1], key.pos - 1) = std::move(func);
    }

    return std::move(keymap);
}
} // namespace niz
