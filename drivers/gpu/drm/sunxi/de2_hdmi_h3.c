/*
 * Allwinner A83T and H3 HDMI lowlevel functions
 *
 * Copyright (C) 2016 Jean-Francois Moine <moinejf@free.fr>
 *
 * Adapted from the files
 *	lichee/linux-3.4/drivers/video/sunxi/disp2/hdmi/aw/
 *		hdmi_bsp_sun8iw6.c and hdmi_bsp_sun8iw7.c
 * with no license nor copyright.
 */

/*
 * from https://linux-sunxi.org/DWC_HDMI_Controller
 * Synopsys DesignWare HDMI controller
 * doc: https://www.synopsys.com/dw/doc.php/ds/c/dwc_hdmi_tx_csds.pdf
 * PHY (unknown)
 *	10000: ?
 *		01
 *	10001:
 *		03 or 00
 *	10002:
 *		100+5 = 69
 *	10003:
 *		00
 *	10007: (a83t only)
 *		init: a0, stanby: 20
 *	10010: HDMI_READ_LOCK
 *	10020: HDMI_H3_PHY_CTRL
 *		00000001: enable ?
 *		00000002: ??
 *		00000004: ??
 *		00000008: ??
 *		00000070: ??
 *		00000080: ??
 *		00000f00: video enable
 *		00010000: ??
 *		00040000: ??
 *		00080000: ??
 *		init: 01ff0f7f
 *	10024:
 *		init: 80639000
 *	10028:
 *		init: 0f81c405
 *	1002c:
 *		init: 39dc5040, | 02000000, | (parts of 10038)
 *	10030: HDMI_H3_PHY_CLK (clock divider parent PLL3 (video)
 *		init: 80084343
 *	10034:
 *		init:00000001
 *	10038: HDMI_H3_PHY_STATUS
 *		00000080: reset ok ?
 *		0001f800: ? -> (>> 11) | 1002c
 *		00080000: HPD status
 *		c0000000: ?? -> | 1002c
 */

#include <drm/drmP.h>

#include "de2_hdmi.h"
#include "de2_hdmi_h3.h"

/*
 * [0] = vic (cea Video ID)
 * [1] used in hdmi_phy_set / bsp_hdmi_audio
 * [2..17] used in bsp_hdmi_video
 */
static const struct para_tab {
	u32 para[18];
} ptbl[] = {
	{{  6,  1, 1,  1,  5,  3, 0, 1, 4, 0, 0, 160,  20,  38, 124, 240, 22, 0}},
	{{ 21, 11, 1,  1,  5,  3, 1, 1, 2, 0, 0, 160,  32,  24, 126,  32, 24, 0}},
	{{  2, 11, 0,  0,  2,  6, 1, 0, 9, 0, 0, 208, 138,  16,  62, 224, 45, 0}},
	{{ 17, 11, 0,  0,  2,  5, 2, 0, 5, 0, 0, 208, 144,  12,  64,  64, 49, 0}},
	{{ 19,  4, 0, 96,  5,  5, 2, 2, 5, 1, 0,   0, 188, 184,  40, 208, 30, 1}},
	{{  4,  4, 0, 96,  5,  5, 2, 1, 5, 0, 0,   0, 114, 110,  40, 208, 30, 1}},
	{{ 20,  4, 0, 97,  7,  5, 4, 2, 2, 2, 0, 128, 208,  16,  44,  56, 22, 1}},
	{{  5,  4, 0, 97,  7,  5, 4, 1, 2, 0, 0, 128,  24,  88,  44,  56, 22, 1}},
	{{ 31,  2, 0, 96,  7,  5, 4, 2, 4, 2, 0, 128, 208,  16,  44,  56, 45, 1}},
	{{ 16,  2, 0, 96,  7,  5, 4, 1, 4, 0, 0, 128,  24,  88,  44,  56, 45, 1}},
	{{ 32,  4, 0, 96,  7,  5, 4, 3, 4, 2, 0, 128,  62, 126,  44,  56, 45, 1}},
	{{ 33,  4, 0,  0,  7,  5, 4, 2, 4, 2, 0, 128, 208,  16,  44,  56, 45, 1}},
	{{ 34,  4, 0,  0,  7,  5, 4, 1, 4, 0, 0, 128,  24,  88,  44,  56, 45, 1}},
#if 0
	{{160,  2, 0, 96,  7,  5, 8, 3, 4, 1, 0, 128,  62, 126,  44, 157, 45, 1}},
	{{147,  2, 0, 96,  5,  5, 5, 2, 5, 1, 0,   0, 188, 184,  40, 190, 30, 1}},
	{{132,  2, 0, 96,  5,  5, 5, 1, 5, 0, 0,   0, 114, 110,  40, 160, 30, 1}},
	{{257,  1, 0, 96, 15, 10, 8, 2, 8, 0, 0,   0,  48, 176,  88, 112, 90, 1}},
	{{258,  1, 0, 96, 15, 10, 8, 5, 8, 4, 0,   0, 160,  32,  88, 112, 90, 1}},
#endif
};

#if IS_ENABLED(CONFIG_SND_SOC)
/* audio */
#if 0 /* 'ca' always 0 */
static const unsigned char ca_table[] = {
	0x11, 0x13, 0x31, 0x33, 0x15, 0x17, 0x35, 0x37,
	0x55, 0x57, 0x75, 0x77, 0x5d, 0x5f, 0x7d, 0x7f, 
	0xdd, 0xdf, 0xfd, 0xff, 0x99, 0x9b, 0xb9, 0xbb,
	0x9d, 0x9f, 0xbd, 0xbf, 0xdd, 0xdf, 0xfd, 0xff,
};
#endif

