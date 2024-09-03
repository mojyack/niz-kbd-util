#include <unistd.h>

#include "common.hpp"
#include "macros/unwrap.hpp"
#include "util/charconv.hpp"
#include "util/file-io.hpp"

namespace niz {
auto flush_firmware(int fd, const char* const firmware_path) -> bool {
    unwrap(bin, read_file(firmware_path));
    auto firmware = std::vector<std::vector<uint8_t>>();
    // validate virmware
    for(auto i = 0u; i < bin.size(); i += 1) {
        ensure(bin[i] == std::byte(PacketType::Firmware)); // each line should begin with 0x3a(':')
        i += 1;

        const auto begin = i;
        for(; bin[i] != std::byte('\n'); i += 1) {
        }
        const auto len = i - begin - 1;
        ensure(len / 2 <= 63);
        ensure(len % 2 == 0);

        auto       part = std::vector<uint8_t>(len / 2);
        const auto line = std::string_view((char*)&bin[begin], len);
        // print(line);
        for(auto i = 0u; i < len; i += 2) {
            const auto byte_str = line.substr(i, 2);
            unwrap(byte, from_chars<uint8_t>(byte_str, 16));
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
        ensure(write(fd, buf.data(), buf.size()) == buf.size());
    }
    return true;
}
} // namespace niz
