#include <unistd.h>

#include "common.hpp"
#include "macros/unwrap.hpp"
#include "util/charconv.hpp"
#include "util/misc.hpp"

namespace niz {
auto flush_firmware(int fd, std::string_view firmware_path) -> bool {
    unwrap_re(bin, read_binary<char>(firmware_path));
    auto firmware = std::vector<std::vector<uint8_t>>();
    // validate virmware
    for(auto i = 0u; i < bin.size(); i += 1) {
        assert_b(bin[i] == PacketType::Firmware); // each line should begin with 0x3a(':')
        i += 1;

        const auto begin = i;
        for(; bin[i] != '\n'; i += 1) {
        }
        const auto len = i - begin - 1;
        assert_b(len / 2 <= 63);
        assert_b(len % 2 == 0);

        auto       part = std::vector<uint8_t>(len / 2);
        const auto line = std::string_view(&bin[begin], len);
        // print(line);
        for(auto i = 0u; i < len; i += 2) {
            const auto byte_str = line.substr(i, 2);
            unwrap_ob(byte, from_chars<uint8_t>(byte_str, 16));
            part[i / 2] = byte;
        }
        firmware.emplace_back(std::move(part));
    }

    print("sending firmware");
    for(auto i = 0u; i < firmware.size(); i += 1) {
        auto& part = firmware[i];
        auto  buf  = std::array<uint8_t, 65>();
        buf[0]     = 0; // report id
        buf[1]     = 0; // Packet::unknown1
        buf[2]     = PacketType::Firmware;
        memcpy(&buf[3], part.data(), part.size());
        print(i + 1, "/", firmware.size(), " ", part.size(), " bytes");
        assert_b(write(fd, buf.data(), buf.size()) == buf.size());
    }
    return true;
}
} // namespace niz