/* HDMI_FC_AUDSCHNLS7 values */
static const struct pcm_sf {
	u32 	sf;
	unsigned char	cs_sf;
} sf[] = {
	{44100,	0x00},
	{48000, 0x02},
	{96000, 0x0a},
	{192000,0x0e},
	{22050,	0x04},
	{24000,	0x06},
	{32000, 0x03},
	{88200,	0x08},
	{768000,0x09},
	{176400,0x0c},
};

static const struct {
	int rate;
	unsigned short n1, n2;
} n_table[] = {
	{32000,	3072,	4096},
	{44100,	4704,	6272},
	{88200,	4704*2,	6272*2},
	{176400,4704*4,	6272*4},
	{48000,	5120,	6144},
	{96000,	5120*2,	6144*2},
	{192000,5120*4,	6144*4},
};
#endif

static inline void hdmi_writeb(struct de2_hdmi_priv *priv,
			u32 addr, u8 data)
{
	writeb_relaxed(data, priv->mmio + addr);
}

static inline void hdmi_writel(struct de2_hdmi_priv *priv,
			u32 addr, u32 data)
{
	writel_relaxed(data, priv->mmio + addr);
}

static inline u8 hdmi_readb(struct de2_hdmi_priv *priv,
			u32 addr)
{
	return readb_relaxed(priv->mmio + addr);
}

static inline u32 hdmi_readl(struct de2_hdmi_priv *priv,
			u32 addr)
{
	return readl_relaxed(priv->mmio + addr);
}

static void hdmi_read_lock(struct de2_hdmi_priv *priv)
{
	hdmi_writeb(priv, 0x10010, 0x45);
	hdmi_writeb(priv, 0x10011, 0x45);
	hdmi_writeb(priv, 0x10012, 0x52);
	hdmi_writeb(priv, 0x10013, 0x54);
}
static void hdmi_read_unlock(struct de2_hdmi_priv *priv)
{
	hdmi_writeb(priv, 0x10010, 0x52);
	hdmi_writeb(priv, 0x10011, 0x54);
	hdmi_writeb(priv, 0x10012, 0x41);
	hdmi_writeb(priv, 0x10013, 0x57);
}

static void bsp_hdmi_inner_init(struct de2_hdmi_priv *priv)
{
	hdmi_read_lock(priv);

	// software reset
	hdmi_writeb(priv, 0x8080,  0x00);		// 4002 HDMI_MC_SWRSTZ
	udelay(2);

	// mask all interrupt
	hdmi_writeb(priv, 0xf01f, 0x00);		// 01ff HDMI_IH_MUTE
	hdmi_writeb(priv, 0x8403, 0xff);		// 0807 HDMI_VP_MASK
	hdmi_writeb(priv, 0x904c, 0xff);		// 10d2 HDMI_FC_MASK0
	hdmi_writeb(priv, 0x904e, 0xff);		// 10d6 HDMI_FC_MASK1
	hdmi_writeb(priv, 0xd04c, 0xff);		// 10da HDMI_FC_MASK2
	hdmi_writeb(priv, 0x8250, 0xff);		// 3102 HDMI_AUD_INT
	hdmi_writeb(priv, 0x8a50, 0xff);		// 3302 HDMI_AUD_SPDIFINT
	hdmi_writeb(priv, 0x8272, 0xff);		// 3506 HDMI_GP_POL
	hdmi_writeb(priv, 0x40c0, 0xff);		// 5008 HDMI_A_APIINTMSK
	hdmi_writeb(priv, 0x86f0, 0xff);		// 7d02 HDMI_CEC_MASK
	hdmi_writeb(priv, 0x0ee3, 0xff);		// 7e05 HDMI_I2CM_INT
	hdmi_writeb(priv, 0x8ee2, 0xff);		// 7e06 HDMI_I2CM_CTLINT

	hdmi_writeb(priv, 0xa049, 0xf0);		// 1063 HDMI_FC_AUDSCONF
	hdmi_writeb(priv, 0xb045, 0x1e);		// 10b3 HDMI_FC_DATAUTO0
	hdmi_writeb(priv, 0x00c1, 0x00);		// 5001 HDMI_A_HDCPCFG1
	hdmi_writeb(priv, 0x00c1, 0x03);
	hdmi_writeb(priv, 0x00c0, 0x00);		// 5000 HDMI_A_HDCPCFG0
	hdmi_writeb(priv, 0x40c1, 0x10);		// 5009 HDMI_A_VIDPOLCFG
	if (priv->soc_type == SOC_H3) {
// from FriendlyARM - tkaiser
		hdmi_writeb(priv, 0x0081, 0xfd);	// 4001 HDMI_MC_CLKDIS
		hdmi_writeb(priv, 0x0081, 0x00);
		hdmi_writeb(priv, 0x0081, 0xfd);
	} else {
		hdmi_writeb(priv, 0x0081, 0xff);	// 4001 HDMI_MC_CLKDIS
		hdmi_writeb(priv, 0x0081, 0x00);
		hdmi_writeb(priv, 0x0081, 0xff);
	}
	hdmi_writeb(priv, 0x0010, 0xff);		// 0100 HDMI_IH_FC_STAT0
	hdmi_writeb(priv, 0x0011, 0xff);		// 0101 HDMI_IH_FC_STAT1
	hdmi_writeb(priv, 0x8010, 0xff);		// 0102 HDMI_IH_FC_STAT2
	hdmi_writeb(priv, 0x8011, 0xff);		// 0103 HDMI_IH_AS_STAT0
	hdmi_writeb(priv, 0x0013, 0xff);		// 0105 HDMI_IH_I2CM_STAT0
	hdmi_writeb(priv, 0x8012, 0xff);		// 0106 HDMI_IH_CEC_STAT0
	hdmi_writeb(priv, 0x8013, 0xff);		// 0107 HDMI_IH_VP_STAT0
}

