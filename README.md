# PreLoader

A library preloader for loading LeviLamina

This is the library preloader for [LeviLamina](https://github.com/LiteLDev/LeviLamina). It loads LeviLamina before game starts and provides MCAPIs required by LeviLamina and various mods. We uses windows delay load to expose and remap original symbol to our own symbol, and then we will provide them to mods.

Due to the requirements of the copyright holder, this software has been closed-source since v1.10.0, if you have any problems, feel free to file an issue.

## Install

```sh
lip install github.com/LiteLDev/PreLoader
```

## Usage

This is one of the prerequisites for using LeviLamina. You need to install this library preloader before using LeviLamina.

## Used Projects

| Project          | License    | Link                                                             |
| ---------------- | ---------- | ---------------------------------------------------------------- |
| pe_bliss         | Special    | <https://code.google.com/archive/p/portable-executable-library/> |
| fmt              | MIT        | <https://github.com/fmtlib/fmt>                                  |
| nlohmann_json    | MIT        | <https://github.com/nlohmann/json>                               |
| parallel-hashmap | Apache-2.0 | <https://github.com/greg7mdp/parallel-hashmap>                   |
| detours          | MIT        | <https://github.com/microsoft/Detours>                           |
| asmjit           | Zlib       | <https://github.com/asmjit/asmjit>                               |

## License

Copyright © 2024 LeviMC, All rights reserved.

Refer to EULA for more information.
