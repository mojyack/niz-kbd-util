#include <unistd.h>

#include "common.hpp"
#include "macros/assert.hpp"
#include "niz.hpp"

namespace niz {
namespace {
auto read_and_expect(const int fd, const uint16_t type, const int expect) -> bool {
    ensure(send_packet(fd, type, {}));

    auto       buf = std::array<uint8_t, 64>();
    const auto len = read(fd, buf.data(), buf.size());
    ensure(len > 0);

    auto& count = *std::bit_cast<Packet*>(buf.data());
    ensure(count.type == expect);

    return true;
}
} // namespace

auto enable_keypress(const int fd, const bool flag) -> bool {
    const auto array = std::array{uint8_t(flag)};
    ensure(send_packet(fd, PacketType::Keylock, array));
    return true;
}

auto do_initial_calibration(const int fd) -> bool {
    ensure(read_and_expect(fd, PacketType::InitialCalib, PacketType::InitialCalibDone));
    return true;
}

auto do_press_calibration(const int fd) -> bool {
    ensure(read_and_expect(fd, PacketType::PressCalib, PacketType::PressCalibDone));
    return true;
}
} // namespace niz
