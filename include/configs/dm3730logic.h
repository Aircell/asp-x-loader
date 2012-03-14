/*
 * Copyright (C) 2011 Logic Product Development, Inc.
 *
 * X-Loader Configuation settings for the DM3730LOGIC SOM LV/Torpedo boards.
 *
 * Derived from /include/configs/omap3evm.h
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* serial printf facility takes about 3.5K */
#define CFG_PRINTF
//#undef CFG_PRINTF

#ifndef __ASSEMBLY__
extern void nand_wait_rdy(void);
#endif

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMCORTEXA8       1    /* This is an ARM V7 CPU core */
#define CONFIG_OMAP              1    /* in a TI OMAP core */
#define CONFIG_OMAP34XX          1    /* which is a 34XX */
#define CONFIG_OMAP3430          1    /* which is in a 3430 */

#define CONFIG_DM3730LOGIC	1	/* working with the DM3730 SOM LV/Torpedo boards */
#define CFG_BOARD_NAME	"dm3730logic"

#if 0
#define CFG_HANG_BDI		1	/* Hang board waiting for BDI to connect */
#endif

/* Enable the below macro if MMC boot support is required */
#define CONFIG_MMC               1
#if defined(CONFIG_MMC)
	#define CFG_CMD_MMC              1
	#define CFG_CMD_FAT              1
#endif

#include <asm/arch/cpu.h>        /* get chip and board defs */

/* uncomment it if you need timer based udelay(). it takes about 250 bytes */
//#define CFG_UDELAY

/* Clock Defines */
#define V_OSCK                   26000000  /* Clock output from T2 */

#if (V_OSCK > 19200000)
#define V_SCLK                   (V_OSCK >> 1)
#else
#define V_SCLK                   V_OSCK
#endif

//#define PRCM_CLK_CFG2_266MHZ   1    /* VDD2=1.15v - 133MHz DDR */
#define PRCM_CLK_CFG2_332MHZ     1    /* VDD2=1.15v - 166MHz DDR */
#define PRCM_PCLK_OPP2           1    /* ARM=381MHz - VDD1=1.20v */

/* Memory type */
#define CFG_OMAPEVM_DDR		1
#define CFG_3430SDRAM_DDR	1
/* #define CONFIG_DDR_256MB_STACKED */

/* The actual register values are defined in u-boot- mem.h */
/* SDRAM Bank Allocation method */
//#define SDRC_B_R_C             1
//#define SDRC_B1_R_B0_C         1
#define SDRC_R_B_C               1


# define NAND_BASE_ADR           NAND_BASE  /* NAND flash */

#define OMAP34XX_GPMC_CS0_SIZE GPMC_SIZE_128M

#ifdef CFG_PRINTF

#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE     (-4)
#define CFG_NS16550_CLK          (48000000)
#define CFG_NS16550_COM1         OMAP34XX_UART1

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL1           1    /* UART1 on DM3730LOGIC board(s) */
#define CONFIG_CONS_INDEX        1

#define CONFIG_BAUDRATE          115200
#define CFG_PBSIZE               256

#endif /* CFG_PRINTF */

/*
 * Miscellaneous configurable options
 */
#define CFG_LOADADDR             0x80400000

#undef	CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE         (128*1024) /* regular stack */

/*-----------------------------------------------------------------------
 * Board NAND Info.
 */


#define CFG_NAND_K9F1G08R0A    /* Samsung 8-bit 128MB chip large page NAND chip*/
#define NAND_16BIT
/* Use OMAP HW ECC method to read u-boot; ecc position matches bootrom */
#define NAND_HW_ECC_METHOD

/* NAND is partitioned:
 * 0x00000000 - 0x0007FFFF  Booting Image
 * 0x00080000 - 0x0023FFFF  U-Boot Image
 * 0x00240000 - 0x0027FFFF  U-Boot Env Data (X-loader doesn't care)
 * 0x00280000 - 0x0077FFFF  Kernel Image
 * 0x00780000 - 0x08000000  depends on application
 */
