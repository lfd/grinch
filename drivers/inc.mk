DRIVERS_OBJS = device.o
DRIVERS_OBJS += driver.o
DRIVERS_OBJS += fb/bochs.o
DRIVERS_OBJS += pci/pci.o

DRIVERS_IRQ_OBJS = irq/irqchip.o

DRIVERS_SERIAL_OBJS = serial/chardev.o
DRIVERS_SERIAL_OBJS += serial/serial.o
DRIVERS_SERIAL_OBJS += serial/uart-dummy.o
DRIVERS_SERIAL_OBJS += serial/uart-8250.o
DRIVERS_SERIAL_OBJS += serial/uart-apbuart.o

ifeq ($(ARCH),riscv)
	DRIVERS_OBJS += sbi-tty.o
	DRIVERS_SERIAL_OBJS += serial/uart-uartlite.o

	DRIVERS_IRQ_OBJS += irq/riscv/aplic.o
	DRIVERS_IRQ_OBJS += irq/riscv/plic.o
	DRIVERS_IRQ_OBJS += irq/riscv/xplic.o
endif

DRIVERS_OBJS := $(addprefix drivers/, $(DRIVERS_OBJS))
DRIVERS_IRQ_OBJS := $(addprefix drivers/, $(DRIVERS_IRQ_OBJS))
DRIVERS_SERIAL_OBJS := $(addprefix drivers/, $(DRIVERS_SERIAL_OBJS))

drivers/irq/built-in.a: $(DRIVERS_IRQ_OBJS)
drivers/serial/built-in.a: $(DRIVERS_SERIAL_OBJS)

drivers/built-in.a: drivers/irq/built-in.a drivers/serial/built-in.a $(DRIVERS_OBJS)

clean_drivers:
	$(call clean_objects,drivers/serial,$(DRIVERS_SERIAL_OBJS))
	$(call clean_objects,drivers/irq,$(DRIVERS_IRQ_OBJS))
	$(call clean_objects,drivers,$(DRIVERS_OBJS))
