# Niz Keyboard Utility 
Open source alternative of Niz keyboard softwares  
Currently supports Linux

# Implemented Features 
- Read keymap from keyboard to file
- Write keymap from file to keyboard
- Update firmware
- Get keycounts
- Do calibration

# Build
```
git clone --recursive https://github.com/mojyack/niz-kbd-util.git
cd niz-kbd-util
meson setup --buildtype=release build
ninja -C build
```

# Usage
After connecting the keyboard to the PC, run `scripts/find-hidraw.sh` to check the device name.  
For command options, run `niz-kbd-util help`.  
For the format of the keymap file, read `configs/example.niz`.

# Credits
Reference for the protocol:  
https://github.com/cho45/niz-tools-ruby
