/*
 * (C) Copyright 2004 Texas Instruments
 * Jian Zhang <jzhang@ti.com>
 *
 *  Samsung K9F1G08R0AQ0C NAND chip driver for an OMAP2420 board
 *
 * This file is based on the following u-boot file:
 *	common/cmd_nand.c
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>

#ifdef CFG_NAND_K9F1G08R0A

#define K9F1G08R0A_MFR		0xec  /* Samsung */
#define K9F1G08R0A_ID		0xa1  /* part # */

/* Since Micron and Samsung parts are similar in geometry and bus width
 * we can use the same driver. Need to revisit to make this file independent
 * of part/manufacturer
 */
#define MT29F1G_MFR		0x2c  /* Micron */
#define MT29F1G_ID		0xa1  /* x8, 1GiB */
#define MT29F2G_ID		0xba	/* x16, 2GiB */
#define MT29F4G_ID		0xbc	/* x16, 4GiB */
#define MT29C4G_ID		0xbc    /* x16, 4GiB, internal ECC */

#define HYNIX_MFR		0xAD  /* Hynix */
#define HYNIX2GiB_ID		0xBA  /* x16, 2GiB */
#define HYNIX4GiB_ID		0xBC  /* x16, 4GiB */

#define ADDR_COLUMN		1
#define ADDR_PAGE		2
#define ADDR_COLUMN_PAGE	(ADDR_COLUMN | ADDR_PAGE)

#define ADDR_OOB		(0x4 | ADDR_COLUMN_PAGE)

#define PAGE_SIZE		2048
#define OOB_SIZE		64
#define MAX_NUM_PAGES		64

#define ECC_CHECK_ENABLE

#if !defined(FOUR_BIT_ERROR_CORRECT) && !defined(EIGHT_BIT_ERROR_CORRECT)
#define ONE_BIT_ERROR_CORRECT
#endif

#ifdef ONE_BIT_ERROR_CORRECT
#define ECC_SIZE		24
#define ECC_STEPS		3
#endif

#ifdef FOUR_BIT_ERROR_CORRECT
#define ECC_SIZE                28 
#define ECC_STEPS               28 

void omap_enable_hwecc_bch4(uint32_t bus_width, int32_t mode);
int omap_correct_data_bch4(uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc);
int omap_calculate_ecc_bch4(const uint8_t *dat, uint8_t *ecc_code);
#endif

#ifdef EIGHT_BIT_ERROR_CORRECT
#define ECC_SIZE                52
#define ECC_STEPS               52 

void omap_enable_hwecc_bch8(uint32_t bus_width, int32_t mode);
int omap_correct_data_bch8(uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc);
int omap_calculate_ecc_bch8(const uint8_t *dat, uint8_t *ecc_code);
#endif

#ifndef ECC_SIZE
#error "Must define ONE_BIT_ERROR_CORRECT, FOUR_BIT_ERROR_CORRECT, \
or EIGHT_BIT_ERROR_CORRECT"
#endif

/* Non-zero if NAND chip has built-in ECC support.  If so, then
 * set the features to enable the ECC and use it. */
static int chip_has_ecc;

unsigned int is_ddr_166M;
/*******************************************************
 * Routine: delay
 * Description: spinning delay to use before udelay works
 ******************************************************/
static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
					  "subs %0, %0, #1\n"
					  "bne 1b":"=r" (loops):"0" (loops));
}

static int nand_read_page(u_char *buf, ulong page_addr);
static int nand_read_oob(u_char * buf, ulong page_addr);

