#include <fcntl.h>

#include "macros/unwrap.hpp"
#include "niz.hpp"
#include "util/fd.hpp"
#include "util/file-io.hpp"

namespace {
auto usage = R"(Read/Write Keymap from/to keyboard
    niz-kbd-util read-keymap DEVICE CONFIG
    niz-kbd-util write-keymap DEVICE CONFIG

    DEVICE: hidraw device file(e.g. /dev/hidraw0)
    CONFIG: keymap file(.niz)


Flush firmware
    niz-kbd-util flush-firmware DEVICE FIRMWARE

    FIRMWARE: firmware file(.bin)


Print Keycounts
    niz-kbd-util print-keycounts DEVICE


Enable/Disable keypress for calibration
    niz-kbd-util enable-keypress DEVICE
    niz-kbd-util disable-keypress DEVICE


Calibration
    niz-kbd-util initial-calib DEVICE
    niz-kbd-util press-calib DEVICE


Print this help
    niz-kbd-util help
    niz-kbd-util -h
    niz-kbd-util --help)";
} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    if(argc < 2) {
        print(usage);
        return 0;
    }
    const auto action = std::string_view(argv[1]);
    if(action == "help" || action == "-h" || action == "--help") {
        print(usage);
        return 0;
    }

    ensure(argc >= 3);

    const auto fd = FileDescriptor(open(argv[2], O_RDWR));
    ensure(fd.as_handle() >= 0, strerror(errno));

    unwrap(version, niz::get_version(fd.as_handle()));
    print("version: ", version);

    if(action == "read-keymap") {
        ensure(argc == 4);
        auto conf = FileDescriptor(open(argv[3], O_RDWR | O_CREAT | O_TRUNC, 0644));
        ensure(conf.as_handle() >= 0);
        unwrap(keymap, niz::KeyMap::from_keyboard(fd.as_handle()));
        const auto keymap_str = keymap.to_string();
        ensure(conf.write(keymap_str.data(), keymap_str.size()));
    } else if(action == "write-keymap") {
        ensure(argc == 4);
        unwrap(keymap_txt, read_file(argv[3]));
        unwrap(keymap, niz::KeyMap::from_string(std::string_view((char*)keymap_txt.data(), keymap_txt.size())));
        ensure(keymap.write_to_keyboard(fd.as_handle()));
    } else if(action == "flush-firmware") {
        ensure(argc == 4);
        ensure(niz::flush_firmware(fd.as_handle(), argv[3]));
    } else if(action == "print-keycounts") {
        ensure(argc == 3);
        unwrap(counts, niz::read_counts(fd.as_handle()));
        for(const auto c : counts) {
            printf("%u ", c);
        }
        printf("\n");
    } else if(action == "enable-keypress") {
        ensure(argc == 3);
        ensure(niz::enable_keypress(fd.as_handle(), true));
    } else if(action == "disable-keypress") {
        ensure(argc == 3);
        ensure(niz::enable_keypress(fd.as_handle(), false));
    } else if(action == "initial-calib") {
        ensure(argc == 3);
        ensure(niz::do_initial_calibration(fd.as_handle()));
    } else if(action == "press-calib") {
        ensure(argc == 3);
        ensure(niz::do_press_calibration(fd.as_handle()));
    } else {
        bail("unknown action");
    }

    print("done");
    return 0;
}
