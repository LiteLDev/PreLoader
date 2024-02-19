# PreLoader

A library preloader for loading LeviLamina

This is the library preloader for [LeviLamina](https://github.com/LiteLDev/LeviLamina). It loads LeviLamina before BDS starts and provides MCAPIs required by LeviLamina and various plugins. We uses windows delay load to expose and remap original bds symbol to our own symbol, and then we will provide them to plugins.

## Install

```sh
lip install github.com/LiteLDev/PreLoader
```

## Usage

This is one of the prerequisites for using LeviLamina. You need to install this library preloader before using LeviLamina.

## Contributing

Ask questions by creating an issue.

PRs accepted.

## License

LGPL-3.0-or-later © LiteLDev
