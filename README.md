Grinch
======

Introduction
------------

Grinch is a minimalist operating systems, mainly developed for educational and
testing purposes. Grinch is designed to run on RISC-V platforms. It is able to
run userspace processes (ELF format), and virtual machines. Userland
applications shall have basic POSIX support. Therefore, grinch comes with its
own minimalist libc implementation. For compatibility reasons, the syscall
interface tries to be compatible with Linux as good as possible.

Grinch contains a minimalist Virtual Machine Monitor that is able to boot
itself as virtual machine.

Anyway, don't expect anything to work.

<img src="res/logo.png" width="35%"/>

General features
----------------
- Device Tree support
- Simple scheduler
- ELF Userland applications
- Minimalist driver model
- Minimalist VFS layer
- Virtual Machine Monitor
- GCOV support

Architectural support
---------------------

#### RISC-V architecture
- RISC-V base platform:
  - MMU support
    - SV39 and SV48 paging
    - SV39x and SV48x paging for VMs
  - SMP support
  - RISC-V platform Timers
  - RISC-V PLIC interrupt controller
  - RISC-V SBI console
  - RISC-V H-Extension support
  - SBI support
  - H-Extensions

#### ARM64 architecture
- Work in Progress

#### General driver support
- UART
  - AXI uartlite
  - 8250/16550 UART
  - apbuart

Platform support
----------------
- Supported RISC-V boards:
  - Virtual Target (qemu)
  - Starfive VisionFive 2

- ARM64
  - Ongoing

Build & Installation
--------------------

It is recommended to compile everything on your local machine with a cross
compiler. You will need a cross toolchain and following requirements:

- dtc
- cpio
- qemu-system-riscv64
- riscv64-linux-gnu-
- Python3
- python-pillow

To clone grinch, run:

    git clone https://github.com/lfd/grinch.git
    cd grinch
    git submodule update --init --recursive

For compiling grinch, simply run:

    make

For addition debug output, run:

    make DEBUG=1

For verbose compiler output, run:

    make V=1

This will create `kernel.bin` and `user/initrd.cpio`. `kernel.bin` is the
loadable kernel image, which is directly loadable via Qemu on virtual targets,
or via U-Boot on real platforms. `user/initrd.cpio` contains userland
applications, as well as grinch itself (grinch is able to recursively boot
itself as virtual machine).

Demonstration in QEMU
---------------------

For running grinch inside QEMU, simply run:

    make qemu


For debugging grinch, you can attach with a debugger:

    make qemudb

In a second terminal, run:

    make debug

Grinch is also able to boot on Qemu via U-Boot. For booting grinch via U-Boot
in Qemu, type:

    make qemuu


Grinch Parameters
-----------------

To pass parameters to grinch on real board, you can use the `bootargs` variable
in U-Boot. Grinch parses those parameters. To pass parameters in Qemu, use:

    make qemu CMDLINE='"arg1=val1 arg2=val2"'

Basic features:

| Parameter   | Values | Description                   |
| ---         | ---    | ---                           |
| timer_hz    | int    | Timer frequency               |
| loglevel    | int    | loglevel. Highest=0, Default=1|
| kheap_size  | int    | Kernel Heap size (e.g., 8M)   |
| console     | str    | boot console (ttyS0)          |
| init        | str    | init executable               |

Development features:
| Parameter   | Values | Description                   |
| ---         | ---    | ---                           |
| memtest     | /      | Do memory test                |
| malloc_fsck | /      | Run sanity checker for kalloc |

Authors & License
-----------------

Grinch is developed by Ralf Ramsauer at OTH Regensburg. It is licensed under
GPLv2. Parts of grinch are copied from Linux kernel sources. These parts
involve:
  - ctype implementation
  - List implementation (list.h)
  - Auxiliary headers (compiler_attributes.h, const.h, minmax.h)
  - vsprintf support
  - ELF headers
  - strtox
  - rwonce
