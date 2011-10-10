#include <irq.h>

int read_irq_num(void)
{
	int irq_num;
	u32 val, shift;

	irq_num = readl(VA(INTCPS_BASE + INTCPS_SIR_IRQ)) & 0x7f;
	if (irq_num >= 29 && irq_num <= 34)
	{
		val = readl(VA(GPIO_IRQ_STATUS1(irq_num - 29)));
		shift = 0;
		while (shift < 32)
		{
			if ((0x1UL << shift) & val)
			{
				break;
			}
			shift++;
		}
		irq_num = INTC_PINS + shift;
	}

//	printf("irq_num %d\n", irq_num);

	return irq_num;
}

static void omap3530_irq_umask(u32 irq_num)
{
	switch (irq_num)
	{
	case 20:
		// GPMC INT
		break;

	case GPIO_IRQ(0) ... GPIO_IRQ(191):
		irq_num -= INTC_PINS;
		writel(VA(GPIO_IRQ_SETENABLE1(irq_num >> 5)), 0x1 << (irq_num & 0x1f));
		irq_num = 29 + (irq_num >> 5);
		break;

		// other condition
	default:
		break;
	}

	writel(VA(INTCPS_BASE + INTCPS_MIRN_CLEN(irq_num >> 5)), 1 << (irq_num & 0x1f));
}

static void omap3530_irq_mask(u32 irq)
{
	writel(VA(INTCPS_BASE + INTCPS_MIRN_SETN(irq >> 5)), 1 << (irq & 0x1f));
}

static void omap3530_set_trigger(u32 irq_num, u32 irq_type)
{
	if (irq_num < INTC_PINS)
		return;

	switch (irq_type)
	{
	case IRQ_TYPE_LOW:
		break;
	case IRQ_TYPE_HIGH:
		break;
	case IRQ_TYPE_FALLING:
		break;
	case IRQ_TYPE_RISING:
		break;
	default:
		break;
	}
}

static struct int_ctrl omap3530_intctl =
{
	.mask  = omap3530_irq_mask,
	.umask = omap3530_irq_umask,
};

static int handle_dev_irq_list(u32 irq, struct irq_dev *idev)
{
	int retval = IRQ_NONE;

	do
	{
		int ret;

		ret = idev->dev_isr(irq, idev->device);
		retval |= ret;

		idev = idev->next;
	} while (idev);

	return retval;
}

static void omap3530_handle(struct int_pin *pin, u32 irq)
{
	u32 irq_status;
	struct irq_dev *dev_list = pin->dev_list;

	BUG_ON(NULL == dev_list);

	handle_dev_irq_list(irq, dev_list);

	switch (irq)
	{
		case 20:
			// GPMC INT
			irq_status = readl(VA(GPMC_BASE + GPMC_IRQ_STATUS));
			writel(VA(GPMC_BASE + GPMC_IRQ_STATUS), irq_status);
			break;

		case GPIO_IRQ(0) ...  GPIO_IRQ(191):
			irq = (irq - INTC_PINS) >> 5;
			irq_status = readl(VA(GPIO_IRQ_STATUS1(irq)));
			writel(VA(GPIO_IRQ_STATUS1(irq)), irq_status);
			break;

		default:
			break;
	}

	writel(VA(INTCPS_BASE + INTCPS_CONTROL), 0x1);
}

int omap3530_irq_init(void)
{
	int irq_num;

	// reset the inc
	writel(VA(INTCPS_BASE + INTCPS_SYSCONFIG), 0x1 << 1);
	while (readl(VA(INTCPS_BASE + INTCPS_SYSCONFIG)) & (0x1 << 1));

	writel(VA(INTCPS_BASE + INTCPS_SYSCONFIG), 0x1);

	for (irq_num = 0; irq_num < MAX_IRQ_NUM; irq_num++)
	{
		irq_assoc_intctl(irq_num, &omap3530_intctl);
		irq_set_handler(irq_num, omap3530_handle, 0);
		if (irq_num < INTC_PINS)
		{
			writel(VA(INTCPS_BASE + INTCPS_ILRM(irq_num)), 0x0);
		}
	}

	return 0;
}
