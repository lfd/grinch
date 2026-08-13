DRIVERS_OBJS-y := device.o
DRIVERS_OBJS-y += driver.o
DRIVERS_OBJS-y += fb/bochs.o
DRIVERS_OBJS-y += fb/host.o
DRIVERS_OBJS-y += pci/pci.o
DRIVERS_OBJS-$(CONFIG_SBI_TTY) += sbi-tty.o
DRIVERS_OBJS := $(DRIVERS_OBJS-y)

DRIVERS_IRQ_OBJS-y := irq/irqchip.o
DRIVERS_IRQ_OBJS-$(CONFIG_IRQ_APLIC) += irq/riscv/aplic.o
DRIVERS_IRQ_OBJS-$(CONFIG_IRQ_PLIC) += irq/riscv/plic.o
DRIVERS_IRQ_OBJS-$(CONFIG_IRQ_XPLIC) += irq/riscv/xplic.o
DRIVERS_IRQ_OBJS-$(CONFIG_IRQ_GIC_V2) += irq/gic-v2.o
DRIVERS_IRQ_OBJS := $(DRIVERS_IRQ_OBJS-y)

DRIVERS_SERIAL_OBJS-y := serial/chardev.o
DRIVERS_SERIAL_OBJS-y += serial/serial.o
DRIVERS_SERIAL_OBJS-$(CONFIG_UART_8250) += serial/uart-8250.o
DRIVERS_SERIAL_OBJS-$(CONFIG_UART_APBUART) += serial/uart-apbuart.o
DRIVERS_SERIAL_OBJS-$(CONFIG_UART_LITEUART) += serial/uart-liteuart.o
DRIVERS_SERIAL_OBJS-$(CONFIG_UART_UARTLITE) += serial/uart-uartlite.o
DRIVERS_SERIAL_OBJS-$(CONFIG_UART_PL011) += serial/uart-pl011.o
DRIVERS_SERIAL_OBJS := $(DRIVERS_SERIAL_OBJS-y)

DRIVERS_OBJS := $(addprefix drivers/, $(DRIVERS_OBJS))
DRIVERS_IRQ_OBJS := $(addprefix drivers/, $(DRIVERS_IRQ_OBJS))
DRIVERS_SERIAL_OBJS := $(addprefix drivers/, $(DRIVERS_SERIAL_OBJS))

OBJ_DIRS += $(dir $(DRIVERS_OBJS) $(DRIVERS_IRQ_OBJS) $(DRIVERS_SERIAL_OBJS))

drivers/irq/built-in.a: $(DRIVERS_IRQ_OBJS)
drivers/serial/built-in.a: $(DRIVERS_SERIAL_OBJS)
drivers/built-in.a: drivers/irq/built-in.a drivers/serial/built-in.a $(DRIVERS_OBJS)

clean_drivers:
	$(call clean_objects,drivers/serial,$(DRIVERS_SERIAL_OBJS))
	$(call clean_objects,drivers/irq,$(DRIVERS_IRQ_OBJS))
	$(call clean_objects,drivers,$(DRIVERS_OBJS))
