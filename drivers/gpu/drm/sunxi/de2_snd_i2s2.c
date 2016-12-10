/*
 * DE2 sound card
 *
 * Copyright (C) 2016 Jean-Francois Moine <moinejf@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>

/* --- hardware --- */

#define SUNXI_DAUDIOCTL 	  	0x00
	/* common */
	#define SUNXI_DAUDIOCTL_SDO3EN		BIT(11)
	#define SUNXI_DAUDIOCTL_SDO2EN		BIT(10)
	#define SUNXI_DAUDIOCTL_SDO1EN		BIT(9)
	#define SUNXI_DAUDIOCTL_SDO0EN		BIT(8)
//	#define SUNXI_DAUDIOCTL_LOOP		BIT(3)
	#define SUNXI_DAUDIOCTL_TXEN		BIT(2)
	#define SUNXI_DAUDIOCTL_RXEN		BIT(1)
	#define SUNXI_DAUDIOCTL_GEN		BIT(0)
	/* pcm */
	#define SUNXI_PCM_CTL_MS		BIT(5)
	#define SUNXI_PCM_CTL_PCM		BIT(4)
	/* tdm */
	#define SUNXI_TDM_CTL_BCLKOUT		BIT(18)
	#define SUNXI_TDM_CTL_LRCKOUT		BIT(17)
//	#define SUNXI_TDM_CTL_LRCKROUT		BIT(16)
//	#define SUNXI_TDM_CTL_OUTMUTE		BIT(6)
	#define SUNXI_TDM_CTL_MODE_MSK		(3 << 4)
//		#define SUNXI_TDM_CTL_MODE_DSP_A	(0 << 4)
//		#define SUNXI_TDM_CTL_MODE_DSP_B	(0 << 4)
		#define SUNXI_TDM_CTL_MODE_I2S		(1 << 4)
//		#define SUNXI_TDM_CTL_MODE_LEFT_J	(1 << 4)
//		#define SUNXI_TDM_CTL_MODE_RIGHT_J	(2 << 4)

#define SUNXI_DAUDIOFAT0 		0x04
	/* common */
	/* pcm */
	#define SUNXI_PCM_FAT0_LRCP		BIT(7)
	#define SUNXI_PCM_FAT0_BCP		BIT(6)
	#define SUNXI_PCM_FAT0_SR_16BIT		(0 << 4)
	#define SUNXI_PCM_FAT0_SR_24BIT		(2 << 4)
	#define SUNXI_PCM_FAT0_SR_MSK		(3 << 4)
	#define SUNXI_PCM_FAT0_WSS_32BCLK	(3 << 2)
	#define SUNXI_PCM_FAT0_FMT_I2S1		(0 << 0)
	#define SUNXI_PCM_FAT0_FMT_LFT		(1 << 0)
	#define SUNXI_PCM_FAT0_FMT_RGT		(2 << 0)
	#define SUNXI_PCM_FAT0_FMT_MSK		(3 << 0)
	/* tdm */
//	#define SUNXI_TDM_FAT0_SDI_SYNC_SEL	BIT(31)
//	#define SUNXI_TDM_FAT0_LRCK_WIDTH	BIT(30)
	#define SUNXI_TDM_FAT0_LRCKR_PERIOD(v) ((v) << 20)
	#define SUNXI_TDM_FAT0_LRCKR_PERIOD_MSK (0x3ff << 20)
	#define SUNXI_TDM_FAT0_LRCK_POLARITY	BIT(19)
	#define SUNXI_TDM_FAT0_LRCK_PERIOD(v)	((v) << 8)
	#define SUNXI_TDM_FAT0_LRCK_PERIOD_MSK (0x3ff << 8)
	#define SUNXI_TDM_FAT0_BCLK_POLARITY	BIT(7)
	#define SUNXI_TDM_FAT0_SR_16		(3 << 4)
//	#define SUNXI_TDM_FAT0_SR_20		(4 << 4)
	#define SUNXI_TDM_FAT0_SR_24		(5 << 4)
	#define SUNXI_TDM_FAT0_SR_MSK		(7 << 4)
//	#define SUNXI_TDM_FAT0_EDGE_TRANSFER	BIT(3)
	#define SUNXI_TDM_FAT0_SW_16		(3 << 0)
