// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2024 Ventana Micro Systems
 *
 * Anup Patel <apatel@ventanamicro.com>
 * Mayuresh Chitale <mchitale@ventanamicro.com>
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//#include <vt1/vt1_pcl_hal.h>
#include <mmc.h>

/* ==== HAL functions expected by the library ==== */
#define GB	(1024*1024*1024ULL)
#define SD_CARD_SIZE (32*GB)
#define MMC_DEV_NUM 0
#define HAL_BLOCK_SIZE	512

struct blk_desc *blk_dev = NULL;
unsigned long long hal_num_blocks(void);
int hal_read_block(unsigned long lba, void *buf);
int hal_load_dram(unsigned long offset, unsigned long total, void *buf, size_t buf_len);

#define swea(lladdr, data) ({  __asm__ __volatile__ ( \
		"swea\t%0,%M1,%L1\n" :: "d" (data), "d" (lladdr) \
		); \
	})

#define sbea(lladdr, data) ({  __asm__ __volatile__ ( \
		"sbea\t%0,%M1,%L1\n" :: "d" (data), "d" (lladdr) \
		); \
	})

/* ==== Actual library ==== */

struct gpt_mbr {
	uint8_t		boot_indicator;
	uint8_t		starting_chs[3];
	uint8_t		os_type;
	uint8_t		ending_chs[3];
	uint32_t	starting_lba;
	uint32_t	ending_lba;
	uint8_t		reserved[HAL_BLOCK_SIZE - 0x10];
};

struct gpt_header {
#define GPT_HEADER_SIGNATURE	{ 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 }
	uint8_t		signature[8];
	uint32_t	gpt_version;
	uint32_t	header_size;
	uint32_t	header_crc32;
	uint32_t	reserved1;
	uint64_t	header_lba;
	uint64_t	header_alt_lba;
	uint64_t	first_usable_lba;
	uint64_t	last_usable_lba;
	uint8_t		disk_guid[16];
	uint64_t	part_table_lba;
	uint32_t	part_table_num_entries;
	uint32_t	part_table_entry_size;
	uint32_t	part_table_crc32;
	uint8_t		reserved2[HAL_BLOCK_SIZE - 0x5c];
};

struct gpt_part_entry {
/* EFI System Partition (ESP) Type GUID: C12A7328-F81F-11D2-BA4B-00A0C93EC93B */
#define GPT_PART_TYPE_GUID_ESP	\
{ 0x28, 0x73, 0x2a, 0xc1, \
  0x1f, 0xf8, \
  0xd2, 0x11, \
  0xba, 0x4b, \
  0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b }
/* Bios Boot Partition Type GUID: 21686148-6449-6E6F-744E-656564454649 */
#define GPT_PART_TYPE_GUID_BIOS_BOOT	\
{ 0x48, 0x61, 0x68, 0x21, \
  0x49, 0x64, \
  0x6f, 0x6e, \
  0x4e, 0x74, \
  0x65, 0x65, 0x64, 0x45, 0x46, 0x49 }
	uint8_t		part_type_guid[16];
	uint8_t		part_guid[16];
	uint64_t	starting_lba;
	uint64_t	ending_lba;
	uint64_t	attributes;
	uint8_t		name[72];
};

#define GPT_PART_ENTRY_PER_BLOCK	(HAL_BLOCK_SIZE / sizeof(struct gpt_part_entry))

int gpt_read_header(struct gpt_header *out)
{
	uint8_t	sig[] = GPT_HEADER_SIGNATURE;
	int ret;

	ret = hal_read_block(1, out);
	if (ret)
		return ret;

	if (memcmp(out->signature, sig, sizeof(out->signature)))
		return -1;
	if (out->part_table_entry_size != sizeof(struct gpt_part_entry))
		return -1;

	return 0;
}

int gpt_find_part_entry(const struct gpt_header *hdr,
			const void *type_guid, const void *guid,
			struct gpt_part_entry *out)
{
	struct gpt_part_entry *ent, entries[GPT_PART_ENTRY_PER_BLOCK];
	uint8_t	empty_guid[16] = { 0 };
	int ret, i = 0, j, num;