#define NAND_UBOOT_START         0x0080000 /* Leaving first 4 blocks for x-load */
#define NAND_UBOOT_END           0x0240000 /* Giving a space of 2 blocks = 256KB */
#define NAND_BLOCK_SIZE          0x20000

#define GPMC_CONFIG              (OMAP34XX_GPMC_BASE+0x50)
#define GPMC_NAND_COMMAND_0      (OMAP34XX_GPMC_BASE+0x7C)
#define GPMC_NAND_ADDRESS_0      (OMAP34XX_GPMC_BASE+0x80)
#define GPMC_NAND_DATA_0         (OMAP34XX_GPMC_BASE+0x84)
#define GPMC_NAND_ECC_CONFIG     (OMAP34XX_GPMC_BASE+0x1f4)
#define GPMC_NAND_ECC_CONTROL    (OMAP34XX_GPMC_BASE+0x1f8)
#define GPMC_NAND_ECC1_RESULT    (OMAP34XX_GPMC_BASE+0x200)

#define WRITE_NAND_ECC_CONTROL(d) \
	do {*(volatile u32 *)GPMC_NAND_ECC_CONTROL = (d); } while (0)
#define WRITE_NAND_ECC_CONFIG(d) \
	do {*(volatile u32 *)GPMC_NAND_ECC_CONFIG = (d); } while (0)
#define READ_NAND_ECC_RESULT() \
	*((volatile u32 *)GPMC_NAND_ECC1_RESULT)

#ifdef NAND_16BIT
#define WRITE_NAND_COMMAND(d, adr) \
        do {*(volatile u16 *)GPMC_NAND_COMMAND_0 = d;} while(0)
#define WRITE_NAND_ADDRESS(d, adr) \
        do {*(volatile u16 *)GPMC_NAND_ADDRESS_0 = d;} while(0)
#define WRITE_NAND(d, adr) \
        do {*(volatile u16 *)GPMC_NAND_DATA_0 = d;} while(0)
#define READ_NAND(adr) \
        (*(volatile u16 *)GPMC_NAND_DATA_0)
#define NAND_WAIT_READY() nand_wait_rdy()
#define NAND_WP_OFF()  \
        do {*(volatile u32 *)(GPMC_CONFIG) |= 0x00000010;} while(0)
#define NAND_WP_ON()  \
        do {*(volatile u32 *)(GPMC_CONFIG) &= ~0x00000010;} while(0)

#else /* to support 8-bit NAND devices */
#define WRITE_NAND_COMMAND(d, adr) \
        do {*(volatile u8 *)GPMC_NAND_COMMAND_0 = d;} while(0)
#define WRITE_NAND_ADDRESS(d, adr) \
        do {*(volatile u8 *)GPMC_NAND_ADDRESS_0 = d;} while(0)
#define WRITE_NAND(d, adr) \
        do {*(volatile u8 *)GPMC_NAND_DATA_0 = d;} while(0)
#define READ_NAND(adr) \
        (*(volatile u8 *)GPMC_NAND_DATA_0);
#define NAND_WAIT_READY() nand_wait_rdy()
#define NAND_WP_OFF()  \
        do {*(volatile u32 *)(GPMC_CONFIG) |= 0x00000010;} while(0)
#define NAND_WP_ON()  \
        do {*(volatile u32 *)(GPMC_CONFIG) &= ~0x00000010;} while(0)

#endif

#define NAND_CTL_CLRALE(adr)
#define NAND_CTL_SETALE(adr)
#define NAND_CTL_CLRCLE(adr)
#define NAND_CTL_SETCLE(adr)
#define NAND_DISABLE_CE()
#define NAND_ENABLE_CE()

/*-----------------------------------------------------------------------
 * Board oneNAND Info.
 */
#define CFG_SYNC_BURST_READ      1

#endif /* __CONFIG_H */