//	#define SUNXI_TDM_FAT0_SW_20		(4 << 0)
//	#define SUNXI_TDM_FAT0_SW_24		(5 << 0)
	#define SUNXI_TDM_FAT0_SW_32		(7 << 0)
	#define SUNXI_TDM_FAT0_SW_MSK		(7 << 0)

#define SUNXI_DAUDIOFAT1		0x08
//	#define SUNXI_DAUDIOFAT1_RX_MLS		BIT(7)
//	#define SUNXI_DAUDIOFAT1_TX_MLS		BIT(6)
//	#define SUNXI_DAUDIOFAT1_SEXT		(3 << 4)
//	#define SUNXI_DAUDIOFAT1_RX_PDM		(3 << 2)
//	#define SUNXI_DAUDIOFAT1_TX_PDM		(3 << 0)

//#define SUNXI_DAUDIOISTA 		0x0c
//	#define SUNXI_DAUDIOSTA_TXU_INT		BIT(6)
//	#define SUNXI_DAUDIOSTA_TXO_INT		BIT(5)
//	#define SUNXI_DAUDIOSTA_TXE_INT		BIT(4)
//	#define SUNXI_DAUDIOSTA_RXU_INT		BIT(2)
//	#define SUNXI_DAUDIOSTA_RXO_INT		BIT(1)
//	#define SUNXI_DAUDIOSTA_RXA_INT		BIT(0)

//#define SUNXI_DAUDIORXFIFO		0x10

#define SUNXI_DAUDIOFCTL		0x14
//	#define SUNXI_DAUDIOFCTL_HUBEN		BIT(31)
	#define SUNXI_DAUDIOFCTL_FTX		BIT(25)
	#define SUNXI_DAUDIOFCTL_FRX		BIT(24)
	#define SUNXI_DAUDIOFCTL_TXTL(v)	((v) << 12)
//	#define SUNXI_DAUDIOFCTL_RXTL(v)	((v) << 4)
	#define SUNXI_DAUDIOFCTL_TXIM		BIT(2)
//	#define SUNXI_DAUDIOFCTL_RXOM		BIT(0)

//#define SUNXI_DAUDIOFSTA   		0x18
//	#define SUNXI_DAUDIOFSTA_TXE		BIT(28)
//	#define SUNXI_DAUDIOFSTA_TXECNT(v)	((v) << 16)
//	#define SUNXI_DAUDIOFSTA_RXA		BIT(8)
//	#define SUNXI_DAUDIOFSTA_RXACNT(v)	((v) << 0)
	
#define SUNXI_DAUDIOINT    		0x1c
	#define SUNXI_DAUDIOINT_TXDRQEN		BIT(7)
//	#define SUNXI_DAUDIOINT_TXUIEN		BIT(6)
//	#define SUNXI_DAUDIOINT_TXOIEN		BIT(5)
//	#define SUNXI_DAUDIOINT_TXEIEN		BIT(4)
//	#define SUNXI_DAUDIOINT_RXDRQEN		BIT(3)
//	#define SUNXI_DAUDIOINT_RXUIEN		BIT(2)
//	#define SUNXI_DAUDIOINT_RXOIEN		BIT(1)
//	#define SUNXI_DAUDIOINT_RXAIEN		BIT(0)

#define SUNXI_DAUDIOTXFIFO		0x20
	
#define SUNXI_DAUDIOCLKD   		0x24
	/* common */
	#define SUNXI_DAUDIOCLKD_BCLKDIV(v)	((v) << 4)
	#define SUNXI_DAUDIOCLKD_MCLKDIV(v)	((v) << 0)
	/* pcm */
	#define SUNXI_PCM_CLKD_MCLKOEN		BIT(7)
	/* tdm */
	#define SUNXI_TDM_CLKD_MCLKOEN		BIT(8)

#define SUNXI_DAUDIOTXCNT  		0x28

#define SUNXI_DAUDIORXCNT  		0x2c

/* --- pcm --- */
#define SUNXI_PCM_TXCHSEL		0x30
	#define SUNXI_PCM_TXCHSEL_CHNUM(v)	(((v) - 1) << 0)
	#define SUNXI_PCM_TXCHSEL_CHNUM_MSK	(7 << 0)