	while (i < hdr->part_table_num_entries) {
		num = hdr->part_table_num_entries - i;
		num = (num < GPT_PART_ENTRY_PER_BLOCK) ? num : GPT_PART_ENTRY_PER_BLOCK;

		ret = hal_read_block(hdr->part_table_lba + i / GPT_PART_ENTRY_PER_BLOCK,
				     entries);
		if (ret)
			return ret;

		for (j = 0; j < num; j++) {
			ent = &entries[j];
			if (!memcmp(ent->part_type_guid, empty_guid, sizeof(empty_guid)))
				continue;

			if (type_guid && !guid &&
			    !memcmp(ent->part_type_guid, type_guid, sizeof(empty_guid))) {
				memcpy(out, ent, sizeof(*ent));
				return 0;
			}

			if (!type_guid && guid &&
			    !memcmp(ent->part_guid, guid, sizeof(empty_guid))) {
				memcpy(out, ent, sizeof(*ent));
				return 0;
			}

			if (type_guid && guid &&
			    !memcmp(ent->part_type_guid, type_guid, sizeof(empty_guid)) &&
			    !memcmp(ent->part_guid, guid, sizeof(empty_guid))) {
				memcpy(out, ent, sizeof(*ent));
				return 0;
			}
		}

		i += num;
	}

	return -1;
}

int gpt_find_boot_partition(struct gpt_part_entry *out)
{
	uint8_t bios_boot_guid[16] = GPT_PART_TYPE_GUID_BIOS_BOOT;
	uint8_t esp_guid[16] = GPT_PART_TYPE_GUID_ESP;
	struct gpt_header hdr;
	int ret = 0;

	ret = gpt_read_header(&hdr);
	if (ret)
		return ret;

	ret = gpt_find_part_entry(&hdr, esp_guid, NULL, out);
	if (ret) {
		ret = gpt_find_part_entry(&hdr, bios_boot_guid, NULL, out);
		if (ret)
			return ret;
	}

	return 0;
}

unsigned long gpt_partition_num_blocks(struct gpt_part_entry *part)
{
	return part->ending_lba - part->starting_lba + 1;
}

int gpt_partition_read_block(struct gpt_part_entry *part, unsigned long block, void *buf)
{
	if (gpt_partition_num_blocks(part) <= block)
		return -1;

	return hal_read_block(part->starting_lba + block, buf);
}

struct fat_boot_sector {
	uint8_t		ignored[3];	/* Bootstrap code */
	char		system_id[8];	/* Name of fs */
	uint8_t		sector_size[2];	/* Bytes/sector */
	uint8_t		cluster_size;	/* Sectors/cluster */
	uint16_t	reserved;	/* Number of reserved sectors */
	uint8_t		fats;		/* Number of FATs */
	uint8_t		dir_entries[2];	/* Number of root directory entries */
	uint8_t		sectors[2];	/* Number of sectors */
	uint8_t		media;		/* Media code */
	uint16_t	fat_length;	/* Sectors/FAT */
	uint16_t	secs_track;	/* Sectors/track */
	uint16_t	heads;		/* Number of heads */
	uint32_t	hidden;		/* Number of hidden sectors */
	uint32_t	total_sect;	/* Number of sectors (if sectors == 0) */

	/* FAT32 only */
	uint32_t	fat32_length;	/* Sectors/FAT */
	uint16_t	flags;		/* Bit 8: fat mirroring, low 4: active fat */
	uint8_t		version[2];	/* Filesystem version */
	uint32_t	root_cluster;	/* First cluster in root directory */
	uint16_t	info_sector;	/* Filesystem info sector */
	uint16_t	backup_boot;	/* Backup boot sector */
	uint16_t	reserved2[6];	/* Unused */
};

struct fat_volume_info {
	uint8_t drive_number;	/* BIOS drive number */
	uint8_t reserved;		/* Unused */
	uint8_t ext_boot_sign;	/* 0x29 if fields below exist (DOS 3.3+) */
	uint8_t volume_id[4];	/* Volume ID number */
	char volume_label[11];	/* Volume label */
	char fs_type[8];	/* Typically FAT12, FAT16, or FAT32 */
	/* Boot code comes next, all but 2 bytes to fill up sector */
	/* Boot sign comes last, 2 bytes */
};

