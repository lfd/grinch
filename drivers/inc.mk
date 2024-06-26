DRIVERS_OBJS = driver.o
DRIVERS_IRQ_OBJS = irq/irqchip.o
DRIVERS_SERIAL_OBJS = serial/chardev.o serial/serial.o serial/uart-dummy.o
DRIVERS_SERIAL_OBJS += serial/uart-8250.o serial/uart-apbuart.o

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
	$(RMRF) $(DRIVERS_OBJS)
	$(RMRF) $(DRIVERS_SERIAL_OBJS)
	$(RMRF) $(DRIVERS_IRQ_OBJS)
	$(RMRF) drivers/irq/built-in.a drivers/serial/built-in.a drivers/built-in.a
