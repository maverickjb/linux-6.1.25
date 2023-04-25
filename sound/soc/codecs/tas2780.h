/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * TAS2780.h - ALSA SoC Texas Instruments TAS2780 Mono Audio Amplifier
 *
 * Copyright (C) 2020-2022 Texas Instruments Incorporated - https://www.ti.com
 *
 * Author: Raphael Xu <raphael-xu@ti.com>
 */

#ifndef __TAS2780_H__
#define __TAS2780_H__

/* Book Control Register */
#define TAS2780_BOOKCTL_PAGE	0
#define TAS2780_BOOKCTL_REG	127
#define TAS2780_REG(page, reg)	((page * 128) + reg)

/* Page */
#define TAS2780_PAGE		TAS2780_REG(0X0, 0x00)
#define TAS2780_PAGE_PAGE_MASK	255

/* Software Reset */
#define TAS2780_SW_RST	TAS2780_REG(0X0, 0x01)
#define TAS2780_RST	BIT(0)

/* Power Control */
#define TAS2780_PWR_CTRL		TAS2780_REG(0X0, 0x02)
#define TAS2780_PWR_CTRL_MASK		GENMASK(1, 0)
#define TAS2780_PWR_CTRL_ACTIVE		0x0
#define TAS2780_PWR_CTRL_MUTE		BIT(0)
#define TAS2780_PWR_CTRL_SHUTDOWN	BIT(1)

#define TAS2780_VSENSE_POWER_EN		3
#define TAS2780_ISENSE_POWER_EN		4

/* Digital Volume Control */
#define TAS2780_DVC	TAS2780_REG(0X0, 0x1a)
#define TAS2780_DVC_MAX	0xc9

#define TAS2780_CHNL_0  TAS2780_REG(0X0, 0x03)

/* TDM Configuration Reg0 */
#define TAS2780_TDM_CFG0		TAS2780_REG(0X0, 0x08)
#define TAS2780_TDM_CFG0_SMP_MASK	BIT(5)
#define TAS2780_TDM_CFG0_SMP_48KHZ	0x0
#define TAS2780_TDM_CFG0_SMP_44_1KHZ	BIT(5)
#define TAS2780_TDM_CFG0_MASK		GENMASK(3, 1)
#define TAS2780_TDM_CFG0_44_1_48KHZ	BIT(3)
#define TAS2780_TDM_CFG0_88_2_96KHZ	(BIT(3) | BIT(1))

/* TDM Configuration Reg1 */
#define TAS2780_TDM_CFG1		TAS2780_REG(0X0, 0x09)
#define TAS2780_TDM_CFG1_MASK		GENMASK(5, 1)
#define TAS2780_TDM_CFG1_51_SHIFT	1
#define TAS2780_TDM_CFG1_RX_MASK	BIT(0)
#define TAS2780_TDM_CFG1_RX_RISING	0x0
#define TAS2780_TDM_CFG1_RX_FALLING	BIT(0)

/* TDM Configuration Reg2 */
#define TAS2780_TDM_CFG2		TAS2780_REG(0X0, 0x0a)
#define TAS2780_TDM_CFG2_RXW_MASK	GENMASK(3, 2)
#define TAS2780_TDM_CFG2_RXW_16BITS	0x0
#define TAS2780_TDM_CFG2_RXW_24BITS	BIT(3)
#define TAS2780_TDM_CFG2_RXW_32BITS	(BIT(3) | BIT(2))
#define TAS2780_TDM_CFG2_RXS_MASK	GENMASK(1, 0)
#define TAS2780_TDM_CFG2_RXS_16BITS	0x0
#define TAS2780_TDM_CFG2_RXS_24BITS	BIT(0)
#define TAS2780_TDM_CFG2_RXS_32BITS	BIT(1)
#define TAS2780_TDM_CFG2_SCFG_MASK	GENMASK(5, 4)
#define TAS2780_TDM_CFG2_SCFG_I2S	0x0
#define TAS2780_TDM_CFG2_SCFG_LEFT_J	BIT(4)
#define TAS2780_TDM_CFG2_SCFG_RIGHT_J	BIT(5)

/* TDM Configuration Reg3 */
#define TAS2780_TDM_CFG3		TAS2780_REG(0X0, 0x0c)
#define TAS2780_TDM_CFG3_RXS_MASK	GENMASK(7, 4)
#define TAS2780_TDM_CFG3_RXS_SHIFT	0x4
#define TAS2780_TDM_CFG3_MASK		GENMASK(3, 0)

/* TDM Configuration Reg4 */
#define TAS2780_TDM_CFG4		TAS2780_REG(0X0, 0x0d)
#define TAS2780_TDM_CFG4_TX_OFFSET_MASK	GENMASK(3, 1)

/* TDM Configuration Reg5 */
#define TAS2780_TDM_CFG5		TAS2780_REG(0X0, 0x0e)
#define TAS2780_TDM_CFG5_VSNS_MASK	BIT(6)
#define TAS2780_TDM_CFG5_VSNS_ENABLE	BIT(6)
#define TAS2780_TDM_CFG5_50_MASK	GENMASK(5, 0)

/* TDM Configuration Reg6 */
#define TAS2780_TDM_CFG6		TAS2780_REG(0X0, 0x0f)
#define TAS2780_TDM_CFG6_ISNS_MASK	BIT(6)
#define TAS2780_TDM_CFG6_ISNS_ENABLE	BIT(6)
#define TAS2780_TDM_CFG6_50_MASK	GENMASK(5, 0)

/* IC CFG */
#define TAS2780_IC_CFG			TAS2780_REG(0X0, 0x5c)
#define TAS2780_IC_CFG_MASK		GENMASK(7, 6)
#define TAS2780_IC_CFG_ENABLE		(BIT(7) | BIT(6))

#endif /* __TAS2780_H__ */