static void hdmi_phy_init_a83t(struct de2_hdmi_priv *priv)
{
	bsp_hdmi_inner_init(priv);

	hdmi_writeb(priv, 0x10000, 0x01);
	hdmi_writeb(priv, 0x10001, 0x00);
	hdmi_writeb(priv, 0x10002, 100 + 5);
	hdmi_writeb(priv, 0x10003, 0x00);
	hdmi_writeb(priv, 0x10007, 0xa0);
	hdmi_writeb(priv, 0x0083, 0x01);	// 4005 HDMI_MC_PHYRSTZ
	udelay(1);
	hdmi_writeb(priv, 0x0240, 0x06);	// 3000 HDMI_PHY_CONF0
						// seldataenpol | gen2_enhpdrxsense
	hdmi_writeb(priv, 0x0240, 0x16);	// | gen2_pddq
	hdmi_writeb(priv, 0x0240, 0x12);	// ^ gen2_enhpdrxsense
	hdmi_writeb(priv, 0x8242, 0xf0);	// 3006	HDMI_PHY_MASK0
	hdmi_writeb(priv, 0xa243, 0xff);	// 3007 HDMI_PHY_POL0
	hdmi_writeb(priv, 0x6240, 0xff);	// 3000	HDMI_PHY_CONF0
	hdmi_writeb(priv, 0x0012, 0xff);	// 0104 HDMI_IH_PHY_STAT0
	hdmi_writeb(priv, 0x4010, 0xff);	// 0108 HDMI_IH_I2CMPHY_STAT0
	hdmi_writeb(priv, 0x0083, 0x00);	// 4005 HDMI_MC_PHYRSTZ
	hdmi_writeb(priv, 0x0240, 0x16);	// 3000 HDMI_PHY_CONF0
	hdmi_writeb(priv, 0x0240, 0x06);
	hdmi_writeb(priv, 0x0241, 0x20);	// 3001 HDMI_PHY_TST0
						// testclr
	hdmi_writeb(priv, 0x2240, 100 + 5);	// 3020 HDMI_PHY_I2CM_SLAVE_ADDR
	hdmi_writeb(priv, 0x0241, 0x00);	// 3001
}

static void hdmi_phy_init_h3(struct de2_hdmi_priv *priv)
{
	int to_cnt;
	u32 tmp;

	hdmi_writel(priv, 0x10020, 0);
	hdmi_writel(priv, 0x10020, 1 << 0);
	udelay(5);
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 16));
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 1));
	udelay(10);
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 2));
	udelay(5);
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 3));
	usleep_range(40, 50);
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 19));
	usleep_range(100, 120);
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 18));
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (7 << 4));

	to_cnt = 10;
	while (1) {
		if (hdmi_readl(priv, 0x10038) & 0x80)
			break;
		usleep_range(200, 250);
		if (--to_cnt == 0) {
			pr_warn("hdmi phy init timeout\n");
			break;
		}
	}

	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (0xf << 8));
	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) | (1 << 7));

	hdmi_writel(priv, 0x1002c, 0x39dc5040);
	hdmi_writel(priv, 0x10030, 0x80084343);
	msleep(10);
	hdmi_writel(priv, 0x10034, 0x00000001);
	hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) | 0x02000000);
	msleep(100);
	tmp = hdmi_readl(priv, 0x10038);
	hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) | 0xc0000000);
	hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
						((tmp >> 11) & 0x3f));
	hdmi_writel(priv, 0x10020, 0x01ff0f7f);
	hdmi_writel(priv, 0x10024, 0x80639000);
	hdmi_writel(priv, 0x10028, 0x0f81c405);

	bsp_hdmi_inner_init(priv);
}

void bsp_hdmi_init(struct de2_hdmi_priv *priv)
{
	if (priv->soc_type == SOC_H3)
		hdmi_phy_init_h3(priv);
	else
		hdmi_phy_init_a83t(priv);
#if 0
//test
// -> 13:2a:a0:c1 bf 02 fe 00 (A83T and H3)
pr_info("*jfm* hdmi type %02x:%02x:%02x:%02x %02x %02x %02x %02x\n",
hdmi_readb(priv, 0x0000),	// 0000 HDMI_DESIGN_ID
hdmi_readb(priv, 0x0001),	// 0001 HDMI_REVISION_ID
hdmi_readb(priv, 0x8000),	// 0002 HDMI_PRODUCT_ID0 (a0 for HDMI TX)
hdmi_readb(priv, 0x8001),	// 0003 HDMI_PRODUCT_ID1 (01 / c1 if HDCP enscypt)
hdmi_readb(priv, 0x0002),	// 0004 HDMI_CONFIG0_ID
//					pix rep, hbr, spdif, i2s, 1.4, col, cec, hdcp
hdmi_readb(priv, 0x0003),	// 0005 HDMI_CONFIG1_ID
//					x, x, x, sfr, i2c, ocp, apb, ahb
hdmi_readb(priv, 0x8002),	// 0006 HDMI_CONFIG2_ID
//					00: legacy, f2: 3D, e2: 3D+HEC
hdmi_readb(priv, 0x8003));	// 0007 HDMI_CONFIG3_ID
//					x, x, x, x, x, x, x, audio
#endif
}

