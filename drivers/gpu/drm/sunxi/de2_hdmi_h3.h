#ifndef __DE2_HDMI_H3_H__
#define __DE2_HDMI_H3_H__

void bsp_hdmi_set_video_en(struct de2_hdmi_priv *priv,
			unsigned char enable);
int bsp_hdmi_audio(struct de2_hdmi_priv *priv,
		int sample_rate, int sample_bit);
int bsp_hdmi_video(struct de2_hdmi_priv *priv);
int bsp_hdmi_ddc_read(struct de2_hdmi_priv *priv,
			char pointer, char offset,
			int nbyte, char *pbuf);
int bsp_hdmi_get_hpd(struct de2_hdmi_priv *priv);
void bsp_hdmi_init(struct de2_hdmi_priv *priv);
void bsp_hdmi_hrst(struct de2_hdmi_priv *priv);
int bsp_hdmi_mode_valid(int cea_mode);

#endif /* __DE2_HDMI_H3_H__ */
