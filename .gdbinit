#add-symbol-file vmgrinch.elf
add-symbol-file kernel.elf
layout split
fs cmd
target remote :1234
