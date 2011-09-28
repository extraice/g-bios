#include <irq.h>

#ifdef CONFIG_IRQ_SUPPORT

static struct int_pin irq_pin_set[MAX_IRQ_NUM];

static int handle_dev_irq_list(u32 irq, struct irq_dev *dev_list);

int irq_register_isr(u32 irq, IRQ_DEV_HANDLER isr, void *dev)
{
	struct irq_dev *idev, **p;
	struct int_pin *pin;

	if (irq >= MAX_IRQ_NUM)
		return -EINVAL;

	pin = irq_pin_set + irq;

	idev = malloc(sizeof(struct irq_dev));
	if (!idev)
		return -ENOMEM;

	idev->device  = dev;
	idev->dev_isr = isr;
	idev->next    = NULL;

	p = &pin->dev_list;
	while (*p)
	{
		p = &(*p)->next;
	}

	*p = idev;

	pin->intctrl->umask(irq);

	return 0;
}

void irq_handle(u32 irq)
{
	struct int_pin *pin;

	BUG_ON(irq >= MAX_IRQ_NUM);

	// printf("%s(): irq = %d\n", __func__, irq);

	pin = irq_pin_set + irq;

	pin->irq_handle(pin, irq);

#if defined(CONFIG_AT91SAM9261) || defined(CONFIG_AT91SAM9263)
	at91_aic_writel(AIC_EOICR, 1);
#endif
}

///////////

static void default_mack(u32 irq)
{
	struct int_pin *pin = irq_pin_set + irq;

	pin->intctrl->mask(irq);
	pin->intctrl->ack(irq);
}

int irq_assoc_intctl(u32 irq, struct int_ctrl *intctrl)
{
	struct int_pin *pin = irq_pin_set + irq;

	if (NULL == intctrl->mack)
	{
		intctrl->mack = default_mack;
	}

	pin->intctrl = intctrl;

	return 0;
}

int irq_set_trigger(u32 irq, u32 type)
{
	struct int_pin *pin;

	pin = irq_pin_set + irq;

	return pin->intctrl->set_trigger(irq, type);
}

// fixme
void irq_handle_level(struct int_pin *pin, u32 irq)
{
	struct irq_dev *dev_list = pin->dev_list;

	pin->intctrl->mack(irq);

	BUG_ON(NULL == dev_list);

	handle_dev_irq_list(irq, dev_list);

	pin->intctrl->umask(irq);
}

//Don't support vector interrupt :)
void vectorirq_handle_level(struct int_pin *pin, u32 irq)
{
	struct irq_dev *dev_list = pin->dev_list;

	pin->intctrl->mask(irq);

	BUG_ON(NULL == dev_list);

	handle_dev_irq_list(irq, dev_list);

	pin->intctrl->ack(irq);

	pin->intctrl->umask(irq);
}

void irq_handle_edge(struct int_pin *pin, u32 irq)
{
	struct irq_dev *dev_list = pin->dev_list;

	pin->intctrl->ack(irq);

	BUG_ON(NULL == dev_list);

	handle_dev_irq_list(irq, dev_list);
}

void irq_handle_simple(struct int_pin *pin, u32 irq)
{
	struct irq_dev *dev_list = pin->dev_list;

	BUG_ON(NULL == dev_list);

	handle_dev_irq_list(irq, dev_list);
}

void irq_set_handler(u32 irq, IRQ_PIN_HANDLER irq_handle, int chain_flag)
{
	struct int_pin *pin;

	pin = irq_pin_set + irq;

	pin->irq_handle = irq_handle;

	if (chain_flag)
	{
		pin->intctrl->umask(irq);
	}
}

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

#else
u32 read_irq_num(void)
{
	BUG();
}

void irq_handle(u32 irq)
{
}

int irq_set_trigger(u32 irq, u32 type)
{
	return -EINVAL;
}

int irq_register_isr(u32 irq, IRQ_DEV_HANDLER isr, void *dev)
{
	return -EINVAL;
}

#endif