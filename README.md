<h1>ELF file viewer (alpha version)</h1>

<b>Requires:</b>
```
libelf
ncurses-devel
```

<b>How to build (example for the `release` preset):</b>
```
cmake --preset release
cmake --build --preset release
```

Note: the following commands print the available presets:
```
cmake --list-presets
cmake --build --list-presets
```

<b>Usage:</b>
```
./elfibia elf-file
```

<img src="./docs/img/elf-header.png" />

<img src="./docs/img/elf-segments.png" />

<img src="./docs/img/dynamic-section.png" />
