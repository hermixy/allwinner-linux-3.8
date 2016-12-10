#ifndef __DE2_HDMI_H__
#define __DE2_HDMI_H__
/*
 * Copyright (C) 2016 Jean-François Moine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <drm/drmP.h>

/* SoC types */
#define SOC_A83T 0
#define SOC_H3 1

struct de2_hdmi_priv {
	struct device *dev;
	void __iomem *mmio;

	struct drm_encoder encoder;
	struct drm_connector connector;

	struct clk *clk;
	struct clk *clk_ddc;
	struct clk *gate;
	struct reset_control *rstc0;
	struct reset_control *rstc1;

	struct mutex mutex;
	u8 soc_type;
	u8 cea_mode;
	u8 *eld;
#if IS_ENABLED(CONFIG_SND_SOC)
	int (*set_audio_input)(struct device *dev,
				int enable,
				unsigned sample_rate,
				unsigned sample_bit);
#endif
};

#if IS_ENABLED(CONFIG_SND_SOC)
int de2_hdmi_codec_register(struct device *dev);
void de2_hdmi_codec_unregister(struct device *dev);
#endif
#endif /* __DE2_HDMI_H__ */