#define FATBUFBLOCKS		2
#define FATBUFSIZE(__fsdata)	((__fsdata)->sect_size * FATBUFBLOCKS)
#define FAT12BUFSIZE(__fsdata)	((FATBUFSIZE(__fsdata) * 2) / 3)
#define FAT16BUFSIZE(__fsdata)	(FATBUFSIZE(__fsdata) / 2)
#define FAT32BUFSIZE(__fsdata)	(FATBUFSIZE(__fsdata) / 4)

#define FAT2CPU32(__x)		(__x)
#define FAT2CPU16(__x)		(__x)

struct fat_fsdata {
	uint8_t		tmpbuf[HAL_BLOCK_SIZE];
	uint8_t		fatbuf[FATBUFBLOCKS * HAL_BLOCK_SIZE];	/* Current FAT buffer */
	int		fatsize;	/* Size of FAT in bits */
	uint32_t	fatlength;	/* Length of FAT in sectors */
	uint16_t	fat_sect;	/* Starting sector of the FAT */
	uint8_t		fat_dirty;	/* Set if fatbuf has been modified */
	uint32_t	rootdir_sect;	/* Start sector of root directory */
	uint16_t	sect_size;	/* Size of sectors in bytes */
	uint16_t	clust_size;	/* Size of clusters in sectors */
	int		data_begin;	/* The sector of the first cluster, can be negative */
	int		fatbufnum;	/* Used by get_fatent, init to -1 */
	int		rootdir_size;	/* Size of root dir for non-FAT32 */
	uint32_t	root_cluster;	/* First cluster of root dir for FAT32 */
	uint32_t	total_sect;	/* Number of sectors */
	int		fats;		/* Number of FATs */
	struct gpt_part_entry *part;
};

/* Filesystem identifiers */
#define FAT12_SIGN		"FAT12   "
#define FAT16_SIGN		"FAT16   "
#define FAT32_SIGN		"FAT32   "
#define FAT_SIGNLEN		8

#define FAT_MAX_CLUSTSIZE	65536

#define FAT_LFN_MAXLEN		128
#define FAT_LFN_LASTSEQ_MASK	0x40
#define FAT_LFN_SEQNO(s)	((s) & ~0x40)
#define FAT_LFN_LASTSEQ(s) 	((s) & 0x40)
#define FAT_LFN_MINSEQ		1
#define FAT_LFN_MAXSEQ		(FAT_LFN_MAXLEN / 13)

struct fat_dir_entry {
	uint8_t			name[8];	/* Name and extension */
	uint8_t			extn[3];	/* Name and extension */
#define FAT_DIR_ATTR_READONLY	0x01
#define FAT_DIR_ATTR_HIDDEN	0x02
#define FAT_DIR_ATTR_SYSTEM	0x04
#define FAT_DIR_ATTR_VOLUME_ID	0x08
#define FAT_DIR_ATTR_DIRECTORY	0x10
#define FAT_DIR_ATTR_ARCHIVE	0x20
#define FAT_DIR_ATTR_LFN	(FAT_DIR_ATTR_READONLY | \
				 FAT_DIR_ATTR_HIDDEN | \
				 FAT_DIR_ATTR_SYSTEM | \
				 FAT_DIR_ATTR_VOLUME_ID)
	uint8_t			attr;		/* Attribute bits */
	uint8_t			lcase;		/* Case for name and ext (CASE_LOWER_x) */
	uint8_t			ctime_ms;	/* Creation time, milliseconds */
	uint16_t		ctime;		/* Creation time */
	uint16_t		cdate;		/* Creation date */
	uint16_t		adate;		/* Last access date */
	uint16_t		starthi;	/* High 16 bits of cluster in FAT32 */
	uint16_t		time;		/* Time */
	uint16_t		date;		/* Date */
	uint16_t		start;		/* First cluster */
	uint32_t		size;		/* File size in bytes */
};

struct fat_dir_slot {
	uint8_t		id;		/* Sequence number for slot */
	uint8_t		name0_4[10];	/* First 5 characters in name */
	uint8_t		attr;		/* Attribute byte */
	uint8_t		reserved;	/* Unused */
	uint8_t		alias_checksum;	/* Checksum for 8.3 alias */
	uint8_t		name5_10[12];	/* 6 more characters in name */
	uint16_t	start;		/* Unused */
	uint8_t		name11_12[4];	/* Last 2 characters in name */
};

