/*
 * Allwinner DRM driver - DE2 HDMI codec
 *
 * Copyright (C) 2016 Jean-Francois Moine <moinejf@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <sound/pcm_drm_eld.h>
#include <sound/pcm_params.h>

#include "de2_hdmi.h"

#define DE2_HDMI_FORMATS (SNDRV_PCM_FMTBIT_S8 | \
			  SNDRV_PCM_FMTBIT_S16_LE | \
			  SNDRV_PCM_FMTBIT_S20_3LE | \
			  SNDRV_PCM_FMTBIT_S24_LE | \
			  SNDRV_PCM_FMTBIT_S32_LE)

static int de2_hdmi_codec_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct de2_hdmi_priv *priv = dev_get_drvdata(dai->dev);
	u8 *eld;

	eld = priv->eld;
	if (!eld)
		return -ENODEV;

	return snd_pcm_hw_constraint_eld(runtime, eld);
}

static int de2_hdmi_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params,
			      struct snd_soc_dai *dai)
{
	struct de2_hdmi_priv *priv = dev_get_drvdata(dai->dev);
	unsigned sample_bit;

	if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE)
		sample_bit = 16;
	else
		sample_bit = 24;
//fixme params_channels(params) may change 'ca'
	return priv->set_audio_input(dai->dev, true,
					params_rate(params),
					sample_bit);
}

static void de2_hdmi_codec_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct de2_hdmi_priv *priv = dev_get_drvdata(dai->dev);

	priv->set_audio_input(dai->dev, false, 0, 0);
}

static const struct snd_soc_dai_ops de2_hdmi_codec_ops = {
	.startup = de2_hdmi_codec_startup,
	.hw_params = de2_hdmi_hw_params,
	.shutdown = de2_hdmi_codec_shutdown,
};

static struct snd_soc_dai_driver de2_hdmi_codec = {
	.name = "hdmi-codec",
	.playback = {
		.stream_name	= "HDMI Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min	= 8000,
		.rate_max	= 192000,
		.formats	= DE2_HDMI_FORMATS,
	},
	.ops = &de2_hdmi_codec_ops,
};

static const struct snd_soc_codec_driver de2_hdmi_codec_drv = {
	.ignore_pmdown_time = true,
};

int de2_hdmi_codec_register(struct device *dev)
{
	return snd_soc_register_codec(dev, &de2_hdmi_codec_drv,
					&de2_hdmi_codec, 1);
}
EXPORT_SYMBOL_GPL(de2_hdmi_codec_register);

void de2_hdmi_codec_unregister(struct device *dev)
{
	snd_soc_unregister_codec(dev);
}
EXPORT_SYMBOL_GPL(de2_hdmi_codec_unregister);

MODULE_AUTHOR("Jean-Francois Moine <moinejf@free.fr>");
MODULE_DESCRIPTION("Allwinner DE2 HDMI CODEC");
MODULE_LICENSE("GPL");
