#include <unistd.h>

#include "common.hpp"
#include "macros/assert.hpp"
#include "niz.hpp"
#include "util/assert.hpp"

namespace niz {
auto get_version(int fd) -> std::optional<std::string> {
    auto buf = std::array<char, 64>();
    assert_o(send_packet(fd, PacketType::Version, {}));
    assert_o(read(fd, buf.data(), buf.size()) > 0);
    return buf.data() + 1;
}
} // namespace niz