static int get_vid(u32 id)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(ptbl); i++) {
		if (id == ptbl[i].para[0])
			return i;
	}

	return -1;
}

static int hdmi_phy_set_h3(struct de2_hdmi_priv *priv, int i)
{
	u32 tmp;

	hdmi_writel(priv, 0x10020, hdmi_readl(priv, 0x10020) & ~0xf000);
	switch (ptbl[i].para[1]) {
	case 1:
		hdmi_writel(priv, 0x1002c, 0x31dc5fc0);	/* or 0x30dc5fc0 ? */
		hdmi_writel(priv, 0x10030, 0x800863c0);
		msleep(10);
		hdmi_writel(priv, 0x10034, 0x00000001);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
						0x02000000);
		msleep(200);
		tmp = (hdmi_readl(priv, 0x10038) >> 11) & 0x3f;
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0xc0000000);
		if (tmp < 0x3d)
			tmp += 2;
		else
			tmp = 0x3f;
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) | tmp);
		msleep(100);
		hdmi_writel(priv, 0x10020, 0x01ffff7f);
		hdmi_writel(priv, 0x10024, 0x8063b000);
		hdmi_writel(priv, 0x10028, 0x0f8246b5);
		break;
	case 2:				/* 1080P @ 60 & 50 */
		hdmi_writel(priv, 0x1002c, 0x39dc5040);
		hdmi_writel(priv, 0x10030, 0x80084381);
		msleep(10);
		hdmi_writel(priv, 0x10034, 0x00000001);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0x02000000);
		msleep(100);
		tmp = (hdmi_readl(priv, 0x10038) >> 11) & 0x3f;
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0xc0000000);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) | tmp);
		hdmi_writel(priv, 0x10020, 0x01ffff7f);
		hdmi_writel(priv, 0x10024, 0x8063a800);
		hdmi_writel(priv, 0x10028, 0x0f81c485);
		break;
	case 4:				/* 720P @ 50 & 60, 1080I, 1080P */
		hdmi_writel(priv, 0x1002c, 0x39dc5040);
		hdmi_writel(priv, 0x10030, 0x80084343);
		msleep(10);
		hdmi_writel(priv, 0x10034, 0x00000001);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0x02000000);
		msleep(100);
		tmp = (hdmi_readl(priv, 0x10038) >> 11) & 0x3f;
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0xc0000000);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) | tmp);
		hdmi_writel(priv, 0x10020, 0x01ffff7f);
		hdmi_writel(priv, 0x10024, 0x8063b000);
		hdmi_writel(priv, 0x10028, 0x0f81c405);
		break;
	case 11:				/* 480P/576P */
		hdmi_writel(priv, 0x1002c, 0x39dc5040);
		hdmi_writel(priv, 0x10030, 0x8008430a);
		msleep(10);
		hdmi_writel(priv, 0x10034, 0x00000001);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0x02000000);
		msleep(100);
		tmp = (hdmi_readl(priv, 0x10038) >> 11) & 0x3f;
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) |
							0xc0000000);
		hdmi_writel(priv, 0x1002c, hdmi_readl(priv, 0x1002c) | tmp);
		hdmi_writel(priv, 0x10020, 0x01ffff7f);
		hdmi_writel(priv, 0x10024, 0x8063b000);
		hdmi_writel(priv, 0x10028, 0x0f81c405);
		break;
	default:
		return -1;
	}

	return 0;
}

static void hdmi_i2cm_write(struct de2_hdmi_priv *priv,
			    int addr, u8 valh, u8 vall)
{
	hdmi_writeb(priv, 0x2241, addr); // 3021 HDMI_PHY_I2CM_ADDRESS_ADDR
	hdmi_writeb(priv, 0xa240, valh); // 3022 HDMI_PHY_I2CM_DATAO_1_ADDR
	hdmi_writeb(priv, 0xa241, vall); // 3023 HDMI_PHY_I2CM_DATAO_0_ADDR
	hdmi_writeb(priv, 0xa242, 0x10); // 3026 HDMI_PHY_I2CM_OPERATION_ADDR
					 // write
	usleep_range(2000, 2500);
}