#ifdef ONE_BIT_ERROR_CORRECT
#ifdef NAND_HW_ECC_METHOD
/* This is the HW ECC supported by bootrom; usable by "nandecc hw" in u-boot */
static u_char ecc_pos[] =
		{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
#else
/* This is the only SW ECC supported by u-boot. So to load u-boot
 * this should be supported */
static u_char ecc_pos[] =
  {40, 41, 42, 
   43, 44, 45, 46, 47, 48, 49, 
   50, 51, 52, 53, 54, 55, 56, 
   57, 58, 59, 60, 61, 62, 63};
#endif
#endif

#ifdef FOUR_BIT_ERROR_CORRECT
/* This is the only SW ECC supported by u-boot. So to load u-boot
 * this should be supported */
static u_char ecc_pos[] =
  {36, 37, 38, 39, 40, 41, 42, 
   43, 44, 45, 46, 47, 48, 49, 
   50, 51, 52, 53, 54, 55, 56, 
   57, 58, 59, 60, 61, 62, 63};
#endif

#ifdef EIGHT_BIT_ERROR_CORRECT
/* This is the only SW ECC supported by u-boot. So to load u-boot
 * this should be supported */
static u_char ecc_pos[] =
  {12, 13, 14,
   15, 16, 17, 18, 19, 20, 21,
   22, 23, 24, 25, 26, 27, 28,
   29, 30, 31, 32, 33, 34, 35,
   36, 37, 38, 39, 40, 41, 42, 
   43, 44, 45, 46, 47, 48, 49, 
   50, 51, 52, 53, 54, 55, 56, 
   57, 58, 59, 60, 61, 62, 63};
#endif

static unsigned long chipsize = (256 << 20);

#ifdef NAND_16BIT
static int bus_width = 16;
#else
static int bus_width = 8;
#endif

/* NanD_Command: Send a flash command to the flash chip */
static int NanD_Command(unsigned char command)
{
 	NAND_CTL_SETCLE(NAND_ADDR);

 	WRITE_NAND_COMMAND(command, NAND_ADDR);
 	NAND_CTL_CLRCLE(NAND_ADDR);

  	if(command == NAND_CMD_RESET){
		unsigned char ret_val;
		NanD_Command(NAND_CMD_STATUS);
		do{
			ret_val = READ_NAND(NAND_ADDR);/* wait till ready */
  		} while((ret_val & 0x40) != 0x40);
 	}

	NAND_WAIT_READY();
	return 0;
}


/* NanD_Address: Set the current address for the flash chip */
static int NanD_Address(unsigned int numbytes, unsigned long ofs)
{
	uchar u;

 	NAND_CTL_SETALE(NAND_ADDR);

	if (numbytes == ADDR_COLUMN || numbytes == ADDR_COLUMN_PAGE
				|| numbytes == ADDR_OOB)
	{
		ushort col = ofs;

		u = col  & 0xff;
		WRITE_NAND_ADDRESS(u, NAND_ADDR);

		u = (col >> 8) & 0x07;
		if (numbytes == ADDR_OOB)
			u = u | ((bus_width == 16) ? (1 << 2) : (1 << 3));
		WRITE_NAND_ADDRESS(u, NAND_ADDR);
	}

	if (numbytes == ADDR_PAGE || numbytes == ADDR_COLUMN_PAGE
				|| numbytes == ADDR_OOB)
	{
		u = (ofs >> 11) & 0xff;
		WRITE_NAND_ADDRESS(u, NAND_ADDR);
		u = (ofs >> 19) & 0xff;
		WRITE_NAND_ADDRESS(u, NAND_ADDR);

		/* One more address cycle for devices > 128MiB */
		if (chipsize > (128 << 20)) {
			u = (ofs >> 27) & 0xff;
			WRITE_NAND_ADDRESS(u, NAND_ADDR);
		}
	}

 	NAND_CTL_CLRALE(NAND_ADDR);

 	NAND_WAIT_READY();
	return 0;
}

/* read chip mfr and id
 * return 0 if they match board config
 * return 1 if not
 */
int nand_chip()
{
	int mfr, id;
	unsigned id_params[5];

 	NAND_ENABLE_CE();

 	if (NanD_Command(NAND_CMD_RESET)) {
 		printf("Err: RESET\n");
 		NAND_DISABLE_CE();
		return 1;
	}

 	if (NanD_Command(NAND_CMD_READID)) {
 		printf("Err: READID\n");
 		NAND_DISABLE_CE();
		return 1;
 	}

 	NanD_Address(ADDR_COLUMN, 0);

 	mfr = id_params[0] = READ_NAND(NAND_ADDR);
	id = id_params[1] = READ_NAND(NAND_ADDR);
	id_params[2] = READ_NAND(NAND_ADDR);
	id_params[3] = READ_NAND(NAND_ADDR);
	id_params[4] = READ_NAND(NAND_ADDR);

#if 0
	printf("%s: params %02x %02x %02x %02x %02x\n", __FUNCTION__,
		id_params[0], id_params[1], id_params[2], id_params[3], id_params[4]);
#endif

	NAND_DISABLE_CE();

	switch (mfr) {
		/* Hynix NAND Part */
		case HYNIX_MFR:
			is_ddr_166M = 0;
			if (is_cpu_family() == CPU_OMAP36XX)
				return ((id != HYNIX4GiB_ID) && (id != HYNIX2GiB_ID));
			break;
		/* Micron NAND Part */
		case MT29F1G_MFR:
			is_ddr_166M = 1;
			if ((is_cpu_family() == CPU_OMAP34XX) ||
				(is_cpu_family() == CPU_AM35XX) ||
				(is_cpu_family() == CPU_OMAP36XX)) {
				if ((id == MT29C4G_ID) && ((id_params[4] & 0x3) == 0x2)) {
					/* Its a MT29C4G part which has internal ECC */
					chip_has_ecc = 1;
					return 0;
				}
				return (!((id == MT29F1G_ID) ||
						(id == MT29F2G_ID) ||
						(id ==MT29F4G_ID)));
			}
			break;
		case K9F1G08R0A_MFR:
		default:
			is_ddr_166M = 1;
			break;
	}

	return (id != K9F1G08R0A_ID);
}

/* Put NAND chip into ECC mode (if it has it).
 * Return 0 on success. */
static int nand_setup_chip_ecc(void)
{
	static int already_run_setup = 0;

	if (already_run_setup)
		return 0;

	already_run_setup = 1;

	/* If chip doesn't support in-chip ECC mode we'll use
	   the soft ECC method. */
	if (!chip_has_ecc) {
#ifdef NAND_HW_ECC_METHOD
		printf("NAND: HW ECC\n");
#else
		printf("NAND: soft ECC\n");
#endif
		return 0;
	}
	printf("NAND: in-chip ECC\n");
	if (NanD_Command(NAND_CMD_SET_FEATURES)) {
		printf("Err: setup chip_ECC\n");
		NAND_DISABLE_CE();
		return 1;
	}
	NanD_Address(1, 0x90);
	NAND_WAIT_READY();

	WRITE_NAND(0x08, NAND_ADDR);
	WRITE_NAND(0x00, NAND_ADDR);
	WRITE_NAND(0x00, NAND_ADDR);
	WRITE_NAND(0x00, NAND_ADDR);

	NAND_WAIT_READY();
	NAND_DISABLE_CE();

	/* A delay seems to be helping here. needs more investigation */
	delay(10000);

	return 0;
} 

/* read a block data to buf
 * return 1 if the block is bad
 * return -1 if ECC error can't be corrected for any page
 * return 0 on sucess
 */
int nand_read_block(unsigned char *buf, ulong block_addr)
{
	int i, offset = 0;
	int ret;
#ifdef ECC_CHECK_ENABLE
	u16 oob_buf[OOB_SIZE >> 1];
#endif

	nand_setup_chip_ecc();

#ifdef ECC_CHECK_ENABLE
	/* check bad block */
	/* 0th word in spare area needs be 0xff */
	ret = nand_read_oob((unsigned char *)oob_buf, block_addr);
	if (ret || (oob_buf[0] & 0xff) != 0xff) {
		if (ret) {
			printf("Error on read of block at 0x%x [ret %d]\n", block_addr, ret);
			return -1;
		}
		printf("oob:");
		for (i=0; i<sizeof(oob_buf)/sizeof(oob_buf[0]); ++i)
			printf("%c%04x", i && ((i & 0xf) == 0) ? '\n' : ' ', oob_buf[i]);
		printf("\n");
		return 1;    /* skip bad block */
	}
#endif
	/* read the block page by page*/
	for (i=0; i<MAX_NUM_PAGES; i++){
		if (nand_read_page(buf+offset, block_addr + offset))
			return 1;
		offset += PAGE_SIZE;
	}

	return 0;
}

/* Enable HW ECC for NAND read */
static void omap_enable_hwecc(void)
{
	u32 cs = 0; // Assume NAND is on CS 0
	u32 val;

	WRITE_NAND_ECC_CONTROL(0x101);
	val =  (cs << 1) | 1;
#ifdef NAND_16BIT
	val |= (1 << 7);
#endif
	WRITE_NAND_ECC_CONFIG(val);
}

static void omap_calculate_hwecc(u_char *ecc_code)
{
	u32 val;

	val = READ_NAND_ECC_RESULT();
	*ecc_code++ = val;
	*ecc_code++ = val >> 16;
	*ecc_code++ = ((val >> 8) & 0x0f) | ((val >> 20) & 0xf0);
}

#define NAND_Ecc_P1e		(1 << 0)
#define NAND_Ecc_P2e		(1 << 1)
#define NAND_Ecc_P4e		(1 << 2)
#define NAND_Ecc_P8e		(1 << 3)
#define NAND_Ecc_P16e		(1 << 4)
#define NAND_Ecc_P32e		(1 << 5)
#define NAND_Ecc_P64e		(1 << 6)
#define NAND_Ecc_P128e		(1 << 7)
#define NAND_Ecc_P256e		(1 << 8)
#define NAND_Ecc_P512e		(1 << 9)
#define NAND_Ecc_P1024e		(1 << 10)
#define NAND_Ecc_P2048e		(1 << 11)

#define NAND_Ecc_P1o		(1 << 16)
#define NAND_Ecc_P2o		(1 << 17)
#define NAND_Ecc_P4o		(1 << 18)
#define NAND_Ecc_P8o		(1 << 19)
#define NAND_Ecc_P16o		(1 << 20)
#define NAND_Ecc_P32o		(1 << 21)
#define NAND_Ecc_P64o		(1 << 22)
#define NAND_Ecc_P128o		(1 << 23)
#define NAND_Ecc_P256o		(1 << 24)
#define NAND_Ecc_P512o		(1 << 25)
#define NAND_Ecc_P1024o		(1 << 26)
#define NAND_Ecc_P2048o		(1 << 27)

#define TF(value)	(value ? 1 : 0)

#define P2048e(a)	(TF(a & NAND_Ecc_P2048e)	<< 0)
#define P2048o(a)	(TF(a & NAND_Ecc_P2048o)	<< 1)
#define P1e(a)		(TF(a & NAND_Ecc_P1e)		<< 2)
#define P1o(a)		(TF(a & NAND_Ecc_P1o)		<< 3)
#define P2e(a)		(TF(a & NAND_Ecc_P2e)		<< 4)
#define P2o(a)		(TF(a & NAND_Ecc_P2o)		<< 5)
#define P4e(a)		(TF(a & NAND_Ecc_P4e)		<< 6)
#define P4o(a)		(TF(a & NAND_Ecc_P4o)		<< 7)

#define P8e(a)		(TF(a & NAND_Ecc_P8e)		<< 0)
#define P8o(a)		(TF(a & NAND_Ecc_P8o)		<< 1)
#define P16e(a)		(TF(a & NAND_Ecc_P16e)		<< 2)
#define P16o(a)		(TF(a & NAND_Ecc_P16o)		<< 3)
#define P32e(a)		(TF(a & NAND_Ecc_P32e)		<< 4)
#define P32o(a)		(TF(a & NAND_Ecc_P32o)		<< 5)
#define P64e(a)		(TF(a & NAND_Ecc_P64e)		<< 6)
#define P64o(a)		(TF(a & NAND_Ecc_P64o)		<< 7)

#define P128e(a)	(TF(a & NAND_Ecc_P128e)		<< 0)
#define P128o(a)	(TF(a & NAND_Ecc_P128o)		<< 1)
#define P256e(a)	(TF(a & NAND_Ecc_P256e)		<< 2)
#define P256o(a)	(TF(a & NAND_Ecc_P256o)		<< 3)
#define P512e(a)	(TF(a & NAND_Ecc_P512e)		<< 4)
#define P512o(a)	(TF(a & NAND_Ecc_P512o)		<< 5)
#define P1024e(a)	(TF(a & NAND_Ecc_P1024e)	<< 6)
#define P1024o(a)	(TF(a & NAND_Ecc_P1024o)	<< 7)

#define P8e_s(a)	(TF(a & NAND_Ecc_P8e)		<< 0)
#define P8o_s(a)	(TF(a & NAND_Ecc_P8o)		<< 1)
#define P16e_s(a)	(TF(a & NAND_Ecc_P16e)		<< 2)
#define P16o_s(a)	(TF(a & NAND_Ecc_P16o)		<< 3)
#define P1e_s(a)	(TF(a & NAND_Ecc_P1e)		<< 4)
#define P1o_s(a)	(TF(a & NAND_Ecc_P1o)		<< 5)
#define P2e_s(a)	(TF(a & NAND_Ecc_P2e)		<< 6)
#define P2o_s(a)	(TF(a & NAND_Ecc_P2o)		<< 7)

#define P4e_s(a)	(TF(a & NAND_Ecc_P4e)		<< 0)
#define P4o_s(a)	(TF(a & NAND_Ecc_P4o)		<< 1)

/**
 * gen_true_ecc - This function will generate true ECC value
 * @ecc_buf: buffer to store ecc code
 *
 * This generated true ECC value can be used when correcting
 * data read from NAND flash memory core
 */
static void gen_true_ecc(u8 *ecc_buf)
{
	u32 tmp = ecc_buf[0] | (ecc_buf[1] << 16) |
		((ecc_buf[2] & 0xF0) << 20) | ((ecc_buf[2] & 0x0F) << 8);

	ecc_buf[0] = ~(P64o(tmp) | P64e(tmp) | P32o(tmp) | P32e(tmp) |
			P16o(tmp) | P16e(tmp) | P8o(tmp) | P8e(tmp));
	ecc_buf[1] = ~(P1024o(tmp) | P1024e(tmp) | P512o(tmp) | P512e(tmp) |
			P256o(tmp) | P256e(tmp) | P128o(tmp) | P128e(tmp));
	ecc_buf[2] = ~(P4o(tmp) | P4e(tmp) | P2o(tmp) | P2e(tmp) | P1o(tmp) |
			P1e(tmp) | P2048o(tmp) | P2048e(tmp));
}

static int omap_compare_hwecc(u_char *ecc_data1,  /* read from NAND */
			u_char *ecc_data2,	/* read from register */
			u_char *page_data)
{
	u32	i;
	u_char	tmp0_bit[8], tmp1_bit[8], tmp2_bit[8];
	u_char	comp0_bit[8], comp1_bit[8], comp2_bit[8];
	u_char	ecc_bit[24];
	u_char	ecc_sum = 0;
	u_char	find_bit = 0;
	u32	find_byte = 0;
	int	isEccFF;

	isEccFF = ((*(u32 *)ecc_data1 & 0xFFFFFF) == 0xFFFFFF);

	gen_true_ecc(ecc_data1);
	gen_true_ecc(ecc_data2);

	for (i = 0; i <= 2; i++) {
		*(ecc_data1 + i) = ~(*(ecc_data1 + i));
		*(ecc_data2 + i) = ~(*(ecc_data2 + i));
	}

	for (i = 0; i < 8; i++) {
		tmp0_bit[i]     = *ecc_data1 % 2;
		*ecc_data1	= *ecc_data1 / 2;
	}

	for (i = 0; i < 8; i++) {
		tmp1_bit[i]	 = *(ecc_data1 + 1) % 2;
		*(ecc_data1 + 1) = *(ecc_data1 + 1) / 2;
	}

	for (i = 0; i < 8; i++) {
		tmp2_bit[i]	 = *(ecc_data1 + 2) % 2;
		*(ecc_data1 + 2) = *(ecc_data1 + 2) / 2;
	}

	for (i = 0; i < 8; i++) {
		comp0_bit[i]     = *ecc_data2 % 2;
		*ecc_data2       = *ecc_data2 / 2;
	}

	for (i = 0; i < 8; i++) {
		comp1_bit[i]     = *(ecc_data2 + 1) % 2;
		*(ecc_data2 + 1) = *(ecc_data2 + 1) / 2;
	}

	for (i = 0; i < 8; i++) {
		comp2_bit[i]     = *(ecc_data2 + 2) % 2;
		*(ecc_data2 + 2) = *(ecc_data2 + 2) / 2;
	}

	for (i = 0; i < 6; i++)
		ecc_bit[i] = tmp2_bit[i + 2] ^ comp2_bit[i + 2];

	for (i = 0; i < 8; i++)
		ecc_bit[i + 6] = tmp0_bit[i] ^ comp0_bit[i];

	for (i = 0; i < 8; i++)
		ecc_bit[i + 14] = tmp1_bit[i] ^ comp1_bit[i];

	ecc_bit[22] = tmp2_bit[0] ^ comp2_bit[0];
	ecc_bit[23] = tmp2_bit[1] ^ comp2_bit[1];

	for (i = 0; i < 24; i++)
		ecc_sum += ecc_bit[i];

	switch (ecc_sum) {
	case 0:
		/* Not reached because this function is not called if
		 *  ECC values are equal
		 */
		return 0;

	case 1:
		/* Uncorrectable error */
		printf("ECC UNCORRECTED_ERROR 1\n");
		return -1;

	case 11:
		/* UN-Correctable error */
		printf("ECC UNCORRECTED_ERROR B\n");
		return -1;

	case 12:
		/* Correctable error */
		find_byte = (ecc_bit[23] << 8) +
			    (ecc_bit[21] << 7) +
			    (ecc_bit[19] << 6) +
			    (ecc_bit[17] << 5) +
			    (ecc_bit[15] << 4) +
			    (ecc_bit[13] << 3) +
			    (ecc_bit[11] << 2) +
			    (ecc_bit[9]  << 1) +
			    ecc_bit[7];

		find_bit = (ecc_bit[5] << 2) + (ecc_bit[3] << 1) + ecc_bit[1];

		printf("Correcting single bit ECC error at "
				"offset: %d, bit: %d\n", find_byte, find_bit);

		page_data[find_byte] ^= (1 << find_bit);

		return 1;
	default:
		if (isEccFF) {
			if (ecc_data2[0] == 0 &&
			    ecc_data2[1] == 0 &&
			    ecc_data2[2] == 0)
				return 0;
		}
		printf("UNCORRECTED_ERROR default %d\n", ecc_sum);
		return -1;
	}
}

/* Do a correction, assuming a 512 byte block and 3-byte HW ECC */
static int omap_correct_hwecc_data(u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	if (read_ecc[0] == calc_ecc[0] && read_ecc[1] == calc_ecc[1] && read_ecc[2] == calc_ecc[2])
		return 0;

	return omap_compare_hwecc(read_ecc, calc_ecc, dat);
}

static int count;
/* read a page with ECC */
static int nand_read_page(u_char *buf, ulong page_addr)
{
	u16 status;
#ifdef ECC_CHECK_ENABLE
        /* increased size of ecc_code and ecc_calc to match the OOB size, 
           as is done in the kernel */
 	u_char ecc_code[OOB_SIZE];
	u_char ecc_calc[OOB_SIZE];
	u_char oob_buf[OOB_SIZE];
#endif
	u16 val;
	int cntr;
	int len;

#ifdef NAND_16BIT
	u16 *p;
#else
	u_char *p;
#endif

#ifdef NAND_HW_ECC_METHOD
	/* Enable the ECC engine and clear it */
	omap_enable_hwecc();
#endif

	NAND_ENABLE_CE();
	NanD_Command(NAND_CMD_READ0);
	NanD_Address(ADDR_COLUMN_PAGE, page_addr);
	NanD_Command(NAND_CMD_READSTART);
	NAND_WAIT_READY();

	/* A delay seems to be helping here. needs more investigation */
	delay(10000);

	if (chip_has_ecc) {
		/* get the chip status */
		NanD_Command(NAND_CMD_STATUS);
		status = READ_NAND(NAND_DATA);
		NanD_Command(NAND_CMD_READ0);
		NAND_WAIT_READY();
	} else {
#ifdef FOUR_BIT_ERROR_CORRECT
		/* 0 constant means READ mode */
		omap_enable_hwecc_bch4(bus_width, 0);
#endif
#ifdef EIGHT_BIT_ERROR_CORRECT
		omap_enable_hwecc_bch8(bus_width, 0);
#endif
	}

	/* read the page */
	len = (bus_width == 16) ? PAGE_SIZE >> 1 : PAGE_SIZE;
	p = (u16 *)buf;
	for (cntr = 0; cntr < len; cntr+=4){
		*p++ = READ_NAND(NAND_ADDR);
		*p++ = READ_NAND(NAND_ADDR);
		*p++ = READ_NAND(NAND_ADDR);
		*p++ = READ_NAND(NAND_ADDR);
#if 0
		delay(10);
#endif
   	}

#ifdef ECC_CHECK_ENABLE
	if (chip_has_ecc) {
		if (status & 0x1) {
			printf("chip ECC Failed, status 0x%02x page 0x%08x\n", status, page_addr);
			for(;;);
			return 1;
		}
		NAND_DISABLE_CE();
	} else {

#ifdef FOUR_BIT_ERROR_CORRECT
		/* calculate ECC on the page */
		omap_calculate_ecc_bch4(buf, &ecc_calc[0]);
#endif
#ifdef EIGHT_BIT_ERROR_CORRECT
		omap_calculate_ecc_bch8(buf, &ecc_calc[0]);
#endif

		/* read the OOB area */
		p = (u16 *)oob_buf;
		len = (bus_width == 16) ? OOB_SIZE >> 1 : OOB_SIZE;
		for (cntr = 0; cntr < len; cntr+=4){
			*p++ = READ_NAND(NAND_ADDR);
			*p++ = READ_NAND(NAND_ADDR);
			*p++ = READ_NAND(NAND_ADDR);
			*p++ = READ_NAND(NAND_ADDR);
#if 0
			delay(10);
#endif
		}
		count = 0;
		NAND_DISABLE_CE();  /* set pin high */

		/* Need to enable HWECC for READING */

		/* Pick the ECC bytes out of the oob data */
		for (cntr = 0; cntr < ECC_SIZE; cntr++)
			ecc_code[cntr] =  oob_buf[ecc_pos[cntr]];

		for(count = 0; count < ECC_SIZE; count += ECC_STEPS) {
#ifdef ONE_BIT_ERROR_CORRECT
			nand_calculate_ecc (buf, &ecc_calc[0]);
#endif
			if (
#ifdef ONE_BIT_ERROR_CORRECT
				nand_correct_data (buf, &ecc_code[count], &ecc_calc[0]) == -1
#endif
#ifdef FOUR_BIT_ERROR_CORRECT
				omap_correct_data_bch4(buf, &ecc_code[count], &ecc_calc[0]) == -1
#endif
#ifdef EIGHT_BIT_ERROR_CORRECT
				omap_correct_data_bch8(buf, &ecc_code[count], &ecc_calc[0]) == -1
#endif
				) {
				printf ("ECC Failed, page 0x%08x\n", page_addr);
				for (val=0; val <256; val++)
					printf("%x ", buf[val]);
				printf("\n");
				for (;;);
				return 1;
			}
			buf += 256;
			page_addr += 256;
		}
/* #endif */
	}
#endif
	return 0;
}

/* read from the 16 bytes of oob data that correspond to a 512 / 2048 byte page.
 */
static int nand_read_oob(u_char *buf, ulong page_addr)
{
	int cntr;
	int len;

#ifdef NAND_16BIT
	u16 *p;
#else
	u_char *p;
#endif
	p = (u16 *)buf;
        len = (bus_width == 16) ? OOB_SIZE >> 1 : OOB_SIZE;

  	NAND_ENABLE_CE();  /* set pin low */
	NanD_Command(NAND_CMD_READ0);
 	NanD_Address(ADDR_OOB, page_addr);
	NanD_Command(NAND_CMD_READSTART);
	NAND_WAIT_READY();

	/* A delay seems to be helping here. needs more investigation */
	delay(10000);
	for (cntr = 0; cntr < len; cntr+=4) {
		*p++ = READ_NAND(NAND_ADDR);
		*p++ = READ_NAND(NAND_ADDR);
		*p++ = READ_NAND(NAND_ADDR);
		*p++ = READ_NAND(NAND_ADDR);
	}

	NAND_WAIT_READY();
	NAND_DISABLE_CE();  /* set pin high */

	return 0;
}

#endif
