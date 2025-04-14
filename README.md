# kri
<!--[![C/C++ CI](https://github.com/ikozyris/kri/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/ikozyris/kri/actions/workflows/c-cpp.yml)-->
A simple, compact and *fast* text editor using ncurses and a gap buffer, written in C++, in ~1.6k lines of code.
More information is available on the [wiki](https://github.com/ikozyris/kri/wiki).

![A C++ file in kri](https://github.com/user-attachments/assets/7d221564-da5e-41de-a63b-ba5e31c257d1)


## Install
The 0.8.2 release has been packaged for:
 - Arch Linux: [AUR package](https://aur.archlinux.org/packages/kri/).
 - Debian/Ubuntu: [deb package](https://github.com/ikozyris/kri/releases/download/v0.8.2/kri_0.8.2_x64-v2.deb)

## Build
```sh
make build
sudo make install
```

Or use the user-friendly dialog utility `wizard.sh`
which also supports configuring kri

## Usage
```sh
kri text.txt # open existing file or create if it doesn't exist
kri --help # show help page
kri # ask for filename on save file operation
```

### Keybindings
* Save: Ctrl-S
* Exit: Ctrl-X
* Go to start of line: Ctrl-A
* Go to end of line: Ctrl-E
* Open other file Alt-R
* Delete line: Ctrl-K
* Go to previous/next word: Shift + Left/Right arrow
* Enter built-in terminal: Alt-C
* Show info: Alt-I (also command _stats_ in built-in terminal)
* Search: command _find_ in builtin terminal, parameters in seperate(\n) queries, example:
	- find str>
	- find 1-5 c (\n) str  --> count occurences of string on lines [1,5]
	- find 3-10 h (\n) str  --> highlight occurences of sring on lines [3,10]
* Replace: command _replace_ :
	- replace (\n) str1 (\n) str2  -->  replace all str1 with str2
	- replace 0-20 (\n) str1 (\n) str2 (\n)  -->  in range [0,20]

### How fast is it?
kri is several times faster than any other text editor at reading files, searching, and other operations.
See the [benchmarks](https://github.com/ikozyris/kri/wiki/Performance-&-Benchmarks) for more.

### License

Copyright (C) 2025  ikozyris

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

