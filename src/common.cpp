#include <unistd.h>

#include "common.hpp"
#include "macros/assert.hpp"
#include "util/assert.hpp"

namespace niz {
auto keycodes = std::array<const char*, 256>{
#include "keycodes.txt"
};

auto send_packet(const int fd, const int type, const std::span<const uint8_t> data) -> bool {
    assert_b(data.size() < 62);
    auto  buf    = std::array<uint8_t, 65>();
    auto& packet = *std::bit_cast<Packet*>(buf.data() + 1);
    packet.type  = type;
    memcpy(buf.data() + 1 + sizeof(Packet), data.data(), data.size());
    assert_b(write(fd, buf.data(), buf.size()) == buf.size());
    return true;
}

auto dump_buffer(const std::span<const uint8_t> buf) -> void {
    for(auto i = 0;; i += 1) {
        for(auto j = 0; j < 4; j += 1) {
            for(auto k = 0; k < 4; k += 1) {
                const auto index = size_t(i * 16 + j * 4 + k);
                if(index >= buf.size()) {
                    printf("\n");
                    return;
                }
                printf("%02X", buf[index]);
            }
            printf(" ");
        }
        printf("\n");
    }
}
} // namespace niz
