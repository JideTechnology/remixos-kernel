#ifndef __VCM_H__
#define __VCM_H__

#include "vm149c.h"
#include "ad5823_aptina.h"
#include "ad5823.h"
#include "dw9714.h"
#include "bu64243.h"
#include "unicam_ext.h"

struct unicam_vcm {
	int (*power_up)(struct v4l2_subdev *sd);
	int (*power_down)(struct v4l2_subdev *sd);
	int (*init)(struct v4l2_subdev *sd);
	int (*t_focus_vcm)(struct v4l2_subdev *sd, u16 val);
	int (*t_focus_abs)(struct v4l2_subdev *sd, s32 value);
	int (*t_focus_rel)(struct v4l2_subdev *sd, s32 value);
	int (*q_focus_status)(struct v4l2_subdev *sd, s32 *value);
	int (*q_focus_abs)(struct v4l2_subdev *sd, s32 *value);
};

static struct unicam_vcm vm149c_vcm_ops = {
	.power_up = vm149c_vcm_power_up,
	.power_down = vm149c_vcm_power_down,
	.init = vm149c_vcm_init,
	.t_focus_vcm = vm149c_t_focus_vcm,
	.t_focus_abs = vm149c_t_focus_abs,
	.t_focus_rel = vm149c_t_focus_rel,
	.q_focus_status = vm149c_q_focus_status,
	.q_focus_abs = vm149c_q_focus_abs,
};

static struct unicam_vcm ad5823_aptina_vcm_ops = {
	.power_up = ad5823_aptina_vcm_power_up,
	.power_down = ad5823_aptina_vcm_power_down,
	.init = ad5823_aptina_vcm_init,
	.t_focus_vcm = ad5823_aptina_t_focus_vcm,
	.t_focus_abs = ad5823_aptina_t_focus_abs,
	.t_focus_rel = ad5823_aptina_t_focus_rel,
	.q_focus_status = ad5823_aptina_q_focus_status,
	.q_focus_abs = ad5823_aptina_q_focus_abs,
};

static struct unicam_vcm dw9714_vcm_ops = {
	.power_up = dw9714_vcm_power_up,
	.power_down = dw9714_vcm_power_down,
	.init = dw9714_vcm_init,
	.t_focus_vcm = dw9714_t_focus_vcm,
	.t_focus_abs = dw9714_t_focus_abs,
	.t_focus_rel = dw9714_t_focus_rel,
	.q_focus_status = dw9714_q_focus_status,
	.q_focus_abs = dw9714_q_focus_abs,
};

static struct unicam_vcm ad5823_vcm_ops = {
	.power_up = ad5823_vcm_power_up,
	.power_down = ad5823_vcm_power_down,
	.init = ad5823_vcm_init,
	.t_focus_vcm = ad5823_t_focus_vcm,
	.t_focus_abs = ad5823_t_focus_abs,
	.t_focus_rel = ad5823_t_focus_rel,
	.q_focus_status = ad5823_q_focus_status,
	.q_focus_abs = ad5823_q_focus_abs,
};

static struct unicam_vcm bu64243_vcm_ops = {
	.power_up = bu64243_vcm_power_up,
	.power_down = bu64243_vcm_power_down,
	.init = bu64243_vcm_init,
	.t_focus_vcm = bu64243_t_focus_vcm,
	.t_focus_abs = bu64243_t_focus_abs,
	.t_focus_rel = bu64243_t_focus_rel,
	.q_focus_status = bu64243_q_focus_status,
	.q_focus_abs = bu64243_q_focus_abs,
};

struct unicam_vcm_mapping {
	char name[32];
	int  id;
	struct unicam_vcm * vcm;
};

const struct unicam_vcm_mapping supported_vcms[] = {
	{"dw9714",UNI_DEV_AF_DW9714,&dw9714_vcm_ops},
	{"ad5823",UNI_DEV_AF_AD5823,&ad5823_vcm_ops},
	{"vm149c",UNI_DEV_AF_VM149C,&vm149c_vcm_ops},
	{"ad5823_aptina",UNI_DEV_AF_AD5823_APTINA,&ad5823_aptina_vcm_ops},
	{"bu64243",UNI_DEV_AF_BU64243,&bu64243_vcm_ops},
	{},
};
#endif
