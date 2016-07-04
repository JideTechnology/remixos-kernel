/*
 * rt5651_ioctl.h  --  RT5651 ALSA SoC audio driver IO control
 *
 * Copyright 2012 Realtek Microelectronics
 * Author: Bard <bardliao@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <sound/soc.h>
#include "rt56xx_ioctl.h"
#include "rt5651_ioctl.h"
#include "rt5651.h"

hweq_t rt5651_hweq_param[] = {
	{			/* NORMAL */
	 {0}
	 ,
	 {0}
	 ,
	 0x0000,
	 }
	,
		{			/* SPK */
	 {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
	  0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4}
	 ,
	 {0x1279, 0x0000, 0xDFA9, 0x08C4, 0xF988, 0xC45C, 0x1CD0, 0x0424,
	  0xE904, 0x1C10, 0x0000, 0x0000, 0x1C10, 0x0000, 0x1D31, 0x0000,
	  0x1F1C, 0x00DD, 0x1F1F,0x0402,0x0800}
	 ,
	 0x0040,
	 }
	,
	{			/* ADC */
	 {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
	  0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4}
	 ,
	 {0x1561, 0x0000, 0xeeeb, 0xf829, 0xf405, 0xeeeb, 0xf829, 0xf405,
	  0xe904, 0x1c10, 0x01f4, 0xe904, 0x1c10, 0x01f4, 0x1c10, 0x01f4,
	  0x2000, 0x0000, 0x2000,0x065a,0x0800}
	 ,
	 0x0087,
	 }
	,
};

#define RT5651_HWEQ_LEN ARRAY_SIZE(rt5651_hweq_param)

int rt5651_update_eqmode(struct snd_soc_codec *codec, int mode)
{
	struct rt56xx_ops *ioctl_ops = rt56xx_get_ioctl_ops();
	int i;
	static int eqmode;

	if (codec == NULL || mode >= RT5651_HWEQ_LEN)
		return -EINVAL;

	dev_dbg(codec->dev, "%s(): mode=%d\n", __func__, mode);
	if (mode == eqmode)
		return 0;

	for (i = 0; i < EQ_REG_NUM; i++) {
		if (rt5651_hweq_param[mode].reg[i])
			rt5651_index_write(codec, rt5651_hweq_param[mode].reg[i],
					       rt5651_hweq_param[mode].value[i]);
		else
			break;
	}
	snd_soc_update_bits(codec, RT5651_EQ_CTRL2, RT5651_EQ_CTRL_MASK,
			    rt5651_hweq_param[mode].ctrl);
	snd_soc_update_bits(codec, RT5651_EQ_CTRL1,
			    RT5651_EQ_SRC_ADC|RT5651_EQ_UPD, RT5651_EQ_SRC_ADC|RT5651_EQ_UPD); // for ADC 
	//	snd_soc_update_bits(codec, RT5651_EQ_CTRL1,
	//		    RT5651_EQ_UPD, RT5651_EQ_UPD);  // for DAC SPK
	snd_soc_update_bits(codec, RT5651_EQ_CTRL1, RT5651_EQ_UPD, 0);

	eqmode = mode;

	return 0;
}
int rt5651_ioctl_common(struct snd_hwdep *hw, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	struct snd_soc_codec *codec = hw->private_data;
	struct rt56xx_cmd __user *_rt56xx = (struct rt56xx_cmd *)arg;
	struct rt56xx_cmd rt56xx;
	struct rt56xx_ops *ioctl_ops = rt56xx_get_ioctl_ops();
	int *buf, mask1 = 0, mask2 = 0;
	static int eq_mode;

	if (copy_from_user(&rt56xx, _rt56xx, sizeof(rt56xx))) {
		dev_err(codec->dev, "copy_from_user faild\n");
		return -EFAULT;
	}
	dev_dbg(codec->dev, "%s(): rt56xx.number=%d, cmd=%d\n",
			__func__, rt56xx.number, cmd);
	buf = kmalloc(sizeof(*buf) * rt56xx.number, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;
	if (copy_from_user(buf, rt56xx.buf, sizeof(*buf) * rt56xx.number))
		goto err;

	switch (cmd) {
	case RT_GET_CODEC_ID:
		*buf = snd_soc_read(codec, RT5651_DEVICE_ID);
		if (copy_to_user(rt56xx.buf, buf, sizeof(*buf) * rt56xx.number))
			goto err;
		break;


	default:
		break;
	}

	kfree(buf);
	return 0;

err:
	kfree(buf);
	return -EFAULT;
}
EXPORT_SYMBOL_GPL(rt5651_ioctl_common);
