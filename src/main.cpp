#include "macros/unwrap.hpp"
#include "niz.hpp"
#include "util/fd.hpp"
#include "util/misc.hpp"

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

auto run(const int argc, const char* const argv[]) -> int {
    if(argc < 2) {
        print(usage);
        return false;
    }
    const auto action = std::string_view(argv[1]);
    if(action == "help" || action == "-h" || action == "--help") {
        print(usage);
        return false;
    }

    assert_b(argc >= 3);

    const auto fd = FileDescriptor(open(argv[2], O_RDWR));
    assert_b(fd.as_handle() >= 0, strerror(errno));

    unwrap_ob(version, niz::get_version(fd.as_handle()));
    print("version: ", version);

    if(action == "read-keymap") {
        assert_b(argc == 4);
        auto conf = FileDescriptor(open(argv[3], O_RDWR | O_CREAT | O_TRUNC, 0644));
        assert_b(conf.as_handle() >= 0);
        unwrap_ob(keymap, niz::KeyMap::from_keyboard(fd.as_handle()));
        const auto keymap_str = keymap.to_string();
        assert_b(conf.write(keymap_str.data(), keymap_str.size()));
    } else if(action == "write-keymap") {
        assert_b(argc == 4);
        const auto keymap_txt_r = read_binary<char>(argv[3]);
        assert_b(keymap_txt_r, keymap_txt_r.as_error().cstr());
        const auto keymap_txt = keymap_txt_r.as_value();
        unwrap_ob(keymap, niz::KeyMap::from_string(std::string_view(keymap_txt.data(), keymap_txt.size())));
        assert_b(keymap.write_to_keyboard(fd.as_handle()));
    } else if(action == "flush-firmware") {
        assert_b(argc == 4);
        assert_b(niz::flush_firmware(fd.as_handle(), argv[3]));
    } else if(action == "print-keycounts") {
        assert_b(argc == 3);
        unwrap_ob(counts, niz::read_counts(fd.as_handle()));
        for(const auto c : counts) {
            printf("%u ", c);
        }
        printf("\n");
    } else if(action == "enable-keypress") {
        assert_b(argc == 3);
        assert_b(niz::enable_keypress(fd.as_handle(), true));
    } else if(action == "disable-keypress") {
        assert_b(argc == 3);
        assert_b(niz::enable_keypress(fd.as_handle(), false));
    } else if(action == "initial-calib") {
        assert_b(argc == 3);
        assert_b(niz::do_initial_calibration(fd.as_handle()));
    } else if(action == "press-calib") {
        assert_b(argc == 3);
        assert_b(niz::do_press_calibration(fd.as_handle()));
    } else {
        WARN("unknown action");
        return false;
    }

    print("done");
    return true;
}
} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    return run(argc, argv) ? 0 : 1;
}
