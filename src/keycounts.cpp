#include <unistd.h>

#include "common.hpp"
#include "macros/assert.hpp"
#include "util/assert.hpp"

namespace niz {
namespace {
struct KeyCount : Packet {
    uint8_t  data_size;
    uint32_t count[]; // not aligned!
} __attribute__((packed));
} // namespace

auto read_counts(const int fd) -> std::optional<std::vector<uint32_t>> {
    assert_o(send_packet(fd, PacketType::ReadCounter, {}));

    auto counts = std::vector<uint32_t>();
    auto buf    = std::array<uint8_t, 64>();
    while(true) {
        const auto len = read(fd, buf.data(), buf.size());
        assert_o(len > 0);
        auto& count = *std::bit_cast<KeyCount*>(buf.data());
        if(count.type != PacketType::ReadCounter) {
            break;
        }
        for(auto i = 0; i < count.data_size / 4; i += 1) {
            counts.push_back(count.count[i]);
        }
    }

    return counts;
}
} // namespace niz