static int hdmi_phy_set_a83t(struct de2_hdmi_priv *priv, int i)
{
	switch (ptbl[i].para[1]) {
	case 1:
		hdmi_i2cm_write(priv, 0x06, 0x00, 0x00);
		hdmi_i2cm_write(priv, 0x15, 0x00, 0x0f);
		hdmi_i2cm_write(priv, 0x10, 0x00, 0x00);
		hdmi_i2cm_write(priv, 0x19, 0x00, 0x02);
		hdmi_i2cm_write(priv, 0x0e, 0x00, 0x00);
		hdmi_i2cm_write(priv, 0x09, 0x80, 0x2b);
		break;
	case 2:				/* 1080P @ 60 & 50 */
		hdmi_i2cm_write(priv, 0x06, 0x04, 0xa0);
		hdmi_i2cm_write(priv, 0x15, 0x00, 0x0a);
		hdmi_i2cm_write(priv, 0x10, 0x00, 0x00);
		hdmi_i2cm_write(priv, 0x19, 0x00, 0x02);
		hdmi_i2cm_write(priv, 0x0e, 0x00, 0x21);
		hdmi_i2cm_write(priv, 0x09, 0x80, 0x29);
		break;
	case 4:				/* 720P @ 50 & 60, 1080I, 1080P */
		hdmi_i2cm_write(priv, 0x06, 0x05, 0x40);
		hdmi_i2cm_write(priv, 0x15, 0x00, 0x05);
		hdmi_i2cm_write(priv, 0x10, 0x00, 0x00);
		hdmi_i2cm_write(priv, 0x19, 0x00, 0x07);
		hdmi_i2cm_write(priv, 0x0e, 0x02, 0xb5);
		hdmi_i2cm_write(priv, 0x09, 0x80, 0x09);
		break;
	case 11:				/* 480P/576P */
		hdmi_i2cm_write(priv, 0x06, 0x01,
					ptbl[i].para[2] ? 0xe3 : 0xe0);
		hdmi_i2cm_write(priv, 0x15, 0x00, 0x00);
		hdmi_i2cm_write(priv, 0x10, 0x08, 0xda);
		hdmi_i2cm_write(priv, 0x19, 0x00, 0x07);
		hdmi_i2cm_write(priv, 0x0e, 0x03, 0x18);
		hdmi_i2cm_write(priv, 0x09, 0x80, 0x09);
		break;
	default:
		return -1;
	}
	hdmi_i2cm_write(priv, 0x1e, 0x00, 0x00);
	hdmi_i2cm_write(priv, 0x13, 0x00, 0x00);
	hdmi_i2cm_write(priv, 0x17, 0x00, 0x00);
	hdmi_writeb(priv, 0x0240, 0x0e);	// 3000 HDMI_PHY_CONF0

	return 0;
}

void bsp_hdmi_set_video_en(struct de2_hdmi_priv *priv,
			unsigned char enable)
{
	if (priv->soc_type == SOC_H3) {
		if (enable)
			hdmi_writel(priv, 0x10020,
				hdmi_readl(priv, 0x10020) | (0x0f << 12));
		else
			hdmi_writel(priv, 0x10020,
				hdmi_readl(priv, 0x10020) & ~(0x0f << 12));
	} else {
#if 0
		hdmi_writeb(priv, 0x0840, enable ? 0 : 1);	// 1200
#endif
	}
}

#if IS_ENABLED(CONFIG_SND_SOC)
/* start audio */
int bsp_hdmi_audio(struct de2_hdmi_priv *priv,
		int sample_rate, int sample_bit)
{
	int id = get_vid(priv->cea_mode);	/* ptbl index */
	unsigned int i, n;

//test
pr_info("hdmi audio cea %d id %d rate %d bit %d\n",
 priv->cea_mode, id, sample_rate, sample_bit);
	if (id < 0)
		return id;

// always 2 channels
//	hdmi_writeb(priv, 0xa049, audio->ch_num > 2 ? 0xf1 : 0xf0);
	hdmi_writeb(priv, 0xa049, 0xf0);		// 1063 HDMI_FC_AUDSCONF

#if 1 // audio->ca always 0
	hdmi_writeb(priv, 0x204b, ~0x11);		// 1065 HDMI_FC_AUDSV
#else
	i = audio->ca;
	if (i < ARRAY_SIZE(ca_table))
		hdmi_writeb(priv, 0x204b, ~ca_table[i]);
#endif

	hdmi_writeb(priv, 0xa04a, 0x00);		// 1066 HDMI_FC_AUDSU
	hdmi_writeb(priv, 0xa04b, 0x30);		// 1067 HDMI_FC_AUDSCHNLS0
	hdmi_writeb(priv, 0x6048, 0x00);		// 1068 HDMI_FC_AUDSCHNLS1
	hdmi_writeb(priv, 0x6049, 0x01);		// 1069 HDMI_FC_AUDSCHNLS2
	hdmi_writeb(priv, 0xe048, 0x42);		// 106a HDMI_FC_AUDSCHNLS3
	hdmi_writeb(priv, 0xe049, 0x86);		// 106b HDMI_FC_AUDSCHNLS4
	hdmi_writeb(priv, 0x604a, 0x31);		// 106c HDMI_FC_AUDSCHNLS5
	hdmi_writeb(priv, 0x604b, 0x75);		// 106d HDMI_FC_AUDSCHNLS6
	hdmi_writeb(priv, 0xe04a, 0x01);		// 106e HDMI_FC_AUDSCHNLS7
	for (i = 0; i < ARRAY_SIZE(sf); i++) {
		if (sample_rate == sf[i].sf) {
			hdmi_writeb(priv, 0xe04a, sf[i].cs_sf); // 106e
			break;
		}
	}
	hdmi_writeb(priv, 0xe04b,			// 106f HDMI_FC_AUDSCHNLS8
		(sample_bit == 16) ? 0x02 :
			(sample_bit == 24 ? 0x0b :
					    0x00));

	hdmi_writeb(priv, 0x0251, sample_bit);		// 3101 HDMI_AUD_CONF1

	n = 6272;
	for (i = 0; i < ARRAY_SIZE(n_table); i++) {
		if (sample_rate == n_table[i].rate) {
			if (ptbl[id].para[1] == 1)
				n = n_table[i].n1;
			else
				n = n_table[i].n2;
			break;
		}
	}

	hdmi_writeb(priv, 0x0a40, n);			// 3200 HDMI_AUD_N1
	hdmi_writeb(priv, 0x0a41, n >> 8);		// 3201 HDMI_AUD_N2
	hdmi_writeb(priv, 0x8a40, n >> 16);		// 3202 HDMI_AUD_N3
	hdmi_writeb(priv, 0x0a43, 0x00);		// 3205 HDMI_AUD_CTS3
	hdmi_writeb(priv, 0x8a42, 0x04);		// 3206 HDMI_AUD_INPUTCLKFS
// always 2 channels
//	hdmi_writeb(priv, 0xa049, audio->ch_num > 2 ? 0x01 : 0x00);
//	hdmi_writeb(priv, 0x2043, audio->ch_num * 16);
	hdmi_writeb(priv, 0xa049, 0x00);		// 1063 HDMI_FC_AUDSCONF
							//	layout0
	hdmi_writeb(priv, 0x2043, 2 * 16);		// 1025 HDMI_FC_AUDICONF0
	hdmi_writeb(priv, 0xa042, 0x00);		// 1026 HDMI_FC_AUDICONF1
// audio->ca always 0 for 2 channels
//	hdmi_writeb(priv, 0xa043, audio->ca);
	hdmi_writeb(priv, 0xa043, 0x00);		// 1027 HDMI_FC_AUDICONF2
	hdmi_writeb(priv, 0x6040, 0x00);		// 1028 HDMI_FC_AUDICONF3

#if 1 // PCM only
	hdmi_writeb(priv, 0x8251, 0x00);		// 3103 HDMI_AUD_CONF2
#else
	if (audio->type == PCM) {
		hdmi_writeb(priv, 0x8251, 0x00);
	} else if (audio->type == DTS_HD || audio->type == DDP) {
		hdmi_writeb(priv, 0x8251, 0x03);
		hdmi_writeb(priv, 0x0251, 0x15);
		hdmi_writeb(priv, 0xa043, 0);
	} else {
		hdmi_writeb(priv, 0x8251, 0x02);
		hdmi_writeb(priv, 0x0251, 0x15);
		hdmi_writeb(priv, 0xa043, 0);
	}
#endif

	hdmi_writeb(priv, 0x0250, 0x00);		// 3100 HDMI_AUD_CONF0
	hdmi_writeb(priv, 0x0081, 0x08);		// 4001 HDMI_MC_CLKDIS
							//	AUDCLK_DISABLE
	hdmi_writeb(priv, 0x8080, 0xf7);		// 4002 HDMI_MC_SWRSTZ
	usleep_range(100, 120);
	hdmi_writeb(priv, 0x0250, 0xaf);		// 3100 HDMI_AUD_CONF0
	usleep_range(100, 120);
	hdmi_writeb(priv, 0x0081, 0x00);		// 4001 HDMI_MC_CLKDIS
							//	enable all clocks

	return 0;
}
#endif