#define SUNXI_PCM_TXCHMAP		0x34

/* --- tdm --- */
#define SUNXI_TDM_TXCHCFG			0x30
//	#define SUNXI_TDM_TXCHCFG_TX_SLOT_HIZ	BIT(9)
//	#define SUNXI_TDM_TXCHCFG_TX_STATE	BIT(8)
//	#define SUNXI_TDM_TXCHCFG_RX_SLOT_NUM	(7 << 4)
	#define SUNXI_TDM_TXCHCFG_TX_SLOT_NUM_MSK (7 << 0)
	#define SUNXI_TDM_TXCHCFG_TX_SLOT_NUM(v) ((v) << 0)

#define SUNXI_TDM_TX0CHSEL		0x34
//#define SUNXI_TDM_TX1CHSEL		0x38
//#define SUNXI_TDM_TX2CHSEL		0x3c
//#define SUNXI_TDM_TX3CHSEL		0x40
	#define SUNXI_TDM_TXn_OFFSET_MSK	(3 << 12)
	#define SUNXI_TDM_TXn_OFFSET(v)		((v) << 12)
	#define SUNXI_TDM_TXn_CHEN_MSK		(0xff << 4)
	#define SUNXI_TDM_TXn_CHEN(v)		((v) << 4)
	#define SUNXI_TDM_TXn_CHSEL_MSK		(7 << 0)
	#define SUNXI_TDM_TXn_CHSEL(v)		((v) << 0)

#define SUNXI_TDM_TX0CHMAP		0x44
//#define SUNXI_TDM_TX1CHMAP		0x48
//#define SUNXI_TDM_TX2CHMAP		0x4c
//#define SUNXI_TDM_TX3CHMAP		0x50
//	#define SUNXI_TDM_TXn_CH7_MAP		(7 << 28)
//	#define SUNXI_TDM_TXn_CH6_MAP		(7 << 24)
//	#define SUNXI_TDM_TXn_CH5_MAP		(7 << 20)
//	#define SUNXI_TDM_TXn_CH4_MAP		(7 << 16)
//	#define SUNXI_TDM_TXn_CH3_MAP		(7 << 12)
//	#define SUNXI_TDM_TXn_CH2_MAP		(7 << 8)
//	#define SUNXI_TDM_TXn_CH1_MAP		(7 << 4)
//	#define SUNXI_TDM_TXn_CH0_MAP		(7 << 0)
//#define SUNXI_TDM_RXCHSEL		0x54
//	#define SUNXI_TDM_RXCHSEL_RXOFFSET	(3 << 12)
//	#define SUNXI_TDM_RXCHSEL_RXCHSET	(7 << 0)

//#define SUNXI_TDM_RXCHMAP		0x58
//	#define SUNXI_TDM_RXCHMAP_CH7		(7 << 28)
//	#define SUNXI_TDM_RXCHMAP_CH6		(7 << 24)
//	#define SUNXI_TDM_RXCHMAP_CH5		(7 << 20)
//	#define SUNXI_TDM_RXCHMAP_CH4		(7 << 16)
//	#define SUNXI_TDM_RXCHMAP_CH3		(7 << 12)
//	#define SUNXI_TDM_RXCHMAP_CH2		(7 << 8)
//	#define SUNXI_TDM_RXCHMAP_CH1		(7 << 4)
//	#define SUNXI_TDM_RXCHMAP_CH0		(7 << 0)

//#define SUNXI_TDM_DBG			0x5c

/* --- driver --- */

#define DRV_NAME "de2-hdmi-audio"

#define I2S2_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | \
	 SNDRV_PCM_FMTBIT_S24_LE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

#define PCM_LRCK_PERIOD 32
#define PCM_LRCKR_PERIOD 1

struct priv {
	void __iomem *mmio;
	struct clk *clk;
	struct clk *clk_i2s2;
	struct clk *gate;
	struct reset_control *rstc;
	int type;
#define SOC_A83T 0
#define SOC_H3 1
	struct snd_dmaengine_dai_dma_data dma_data;
};

