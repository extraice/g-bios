#include <g-bios.h>
#include <flash/nand.h>
#include <arm/s3c6410.h>

#define s3c6410_nfc_readl(reg) \
	readl(NAND_CTRL_BASE + reg)

#define s3c6410_nfc_writel(reg, val) \
	writel(NAND_CTRL_BASE + reg, val)

//
static int s3c6410_nand_ready(struct nand_chip *nand)
{
	return s3c6410_nfc_readl(NFSTAT) & 0x1;
}

int nand_init(struct nand_chip *nand)
{
	s3c6410_nfc_writel(NFCONF, 0 << 12 | 2 << 8 | 1 << 4 | 0x4);
	s3c6410_nfc_writel(NFCONT, 0x1);

	nand->cmmd_port = NAND_CTRL_BASE + NFCMMD;
	nand->addr_port = NAND_CTRL_BASE + NFADDR;
	nand->data_port = NAND_CTRL_BASE + NFDATA;
	nand->flash_ready = s3c6410_nand_ready;

	return nand_probe(nand);
}