/* initialize */
int bsp_hdmi_video(struct de2_hdmi_priv *priv)
{
	int i = get_vid(priv->cea_mode);	/* ptbl index */
	int csc;				/* color space */

	if (i < 0)
		return i;

	switch (priv->cea_mode) {
	case 2:				/* 480P */
	case 4: // (a83t)
	case 6:				/* 1440x480I */
	case 17:			/* 576P */
	case 19: // (a83t)
	case 21:			/* 1440x576I */
		csc = 1;		/* BT601 */
		break;
	default:
		csc = 2;		/* BT709 */
		break;
	}

	if (priv->soc_type == SOC_H3) {
		if (hdmi_phy_set_h3(priv, i) != 0)
			return -1;
		bsp_hdmi_inner_init(priv);
	} else {
		bsp_hdmi_init(priv);
	}

	hdmi_writeb(priv, 0x0840, 0x01);		// 1200 HDMI_FC_DBGFORCE
	hdmi_writeb(priv, 0x4845, 0x00);		// 1219 HDMI_FC_DBGTMDS0
	hdmi_writeb(priv, 0x0040, ptbl[i].para[3] | 0x10); // 1000 HDMI_FC_INVIDCONF
	hdmi_writeb(priv, 0x10001, ptbl[i].para[3] < 96 ? 0x03 : 0x00);
	hdmi_writeb(priv, 0x8040, ptbl[i].para[4]);	// 1200 HDMI_FC_DBGFORCE
	hdmi_writeb(priv, 0x4043, ptbl[i].para[5]);	// 100d HDMI_FC_VSYNCINWIDTH
	hdmi_writeb(priv, 0x8042, ptbl[i].para[6]);	// 1006 HDMI_FC_INVACTV1
	hdmi_writeb(priv, 0x0042, ptbl[i].para[7]);	// 1004 HDMI_FC_INHBLANK1
	hdmi_writeb(priv, 0x4042, ptbl[i].para[8]);	// 100c HDMI_FC_VSYNCINDELAY
	hdmi_writeb(priv, 0x4041, ptbl[i].para[9]);	// 1009 HDMI_FC_HSYNCINDELAY1
	hdmi_writeb(priv, 0xc041, ptbl[i].para[10]);	// 100b HDMI_FC_HSYNCINWIDTH1
	hdmi_writeb(priv, 0x0041, ptbl[i].para[11]);	// 1001 HDMI_FC_INHACTV0
	hdmi_writeb(priv, 0x8041, ptbl[i].para[12]);	// 1003 HDMI_FC_INHBLANK0
	hdmi_writeb(priv, 0x4040, ptbl[i].para[13]);	// 1008 HDMI_FC_HSYNCINDELAY0
	hdmi_writeb(priv, 0xc040, ptbl[i].para[14]);	// 100a HDMI_FC_HSYNCINWIDTH1
	hdmi_writeb(priv, 0x0043, ptbl[i].para[15]);	// 1005 HDMI_FC_INVACTV0
	hdmi_writeb(priv, 0x8043, ptbl[i].para[16]);	// 1007 HDMI_FC_INVBLANK
	hdmi_writeb(priv, 0x0045, 0x0c);		// 1011 HDMI_FC_CTRLDUR
	hdmi_writeb(priv, 0x8044, 0x20);		// 1012 HDMI_FC_EXCTRLDUR
	hdmi_writeb(priv, 0x8045, 0x01);		// 1013 HDMI_FC_EXCTRLSPAC
	hdmi_writeb(priv, 0x0046, 0x0b);		// 1014 HDMI_FC_CH0PREAM
	hdmi_writeb(priv, 0x0047, 0x16);		// 1015 HDMI_FC_CH1PREAM
	hdmi_writeb(priv, 0x8046, 0x21);		// 1016 HDMI_FC_CH2PREAM
	hdmi_writeb(priv, 0x3048, ptbl[i].para[2] ? 0x21 : 0x10); // 10e0 HDMI_FC_PRCONF
	hdmi_writeb(priv, 0x0401, ptbl[i].para[2] ? 0x41 : 0x40); // 0801 HDMI_VP_PR_CD
	hdmi_writeb(priv, 0x8400, 0x07);		// 0802 HDMI_VP_STUFF
	hdmi_writeb(priv, 0x8401, 0x00);		// 0803 HDMI_VP_REMAP
	hdmi_writeb(priv, 0x0402, 0x47);		// 0804 HDMI_VP_CONF
	hdmi_writeb(priv, 0x0800, 0x01);		// 0200 HDMI_TX_INVID0
	hdmi_writeb(priv, 0x0801, 0x07);		// 0201 HDMI_TX_INSTUFFING
	hdmi_writeb(priv, 0x8800, 0x00);		// 0202 HDMI_TX_GYDATA0
	hdmi_writeb(priv, 0x8801, 0x00);		// 0203 HDMI_TX_GYDATA1
	hdmi_writeb(priv, 0x0802, 0x00);		// 0204 HDMI_TX_RCRDATA0
	hdmi_writeb(priv, 0x0803, 0x00);		// 0205 HDMI_TX_RCRDATA1
	hdmi_writeb(priv, 0x8802, 0x00);		// 0206 HDMI_TX_BCBDATA0
	hdmi_writeb(priv, 0x8803, 0x00);		// 0207 HDMI_TX_BCBDATA1

	if (priv->eld) {			/* if audio */
		hdmi_writeb(priv, 0xb045, 0x08);	// 10b3 HDMI_FC_DATAUTO0
		hdmi_writeb(priv, 0x2045, 0x00);	// 1031 HDMI_FC_VSDIEEEID2
		hdmi_writeb(priv, 0x2044, 0x0c);	// 1030 HDMI_FC_VSDIEEEID1
		hdmi_writeb(priv, 0x6041, 0x03);	// 1029 HDMI_FC_VSDIEEEID0
		hdmi_writeb(priv, 0xa044, (ptbl[i].para[0] & 0x100) ?	// 1032 HDMI_FC_VSDPAYLOAD0
					0x20 : (ptbl[i].para[0] & 0x80) ?
					0x40 :
					0x00 );
		hdmi_writeb(priv, 0xa045, (ptbl[i].para[0] & 0x100) ?	// 1033 HDMI_FC_VSDPAYLOAD1
					(ptbl[i].para[0] & 0x7f) : 0x00);
		hdmi_writeb(priv, 0x2046, 0x00);	// 1034 HDMI_FC_VSDPAYLOAD2
		hdmi_writeb(priv, 0x3046, 0x01);	// 10b4 HDMI_FC_DATAUTO1
		hdmi_writeb(priv, 0x3047, 0x11);	// 10b5 HDMI_FC_DATAUTO2
		hdmi_writeb(priv, 0x4044, 0x00);	// 1018 HDMI_FC_GCP
		hdmi_writeb(priv, 0x0052, 0x00);	// 1104 HDMI_FC_GMD_HB
		hdmi_writeb(priv, 0x8051, 0x11);	// 1103 HDMI_FC_GMD_CONF

		hdmi_read_lock(priv);
		hdmi_writeb(priv, 0x0040, hdmi_readb(priv, 0x0040) | 0x08); // 1000 HDMI_FC_INVIDCONF
		hdmi_read_unlock(priv);

		/* AVI */
		hdmi_writeb(priv, 0x4045, 0x00);	// 1019 HDMI_FC_AVICONF0
		if (ptbl[i].para[17] == 0)
			hdmi_writeb(priv, 0xc044, (csc << 6) | 0x18);	// 101a HDMI_FC_AVICONF1
		else if (ptbl[i].para[17] == 1)
			hdmi_writeb(priv, 0xc044, (csc << 6) | 0x28);
		else
			hdmi_writeb(priv, 0xc044, (csc << 6) | 0x08);

		if (priv->soc_type == SOC_H3)
			hdmi_writeb(priv, 0xc045, 0x08);	// 101b HDMI_FC_AVICONF2
		else
			hdmi_writeb(priv, 0xc045, 0x00);		// 101b HDMI_FC_AVICONF2
		hdmi_writeb(priv, 0x4046, ptbl[i].para[0] & 0x7f);	// 101c HDMI_FC_AVIVID
	}

	hdmi_writeb(priv, 0x0082, 0x00);		// 4004 HDMI_MC_FLOWCTRL
	hdmi_writeb(priv, 0x0081, 0x00);		// 4001 HDMI_MC_CLKDIS
							//	enable all clocks

	if (priv->soc_type != SOC_H3) {
		if (hdmi_phy_set_a83t(priv, i) != 0)
			return -1;
	}

	hdmi_writeb(priv, 0x0840, 0x00);		// 1200 HDMI_FC_DBGFORCE

	return 0;
}