struct fat_file {
	int		is_directory;
	uint8_t		lname[FAT_LFN_MAXLEN];
	uint32_t	first_cluster;
	uint32_t	size;
};

#define IS_LAST_CLUST(x, fatsize) ((x) >= ((fatsize) != 32 ? \
					((fatsize) != 16 ? 0xff8 : 0xfff8) : \
					0xffffff8))
#define CHECK_CLUST(x, fatsize) ((x) <= 1 || \
				(x) >= ((fatsize) != 32 ? \
					((fatsize) != 16 ? 0xff0 : 0xfff0) : \
					0xffffff0))

static inline uint8_t fat_tolower(uint8_t c)
{
	if ('A' <= c && c <= 'Z')
		c -= 'A'-'a';
	return c;
}

static uint32_t fat_get_entry(struct fat_fsdata *fsdata, uint32_t entry)
{
	uint32_t bufnum;
	uint32_t offset, off8;
	uint32_t ret = 0x00;

	if (CHECK_CLUST(entry, fsdata->fatsize))
		return ret;

	switch (fsdata->fatsize) {
	case 32:
		bufnum = entry / FAT32BUFSIZE(fsdata);
		offset = entry - bufnum * FAT32BUFSIZE(fsdata);
		break;
	case 16:
		bufnum = entry / FAT16BUFSIZE(fsdata);
		offset = entry - bufnum * FAT16BUFSIZE(fsdata);
		break;
	case 12:
		bufnum = entry / FAT12BUFSIZE(fsdata);
		offset = entry - bufnum * FAT12BUFSIZE(fsdata);
		break;
	default:
		/* Unsupported FAT size */
		return ret;
	}

	/* Read a new block of FAT entries into the cache. */
	if (bufnum != fsdata->fatbufnum) {
		uint32_t i, getsize = FATBUFBLOCKS;
		uint8_t *bufptr = fsdata->fatbuf;
		uint32_t fatlength = fsdata->fatlength;
		uint32_t startblock = bufnum * FATBUFBLOCKS;
		int rc;

		/* Cap length if fatlength is not a multiple of FATBUFBLOCKS */
		if (startblock + getsize > fatlength)
			getsize = fatlength - startblock;

		startblock += fsdata->fat_sect;	/* Offset from start of disk */

		for (i = 0; i < getsize; i++) {
			rc = gpt_partition_read_block(fsdata->part, startblock + i,
							bufptr + (fsdata->sect_size * i));
			if (rc)
				return rc;
		}
		fsdata->fatbufnum = bufnum;
	}

	/* Get the actual entry from the table */
	switch (fsdata->fatsize) {
	case 32:
		ret = FAT2CPU32(((uint32_t *) fsdata->fatbuf)[offset]);
		break;
	case 16:
		ret = FAT2CPU16(((uint16_t *) fsdata->fatbuf)[offset]);
		break;
	case 12:
		off8 = (offset * 3) / 2;
		/* fatbut + off8 may be unaligned, read in byte granularity */
		ret = fsdata->fatbuf[off8] + (fsdata->fatbuf[off8 + 1] << 8);

		if (offset & 0x1)
			ret >>= 4;
		ret &= 0xfff;
	}

	return ret;
}

