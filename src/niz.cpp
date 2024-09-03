#include <unistd.h>

#include "common.hpp"
#include "macros/assert.hpp"
#include "niz.hpp"

namespace niz {
auto get_version(int fd) -> std::optional<std::string> {
    auto buf = std::array<char, 64>();
    ensure(send_packet(fd, PacketType::Version, {}));
    ensure(read(fd, buf.data(), buf.size()) > 0);
    return buf.data() + 1;
}
} // namespace niz