static const struct of_device_id de2_i2s2_of_match[] = {
	{ .compatible = "allwinner,sun8i-a83t-hdmi-audio",
				.data = (void *) SOC_A83T },
	{ .compatible = "allwinner,sun8i-h3-hdmi-audio",
				.data = (void *) SOC_H3 },
	{ }
};
MODULE_DEVICE_TABLE(of, de2_i2s2_of_match);

/* --- CPU DAI --- */

static void de2_i2s2_init(struct priv *priv)
{
	u32 reg;

	/* disable global */
	reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
	reg &= ~(SUNXI_DAUDIOCTL_GEN |
		 SUNXI_DAUDIOCTL_RXEN |
		 SUNXI_DAUDIOCTL_TXEN);
	writel(reg, priv->mmio + SUNXI_DAUDIOCTL);

	/* PCM */
	if (priv->type == SOC_A83T) {
		reg &= ~(SUNXI_PCM_CTL_MS |	/* codec clk & FRM slave */
			 SUNXI_PCM_CTL_PCM);	/* I2S mode */
		writel(reg, priv->mmio + SUNXI_DAUDIOCTL);

		reg = readl(priv->mmio + SUNXI_DAUDIOFAT0);
		reg &= ~SUNXI_PCM_FAT0_FMT_MSK;
		reg |= SUNXI_PCM_FAT0_FMT_I2S1;
//		writel(reg, priv->mmio + SUNXI_DAUDIOFAT0);

//		reg = readl(priv->mmio + SUNXI_DAUDIOFAT0);
		reg &= ~(SUNXI_PCM_FAT0_LRCP | SUNXI_PCM_FAT0_BCP);
		writel(reg, priv->mmio + SUNXI_DAUDIOFAT0);

		reg = SUNXI_DAUDIOFCTL_TXIM |			/* fifo */
			 SUNXI_DAUDIOFCTL_TXTL(0x40);
		writel(reg, priv->mmio + SUNXI_DAUDIOFCTL);

		/* normal bit clock + frame */
		reg = readl(priv->mmio + SUNXI_DAUDIOFAT0);
		reg &= ~(SUNXI_PCM_FAT0_LRCP |
			 SUNXI_PCM_FAT0_BCP);
		writel(reg, priv->mmio + SUNXI_DAUDIOFAT0);

		return;
	}

	/* TDM */
	/* clear the FIFOs */
	reg = readl(priv->mmio + SUNXI_DAUDIOFCTL);
	reg &= ~(SUNXI_DAUDIOFCTL_FRX | SUNXI_DAUDIOFCTL_FTX);
	writel(reg, priv->mmio + SUNXI_DAUDIOFCTL);

	/* clear the FIFO counters */
	writel(0, priv->mmio + SUNXI_DAUDIOTXCNT);
	writel(0, priv->mmio + SUNXI_DAUDIORXCNT);

	/* codec clk & FRM slave */
	reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
	reg |= SUNXI_TDM_CTL_LRCKOUT | SUNXI_TDM_CTL_BCLKOUT;

	/* I2S mode */
	reg &= ~SUNXI_TDM_CTL_MODE_MSK;
	reg |= SUNXI_TDM_CTL_MODE_I2S;
	writel(reg, priv->mmio + SUNXI_DAUDIOCTL);
	reg = readl(priv->mmio + SUNXI_TDM_TX0CHSEL) &
				~SUNXI_TDM_TXn_OFFSET_MSK;
	reg |= SUNXI_TDM_TXn_OFFSET(1);
	writel(reg, priv->mmio + SUNXI_TDM_TX0CHSEL);

	/* normal bit clock + frame */
	reg = readl(priv->mmio + SUNXI_DAUDIOFAT0);
	reg &= ~(SUNXI_TDM_FAT0_BCLK_POLARITY |
		 SUNXI_TDM_FAT0_LRCK_POLARITY);
	writel(reg, priv->mmio + SUNXI_DAUDIOFAT0);
}

