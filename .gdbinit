add-symbol-file vmgrinch.elf
add-symbol-file kernel.elf
add-symbol-file user/apps/init/init.echse
add-symbol-file res/u-boot/u-boot/u-boot
layout split
fs cmd
target remote :1234
