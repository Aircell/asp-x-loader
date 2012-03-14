/*
 * (C) Copyright 2011
 * Logic Product Development, <www.logicpd.com>
 * Peter Barada <peter.barada@logicpd.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <part.h>
#include <fat.h>
#include <asm/arch/cpu.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include "dm3730logic-ddr.h"

/*******************************************************
 * Routine: delay
 * Description: spinning delay to use before udelay works
 ******************************************************/
static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}

#define __raw_readl(a)    (*(volatile unsigned int *)(a))
#define __raw_writel(v,a) (*(volatile unsigned int *)(a) = (v))
#define __raw_readw(a)    (*(volatile unsigned short *)(a))
#define __raw_writew(v,a) (*(volatile unsigned short *)(a) = (v))

struct sdram_timings {
	char	*name;
	u32	offset;
	u32	mcfg[2];
	u32	rfr[2];
	u32	actima[2];
	u32	actimb[2];
	u32	mr[2];
	u32	emr[2];
	u32	dlla, dllb;
	u32	power, cfg;
};

static struct sdram_timings sdram_timings[] = {
	{
	.name		= "MT29C4G48MAPLCJI6 256M",
	.offset		= 0x08000000,
	.mcfg		= { 0x02584099, 0x02584099 },
	.rfr		= { 0x0003c701, 0x0003c701 },
	.actima		= { 0xaa9db4c6, 0xaa9db4c6 },
	.actimb		= { 0x00011517, 0x00011517 },
	.mr		= { 0x00000032, 0x00000032 },
	.emr		= { 0x00000020, 0x00000020 },
	.dlla		= 0x0000000a,
	.dllb		= 0x0000000a,
	.power		= 0x00000009,
	.cfg		= 0x00000001,
	},
	{
	.name		= "MT29C4G48MAZAPAKQ5 256MB",
	.offset		= 0x02000000,
	.mcfg		= { 0x03588019, 0x00000000 },
	.rfr		= { 0x0003c701, 0x00000000 },
	.actima		= { 0x6ae24707, 0x00000000 },
	.actimb		= { 0x00011617, 0x00000000 },
	.mr		= { 0x00000032, 0x00000000 },
	.emr		= { 0x00000020, 0x00000000 },
	.dlla		= 0x0000000a,
	.dllb		= 0x00000000,
	.power		= 0x00000009,
	.cfg		= 0x00000002,
	},
	{
	.name		= "H8KCS0SJ0AER_MT29C2G24MAKLAJG6 128MB",
	.offset		= 0x04000000,
	.mcfg		= { 0x02584099, 0x00000000 },
	.rfr		= { 0x0003c701, 0x00000000 },
	.actima		= { 0xa2e24707, 0x00000000 },
	.actimb		= { 0x00011324, 0x00000000 },
	.mr		= { 0x00000032, 0x00000000 },
	.emr		= { 0x00000020, 0x00000000 },
	.dlla		= 0x0000000a,
	.dllb		= 0x00000000,
	.power		= 0x00000009,
	.cfg		= 0x00000001,
	},
#if 0
	{
	.name		= "MHMJX00X0MER0EM 1GB",
	.offset		= 0x10000000,
	.mcfg		= { 0x03690099, 0x03690099 },
	.rfr		= { 0x0005e601, 0x0005e601 },
	.actima		= { 0xa2e1b4c6, 0xa2e1b4c6 },
	.actimb		= { 0x0002131c, 0x0002131c },
	.mr		= { 0x00000032, 0x00000032 },
	.emr		= { 0x00000020, 0x00000020 },
	.dlla		= 0x0000000a,
	.dllb		= 0x0000000a,
	.power		= 0x00000085,
	.cfg		= 0x00000084,
	},
	{
	.name		= "H8MBX00U0MER0EM 512MB",
	.offset		= 0x02000000,
	.mcfg		= { 0x03588019, 0x03588019 },
	.rfr		= { 0x0005e601, 0x0005e601 },
	.actima		= { 0xa2e1b4c6, 0xa2e1b4c6 },
	.actimb		= { 0x0002131c, 0x0002131c },
	.mr		= { 0x00000032, 0x00000032 },
	.emr		= { 0x00000020, 0x00000020 },
	.dlla		= 0x0000000a,
	.dllb		= 0x0000000a,
	.power		= 0x00000085,
	.cfg		= 0x00000002,
	},
#endif
};

#define PATTERN_1	0xfeedface
#define PATTERN_2	0xc001d00d
int sdrc_probe(struct sdram_timings *p, u32 base)
{
	u32 base_orig;
	u32 offset_orig;
	u32 data;

	base_orig = *(volatile u32 *)base;
	offset_orig = *(volatile u32 *)(base + p->offset);

	*(volatile u32 *)(base + p->offset) = PATTERN_2;
	*(volatile u32 *)(base) = PATTERN_1;

	data = *(volatile u32 *)(base + p->offset);
	*(volatile u32 *)(base + p->offset) = offset_orig;
	*(volatile u32 *)(base) = base_orig;

	if (data == PATTERN_2)
		return 1;
	return 0;
}