static int de2_i2s2_set_clock(struct priv *priv,
				unsigned long rate)
{
	unsigned long freq;
	int ret, i, div;
	u32 reg;
	static const u8 div_tb[] = {
		1, 2, 4, 6, 8, 12, 16, 24, // 32, 48, 64, 96, 128, 176, 192
	};

	/* compute the sys clock rate and divide values */
	if (rate % 1000 == 0)
		freq = 24576000;
	else
		freq = 22579200;
	div = freq / 2 / PCM_LRCK_PERIOD / rate;
	if (priv->type == SOC_A83T)
		div /= 2;			/* bclk_div==0 => mclk/2 */
	for (i = 0; i < ARRAY_SIZE(div_tb) - 1; i++)
		if (div_tb[i] >= div)
			break;

	ret = clk_set_rate(priv->clk, freq);
	if (ret) {
		pr_info("Setting sysclk rate failed %d\n", ret);
		return ret;
	}

	/* set the mclk and bclk dividor register */
	if (priv->type == SOC_A83T) {
		reg = SUNXI_PCM_CLKD_MCLKOEN | SUNXI_DAUDIOCLKD_MCLKDIV(i);
	} else {
		reg = SUNXI_TDM_CLKD_MCLKOEN | SUNXI_DAUDIOCLKD_MCLKDIV(1)
				| SUNXI_DAUDIOCLKD_BCLKDIV(i + 1);
	}
	writel(reg, priv->mmio + SUNXI_DAUDIOCLKD);

	/* format */
	reg = readl(priv->mmio + SUNXI_DAUDIOFAT0);
	if (priv->type == SOC_A83T) {
		reg |= SUNXI_PCM_FAT0_WSS_32BCLK;
		reg &= ~SUNXI_PCM_FAT0_SR_MSK;
		reg |= SUNXI_PCM_FAT0_SR_16BIT;
	} else {
		reg &= ~(SUNXI_TDM_FAT0_LRCKR_PERIOD_MSK |
			 SUNXI_TDM_FAT0_LRCK_PERIOD_MSK);
		reg |= SUNXI_TDM_FAT0_LRCK_PERIOD(PCM_LRCK_PERIOD - 1) |
			SUNXI_TDM_FAT0_LRCKR_PERIOD(PCM_LRCKR_PERIOD - 1);

		reg &= ~SUNXI_TDM_FAT0_SW_MSK;
		reg |= SUNXI_TDM_FAT0_SW_16;

		reg &= ~SUNXI_TDM_FAT0_SR_MSK;
		reg |= SUNXI_TDM_FAT0_SR_16;
	}
	writel(reg, priv->mmio + SUNXI_DAUDIOFAT0);

	writel(0, priv->mmio + SUNXI_DAUDIOFAT1);

	return 0;
}

static int de2_i2s2_prepare(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct priv *priv = snd_soc_card_get_drvdata(card);
	int nchan = substream->runtime->channels;
	u32 reg;

	if (priv->type == SOC_A83T) {
		reg = readl(priv->mmio + SUNXI_PCM_TXCHSEL);
		reg &= ~SUNXI_PCM_TXCHSEL_CHNUM_MSK;
		reg |= SUNXI_PCM_TXCHSEL_CHNUM(substream->runtime->channels);
		writel(reg, priv->mmio + SUNXI_PCM_TXCHSEL);

		switch (substream->runtime->channels) {
		case 1:
			reg = 0x76543200;
			break;
		case 8:
			reg = 0x54762310;
			break;
		default:
//fixme: left/right inversion
//			reg = 0x76543210;
			reg = 0x76543201;
			break;
		}
		writel(reg, priv->mmio + SUNXI_PCM_TXCHMAP);
	} else {
		reg = readl(priv->mmio + SUNXI_TDM_TXCHCFG) &
					~SUNXI_TDM_TXCHCFG_TX_SLOT_NUM_MSK;
		if (nchan != 1)
			reg |= SUNXI_TDM_TXCHCFG_TX_SLOT_NUM(1);
		writel(reg, priv->mmio + SUNXI_TDM_TXCHCFG);

		reg = readl(priv->mmio + SUNXI_TDM_TX0CHSEL);
//fixme: set in de2_i2s2_set_fmt
//		reg |= SUNXI_TDM_TXn_OFFSET(1);
		reg &= ~(SUNXI_TDM_TXn_CHEN_MSK |
			 SUNXI_TDM_TXn_CHSEL_MSK);
		reg |= SUNXI_TDM_TXn_CHEN(3) |
			SUNXI_TDM_TXn_CHSEL(1);
		writel(reg, priv->mmio + SUNXI_TDM_TX0CHSEL);

		reg = nchan == 1 ? 0 : 0x10;
		writel(reg, priv->mmio + SUNXI_TDM_TX0CHMAP);
	}

	reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
	reg &= ~(SUNXI_DAUDIOCTL_SDO3EN |
		 SUNXI_DAUDIOCTL_SDO2EN |
		 SUNXI_DAUDIOCTL_SDO1EN);
	if (nchan >= 7)
		reg |= SUNXI_DAUDIOCTL_SDO3EN;
	if (nchan >= 5)
		reg |= SUNXI_DAUDIOCTL_SDO2EN;
	if (nchan >= 3)
		reg |= SUNXI_DAUDIOCTL_SDO1EN;
	reg |= SUNXI_DAUDIOCTL_SDO0EN;
	writel(reg, priv->mmio + SUNXI_DAUDIOCTL);

	writel(0, priv->mmio + SUNXI_DAUDIOTXCNT);

	return 0;
}

