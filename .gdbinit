add-symbol-file vmgrinch.elf
add-symbol-file kernel.elf
add-symbol-file user/user.elf
layout split
fs cmd
target remote :1234