int fat_load_fsdata(struct gpt_part_entry *part, struct fat_fsdata *fsdata)
{
	struct fat_boot_sector *bs;
	struct fat_volume_info *vi;
	int ret;

	ret = gpt_partition_read_block(part, 0, fsdata->tmpbuf);
	if (ret)
		return ret;
	bs = (struct fat_boot_sector *)fsdata->tmpbuf;

	/* Populate filesystem data */
	if (bs->fat_length == 0) {
		fsdata->fatsize = 32;
		vi = (struct fat_volume_info *)(fsdata->tmpbuf + sizeof(*bs));
	} else {
		fsdata->fatsize = 0;
		vi = (struct fat_volume_info *)(unsigned long)&bs->fat32_length;
	}

	/* Check signature */
	if (fsdata->fatsize == 32) {
		if (memcmp(FAT32_SIGN, vi->fs_type, FAT_SIGNLEN))
			return -1;
	} else {
		if (!memcmp(FAT12_SIGN, vi->fs_type, FAT_SIGNLEN)) {
			fsdata->fatsize = 12;
		} else if (!memcmp(FAT16_SIGN, vi->fs_type, FAT_SIGNLEN)) {
			fsdata->fatsize = 16;
		} else {
			return -1;
		}
	}

	/* Determine FAT table coordinates and total sectors */
	if (fsdata->fatsize == 32) {
		fsdata->fatlength = FAT2CPU32(bs->fat32_length);
		fsdata->total_sect = FAT2CPU32(bs->total_sect);
	} else {
		fsdata->fatlength = FAT2CPU16(bs->fat_length);
		fsdata->total_sect = (bs->sectors[1] << 8) + bs->sectors[0];
		if (!fsdata->total_sect)
			fsdata->total_sect = bs->total_sect;
	}
	if (!fsdata->total_sect) /* unlikely */
		fsdata->total_sect = gpt_partition_num_blocks(part);
	fsdata->fats = bs->fats;
	fsdata->fat_sect = FAT2CPU16(bs->reserved);

	/* Root directory sector */
	fsdata->rootdir_sect = fsdata->fat_sect + fsdata->fatlength * bs->fats;

	/* Sector size and cluster size */
	fsdata->sect_size = (bs->sector_size[1] << 8) + bs->sector_size[0];
	fsdata->clust_size = bs->cluster_size;

	/* Root cluster and data begin */
	if (fsdata->fatsize == 32) {
		fsdata->rootdir_size = 0;
		fsdata->data_begin = fsdata->rootdir_sect - (fsdata->clust_size * 2);
		fsdata->root_cluster = FAT2CPU32(bs->root_cluster);
	} else {
		fsdata->rootdir_size = ((bs->dir_entries[1]  * (int)256 +
					 bs->dir_entries[0]) *
					 sizeof(struct fat_dir_entry)) /
					 fsdata->sect_size;
		fsdata->data_begin = fsdata->rootdir_sect + fsdata->rootdir_size -
					(fsdata->clust_size * 2);

		/*
		 * The root directory is not cluster-aligned and may be on a
		 * "negative" cluster, this will be handled specially in
		 * fat_next_cluster().
		 */
		fsdata->root_cluster = 0;
	}

	/* Sanity checks */
	if (fsdata->sect_size != HAL_BLOCK_SIZE)
		return -1;
	if (fsdata->clust_size == 0)
		return -1;
	if ((unsigned int)fsdata->clust_size * fsdata->sect_size > FAT_MAX_CLUSTSIZE)
		return -1;

	fsdata->fatbufnum = -1;
	fsdata->fat_dirty = 0;
	fsdata->part = part;

	return 0;
}

static uint32_t fat_clust_to_sect(struct fat_fsdata *fsdata, uint32_t clust)
{
	return fsdata->data_begin + clust * fsdata->clust_size;
}

static uint32_t fat_clust_chain_count(struct fat_fsdata *fsdata, uint32_t clust)
{
	uint32_t ret = 0;

	if (!clust)
		return 0;

	while (!IS_LAST_CLUST(clust, fsdata->fatsize)) {
		clust = fat_get_entry(fsdata, clust);
		ret++;
	}

	return ret;
}