static void de2_i2s2_shutdown(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct priv *priv = snd_soc_card_get_drvdata(card);
	u32 reg;

	reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
	reg &= ~SUNXI_DAUDIOCTL_GEN;
	writel(reg, priv->mmio + SUNXI_DAUDIOCTL);
}

static int de2_i2s2_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct priv *priv = snd_soc_card_get_drvdata(card);
	u32 reg, reg2;
	int sample_resolution;
	int ret;

//test:trace
pr_info("de2 i2s2 fmt %d rate %d\n",
 params_format(params) == SNDRV_PCM_FORMAT_S16_LE ? 16 : 24,
 params_rate(params));

	ret = de2_i2s2_set_clock(priv, params_rate(params));
	if (ret)
		return ret;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		priv->dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		sample_resolution = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		priv->dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		sample_resolution = 24;
		break;
	default:
		return -EINVAL;
	}
	reg = readl(priv->mmio + SUNXI_DAUDIOFAT0);
	reg2 = readl(priv->mmio + SUNXI_DAUDIOFCTL);
	if (priv->type == SOC_A83T) {
		reg &= ~SUNXI_PCM_FAT0_SR_MSK;
		if (sample_resolution == 16) {
			reg |= SUNXI_PCM_FAT0_SR_16BIT;
			reg2 |= SUNXI_DAUDIOFCTL_TXIM;
		} else {
			reg |= SUNXI_PCM_FAT0_SR_24BIT;
			reg2 &= ~SUNXI_DAUDIOFCTL_TXIM;
		}
	} else {
		reg &= ~(SUNXI_TDM_FAT0_SR_MSK | SUNXI_TDM_FAT0_SW_MSK);
		if (sample_resolution == 16) {
			reg |= SUNXI_TDM_FAT0_SR_16 |
					SUNXI_TDM_FAT0_SW_16;
			reg2 |= SUNXI_DAUDIOFCTL_TXIM;
		} else {
			reg |= SUNXI_TDM_FAT0_SR_24 |
					SUNXI_TDM_FAT0_SW_32;
			reg2 &= ~SUNXI_DAUDIOFCTL_TXIM;
		}
	}
	writel(reg, priv->mmio + SUNXI_DAUDIOFAT0);
	writel(reg2, priv->mmio + SUNXI_DAUDIOFCTL);

	/* enable audio interface */
	reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
	reg |= SUNXI_DAUDIOCTL_GEN;
	writel(reg, priv->mmio + SUNXI_DAUDIOCTL);
	msleep(10);

	/* flush TX FIFO */
	reg = readl(priv->mmio + SUNXI_DAUDIOFCTL);
	reg |= SUNXI_DAUDIOFCTL_FTX;
	writel(reg, priv->mmio + SUNXI_DAUDIOFCTL);

	return 0;
}

static int de2_i2s2_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct priv *priv = snd_soc_card_get_drvdata(card);
	u32 reg;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
		reg |= SUNXI_DAUDIOCTL_TXEN;
		writel(reg, priv->mmio + SUNXI_DAUDIOCTL);

		/* enable DMA DRQ mode */
		reg = readl(priv->mmio + SUNXI_DAUDIOINT);
		reg |= SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg, priv->mmio + SUNXI_DAUDIOINT);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		reg = readl(priv->mmio + SUNXI_DAUDIOINT);
		reg &= ~SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg, priv->mmio + SUNXI_DAUDIOINT);

