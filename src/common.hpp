#pragma once
#include <array>
#include <span>
#include <vector>

namespace niz {
struct PacketType {
    enum : uint8_t {
        ReadSerial       = 0x10,
        Firmware         = 0x3a,
        Keylock          = 0xd9,
        InitialCalibDone = 0xda,
        InitialCalib     = 0xdb,
        PressCalib       = 0xdd,
        PressCalibDone   = 0xde,
        XXXData          = 0xe0, // TODO maybe macro
        ReadXXX          = 0xe2, // TODO maybe macro
        ReadCounter      = 0xe3,
        XXXEnd           = 0xe6,
        KeyData          = 0xf0,
        WriteAll         = 0xf1,
        ReadAll          = 0xf2,
        DataEnd          = 0xf6,
        Version          = 0xf9,
    };
};

struct Packet {
    uint8_t unknown1;
    uint8_t type;
} __attribute__((packed));

extern std::array<const char*, 256> keycodes;

template <class T>
auto may_enlarge(std::vector<T>& vec, const size_t index) -> T& {
    if(vec.size() <= index) {
        vec.resize(index + 1);
    }
    return vec[index];
}

auto send_packet(int fd, int type, std::span<const uint8_t> data) -> bool;
auto dump_buffer(std::span<const uint8_t> buf) -> void;
} // namespace niz
