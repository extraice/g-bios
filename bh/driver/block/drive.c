#include <stdio.h>
#include <block.h>
#include <drive.h>
#include <malloc.h>
#include <string.h>

#define MBR_PART_TAB_OFF 0x1BE
#define MSDOS_MAX_PARTS 16

struct msdos_part {
	__u8  boot_flag;
	__u8  chs_start[3];
	__u8  type;
	__u8  chs_end[3];
	__u32 lba_start;
	__u32 lba_size;
};

static struct list_node g_master_list;

// fixme: to support extended partion
static int msdos_part_scan(struct disk_drive *drive, struct part_attr part_tab[])
{
	int i;
	struct msdos_part dos_pt[MSDOS_MAX_PARTS];

	assert(drive != NULL);

	__u8 buff[drive->sect_size];

	drive->get_block(drive, 0, buff);

	memcpy(dos_pt, buff + MBR_PART_TAB_OFF, sizeof(dos_pt));

	for (i = 0; i < 4; i++) {
		if (dos_pt[i].lba_size == 0)
			break;

		// fixme:
		// 1. fix part_type
		// 2. support size large than 4G
		part_tab[i].part_type = PT_NONE;
		part_tab[i].part_name[0] = '\0';

		part_tab[i].part_base = dos_pt[i].lba_start * drive->sect_size;
		part_tab[i].part_size = dos_pt[i].lba_size * drive->sect_size;

		DPRINT("0x%08x - 0x%08x (%dM)\n",
			dos_pt[i].lba_start, dos_pt[i].lba_start + dos_pt[i].lba_size,
			dos_pt[i].lba_size * drive->sect_size >> 20);
	}

	return i;
}

static int drive_get_block(struct disk_drive *drive, int start, void *buff)
{
	struct disk_drive *master = drive->master;

	return master->get_block(master, drive->bdev.bdev_base + start, buff);
}

static int drive_put_block(struct disk_drive *drive, int start, const void *buff)
{
	struct disk_drive *master = drive->master;

	return master->put_block(master, drive->bdev.bdev_base + start, buff);
}

int disk_drive_register(struct disk_drive *drive)
{
	int ret, i, n;
	struct disk_drive *slave;
	struct part_attr part_tab[MSDOS_MAX_PARTS];

	printf("registering disk drive \"%s\":\n", drive->bdev.name);

	ret = block_device_register(&drive->bdev);
	// if ret < 0 ...
	list_head_init(&drive->slave_list);
	list_add_tail(&drive->master_node, &g_master_list);

	n = msdos_part_scan(drive, part_tab);
	// if n < 0 ...

	for (i = 0; i < n; i++) {
		slave = zalloc(sizeof(*slave));
		if (NULL == slave)
			return -ENOMEM;

		snprintf(slave->bdev.name, PART_NAME_LEN, "%sp%d",
			drive->bdev.name, i + 1);

		slave->bdev.bdev_base = part_tab[i].part_base;
		slave->bdev.bdev_size = part_tab[i].part_size;

		slave->sect_size = drive->sect_size;
		slave->master    = drive;
		list_add_tail(&slave->slave_node, &drive->slave_list);

		slave->get_block = drive_get_block;
		slave->put_block = drive_put_block;

		ret = block_device_register(&slave->bdev);
		// if ret < 0 ...
	}

#if 0
	struct list_node *iter;

	printf("%s:", drive->bdev.name);
	list_for_each(iter, &drive->slave_list)
	{
		slave = container_of(iter, struct disk_drive, slave_list);
		printf(" %s", slave->bdev.name);
	}
	printf("\n");
#endif

	return ret;
}

#ifdef CONFIG_HOST_DEMO
int disk_drive_init(void)
#else
static int __INIT__ disk_drive_init(void)
#endif
{
	list_head_init(&g_master_list);
	return 0;
}

#ifndef CONFIG_HOST_DEMO
SUBSYS_INIT(disk_drive_init);
#endif