//fixme: test: not in Allwinner's
		reg = readl(priv->mmio + SUNXI_DAUDIOCTL);
		reg &= ~SUNXI_DAUDIOCTL_TXEN;
		writel(reg, priv->mmio + SUNXI_DAUDIOCTL);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops de2_i2s2_dai_ops = {
	.hw_params = de2_i2s2_hw_params,
	.prepare = de2_i2s2_prepare,
	.trigger = de2_i2s2_trigger,
	.shutdown = de2_i2s2_shutdown,
};

static int de2_i2s2_dai_probe(struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct priv *priv = snd_soc_card_get_drvdata(card);

	snd_soc_dai_init_dma_data(dai, &priv->dma_data, NULL);

	return 0;
}

static struct snd_soc_dai_driver i2s2_dai = {
	.name = "i2s2",
	.probe = de2_i2s2_dai_probe,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min = 32000,
		.rate_max = 192000,
		.formats = I2S2_FORMATS,
	},
	.ops = &de2_i2s2_dai_ops,
};

static const struct snd_soc_component_driver i2s2_component = {
	.name = DRV_NAME,
};

/* --- dma --- */

static const struct snd_pcm_hardware de2_i2s2_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats = I2S2_FORMATS,
	.rates = SNDRV_PCM_RATE_CONTINUOUS,
	.rate_min = 32000,
	.rate_max = 192000,
	.channels_min = 1,
	.channels_max = 8,
	.buffer_bytes_max = 1024 * 1024,
	.period_bytes_min = 156,
	.period_bytes_max = 1024 * 1024,
	.periods_min = 1,
	.periods_max = 8,
	.fifo_size = 128,
};

static const struct snd_dmaengine_pcm_config de2_i2s2_config = {
	.prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config,
	.pcm_hardware = &de2_i2s2_pcm_hardware,
	.prealloc_buffer_size = 1024 * 1024,
};

/* --- audio card --- */

static struct device_node *de2_get_hdmi_codec(struct device *dev)
{
	struct device_node *ep, *remote;

	ep = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!ep)
		return NULL;
	remote = of_graph_get_remote_port_parent(ep);
	of_node_put(ep);

	return remote;
}

static int de2_card_create(struct device *dev, struct priv *priv)
{
	struct snd_soc_card *card;
	struct snd_soc_dai_link *dai_link;
	struct snd_soc_dai_link_component *codec;

	card = devm_kzalloc(dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;
	dai_link = devm_kzalloc(dev, sizeof(*dai_link), GFP_KERNEL);
	if (!dai_link)
		return -ENOMEM;
	codec = devm_kzalloc(dev, sizeof(*codec), GFP_KERNEL);
	if (!codec)
		return -ENOMEM;

	card->name = "hdmi-audio";
	card->dai_link = dai_link;
	card->num_links = 1;
	dai_link->name = "HDMI Audio";
	dai_link->stream_name = "HDMI Audio";
	dai_link->platform_name = dev_name(dev);
	dai_link->cpu_name = dev_name(dev);

	dai_link->codecs = codec;
	dai_link->num_codecs = 1;
	codec->of_node = de2_get_hdmi_codec(dev);
	if (!codec->of_node) {
		dev_err(dev, "no HDMI port node\n");
		return -ENXIO;
	}
	codec->dai_name = "hdmi-codec";

	card->dev = dev;
	dev_set_drvdata(dev, card);
	snd_soc_card_set_drvdata(card, priv);

	return devm_snd_soc_register_card(dev, card);
}

/* --- module init --- */

static int de2_i2s2_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct priv *priv;
	struct resource *mem;
	int ret;

	if (!dev->of_node) {
		dev_err(dev, "no DT!\n");
		return -EINVAL;
	}

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* get the resources */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->mmio = devm_ioremap_resource(dev, mem);
	if (IS_ERR(priv->mmio))
		return PTR_ERR(priv->mmio);

	/* get SoC type */
	priv->type = (int) of_match_device(de2_i2s2_of_match,
						&pdev->dev)->data;

	/* get and enable the clocks */
#if 1
	priv->gate = devm_clk_get(dev, "gate");	/* optional */
//	if (IS_ERR(priv->gate)) {
//		dev_err(dev, "no gate\n");
//		return PTR_ERR(priv->gate);
//	}
#endif
	priv->clk = devm_clk_get(dev, "clock");
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "no pll clock\n");
		return PTR_ERR(priv->clk);
	}
	priv->clk_i2s2 = devm_clk_get(dev, "i2s2");
	if (IS_ERR(priv->clk_i2s2)) {
		dev_err(dev, "no i2s2 clock\n");
		return PTR_ERR(priv->clk_i2s2);
	}
	priv->rstc = devm_reset_control_get_optional(dev, NULL);
