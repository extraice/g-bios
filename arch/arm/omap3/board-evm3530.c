#include <io.h>
#include <irq.h>
#include <init.h>
#include <board.h>
#include <platform.h>

#ifdef CONFIG_LAN9220
static int __init lan9220_device_init(struct platform_device *pdev)
{
	__u16 val;

	writew(VA(0x480020B8), 1 << 4 | 1 << 3 | 0);

	writel(VA(0x6E000060 + 5 * 0x30 + 0x00), 0x00001000);
	writel(VA(0x6E000060 + 5 * 0x30 + 0x04), 0x001e1e01);
	writel(VA(0x6E000060 + 5 * 0x30 + 0x08), 0x00080300);
	writel(VA(0x6E000060 + 5 * 0x30 + 0x0c), 0x1c091c09);
	writel(VA(0x6E000060 + 5 * 0x30 + 0x10), 0x04181f1f);
	writel(VA(0x6E000060 + 5 * 0x30 + 0x14), 0x00000fcf);
	writel(VA(0x6E000060 + 5 * 0x30 + 0x18), 0x00000f68);

	val = readw(VA(PADCONF_GPMC_NWE));
	val |= 0x0E00;
	writew(VA(PADCONF_GPMC_NWE), val);

	val = readw(VA(PADCONF_GPMC_NOE));
	val |= 0x0E00;
	writew(VA(PADCONF_GPMC_NOE), val);

	val = readw(VA(PADCONF_GPMC_NADV_ALE));
	val |= 0x0E00;
	writew(VA(PADCONF_GPMC_NADV_ALE), val);

	return 0;
}

static struct platform_device lan9220_device = {
	.dev = {
		.mem = 0x28000000,
		.irq = GPIO_IRQ(19),
	},
	.name = "SMSC LAN9220",
	.init = lan9220_device_init,
};
#endif

// fixme: __INITDATA__
static struct platform_device *evm3530_devices[] = {
#ifdef CONFIG_LAN9220
	&lan9220_device,
#endif
};

static int __init evm3530_init(struct board_desc *board)
{
	int i, ret;

	for (i = 0; i < ARRAY_ELEM_NUM(evm3530_devices); i++) {
		ret = platform_device_register(evm3530_devices[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

BOARD_DESC("evm3530", 1535, evm3530_init);