/* get a block of EDID */
int bsp_hdmi_ddc_read(struct de2_hdmi_priv *priv,
			char pointer, char off,
			int nbyte, char *pbuf)
{
	unsigned to_cnt;
	u8 reg;
	int ret = 0;

//--fixme: from new bsp binary bpim3
//	if (priv->soc_type == SOC_H3) {
		hdmi_read_lock(priv);
		hdmi_writeb(priv, 0x4ee1, 0x00);	// 7e09 HDMI_I2CM_SOFTRSTZ
		to_cnt = 50;
		while (!(hdmi_readb(priv, 0x4ee1) & 0x01)) {
			udelay(10);
			if (--to_cnt == 0) {	/* wait for 500us for timeout */
				pr_warn("hdmi ddc reset timeout\n");
				break;
			}
		}

//fixme: strange values / IMx.6 doc
// (0x08 - fast - for 7e07 and 0x05 for 7e05 would be more logical)
		hdmi_writeb(priv, 0x8ee3, 0x05);	// 7e07 HDMI_I2CM_DIV
								// ?? nothing known
		hdmi_writeb(priv, 0x0ee3, 0x08);	// 7e05 HDMI_I2CM_INT
		hdmi_writeb(priv, 0x4ee2, 0xd8);	// 7e0c HDMI_I2CM_SS_SCL_HCNT_0_ADDR
		hdmi_writeb(priv, 0xcee2, 0xfe);	// 7e0e HDMI_I2CM_SS_SCL_LCNT_0_ADDR
//fixme: registers not initialized (jfm 16-08-30)
		hdmi_writeb(priv, 0xcee1, 0x00);	// 7e0b
		hdmi_writeb(priv, 0x4ee3, 0x00);	// 7e0d
		hdmi_writeb(priv, 0xcee3, 0x00);	// 7e0f
		hdmi_writeb(priv, 0x0ee4, 0x00);	// 7e10
		hdmi_writeb(priv, 0x0ee5, 0x00);	// 7e11
//	} else {
//		hdmi_writeb(priv, 0x8ee3, 0x05);	// 7e07 HDMI_I2CM_DIV
//		hdmi_writeb(priv, 0x0ee3, 0x08);	// 7e05 HDMI_I2CM_INT
//	}

	while (nbyte > 0) {
		hdmi_writeb(priv, 0x0ee0, 0xa0 >> 1);	/* 7e00 HDMI_I2CM_SLAVE */
		hdmi_writeb(priv, 0x0ee1, off);		/* 7e01 HDMI_I2CM_ADDRESS */
		hdmi_writeb(priv, 0x4ee0, 0x60 >> 1);	/* 7e08 HDMI_I2CM_SEGADDR */
		hdmi_writeb(priv, 0xcee0, pointer);	/* 7e0a HDMI_I2CM_SEGPTR */
		hdmi_writeb(priv, 0x0ee2, 0x02);	/* 7e04 HDMI_I2CM_OPERATION */
								/* rd_ext = DDC read */
//--fixme: new bsp (binary bpim3)
//		if (priv->soc_type != SOC_H3)
//			hdmi_read_lock(priv);

		to_cnt = 200;				/* timeout 100ms */
		while (1) {
			reg = hdmi_readb(priv, 0x0013);	// 0105 HDMI_IH_I2CM_STAT0
// from IMX6DQRM.pdf 33.5.14, the bits are cleared on read/write...
//			hdmi_writeb(priv, 0x0013, reg & 0x03);
//			hdmi_writeb(priv, 0x0013, 0x03);
			if (reg & 0x02) {
				*pbuf++ = hdmi_readb(priv, 0x8ee1); // 7e03 HDMI_I2CM_DATAI
				hdmi_writeb(priv, 0x0013, 0x02);
				break;
			}
			if (reg & 0x01) {
				hdmi_writeb(priv, 0x0013, 0x01);
				pr_warn("hdmi ddc read error, byte cnt = %d\n",
					 nbyte);
				ret = -1;
				break;
			}
			if (--to_cnt == 0) {
				if (!ret) {
					pr_warn("hdmi ddc read timeout, byte cnt = %d\n",
						 nbyte);
					ret = -1;
				}
				break;
			}
			usleep_range(800, 1000);
		}
		if (ret)
			break;
		nbyte--;
		off++;
	}
	hdmi_read_unlock(priv);

	return ret;
}

int bsp_hdmi_get_hpd(struct de2_hdmi_priv *priv)
{
	int ret;

	hdmi_read_lock(priv);

	if (priv->soc_type == SOC_H3)
		ret = hdmi_readl(priv, 0x10038) & 0x80000;
	else
		ret = hdmi_readb(priv, 0x0243) & 0x02;	// 3005 HDMI_PHY_INT0

	hdmi_read_unlock(priv);

	return ret != 0;
}

void bsp_hdmi_hrst(struct de2_hdmi_priv *priv)
{
	hdmi_writeb(priv, 0x00c1, 0x04);		// 5001 HDMI_A_HDCPCFG1
}

int bsp_hdmi_mode_valid(int cea_mode)
{
	return get_vid(cea_mode);
}