void sdrc_ddr(struct sdram_timings *p)
{
	/* Issue sofrware reset of SDRAM interface */

	/* No idle ack and RESET enable */
	__raw_writel(0x12, SDRC_SYSCONFIG);
	while (!__raw_readl(SDRC_STATUS))
		asm("nop");

	/* Immediate idle ack, RESET disable */
	__raw_writel(0x10, SDRC_SYSCONFIG);

	/* clear reset sources */
	__raw_writel(0xfff, PRM_RSTTST);

	/* SDRC sharing register
	 * 32-bit SDRAM on data lan [32:0] - CS0, pin tri-stated = 1 */
	__raw_writel(0x00000100, SDRC_SHARING);

	/* SDRC CS0 configuration */
	__raw_writel(p->mcfg[0], SDRC_MCFG_0);
	__raw_writel(p->mcfg[1], SDRC_MCFG_1);

	/* SDRC RFR_CTRL */
	__raw_writel(p->rfr[0], SDRC_RFR_CTRL);
	__raw_writel(p->rfr[1], SDRC_RFR_CTRL_1);

	/* SDRC ACTIMA */
	__raw_writel(p->actima[0], SDRC_ACTIM_CTRLA_0);
	__raw_writel(p->actima[1], SDRC_ACTIM_CTRLA_1);

	/* SDRC ACTIMB */
	__raw_writel(p->actimb[0], SDRC_ACTIM_CTRLB_0);
	__raw_writel(p->actimb[1], SDRC_ACTIM_CTRLB_1);

	/* SDRC power */
	__raw_writel(p->power, SDRC_POWER);

	/* SDRC manual command register */
	__raw_writel(CMD_NOP, SDRC_MANUAL_0);	/* NOP command */
	delay(5000);
	__raw_writel(CMD_PRECHARGE, SDRC_MANUAL_0);	/* NOP command */
	__raw_writel(CMD_AUTOREFRESH, SDRC_MANUAL_0);	/* Auto-refresh command */
	__raw_writel(CMD_AUTOREFRESH, SDRC_MANUAL_0);	/* Auto-refresh command */

	/* SDRC MR0 register */
	__raw_writel(p->mr[0], SDRC_MR_0);

	/* SDRC EMR0 register */
	__raw_writel(p->emr[0], SDRC_EMR_0);

	if (p->mr[1]) {
		__raw_writel(CMD_NOP, SDRC_MANUAL_1);	/* NOP command */
		delay(5000);
		__raw_writel(CMD_PRECHARGE, SDRC_MANUAL_1);	/* NOP command */
		__raw_writel(CMD_AUTOREFRESH, SDRC_MANUAL_1);	/* Auto-refresh command */
		__raw_writel(CMD_AUTOREFRESH, SDRC_MANUAL_1);	/* Auto-refresh command */

		__raw_writel(p->mr[1], SDRC_MR_1);
		__raw_writel(p->emr[1], SDRC_EMR_1);

		__raw_writel(p->cfg, SDRC_CS_CFG);
	}

	/* Clear the enabe DLL bit to use DLLA in lock mode */
	__raw_writel(p->dlla, SDRC_DLLA_CTRL);
	delay(0x2000);		/* give time to lock */

	if (p->dllb) {
		__raw_writel(p->dllb, SDRC_DLLB_CTRL);
		delay(0x2000);		/* give time to lock */
	}
}


void config_dm3730logic_ddr(void)
{
	int i;

	for (i=0; i<sizeof(sdram_timings)/sizeof(sdram_timings[0]); ++i) {
		sdrc_ddr(&sdram_timings[i]);
		if (sdrc_probe(&sdram_timings[i], 0x80000000))
			return;
	}
}

char *config_dm3730logic_ddr_type(void)
{
	int i;
	for (i=0; i<sizeof(sdram_timings)/sizeof(sdram_timings[0]); ++i) {
		struct sdram_timings *p = &sdram_timings[i];

		if (__raw_readl(SDRC_ACTIM_CTRLA_0) != p->actima[0])
			continue;
		if (__raw_readl(SDRC_ACTIM_CTRLA_1) != p->actima[1])
			continue;
		if (__raw_readl(SDRC_ACTIM_CTRLB_0) != p->actimb[0])
			continue;
		if (__raw_readl(SDRC_ACTIM_CTRLB_1) != p->actimb[1])
			continue;
		if (__raw_readl(SDRC_MCFG_0) != p->mcfg[0])
			continue;
		if (__raw_readl(SDRC_MCFG_1) != p->mcfg[1])
			continue;
		if (__raw_readl(SDRC_RFR_CTRL) != p->rfr[0])
			continue;
		if (__raw_readl(SDRC_RFR_CTRL_1) != p->rfr[1])
			continue;
		return p->name;
	}
	return "???";
}