uint32_t fat_file_read(struct fat_fsdata *fsdata, const struct fat_file *file,
		       uint32_t offset, void *buf, uint32_t len)
{
	uint32_t end = (offset + len), sect_size = fsdata->sect_size;
	uint32_t sect_num, sect_bytes, sect_off, sect_blk;
	uint32_t read = 0, clust_num, clust;
	int ret;

	if (!file->size || file->size <= offset)
		return 0;
	if (!file->first_cluster && fsdata->fatsize == 32)
		return 0;

	if (file->size < end)
		end = file->size;

	while (offset < end) {
		sect_num = offset / sect_size;
		sect_off = offset % sect_size;
		sect_bytes = sect_size - sect_off;
		sect_bytes = (sect_bytes <= (end - offset)) ? sect_bytes : (end - offset);

		if (fsdata->fatsize != 32 && !file->first_cluster && file->is_directory) {
			sect_blk = fsdata->rootdir_sect + sect_num;
		} else {
			clust_num = sect_num / fsdata->clust_size;

			clust = file->first_cluster;
			while (clust_num) {
				if (IS_LAST_CLUST(clust, fsdata->fatsize))
					break;
				clust = fat_get_entry(fsdata, clust);
				clust_num--;
			}
			if (clust_num)
				goto done;

			sect_blk = fat_clust_to_sect(fsdata, clust) +
				   (sect_num % fsdata->clust_size);
		}

		ret = gpt_partition_read_block(fsdata->part, sect_blk, fsdata->tmpbuf);
		if (ret)
			goto done;

		memcpy(buf, &fsdata->tmpbuf[sect_off], sect_bytes);
		buf += sect_bytes;
		offset += sect_bytes;
		read += sect_bytes;
	}

done:
	return read;
}

int fat_get_root_directory(struct fat_fsdata *fsdata, struct fat_file *out)
{
	out->is_directory = 1;
	memset(out->lname, 0, sizeof(out->lname));
	out->first_cluster = fsdata->root_cluster;
	if (fsdata->fatsize == 32)
		out->size = fsdata->sect_size * fsdata->clust_size *
			    fat_clust_chain_count(fsdata, out->first_cluster);
	else
		out->size = fsdata->rootdir_size;
	return 0;
}

int fat_directory_lookup(struct fat_fsdata *fsdata, const struct fat_file *dir,
			 const char *name, struct fat_file *out)
{
	uint8_t lname[FAT_LFN_MAXLEN];
	struct fat_dir_entry dent;
	struct fat_dir_slot dslot;
	int i, j;
	uint32_t read;

	if (!dir->is_directory)
		return -1;

	memset(lname, 0, sizeof(lname));
	for (i = 0; i < dir->size; i += sizeof(dent)) {
		read = fat_file_read(fsdata, dir, i, &dent, sizeof(dent));
		if (read != sizeof(dent))
			return -1;

		if ((dent.name[0] == 0xE5) || (dent.name[0] == 0x2E)) {
			continue;
		}

		if (dent.attr != FAT_DIR_ATTR_DIRECTORY &&
		    dent.attr != FAT_DIR_ATTR_ARCHIVE &&
		    dent.attr != FAT_DIR_ATTR_LFN)
			continue;

		if (dent.attr == FAT_DIR_ATTR_LFN) {
			memcpy(&dslot, &dent, sizeof(dent));
			if (FAT_LFN_LASTSEQ(dslot.id)) {
				dslot.id = FAT_LFN_SEQNO(dslot.id);
			}
			if ((dslot.id < FAT_LFN_MINSEQ) || (FAT_LFN_MAXSEQ < dslot.id)) {
				continue;
			}
			j = (dslot.id - 1) * 13;
			lname[j + 0] = (char)(dslot.name0_4[0*2]);
			lname[j + 1] = (char)(dslot.name0_4[1*2]);
			lname[j + 2] = (char)(dslot.name0_4[2*2]);
			lname[j + 3] = (char)(dslot.name0_4[3*2]);
			lname[j + 4] = (char)(dslot.name0_4[4*2]);
			lname[j + 5] = (char)(dslot.name5_10[0*2]);
			lname[j + 6] = (char)(dslot.name5_10[1*2]);
			lname[j + 7] = (char)(dslot.name5_10[2*2]);
			lname[j + 8] = (char)(dslot.name5_10[3*2]);
			lname[j + 9] = (char)(dslot.name5_10[4*2]);
			lname[j + 10] = (char)(dslot.name5_10[5*2]);
			lname[j + 11] = (char)(dslot.name11_12[0*2]);
			lname[j + 12] = (char)(dslot.name11_12[1*2]);
			continue;
		}

		if (!strlen((const char *)lname)) {
			j = 8;
			while (j && (dent.name[j - 1] == ' ')) {
				dent.name[j - 1] = '\0';
				j--;
			}
			j = 3;
			while (j && (dent.extn[j - 1] == ' ')) {
				dent.extn[j - 1] = '\0';
				j--;
			}

			for(j = 0; j < 8 && dent.name[j]; j++) {
				lname[j] = dent.lcase ?
					   fat_tolower(dent.name[j]) : dent.name[j];
			}
			lname[8] = '\0';

			if (dent.extn[0] != '\0') {
				j = strlen((const char *)lname);
				lname[j] = '.';
				lname[j + 1] = fat_tolower(dent.extn[0]);
				lname[j + 2] = fat_tolower(dent.extn[1]);
				lname[j + 3] = fat_tolower(dent.extn[2]);
				lname[j + 4] = '\0';
			}
		}

		if (!strcmp((const char *)lname, name)) {
			out->is_directory = (dent.attr & FAT_DIR_ATTR_DIRECTORY) ?
					    1 : 0;
			memcpy(out->lname, lname, sizeof(lname));
			out->first_cluster = ((uint32_t)FAT2CPU16(dent.starthi) << 16) |
					     FAT2CPU16(dent.start);
			out->size = (!out->is_directory) ? FAT2CPU32(dent.size) :
				    (fsdata->sect_size * fsdata->clust_size *
				    fat_clust_chain_count(fsdata, out->first_cluster));
			return 0;
		}

		memset(lname, 0, sizeof(lname));
	}

	return -1;
}