//	if (IS_ERR(priv->rstc)) {
//		dev_err(dev, "reset controller err %d\n",
//				(int) PTR_ERR(priv->rstc));
//		return PTR_ERR(priv->rstc);
//	}

	if (!IS_ERR(priv->rstc)) {
		ret = reset_control_deassert(priv->rstc);
		if (ret < 0)
			return ret;
	}

#if 1
	if (!IS_ERR(priv->gate)) {
		ret = clk_prepare_enable(priv->gate);
		if (ret < 0)
			goto err_gate;
	}
#endif

#if 1
//--fixme: should be enabled when clk_i2s2 is enabled
	/* activate the audio subsystem */
	ret = clk_prepare_enable(priv->clk);
	if (ret < 0)
		goto err_enable;
#endif

#if 0
//--fixme: default value
	ret = clk_set_rate(priv->clk, 24576000);
	if (ret) {
		dev_err(dev, "cannot set rate of i2s2 clock %d\n", ret);
		goto err_???;
	}
#endif

#if 0
//fixme: done in the DTS
	ret = clk_set_parent(priv->clk_i2s2, priv->clk);
	if (ret < 0) {
		dev_err(dev, "cannot set clock as i2s2 parent %d\n", ret);
		goto err_???;
	}
#endif

	ret = clk_prepare_enable(priv->clk_i2s2);
	if (ret < 0)
		goto err_i2s2;

	de2_i2s2_init(priv);

	ret = devm_snd_soc_register_component(dev, &i2s2_component, &i2s2_dai, 1);
	if (ret) {
		dev_err(dev, "snd_soc_register_component failed %d\n", ret);
		goto err_register;
	}

	ret = devm_snd_dmaengine_pcm_register(dev, &de2_i2s2_config, 0);
	if (ret) {
		dev_err(dev, "pcm_register failed %d\n", ret);
		goto err_register;
	}

	priv->dma_data.maxburst = priv->type == SOC_A83T ? 8 : 4;
	priv->dma_data.addr = mem->start + SUNXI_DAUDIOTXFIFO;
//fixme: useless
	priv->dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;

	ret = de2_card_create(dev, priv);
	if (ret) {
		dev_err(dev, "register card failed %d\n", ret);
		goto err_register;
	}

	return 0;

err_register:
	clk_disable_unprepare(priv->clk_i2s2);
err_i2s2:
	clk_disable_unprepare(priv->clk);
err_enable:
#if 1
	clk_disable_unprepare(priv->gate);
err_gate:
#endif
	if (!IS_ERR(priv->rstc))
		reset_control_assert(priv->rstc);

	return ret;
}

static int de2_i2s2_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = dev_get_drvdata(&pdev->dev);
	struct priv *priv = snd_soc_card_get_drvdata(card); 

	clk_disable_unprepare(priv->clk_i2s2);
	clk_disable_unprepare(priv->clk);
	clk_disable_unprepare(priv->gate);
	if (!IS_ERR_OR_NULL(priv->rstc))
		reset_control_assert(priv->rstc);

	return 0;
}

static struct platform_driver de2_i2s2_driver = {
	.probe  = de2_i2s2_dev_probe,
	.remove = de2_i2s2_dev_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(de2_i2s2_of_match),
	},
};

module_platform_driver(de2_i2s2_driver);

/* Module information */
MODULE_AUTHOR("Jean-Francois Moine <moinejf@free.fr>");
MODULE_DESCRIPTION("Allwinner DE2 I2S2 ASoC Interface");
MODULE_LICENSE("GPL v2");
