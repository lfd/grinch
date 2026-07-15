CROSS_COMPILE ?= aarch64-none-elf-
PLATFORM ?= virt

# arm64-only build options
build_vars += CONFIG_ARM64_SEMIHOSTING