int fat_file_load(struct gpt_part_entry *part, int tokc, char **tokv)
{
	struct fat_fsdata fsdata = { 0 };
	uint8_t buf[HAL_BLOCK_SIZE];
	struct fat_file dir, file;
	uint32_t off = 0, read;
	int ret, tok = 0;

	if (!tokc)
		return -1;

	ret = fat_load_fsdata(part, &fsdata);
	if (ret)
		return ret;

	ret = fat_get_root_directory(&fsdata, &dir);
	if (ret)
		return ret;

	for (tok = 0; tok < tokc; tok++) {
		ret = fat_directory_lookup(&fsdata, &dir, tokv[tok], &file);
		if (ret)
			return ret;
		if (tok < (tokc - 1))
			memcpy(&dir, &file, sizeof(dir));
	}

	while (off < file.size) {
		read = fat_file_read(&fsdata, &file, off, buf, sizeof(buf));
		ret = hal_load_dram(off, file.size, buf, read);
		if (ret)
			return ret;
		off += read;
	};

	return 0;
}

int mmc_blk_dev_setup(void)
{
	struct mmc *mmc;
	int err = 0;

	err = mmc_init_device(MMC_DEV_NUM);
	if (err ) {
		printf("mmc_init_device failed\n");
		goto out;
	}

	mmc = find_mmc_device(MMC_DEV_NUM);
	if (!mmc ) {
		printf("find_mmc_device failed\n");
		goto out;
	}

	err = mmc_init(mmc);
	if (err) {
		printf("mmc_init failed\n");
		goto out;
	}

	blk_dev = mmc_get_blk_desc(mmc);
	if (!blk_dev) {
		printf("Failed to get block dev\n");
		err = -ENODEV;
	};

out:
	return err;
}

int fat_boot_part_file_load(int tokc, char **tokv)
{
	struct gpt_part_entry ent;
	int ret;

	/* Initialize low level disk I/O layer */
	ret = mmc_blk_dev_setup();
	if (ret)
		return ret;

	ret = gpt_find_boot_partition(&ent);
	if (ret)
		return ret;

	ret = fat_file_load(&ent, tokc, tokv);
	if (ret)
		return ret;

	return 0;
}

int hal_read_block(unsigned long lba, void *block_buf)
{
	int ret;

	ret = blk_dread(blk_dev, lba, 1, block_buf);
	if (ret <= 0) {
		printf("Failed to read from block %ld. Err %d\n",lba ,ret);
		return -ENODATA;
	}

	return 0;
}

int hal_load_dram(unsigned long offset, unsigned long total, void *buf, size_t buf_len)
{
	const uint64_t DRAM_START = 0x400000000UL;
	int i, rem;

	rem = buf_len % 4;
	buf_len -= rem;

	for(i = 0; i < buf_len; i+=4)
		swea(DRAM_START + offset + i, *((uint32_t *)(buf + i)));

	for(; i < buf_len + rem; i++)
		sbea(DRAM_START + offset + i, *((uint8_t *)(buf + i)));

	return 0;
}
