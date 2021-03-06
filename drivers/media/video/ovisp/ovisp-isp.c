#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <mach/jz_libdmmu.h>
#include <linux/sched.h>
#include <linux/regulator/consumer.h>

#include "ovisp-isp.h"
#include "ovisp-csi.h"
#include "isp-i2c.h"
#include "isp-ctrl.h"
#include "isp-regs.h"
#include "isp-firmware_array.h"
#include "ovisp-video.h"
#include "ovisp-debugtool.h"
#include "ovisp-base.h"
#include "mipi_test_bypass.h"
#include "isp-debug.h"
#ifdef CONFIG_DVP_OV9712
#include "../ov9712.h"
#endif

#define LIGHT_ON 1
#define LIGHT_OFF 0

#define ALL_SETTING_PATH "/data/back_sensor/All_Setting.txt"
/* Timeouts. */
#define ISP_BOOT_TIMEOUT	(1000) /* ms. */
#define ISP_I2C_TIMEOUT		(1000) /* ms. */
#define ISP_ZOOM_TIMEOUT	(1000) /* ms. */
#define ISP_FORMAT_TIMEOUT	(1000) /* ms. */
#define ISP_CAPTURE_TIMEOUT	(1000) /* ms. */
#define ISP_OFFLINE_TIMEOUT	(1000) /* ms. */
#define iSP_BRACKET_TIMEOUT (1000) /* ms */

/* Clock flags. */
#define ISP_CLK_CGU_ISP			(0x00000001)
#define ISP_CLK_GATE_ISP		(0x00000002)
#define ISP_CLK_GATE_CSI		(0x00000004)
#define ISP_CLK_ALL		(0xffffffff)
#define ISP_PWC_GATE_ISP                (0x00000008)

/* CCLK divider. */
#define ISP_CCLK_DIVIDER	(0x04)
static int isp_set_default_parameters(struct isp_device *isp);
static int isp_mipi_init(struct isp_device *isp);
static int isp_fill_i2c_group(struct isp_device *isp);
static int isp_regs_set(struct isp_device *isp, struct isp_reg_t* isp_setting);

static int isp_update_buffer(struct isp_device *isp, struct isp_buffer *buf, int);
static int isp_get_zoom_ratio(struct isp_device *isp, int zoom);

static int isp_clk_enable(struct isp_device *isp, unsigned int type);

#ifdef CONFIG_BOARD_NETEASE
/** Transmission timer */
static void sgm31324_timer_expire(unsigned long data);
static DEFINE_TIMER(led_timer, sgm31324_timer_expire, 0, 0);
extern void enable_sgm31324(int enable);
extern void change_sgm31324_current(unsigned int cur_level, unsigned int tar_level);
#endif

struct isp_clk_info {
	const char* name;
	unsigned long rate;
	unsigned long flags;
};
#undef ISP_CLK_NUM
#define ISP_CLK_NUM	4
#define DUMMY_CLOCK_RATE		0x0000ffff

/* clk isp , csi */
static struct isp_clk_info isp_clks[ISP_CLK_NUM] = {
	{"cgu_isp", 120000000,	ISP_CLK_CGU_ISP},
	{"isp", DUMMY_CLOCK_RATE,	ISP_CLK_GATE_ISP},
	{"csi", DUMMY_CLOCK_RATE,	ISP_CLK_GATE_CSI},
	{"pwc_isp", DUMMY_CLOCK_RATE,   ISP_PWC_GATE_ISP},
};

static unsigned char default_aec_awb_value[] = 
    {	/* exposure line , gain, awb value */
	0x00, 0x00, 0x17, 0x30,
	0x00, 0x10,
	0x00, 0xd1, 0x00, 0x80, 0x00, 0xe2,
    };

#ifdef CONFIG_VIDEO_OV5648
extern struct isp_reg_t ov5648_setting[];
#endif
#ifdef CONFIG_VIDEO_OV8858
extern struct isp_reg_t ov8858_r2a_setting[];
#endif
#ifdef CONFIG_VIDEO_OV8865
extern struct isp_reg_t ov8865_setting[];
#endif
#ifdef CONFIG_VIDEO_OV8856
extern struct isp_reg_t ov8856_setting[];
extern struct isp_reg_t ov8856_setting_face_exposure[];
extern struct isp_reg_t ov8856_setting_normal_exposure[];
extern struct isp_reg_t ov8856_setting_ccm_colour[];
extern struct isp_reg_t ov8856_setting_ccm_gray[];
#endif
#ifdef CONFIG_VIDEO_OV4689
extern struct isp_reg_t ov4689_setting[];
extern struct isp_reg_t ov4689_setting_ccm_colour[];
extern struct isp_reg_t ov4689_setting_ccm_gray[];
#endif
#ifdef CONFIG_VIDEO_OV6211
extern struct isp_reg_t ov6211_setting[];
#endif


static const struct isp_sensor_setting isp_sensor_settings[] = {
#ifdef CONFIG_VIDEO_OV5648
	{
		.chip_id = 0x5648,
		.isp_setting = ov5648_setting,
		.isp_setting_ccm_colour = NULL,
		.isp_setting_ccm_gray = NULL,
	},
#endif
#ifdef CONFIG_VIDEO_OV8858
	{
		.chip_id = 0x8858,
		.isp_setting = ov8858_r2a_setting,
		.isp_setting_ccm_colour = NULL,
		.isp_setting_ccm_gray = NULL,
	},
#endif
#ifdef CONFIG_VIDEO_OV8865
	{
		.chip_id = 0x8865,
		.isp_setting = ov8865_setting,
		.isp_setting_ccm_colour = NULL,
		.isp_setting_ccm_gray = NULL,
	},
#endif
#ifdef CONFIG_VIDEO_OV8856
	{
		.chip_id = 0x8856,
		.isp_setting = ov8856_setting,
		.isp_setting_ccm_colour = ov8856_setting_ccm_colour,
		.isp_setting_ccm_gray = ov8856_setting_ccm_gray,
		.isp_setting_face_exposure = ov8856_setting_face_exposure,
		.isp_setting_normal_exposure = ov8856_setting_normal_exposure,
	},
#endif
#ifdef CONFIG_VIDEO_OV4689
	{
		.chip_id = 0x4689,
		.isp_setting = ov4689_setting,
		.isp_setting_ccm_colour = ov4689_setting_ccm_colour,
		.isp_setting_ccm_gray = ov4689_setting_ccm_gray,
	},
#endif
#ifdef CONFIG_VIDEO_OV6211
	{
		.chip_id = 0x6211,
		.isp_setting = ov6211_setting,
		.isp_setting_ccm_colour = NULL,
		.isp_setting_ccm_gray = NULL,
	},
#endif
};

#define ISP_SETTING_SIZES (ARRAY_SIZE(isp_sensor_settings))

static int isp_setting_init(struct isp_device *isp)
{
#if 1
	struct sensor_info *info = isp->sensor_info;
	struct isp_sensor_setting* isp_sensor_setting;
	struct isp_reg_t* isp_setting = NULL;
	struct isp_reg_t* isp_setting_ccm = NULL;
	int i = 0;
	int light = isp->light;

	if (info == NULL) 
		return -1;
	for (i = 0; i < ISP_SETTING_SIZES; i++) {
		isp_sensor_setting = &isp_sensor_settings[i];
		if (info->chip_id == isp_sensor_setting->chip_id) {
			isp_setting = isp_sensor_setting->isp_setting;
			if (light == LIGHT_ON) {
			    isp_setting_ccm = isp_sensor_setting->isp_setting_ccm_gray;
			}
			break;
		}
	}
	if (isp_setting != NULL) {
		for (i = 0; isp_setting[i].reg != OVISP_REG_END; i++) {
			if (isp_setting[i].reg & 0x10000)
				isp_firmware_writeb(isp, isp_setting[i].value, isp_setting[i].reg);
			else if (isp_setting[i].reg & 0x60000)
				isp_reg_writeb(isp, isp_setting[i].value, isp_setting[i].reg);
		}
	} else {
		ISP_PRINT(ISP_ERROR, "%s[%d] don't have isp setting %d\n", __func__, __LINE__);
		return -1;
	}
	if (isp_setting_ccm != NULL) {
		for (i = 0; isp_setting_ccm[i].reg != OVISP_REG_END; i++) {
			if (isp_setting_ccm[i].reg & 0x10000)
				isp_firmware_writeb(isp, isp_setting_ccm[i].value, isp_setting_ccm[i].reg);
			else if (isp_setting_ccm[i].reg & 0x60000)
				isp_reg_writeb(isp, isp_setting_ccm[i].value, isp_setting_ccm[i].reg);
		}
	}

	return 0;
#else
	calibrate_setting_by_file(isp, ALL_SETTING_PATH);
	return 0;
#endif
}

static int isp_tlb_map_one_vaddr(struct isp_device *isp, unsigned int vaddr, unsigned int size);
static int isp_intc_disable(struct isp_device *isp, unsigned int mask)
{

	unsigned long flags;
	isp_intc_regs_t intr;
	/* unsigned char h,l; */
	/* l = mask & 0xff; */
	/* h = (mask >> 0x08) & 0xff; */

	intr.intc = mask;
	spin_lock_irqsave(&isp->lock, flags);
	if (intr.bits.c3) {
		isp->intr.bits.c3 &= ~(intr.bits.c3);
		isp_reg_writeb(isp, isp->intr.bits.c3, REG_ISP_INT_EN_C3);
	}
	if (intr.bits.c2) {
		isp->intr.bits.c2 &= ~(intr.bits.c2);
		isp_reg_writeb(isp, isp->intr.bits.c2, REG_ISP_INT_EN_C2);
	}
	if (intr.bits.c1) {
		isp->intr.bits.c1 &= ~(intr.bits.c1);
		isp_reg_writeb(isp, isp->intr.bits.c1, REG_ISP_INT_EN_C1);
	}
	if (intr.bits.c0) {
		isp->intr.bits.c0 &= ~(intr.bits.c0);
		isp_reg_writeb(isp, isp->intr.bits.c0, REG_ISP_INT_EN_C0);
	}
	spin_unlock_irqrestore(&isp->lock, flags);

	return 0;
}
static int isp_intc_enable(struct isp_device * isp, unsigned int mask)
{
	unsigned long flags;
	isp_intc_regs_t intr;
	unsigned char h,l;
	/* l = mask & 0xff; */
	/* h = (mask >> 0x08) & 0xff; */

	intr.intc = mask;
	spin_lock_irqsave(&isp->lock, flags);
	if (intr.bits.c3) {
		isp->intr.bits.c3 |= intr.bits.c3;
		isp_reg_writeb(isp, isp->intr.bits.c3, REG_ISP_INT_EN_C3);
	}
	if (intr.bits.c2) {
		isp->intr.bits.c2 |= intr.bits.c2;
		isp_reg_writeb(isp, isp->intr.bits.c2, REG_ISP_INT_EN_C2);
	}
	if (intr.bits.c1) {
		isp->intr.bits.c1 |= intr.bits.c1;
		isp_reg_writeb(isp, isp->intr.bits.c1, REG_ISP_INT_EN_C1);
	}
	if (intr.bits.c0) {
		isp->intr.bits.c0 |= intr.bits.c0;
		isp_reg_writeb(isp, isp->intr.bits.c0, REG_ISP_INT_EN_C0);
	}
	spin_unlock_irqrestore(&isp->lock, flags);

	return 0;
}

static unsigned int isp_intc_state(struct isp_device *isp)
{
	isp_intc_regs_t intr;
	intr.bits.c3 = isp_reg_readb(isp, REG_ISP_INT_STAT_C3);
	intr.bits.c2 = isp_reg_readb(isp, REG_ISP_INT_STAT_C2);
	intr.bits.c1 = isp_reg_readb(isp, REG_ISP_INT_STAT_C1);
	intr.bits.c0 = isp_reg_readb(isp, REG_ISP_INT_STAT_C0);
	return intr.intc;
}

static int isp_mac_int_mask(struct isp_device *isp, unsigned short mask)
{
	unsigned char mask_l = mask & 0x00ff;
	unsigned char mask_h = (mask >> 8) & 0x00ff;
	unsigned long flags;

	spin_lock_irqsave(&isp->lock, flags);
	if (mask_l) {
		isp->mac_intr.bits.c0 &= ~mask_l;
		isp_reg_writeb(isp, isp->mac_intr.bits.c0, REG_ISP_MAC_INT_EN_L);
	}
	if (mask_h) {
		isp->mac_intr.bits.c1 &= ~mask_h;
		isp_reg_writeb(isp, isp->mac_intr.bits.c1, REG_ISP_MAC_INT_EN_H);
	}
	spin_unlock_irqrestore(&isp->lock, flags);

	return 0;
}

static int isp_mac_int_unmask(struct isp_device *isp, unsigned short mask)
{
	unsigned char mask_l = mask & 0x00ff;
	unsigned char mask_h = (mask >> 8) & 0x00ff;
	unsigned long flags;

	spin_lock_irqsave(&isp->lock, flags);
	if (mask_l) {
		isp->mac_intr.bits.c0 |= mask_l;
		isp_reg_writeb(isp, isp->mac_intr.bits.c0, REG_ISP_MAC_INT_EN_L);
	}
	if (mask_h) {
		isp->mac_intr.bits.c1 |= mask_h;
		isp_reg_writeb(isp, isp->mac_intr.bits.c1, REG_ISP_MAC_INT_EN_H);
	}
	spin_unlock_irqrestore(&isp->lock, flags);

	return 0;
}

static unsigned short isp_mac_int_state(struct isp_device *isp)
{
	unsigned short state_l;
	unsigned short state_h;
	state_l = isp_reg_readb(isp, REG_ISP_MAC_INT_STAT_L);
	state_h = isp_reg_readb(isp, REG_ISP_MAC_INT_STAT_H);

	return (state_h << 8) | state_l;
}


static int isp_wait_cmd_done(struct isp_device *isp, unsigned long timeout)
{
	unsigned long tm;
	int ret = 0;
	tm = wait_for_completion_timeout(&isp->completion,
			msecs_to_jiffies(timeout));
	if (!tm && !isp->completion.done) {
		ret = -ETIMEDOUT;
	}
	return ret;
}
static int isp_wait_frame_eof(struct isp_device *isp)
{
	unsigned long tm, flags;
	int ret = 0;
	INIT_COMPLETION(isp->frame_eof);
	spin_lock_irqsave(&isp->lock, flags);
	isp->wait_eof = true;
	spin_unlock_irqrestore(&isp->lock, flags);
//	isp_intc_enable(isp, MASK_ISP_INT_EOF);
	tm = wait_for_completion_timeout(&isp->frame_eof,
			msecs_to_jiffies(500));
//	isp_intc_disable(isp, MASK_ISP_INT_EOF);
	if (!tm && !isp->frame_eof.done) {
		ret = -ETIMEDOUT;
	}
	return ret;
}
static inline unsigned char get_current_finish_cmd(struct isp_device *isp)
{
	return isp_reg_readb(isp, COMMAND_FINISHED);
}
static int isp_send_cmd(struct isp_device *isp, unsigned char id, unsigned long timeout)
{
	int ret;
	int max_cmd_sendtimes = 5;

	INIT_COMPLETION(isp->completion);
	isp_reg_writeb(isp, 0x0, COMMAND_RESULT);
	isp_reg_writeb(isp, 0x04, REG_ISP_INT_EN_C0);
//	isp_intc_enable(isp, MASK_INT_CMDSET);
	isp_reg_writeb(isp, id, COMMAND_REG0);
	udelay(50);
	while (id != isp_firmware_readb(isp, 0x1c010)) {
		/* printk("id = %d\n",id); */
		if (max_cmd_sendtimes-- == 0) {
			printk("SSSSSSSSSS error\n");
			break;
		}	    
		isp_reg_writeb(isp, id, COMMAND_REG0);
		udelay(50);
	}
	/* Wait for command set done interrupt. */
	ret = isp_wait_cmd_done(isp, timeout);
//	isp_intc_disable(isp, MASK_INT_CMDSET);
	/* determine whether setting cammand successfully */
	if ((CMD_SET_SUCCESS != isp_reg_readb(isp, COMMAND_RESULT))
			|| (id != get_current_finish_cmd(isp))) {
		printk("Failed to send command (%02x:%02x:%02x:%02x)\n",
		       isp_reg_readb(isp, COMMAND_RESULT), isp_reg_readb(isp, COMMAND_FINISHED), 
		       isp_reg_readb(isp, 0x6390f), isp_firmware_readb(isp, 0x1c21b));

		dump_csi_reg();
		dump_isp_timestamp(isp);

		ret = -EINVAL;
	}

	return ret;
}

static int isp_set_address(struct isp_device *isp,
		unsigned int id, unsigned int addr)
{
	struct isp_parm *iparm = &isp->parm;
	unsigned int reg = id ? REG_BASE_ADDR1 : REG_BASE_ADDR0;
	unsigned int regw = reg;
	unsigned int addrw = addr;
	switch (iparm->output[0].addrnums) {
		case 3:
			addrw = addr + iparm->output[0].addroff[2];
			regw = reg + 0x08;
			isp_reg_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_reg_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_reg_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_reg_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
		case 2:
			addrw = addr + iparm->output[0].addroff[1];
			regw = reg + 0x04;
			isp_reg_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_reg_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_reg_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_reg_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
		case 1:
			addrw = addr;
			regw = reg;
			isp_reg_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_reg_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_reg_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_reg_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
			break;
		default:
			ISP_PRINT(ISP_ERROR, "%s[%d] addrnums is wrong; it should be 1 ~ 3,but it is %d",
					__func__,__LINE__,iparm->output[0].addrnums);
			break;
	}
	if (iparm->out_videos == 2) {
		addr += iparm->output[0].sizeimage;
		switch (iparm->output[1].addrnums) {
			case 3:
				addrw = addr + iparm->output[1].addroff[2];
				if (0 == id)
					regw = reg + 0x64;
				else
					regw = reg + 0x5c;
				isp_reg_writeb(isp, (addrw >> 24) & 0xff, regw);
				isp_reg_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
				isp_reg_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
				isp_reg_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
			case 2:
				addrw = addr + iparm->output[1].addroff[1];
				if (0 == id)
					regw = reg + 0x60;
				else
					regw = reg + 0x58;
				isp_reg_writeb(isp, (addrw >> 24) & 0xff, regw);
				isp_reg_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
				isp_reg_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
				isp_reg_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
			case 1:
				addrw = addr;
				regw = reg + 0x0c;
				isp_reg_writeb(isp, (addrw >> 24) & 0xff, regw);
				isp_reg_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
				isp_reg_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
				isp_reg_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
				break;
			default:
				ISP_PRINT(ISP_ERROR, "%s[%d] addrnums is wrong; it should be 1 ~ 3,but it is %d",
						__func__,__LINE__,iparm->output[1].addrnums);
				break;
		}
	}
	return 0;
}

struct isp_idi_parm {
	unsigned int scale;
	int reg;
} idi_array[] = {
	{3000, 2},
	{2500, 4},
	{2000, 1},
	{1667, 0},
	{1500, 3},
	{1000, -1}
};
#define IDI_ARRAY_SIZES (ARRAY_SIZE(idi_array))
static inline unsigned int isp_calc_idi_scale(struct isp_input_parm *input, struct isp_output_parm *output)
{
	unsigned int ret = 0;
	unsigned int in_width = input->idi_in_width;
	unsigned int in_height = input->idi_in_height;
	unsigned int out_width = input->idi_out_width;
	unsigned int out_height = input->idi_out_height;
	int i = 0;
	//printk("idi in %d %d   out %d %d\n", in_width, in_height, out_width, out_height);
	for(i = 0; i < IDI_ARRAY_SIZES; i++){
		if(in_width * 1000 >= out_width * idi_array[i].scale &&
			in_height * 1000 >= out_height * idi_array[i].scale)
			break;
	}
	if (in_width == 2688 && in_height == 1520 &&
	    out_width == 1792 && out_height == 1168) {
		i--;
	} else if (in_width == 1920 && in_height == 1920 &&
	    out_width == 1280 && out_height == 1280) {
	        i = 4;
	} else {
		i = IDI_ARRAY_SIZES - 1;
	}

	if(idi_array[i].reg != -1){
		ret = idi_array[i].reg | 0x08;
		input->idi_in_width = (in_width * 1000 / idi_array[i].scale);
		input->idi_in_height = (in_height * 1000 / idi_array[i].scale);
		input->idi_in_width &= ~0xf;
		input->idi_in_height &= ~0xf;
	}
	return ret;
}
static inline void isp_set_reg_t(struct isp_reg_t *dst, unsigned char value, unsigned int reg)
{
	static int index = 0;
	dst[index].reg = reg;
	dst[index].value = value;
	if(reg == 0)
		index = 0;
	else
		index++;
}
static int isp_set_preview_parameters(struct isp_device *isp, unsigned int videos)
{
	struct ovisp_camera_client *client = isp->client;
	struct isp_parm *iparm = &isp->parm;
	struct v4l2_fmt_data *fmt_data = &isp->fmt_data;
	struct isp_reg_t *preview = isp->preview;
	unsigned int idi = 0, i = 0;
	unsigned int vstart = 0, hstart = 0;

	if (!isp->bypass)
		iparm->input.format |= ISP_PROCESS;/*bypass isp or isp processing*/

	if (client->flags & CAMERA_CLIENT_IF_MIPI) {
		iparm->input.format |= SENSOR_PRIMARY_MIPI;
		for(i = 0; i < videos; i++){
			if(iparm->input.idi_out_width < iparm->output[i].width)
				iparm->input.idi_out_width = iparm->output[i].width;
			if(iparm->input.idi_out_height < iparm->output[i].height)
				iparm->input.idi_out_height = iparm->output[i].height;
			ISP_PRINT(ISP_INFO, "idi output: out_width:%d  out_height:%d\n",
				  iparm->input.idi_out_width, iparm->input.idi_out_height);
		}
	}else{
		/* DVP sensor */
		iparm->input.idi_out_width = iparm->input.width;
		iparm->input.idi_out_height = iparm->input.height;
	}

	/* isp_set_reg_t(preview, 0x01, 0x1fff9); */

	/* 1. INPUT CONFIGURATION */
	isp_set_reg_t(preview, (iparm->input.format >> 8) & 0xff, ISP_INPUT_FORMAT);
	isp_set_reg_t(preview, iparm->input.format & 0xff, ISP_INPUT_FORMAT + 1);

	isp_set_reg_t(preview, (iparm->input.width >> 8) & 0xff,
			SENSOR_OUTPUT_WIDTH);
	isp_set_reg_t(preview, iparm->input.width & 0xff,
			SENSOR_OUTPUT_WIDTH + 1);

	isp_set_reg_t(preview, (iparm->input.height >> 8) & 0xff,
			SENSOR_OUTPUT_HEIGHT);
	isp_set_reg_t(preview, iparm->input.height & 0xff,
			SENSOR_OUTPUT_HEIGHT + 1);

	/*IDI CONTROL, DISABLE ISP IDI SCALE*/
	if(!isp->bypass){
		ISP_PRINT(ISP_INFO, "before idi scale: in_width:%d  in_height:%d\n",
				iparm->input.idi_in_width, iparm->input.idi_in_height);
		idi = isp_calc_idi_scale(&iparm->input, &iparm->output[0]);
		ISP_PRINT(ISP_INFO, "after idi scale:(0x%08x) in_width:%d  in_height:%d\n",
				idi, iparm->input.idi_in_width, iparm->input.idi_in_height);
	}
	isp_set_reg_t(preview, (idi >> 8) & 0xff, ISP_IDI_CONTROL);
	isp_set_reg_t(preview, idi & 0xff, ISP_IDI_CONTROL + 1);
	/* idi w,h */
/* #ifdef CONFIG_VIDEO_OV5648 */
	iparm->input.idi_out_width = iparm->input.idi_in_width;
	iparm->input.idi_out_height = iparm->input.idi_in_height;
/* #endif */
	isp_set_reg_t(preview, (iparm->input.idi_out_width >> 8) & 0xff, ISP_IDI_OUTPUT_WIDTH);
	isp_set_reg_t(preview, iparm->input.idi_out_width & 0xff, ISP_IDI_OUTPUT_WIDTH + 1);
	isp_set_reg_t(preview, (iparm->input.idi_out_height >> 8) & 0xff, ISP_IDI_OUTPUT_HEIGHT);
	isp_set_reg_t(preview, iparm->input.idi_out_height & 0xff, ISP_IDI_OUTPUT_HEIGHT + 1);
	/* set crop */
	if(iparm->input.idi_out_width < iparm->input.idi_in_width)
		hstart = (iparm->input.idi_in_width - iparm->input.idi_out_width) >> 1;
	if(iparm->input.idi_out_height < iparm->input.idi_in_height)
		vstart = (iparm->input.idi_in_height - iparm->input.idi_out_height) >> 1;
	ISP_PRINT(ISP_INFO, "Crop hstart = %d, vstart = %d\n",hstart,vstart);
	isp_set_reg_t(preview, (hstart >> 8) & 0xff, ISP_IDI_OUTPUT_H_START);
	isp_set_reg_t(preview, hstart & 0xff, ISP_IDI_OUTPUT_H_START + 1);
	isp_set_reg_t(preview, (vstart >> 8) & 0xff, ISP_IDI_OUTPUT_V_START);
	isp_set_reg_t(preview, vstart & 0xff, ISP_IDI_OUTPUT_V_START + 1);

	/* 2. OUTPUT CONFIGRATION */
	/* output1 */
	isp_set_reg_t(preview, (iparm->output[0].format >> 8) & 0xff, ISP_OUTPUT_FORMAT);
	isp_set_reg_t(preview, iparm->output[0].format & 0xff, ISP_OUTPUT_FORMAT + 1);

	isp_set_reg_t(preview, (iparm->output[0].width >> 8) & 0xff,
			ISP_OUTPUT_WIDTH);
	isp_set_reg_t(preview, iparm->output[0].width & 0xff,
			ISP_OUTPUT_WIDTH + 1);
	isp_set_reg_t(preview, (iparm->output[0].height >> 8) & 0xff,
			ISP_OUTPUT_HEIGHT);
	isp_set_reg_t(preview, iparm->output[0].height & 0xff,
			ISP_OUTPUT_HEIGHT + 1);
	/*y memwidth*/
	isp_set_reg_t(preview, (iparm->output[0].width >> 8) & 0xff, MAC_MEMORY_WIDTH);
	isp_set_reg_t(preview, iparm->output[0].width & 0xff, MAC_MEMORY_WIDTH + 1);

	isp_set_reg_t(preview, (iparm->output[0].width >> 8) & 0xff, MAC_MEMORY_UV_WIDTH);
	isp_set_reg_t(preview, iparm->output[0].width & 0xff, MAC_MEMORY_UV_WIDTH + 1);
	/*uv memwidth*/
	/* output2 */
	if(videos == 2){
		isp_set_reg_t(preview, (iparm->output[1].format >> 8) & 0xff, ISP_OUTPUT_FORMAT_2);
		isp_set_reg_t(preview, iparm->output[1].format & 0xff, ISP_OUTPUT_FORMAT_2 + 1);

		isp_set_reg_t(preview, (iparm->output[1].width >> 8) & 0xff,
				ISP_OUTPUT_WIDTH_2);
		isp_set_reg_t(preview, iparm->output[1].width & 0xff,
				ISP_OUTPUT_WIDTH_2 + 1);
		isp_set_reg_t(preview, (iparm->output[1].height >> 8) & 0xff,
				ISP_OUTPUT_HEIGHT_2);
		isp_set_reg_t(preview, iparm->output[1].height & 0xff,
				ISP_OUTPUT_HEIGHT_2 + 1);

		isp_set_reg_t(preview, (iparm->output[1].width >> 8) & 0xff, MAC_MEMORY_WIDTH_2);
		isp_set_reg_t(preview, iparm->output[1].width & 0xff, MAC_MEMORY_WIDTH_2 + 1);

		isp_set_reg_t(preview, (iparm->output[1].width >> 8) & 0xff, MAC_MEMORY_UV_WIDTH_2);
		isp_set_reg_t(preview, iparm->output[1].width & 0xff, MAC_MEMORY_UV_WIDTH_2 + 1);
	}
	/* 3. ISP CONFIGURATION */
	//isp config
#ifdef CONFIG_BOARD_M200S_ZBRZFY
	isp_set_reg_t(preview, 0x01, ISP_EXPOSURE_RATIO);
	isp_set_reg_t(preview, 0x3b, ISP_EXPOSURE_RATIO + 1);
#else
	isp_set_reg_t(preview, 0x01, ISP_EXPOSURE_RATIO);
	isp_set_reg_t(preview, 0x00, ISP_EXPOSURE_RATIO + 1);
#endif
	isp_set_reg_t(preview, ((fmt_data->vts - 0x10) >> 8) & 0xff,
						ISP_MAX_EXPOSURE);
	isp_set_reg_t(preview, (fmt_data->vts - 0x10) & 0xff,
						ISP_MAX_EXPOSURE + 1);

	isp_set_reg_t(preview, 0x00, ISP_MIN_EXPOSURE);
	isp_set_reg_t(preview, 0x02, ISP_MIN_EXPOSURE + 1);
	isp_set_reg_t(preview, 0x00, ISP_MAX_GAIN);
	isp_set_reg_t(preview, 0xff, ISP_MAX_GAIN + 1);

	isp_set_reg_t(preview, 0x00, ISP_MIN_GAIN);
	isp_set_reg_t(preview, 0x10, ISP_MIN_GAIN + 1);
	isp_set_reg_t(preview, (fmt_data->vts >> 8) & 0xff, ISP_VTS);
	isp_set_reg_t(preview, fmt_data->vts & 0xff, ISP_VTS + 1);
	//zoom in 1x
	isp_set_reg_t(preview, 0x01, ZOOM_RATIO);
	isp_set_reg_t(preview, 0x00, ZOOM_RATIO + 1);

	/* 4. CAPTURE CONFIGURATION */

	/* isp_set_reg_t(preview, 0x06, 0x1fff9); */
	isp_set_reg_t(preview, 0, 0); //end flag
	return 0;
}
static int isp_set_capture_raw_parameters(struct isp_device *isp, unsigned int videos)
{
	struct ovisp_camera_client *client = isp->client;
	struct isp_parm *iparm = &isp->parm;
	struct isp_reg_t *capture = isp->captureraw;

	if (!isp->bypass)
		iparm->input.format |= ISP_PROCESS;/*bypass isp or isp processing*/

	if (client->flags & CAMERA_CLIENT_IF_MIPI)
		iparm->input.format |= SENSOR_PRIMARY_MIPI;

	/* isp_set_reg_t(capture, 0x01, 0x1fff9); */

	/* 1. INPUT CONFIGURATION */
	isp_set_reg_t(capture, (iparm->input.format >> 8) & 0xff, ISP_INPUT_FORMAT);
	isp_set_reg_t(capture, iparm->input.format & 0xff, ISP_INPUT_FORMAT + 1);

	isp_set_reg_t(capture, (iparm->input.width >> 8) & 0xff,
			SENSOR_OUTPUT_WIDTH);
	isp_set_reg_t(capture, iparm->input.width & 0xff,
			SENSOR_OUTPUT_WIDTH + 1);

	isp_set_reg_t(capture, (iparm->input.height >> 8) & 0xff,
			SENSOR_OUTPUT_HEIGHT);
	isp_set_reg_t(capture, iparm->input.height & 0xff,
			SENSOR_OUTPUT_HEIGHT + 1);

	/* idi w,h */
	isp_set_reg_t(capture, (iparm->input.width >> 8) & 0xff, ISP_IDI_OUTPUT_WIDTH);
	isp_set_reg_t(capture, iparm->input.width & 0xff, ISP_IDI_OUTPUT_WIDTH + 1);
	isp_set_reg_t(capture, (iparm->input.height >> 8) & 0xff, ISP_IDI_OUTPUT_HEIGHT);
	isp_set_reg_t(capture, iparm->input.height & 0xff, ISP_IDI_OUTPUT_HEIGHT + 1);

	isp_set_reg_t(capture, 0x00, ISP_IDI_OUTPUT_H_START);
	isp_set_reg_t(capture, 0x00, ISP_IDI_OUTPUT_H_START + 1);
	isp_set_reg_t(capture, 0x00, ISP_IDI_OUTPUT_V_START);
	isp_set_reg_t(capture, 0x00, ISP_IDI_OUTPUT_V_START + 1);

	/* 2. OUTPUT CONFIGRATION */
	/* output1 */
	isp_set_reg_t(capture, (iparm->output[1].format >> 8) & 0xff, ISP_OUTPUT_FORMAT);
	isp_set_reg_t(capture, iparm->output[1].format & 0xff, ISP_OUTPUT_FORMAT + 1);

	isp_set_reg_t(capture, (iparm->output[1].width >> 8) & 0xff,
			ISP_OUTPUT_WIDTH);
	isp_set_reg_t(capture, iparm->output[1].width & 0xff,
			ISP_OUTPUT_WIDTH + 1);
	isp_set_reg_t(capture, (iparm->output[1].height >> 8) & 0xff,
			ISP_OUTPUT_HEIGHT);
	isp_set_reg_t(capture, iparm->output[1].height & 0xff,
			ISP_OUTPUT_HEIGHT + 1);
	/*y memwidth*/
	isp_set_reg_t(capture, (iparm->output[1].width >> 8) & 0xff, MAC_MEMORY_WIDTH);
	isp_set_reg_t(capture, iparm->output[1].width & 0xff, MAC_MEMORY_WIDTH + 1);

	/* isp_set_reg_t(capture, 0x06, 0x1fff9); */


	isp_set_reg_t(capture, 0, 0); //end flag
	return 0;
}
static int isp_set_capture_parameters(struct isp_device *isp, unsigned int videos)
{
	return 0;
}
static int isp_fill_parm_registers(struct isp_device *isp, struct isp_reg_t *regs)
{
	int i = 0;
	for(i = 0; i < 100 && regs[i].reg != 0; i++){
		isp_firmware_writeb(isp, regs[i].value, regs[i].reg);
	//	printk("^^^^^ reg=0x%x [0x%02x]\n",regs[i].reg, regs[i].value);
	}
	if(i >= 100)
		return -EINVAL;
	else
		return 0;
}
static int isp_i2c_config(struct isp_device *isp)
{
	unsigned char val;

	if (isp->pdata->flags & CAMERA_I2C_FAST_SPEED)
		val = I2C_SPEED_375;
	else
		val = I2C_SPEED_200;

	isp_reg_writeb(isp, val, REG_SCCB_MAST1_SPEED);

	return 0;
}
static int isp_i2c_xfer_cmd_sccb(struct isp_device * isp, struct isp_i2c_cmd * cmd)
{
	unsigned char sccb_cmd = 0;
	isp_reg_writeb(isp, cmd->addr, 0x63601);

	/*sensore reg*/
	if(!(cmd->flags & I2C_CMD_NO_ADDR)){
	    isp_reg_writeb(isp, (cmd->reg >> 8) & 0xff, 0x63602);
	    isp_reg_writeb(isp, cmd->reg & 0xff, 0x63603);
	}

	if(cmd->flags & I2C_CMD_DATA_16BIT)
		sccb_cmd |= (0x01 << 1);
	if(cmd->flags & I2C_CMD_ADDR_16BIT)
		sccb_cmd |= 0x01;
	isp_reg_writeb(isp, sccb_cmd, 0x63606);/*16bit addr enable*/

	if(!(cmd->flags & I2C_CMD_READ)) {
		if(cmd->flags & I2C_CMD_DATA_16BIT) {
			isp_reg_writeb(isp, (cmd->data >> 8) & 0xff, 0x63604);
			isp_reg_writeb(isp, cmd->data & 0xff, 0x63605);
			/**/
		} else {
			/*write data*/
			isp_reg_writeb(isp, (cmd->data >> 8) & 0xff, 0x63604);
			isp_reg_writeb(isp, cmd->data & 0xff, 0x63605);
		}
		if(cmd->flags & I2C_CMD_NO_ADDR){
		    isp_reg_writeb(isp, 0x35, 0x63609);
		}else{
		    isp_reg_writeb(isp, 0x37, 0x63609);
		}
		mdelay(1);
	}
	if (cmd->flags & I2C_CMD_READ) {

		if(cmd->flags & I2C_CMD_NO_ADDR){
		    isp_reg_writeb(isp, 0x31, 0x63609);
		    mdelay(1);
		    isp_reg_writeb(isp, 0xf9, 0x63609);
		    mdelay(1);
		}else{
		    isp_reg_writeb(isp, 0x33, 0x63609);
		    mdelay(1);
		    isp_reg_writeb(isp, 0xf9, 0x63609);
		    mdelay(1);
		}
		if(cmd->flags & I2C_CMD_DATA_16BIT) {
			cmd->data = (isp_reg_readb(isp, 0x63607)<<8) + isp_reg_readb(isp, 0x63608); /*read data*/
		}
		else {
			cmd->data = isp_reg_readb(isp, 0x63608); /*read data*/
		}
	}
	//dump_i2c_regs(isp);
	return 0;
}

static int isp_i2c_xfer_cmd(struct isp_device *isp, struct isp_i2c_cmd *cmd);
struct v4l2_fmt_data gdata;
static int reg_num = 0;
static int isp_i2c_xfer_cmd_grp(struct isp_device * isp , struct isp_i2c_cmd * cmd)
{
	unsigned char val = 0;
	unsigned char i;
	if (cmd->flags & I2C_CMD_READ) {
		isp_i2c_xfer_cmd(isp, cmd);
		return 0;
	}
	/* I2C CMD WRITE */
		if(cmd->reg == 0xffff) { /*cmd list end*/
		/* send group read/write command */

		/*16bit: addr, 8bit:data, 8bit:mask*/
		for (i = 0; i < gdata.reg_num; i++) {
			isp_reg_writew(isp, gdata.reg[i].addr, COMMAND_BUFFER + i * 4 + 2);
			if (gdata.i2cflags & I2C_CMD_DATA_16BIT) {
				isp_reg_writew(isp, gdata.reg[i].data,
						COMMAND_BUFFER + i * 4);
			} else {
				isp_reg_writeb(isp, 0xff, COMMAND_BUFFER + i * 4);
				isp_reg_writeb(isp, gdata.reg[i].data & 0xff,
						COMMAND_BUFFER + i * 4 + 1);
			}
		}

		if (gdata.reg_num) {
			val |= SELECT_I2C_PRIMARY | SELECT_I2C_WRITE;
			if (gdata.i2cflags & V4L2_I2C_ADDR_16BIT)
				val |= SELECT_I2C_16BIT_ADDR;
			if (gdata.i2cflags & V4L2_I2C_DATA_16BIT)
				val |= SELECT_I2C_16BIT_DATA;

			isp_reg_writeb(isp, val, COMMAND_REG1);
			isp_reg_writeb(isp, gdata.slave_addr, COMMAND_REG2);
			isp_reg_writeb(isp, gdata.reg_num, COMMAND_REG3);
		}

		/* __dump_isp_regs(isp, 0x63900, 0x63911); */

		/* Wait for command set successfully. */
		if (isp_send_cmd(isp, CMD_I2C_GRP_WR, ISP_I2C_TIMEOUT)) {
			ISP_PRINT(ISP_INFO, KERN_ERR "Failed to wait i2c set done (%02x)!\n",
					isp_reg_readb(isp, REG_ISP_INT_EN_C2));
			return -EINVAL;
		}

	} else {
		reg_num ++;
		gdata.reg_num = reg_num;
		gdata.slave_addr = cmd->addr;
		gdata.i2cflags = cmd->flags;
		gdata.reg[reg_num-1].addr = cmd->reg;
		gdata.reg[reg_num-1].data = cmd->data;
		ISP_PRINT(ISP_INFO,"data.slave_addr:%x, flags:%x,reg[%d].addr:%x,data:%x\n", gdata.slave_addr, gdata.i2cflags,
				reg_num-1, gdata.reg[reg_num-1].addr, gdata.reg[reg_num-1].data);
	}
	return 0;
}

static int isp_i2c_xfer_cmd(struct isp_device *isp, struct isp_i2c_cmd *cmd)
{
	unsigned char val = 0;

	//dump_i2c_regs(isp);
	/*isp_firmware_writeb(isp, 0x78, 0x1e056);*/
	isp_reg_writew(isp, cmd->reg, COMMAND_BUFFER + 2);
	if (!(cmd->flags & I2C_CMD_READ)) {
		if (cmd->flags & I2C_CMD_DATA_16BIT) {
			isp_reg_writew(isp, cmd->data, COMMAND_BUFFER);
		} else { /*16bit:8bitdata,8bitmask*/
			isp_reg_writeb(isp, 0xff, COMMAND_BUFFER);
			isp_reg_writeb(isp, cmd->data & 0xff, COMMAND_BUFFER + 1);
		}
	}

	val |= SELECT_I2C_PRIMARY;
	if (!(cmd->flags & I2C_CMD_READ))
		val |= SELECT_I2C_WRITE;
	if (cmd->flags & I2C_CMD_ADDR_16BIT)
		val |= SELECT_I2C_16BIT_ADDR;
	if (cmd->flags & I2C_CMD_DATA_16BIT)
		val |= SELECT_I2C_16BIT_DATA;

	isp_reg_writeb(isp, val, COMMAND_REG1);
	isp_reg_writeb(isp, cmd->addr, COMMAND_REG2);
	isp_reg_writeb(isp, 0x01, COMMAND_REG3);

	/* Wait for command set successfully. */
	if (isp_send_cmd(isp, CMD_I2C_GRP_WR, ISP_I2C_TIMEOUT)) {
		ISP_PRINT(ISP_INFO, KERN_ERR "Failed to wait i2c set done (%02x)!\n",
				isp_reg_readb(isp, REG_ISP_INT_EN_C2));
		return -EINVAL;
	}

	if (cmd->flags & I2C_CMD_READ) {
		if (cmd->flags & I2C_CMD_DATA_16BIT)
			cmd->data = isp_reg_readw(isp, COMMAND_BUFFER);
		else
			cmd->data = isp_reg_readb(isp, COMMAND_BUFFER);
	}

	//dump_i2c_regs(isp);
	return 0;
}

static int isp_i2c_fill_buffer(struct isp_device *isp)
{
	struct v4l2_fmt_data *data = &isp->fmt_data;
	unsigned char val = 0;
	unsigned char i;

	/*16bit: addr, 8bit:data, 8bit:mask*/
	for (i = 0; i < data->reg_num; i++) {
		isp_reg_writew(isp, data->reg[i].addr, COMMAND_BUFFER + i * 4 + 2);
		if (data->i2cflags & I2C_CMD_DATA_16BIT) {
			isp_reg_writew(isp, data->reg[i].data,
					COMMAND_BUFFER + i * 4);
		} else {
			isp_reg_writeb(isp, 0xff, COMMAND_BUFFER + i * 4);
			isp_reg_writeb(isp, data->reg[i].data & 0xff,
					COMMAND_BUFFER + i * 4 + 1);
		}
	}

	if (data->reg_num) {
		val |= SELECT_I2C_PRIMARY | SELECT_I2C_WRITE;
		if (data->i2cflags & V4L2_I2C_ADDR_16BIT)
			val |= SELECT_I2C_16BIT_ADDR;
		if (data->i2cflags & V4L2_I2C_DATA_16BIT)
			val |= SELECT_I2C_16BIT_DATA;

		isp_reg_writeb(isp, val, COMMAND_REG1);
		isp_reg_writeb(isp, data->slave_addr << 1, COMMAND_REG2);
		isp_reg_writeb(isp, data->reg_num, COMMAND_REG3);
	} else {
		isp_reg_writeb(isp, 0x00, COMMAND_REG1);
		isp_reg_writeb(isp, 0x00, COMMAND_REG2);
		isp_reg_writeb(isp, 0x00, COMMAND_REG3);
	}
	if(!isp->bypass&&!isp->debug.status&&!isp->snapshot) {
		isp_reg_writeb(isp, 0x00, COMMAND_REG1);
		isp_reg_writeb(isp, 0x00, COMMAND_REG2);
		isp_reg_writeb(isp, 0x00, COMMAND_REG3);
	}

	return 0;
}

static int isp_set_default_parameters(struct isp_device *isp)
{
	int i = 0;
	for(i = 0; i < 6; i++){
	    isp_firmware_writeb(isp, default_aec_awb_value[i], 0x1e030 + i);
	    isp_firmware_writeb(isp, default_aec_awb_value[i + 6], 0x1c082 + i);
	}
	return 0;
}

static int isp_set_banding_parameters(struct isp_device *isp, bool is_preview_mode)
{
	struct v4l2_fmt_data *data = &isp->fmt_data;

	/* the default fps is 24, if others ,wrong . banding step=（sensor_sys_pclk / hts / 100）*0x10*/
	unsigned int value_fifty = 0;
	unsigned int value_sixty = 0;
	value_fifty = (16000000 * (unsigned int)data->sensor_sys_pclk)/(100 * (unsigned int)data->hts);
	value_sixty = (16000000 * (unsigned int)data->sensor_sys_pclk)/(120 * (unsigned int)data->hts);
	if(is_preview_mode){
	    /* in preview mode ,  reg ISP_BANDING_50HZ do not work*/
	    isp_firmware_writeb(isp, (value_fifty >> 8) & 0xff, 0x1e046);
	    isp_firmware_writeb(isp, value_fifty & 0xff, 0x1e047);
	    isp_firmware_writeb(isp, (value_sixty >> 8) & 0xff, 0x1e044);
	    isp_firmware_writeb(isp, value_sixty & 0xff, 0x1e045);
	}else{
	    isp_firmware_writeb(isp, (value_fifty >> 8) & 0xff, ISP_BANDING_50HZ);
	    isp_firmware_writeb(isp, value_fifty & 0xff, ISP_BANDING_50HZ + 1);
	    isp_firmware_writeb(isp, (value_sixty >> 8) & 0xff, ISP_BANDING_60HZ);
	    isp_firmware_writeb(isp, value_sixty & 0xff, ISP_BANDING_60HZ + 1);
	}
	return 0;
}

static int isp_set_color_parameters(struct isp_device *isp)
{
	struct sensor_info *info = isp->sensor_info;
	struct isp_sensor_setting* isp_sensor_setting;
	struct isp_reg_t* isp_setting_ccm = NULL;
	int i = 0;
	int light = isp->light;

	if (info == NULL) 
		return -1;
	for (i = 0; i < ISP_SETTING_SIZES; i++) {
		isp_sensor_setting = &isp_sensor_settings[i];
		if (info->chip_id == isp_sensor_setting->chip_id) {
			if (light == LIGHT_ON) {
				isp_setting_ccm = isp_sensor_setting->isp_setting_ccm_gray;
			} else {
				isp_setting_ccm = isp_sensor_setting->isp_setting_ccm_colour;
			}
			break;
		}
	}
	if (isp_setting_ccm != NULL) {
		for (i = 0; isp_setting_ccm[i].reg != OVISP_REG_END; i++) {
			if (isp_setting_ccm[i].reg & 0x10000)
				isp_firmware_writeb(isp, isp_setting_ccm[i].value, isp_setting_ccm[i].reg);
			else if (isp_setting_ccm[i].reg & 0x60000)
				isp_reg_writeb(isp, isp_setting_ccm[i].value, isp_setting_ccm[i].reg);
		}
	} else {
		ISP_PRINT(ISP_ERROR, "%s[%d] don't have isp setting %d\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int isp_set_face_exposure_parameters(struct isp_device *isp)
{
	struct isp_sensor_setting* isp_sensor_setting;
	struct isp_reg_t* isp_setting_exposure = NULL;
	int i = 0;
	int exposure = isp->face_exposure;

	for (i = 0; i < ISP_SETTING_SIZES; i++) {
		isp_sensor_setting = &isp_sensor_settings[i];
		if (0x8856 == isp_sensor_setting->chip_id) {
			if (exposure == 0) {
				isp_setting_exposure = isp_sensor_setting->isp_setting_face_exposure;
			} else {
				isp_setting_exposure = isp_sensor_setting->isp_setting_normal_exposure;
			}
			break;
		}
	}

	if (isp_setting_exposure != NULL) {
		for (i = 0; isp_setting_exposure[i].reg != OVISP_REG_END; i++) {
			if (isp_setting_exposure[i].reg & 0x10000)
				isp_firmware_writeb(isp, isp_setting_exposure[i].value, isp_setting_exposure[i].reg);
			else if (isp_setting_exposure[i].reg & 0x60000)
				isp_reg_writeb(isp, isp_setting_exposure[i].value, isp_setting_exposure[i].reg);
		}
	} else {
		ISP_PRINT(ISP_ERROR, "%s[%d] don't have isp setting %d\n", __func__, __LINE__);
		return -1;
	}
#if 0
	for (i = 0; isp_setting_exposure[i].reg != OVISP_REG_END; i++)
		printk("%x:%ud\n",isp_setting_exposure[i].reg,isp_reg_readb(isp, isp_setting_exposure[i].reg));
#endif
	return 0;
}

static int isp_set_af_parameters(struct isp_device *isp)
{
	struct isp_parm *iparm = &isp->parm;

	/* set auto focus window size */
	unsigned int startX = 0, startY = 0, windowWidth = 0, windowHeight = 0;
	windowWidth = (iparm->input.idi_out_width/3) & ~0xf;
	windowHeight = (iparm->input.idi_out_height/3) & ~0xf;
	startX = windowWidth;
	startY = windowHeight;
	isp_reg_writeb(isp, (startX >> 8) & 0xff, 0x66304);
	isp_reg_writeb(isp, startX & 0xff, 0x66305);
	isp_reg_writeb(isp, (startY >> 8) & 0xff, 0x66306);
	isp_reg_writeb(isp, startY & 0xff, 0x66307);
	isp_reg_writeb(isp, (windowWidth >> 8) & 0xff, 0x66308);
	isp_reg_writeb(isp, windowWidth & 0xff, 0x66309);
	isp_reg_writeb(isp, (windowHeight >> 8) & 0xff, 0x6630a);
	isp_reg_writeb(isp, windowHeight & 0xff, 0x6630b);
	return 0;
}

static int isp_get_af_state(struct isp_device *isp, int *af_state)
{
	*af_state = isp_firmware_readb(isp, 0x1ed38);
	return 0;
}

static int isp_set_format(struct isp_device *isp, unsigned int videos)
{
	int ret = 0;
	ret = isp_fill_parm_registers(isp, isp->preview);
	if (ret) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "fill preview registers faild!\n");
		return -EINVAL;
	}
	if (!isp->bypass) {
		ret = isp_set_banding_parameters(isp, true);
		if (ret) {
			ISP_PRINT(ISP_ERROR, KERN_ERR "fill auto focus registers faild!\n");
			return -EINVAL;
		}
		ret = isp_set_af_parameters(isp);
		if (ret) {
			ISP_PRINT(ISP_ERROR, KERN_ERR "fill auto focus registers faild!\n");
			return -EINVAL;
		}
	}
	/* set sensor regs */
	ret = isp_fill_i2c_group(isp);
	if (ret) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "isp_cmd_i2c_group faild!\n");
		return -EINVAL;
	}
	if (isp->preview_mode == V4L2_CAP_LIVE_MODE) {
		isp_regs_set(isp, isp_set_live_mode_regs);
	}
	/* isp_i2c_fill_buffer(isp); */
	//dump_isp_configuration(isp);
	isp_reg_writeb(isp, ISP_CCLK_DIVIDER, COMMAND_REG4);
	if (isp->first_init) {
		isp_set_default_parameters(isp);
		isp_firmware_writeb(isp, 0x01, 0x1e022);
		isp_reg_writeb(isp, 0x01, 0x65320);
		isp_firmware_writeb(isp, 0x01, 0x1c07c);
		isp_reg_writeb(isp, 0x80, COMMAND_REG5);
	} else {
		isp_reg_writeb(isp, 0x00, COMMAND_REG5);
	}
	/* isp_firmware_writeb(isp, 0x01, 0x1e010); */
	isp_reg_writeb(isp, 0x00, COMMAND_REG6);
	if(videos == 1)
		isp_reg_writeb(isp, 0x01, COMMAND_REG7);
	else
		isp_reg_writeb(isp, 0x02, COMMAND_REG7);

#ifdef CONFIG_VIDEO_OV5640 
	isp_firmware_writeb(isp, 0x0c, 0x1f001);
	isp_reg_writeb(isp, 0x04, 0x63b35);
#endif

#ifdef CONFIG_BOARD_NETEASE
	isp_reg_writeb(isp, 0x04, 0x63b35);
#endif

	/* isp_reg_writeb(isp, 0x20, 0x65007); */

	/* dump_isp_configuration(isp); */

	if (isp_send_cmd(isp, CMD_SET_FORMAT, ISP_FORMAT_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait format set done!\n");
		return -EINVAL;
	}

	if (isp->first_init) {
		isp_set_default_parameters(isp);
		isp->first_init = false;
	}
#if 0
	{
		unsigned int save_reg = isp_reg_readb(isp, 0x63047);
		save_reg |= (0x1 << 2);
		isp_reg_writeb(isp, save_reg, 0x63047);
		ISP_PRINT(ISP_INFO, "---- IDI reset ----\n");
		ISP_PRINT(ISP_INFO, "pcnt_crop_16:			0x%02x%02x\n", isp_reg_readb(isp, 0x63c42), isp_reg_readb(isp, 0x63c43));
		ISP_PRINT(ISP_INFO, "lcnt_crop_16:			0x%02x%02x\n", isp_reg_readb(isp, 0x63c44), isp_reg_readb(isp, 0x63c45));
		save_reg = isp_reg_readb(isp, 0x63047);

		save_reg &= ~(0x1 << 2);
		isp_reg_writeb(isp, save_reg, 0x63047);
		mdelay(200);
		ISP_PRINT(ISP_INFO, "---- IDI reset ok ----\n");
		ISP_PRINT(ISP_INFO, "pcnt_crop_16:			0x%02x%02x\n", isp_reg_readb(isp, 0x63c42), isp_reg_readb(isp, 0x63c43));
		ISP_PRINT(ISP_INFO, "lcnt_crop_16:			0x%02x%02x\n", isp_reg_readb(isp, 0x63c44), isp_reg_readb(isp, 0x63c45));
	}
#endif
	/* isp_firmware_writeb(isp, 0x01, 0x1ec6a); */
	return 0;
}
static int isp_set_capture_raw(struct isp_device *isp)
{
	struct ovisp_camera_dev *camera_dev;
	int ret = 0;
	camera_dev = (struct ovisp_camera_dev *)(isp->data);
	ret = isp_fill_parm_registers(isp, isp->captureraw);
	if(ret){
		ISP_PRINT(ISP_ERROR, KERN_ERR "fill capture registers faild!\n");
		return -EINVAL;
	}
	/* dump_isp_configuration(isp); */
	/* set cmd register */
	isp_reg_writeb(isp, 0x01, COMMAND_REG1);
	/* Wait for command set successfully. */
	if (isp_send_cmd(isp, CMD_CAPTURE_RAW, ISP_CAPTURE_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait capture set done!\n");
		return -EINVAL;
	}
	isp->capture_raw_enable = true;
	return 0;
}
static int isp_set_capture(struct isp_device *isp)
{
	struct ovisp_camera_dev *camera_dev;
	int ret = 0;
	camera_dev = (struct ovisp_camera_dev *)(isp->data);
	ret = isp_fill_parm_registers(isp, isp->capture);
	if(ret){
		ISP_PRINT(ISP_ERROR, KERN_ERR "fill capture registers faild!\n");
		return -EINVAL;
	}
	/* isp_i2c_fill_buffer(isp); */

	/* Wait for command set successfully. */
	if (isp_send_cmd(isp, CMD_CAPTURE, ISP_CAPTURE_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait capture set done!\n");
		return -EINVAL;
	}

	return 0;
}

static int isp_get_zoom_ratio(struct isp_device *isp, int zoom)
{
	int zoom_ratio;

	switch (zoom) {
	case ZOOM_LEVEL_5:
		zoom_ratio = 0x350;
		break;
	case ZOOM_LEVEL_4:
		zoom_ratio = 0x300;
		break;
	case ZOOM_LEVEL_3:
		zoom_ratio = 0x250;
		break;
	case ZOOM_LEVEL_2:
		zoom_ratio = 0x200;
		break;
	case ZOOM_LEVEL_1:
		zoom_ratio = 0x140;
		break;
	case ZOOM_LEVEL_0:
	default:
		zoom_ratio = 0x100;
		break;
	}

	return zoom_ratio;
}

static int isp_set_zoom(struct isp_device *isp, int zoom)
{
	int zoom_ratio;

	zoom_ratio = isp_get_zoom_ratio(isp, zoom);

	//isp_reg_writeb(isp, 0x01, COMMAND_REG1);
	isp_reg_writeb(isp, zoom_ratio >> 8, COMMAND_REG2);
	isp_reg_writeb(isp, zoom_ratio & 0xFF, COMMAND_REG3);
	/* isp_reg_writeb(isp, zoom_ratio >> 8, COMMAND_REG4); */
	/* isp_reg_writeb(isp, zoom_ratio & 0xFF, COMMAND_REG5); */

	/* Wait for command set successfully. */
	if (isp_send_cmd(isp, CMD_ZOOM_IN_MODE, ISP_ZOOM_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait zoom set done!\n");
		return -EINVAL;
	}

	return 0;
}

static void fw_copy(unsigned int *dst,unsigned int *src, int cnt)
{
	int i = 0;

	for (i = 0; i < cnt; i++) {
		volatile unsigned char *dt = (volatile unsigned char *)dst;
		volatile unsigned char *st = (volatile unsigned char *)src;

		dt[3] = st[0];
		dt[2] = st[1];
		dt[1] = st[2];
		dt[0] = st[3];

		dst++;
		src++;
	}
}

static int isp_boot(struct isp_device *isp)
{
	unsigned char val;

	if (isp->boot)
		return 0;

	/* Mask all interrupts. */
	isp_intc_disable(isp, 0xffffffff);
	isp_mac_int_mask(isp, 0xffff);

	/* Reset ISP.  */
	isp_reg_writeb(isp, DO_SOFT_RST, REG_ISP_SOFT_RST);
	ISP_PRINT(ISP_INFO,"REG_ISP_SOFT_RST:%x\n", isp_reg_readb(isp, REG_ISP_SOFT_RST));

	/* Enable interrupt (only set_cmd_done interrupt). */
	isp_intc_enable(isp, MASK_INT_CMDSET);

	isp_reg_writeb(isp, DO_SOFTWARE_STAND_BY, REG_ISP_SOFT_STANDBY);

	ISP_PRINT(ISP_INFO,"REG_ISP_SOFT_STANDBY:%x\n", isp_reg_readb(isp, REG_ISP_SOFT_STANDBY));
	/* Enable the clk used by mcu. */
	isp_reg_writeb(isp, 0xf1, REG_ISP_CLK_USED_BY_MCU);
	ISP_PRINT(ISP_INFO,"REG_ISP_CLK_USED_BY_MCU:%x\n", isp_reg_readb(isp, REG_ISP_CLK_USED_BY_MCU));

	/* Download firmware to ram of mcu. */
#ifdef OVISP_DEBUGTOOL_ENABLE
	static load_firmware = 0;
	if(load_firmware == 0){
		fw_copy((unsigned int *)(isp->base + FIRMWARE_BASE), (unsigned int *)isp_firmware,
			ARRAY_SIZE(isp_firmware) / 4);
		load_firmware = 1;
	}else{
		ovisp_debugtool_load_firmware(OVISP_DEBUGTOOL_FIRMWARE_FILENAME, (u8*)(isp->base + FIRMWARE_BASE), (u8*)isp_firmware, ARRAY_SIZE(isp_firmware));
	}
#else
	fw_copy((unsigned int *)(isp->base + FIRMWARE_BASE), (unsigned int *)isp_firmware,
		ARRAY_SIZE(isp_firmware) / 4);
#endif
	/* MCU initialize. */
	isp_reg_writeb(isp, 0xf0, REG_ISP_CLK_USED_BY_MCU);

	ISP_PRINT(ISP_INFO,"REG_ISP_CLK_USED_BY_MCU:%x\n", isp_reg_readb(isp, REG_ISP_CLK_USED_BY_MCU));
	/* udelay(1000); */

	/* Wait for command set done interrupt. */
	if (isp_wait_cmd_done(isp, ISP_BOOT_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "MCU not respond when init ISP!\n");
		return -ETIME;
	}

	val = isp_reg_readb(isp, COMMAND_FINISHED);
	if (val != CMD_FIRMWARE_DOWNLOAD) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to download isp firmware (%02x)\n", val);
		return -EINVAL;
	}

	isp_reg_writeb(isp, DO_SOFTWARE_STAND_BY, REG_ISP_SOFT_STANDBY);
	isp_i2c_config(isp);


	ISP_PRINT(ISP_INFO,"versionh  %08x\n",isp_reg_readb(isp, 0x6303d));
	ISP_PRINT(ISP_INFO,"versionl  %08x\n",isp_reg_readb(isp, 0x6303e));

	return 0;
}

#ifdef CONFIG_BOARD_NETEASE
static void sgm31324_timer_expire(unsigned long data)
{
	enable_sgm31324(0);
}
#endif

static int isp_irq_notify(struct isp_device *isp, unsigned int status)
{
	int ret = 0;

	if (isp->irq_notify)
		ret = isp->irq_notify(status, isp->data);

	return ret;
}
//static unsigned long long start_time;
//static unsigned long long end_time;
#ifdef CONFIG_DVP_OV9712
static unsigned int soc_for_aec_test(struct isp_device* isp)
{
	unsigned int expo = 0;
	unsigned short gain = 0;
	void* data = isp->data;
	struct ovisp_camera_dev *camdev = (struct ovisp_camera_dev *)data;
	struct ovisp_camera_subdev *csd = &camdev->csd[camdev->input];
	switch(isp_reg_readb(isp, 0x63910)){
	case 0xf2:
		expo = (unsigned int)(isp_firmware_readb(isp,0x1c070)<<24) | (isp_firmware_readb(isp,0x1c071) <<16) | (isp_firmware_readb(isp,0x1c072) << 8) | isp_firmware_readb(isp,0x1c073);
		expo = expo >> 4;
		ov9712_write(csd->sd, 0x10, (unsigned char)(expo & 0xff));
		ov9712_write(csd->sd, 0x16, (unsigned char)((expo >> 8) & 0xff));
		break;
	case 0xf3:
		gain = (unsigned int)(isp_firmware_readb(isp,0x1c06e)<<8) | isp_firmware_readb(isp,0x1c06f);
		ov9712_write(csd->sd, 0x00, (unsigned char)(gain & 0xff));
		break;
	case 0xf1:
		expo = (unsigned int)(isp_firmware_readb(isp,0x1c070)<<24) | (isp_firmware_readb(isp,0x1c071) <<16) | (isp_firmware_readb(isp,0x1c072) << 8) | isp_firmware_readb(isp,0x1c073);
		expo = expo >> 4;
		gain = (unsigned int)(isp_firmware_readb(isp,0x1c06e)<<8) | isp_firmware_readb(isp,0x1c06f);
		ov9712_write(csd->sd, 0x10, (unsigned char)(expo & 0xff));
		ov9712_write(csd->sd, 0x16, (unsigned char)((expo >> 8) & 0xff));
		ov9712_write(csd->sd, 0x00, (unsigned char)(gain & 0xff));
		break;
	default:
		break;
	}
	return 0;
}


static struct reg_list{
	unsigned char reg;
	unsigned char val;
};
#endif
static irqreturn_t (*hs_isp_irq)(int this_irq, void *dev_id);
void register_hs_isp_irq(irqreturn_t (*handle_irq)(int ,void*))
{
    hs_isp_irq = handle_irq;
}

void unregister_hs_isp_irq(void)
{
	hs_isp_irq = NULL;
}

static irqreturn_t isp_irq(int this_irq, void *dev_id)
{
	struct isp_device *isp = dev_id;
	unsigned int irq_status;
	unsigned short mac_irq_status = 0;
	unsigned int notify = 0;
	unsigned char cmd;
#ifdef CONFIG_BOARD_NETEASE
	unsigned int cur_level = 0;
	unsigned int tar_level = 0;
#endif

	//start_time = sched_clock();
	if (hs_isp_irq)
		return hs_isp_irq(this_irq, dev_id);

	notify |= ISP_NOTIFY_UPDATE_BUF;
	irq_status = isp_intc_state(isp);
	mac_irq_status = isp_mac_int_state(isp);
	cmd = isp_reg_readb(isp, COMMAND_REG0);
	/* Command set done interrupt. */
	if (irq_status & MASK_INT_CMDSET) {
		complete(&isp->completion);
		ISP_PRINT(ISP_INFO,"command[0x%02x] done !!\n", cmd);
		if (!(cmd == 0x02 || cmd == 0x14 || (cmd == 0x05 && isp_reg_readb(isp, COMMAND_REG1)))) {
			notify &= ~ISP_NOTIFY_UPDATE_BUF;
		}
	}
	/* when 04 06 complete, cmd done irq come, but it's too late, so we must do it earlier */
	if (cmd == 0x04 || cmd == 0x06 || isp->is_capturing) {
		notify &= ~ISP_NOTIFY_UPDATE_BUF;
	}
	/* if(irq_status & MASK_ISP_INT_EOF && cmd == 0x02){ */
	/*     //	    printk(KERN_DEBUG"idi count = 0x%02x%02x\n\n",isp_reg_readb(isp, 0x63c44),isp_reg_readb(isp, 0x63c45)); */
	/*     if(isp_reg_readb(isp, 0x63c44) != isp_firmware_readb(isp, ISP_IDI_OUTPUT_HEIGHT)  */
	/*        || isp_reg_readb(isp, 0x63c45) != isp_firmware_readb(isp, ISP_IDI_OUTPUT_HEIGHT + 1))isp->frame_need_drop = true; */
	/* } */
	if (isp->wait_eof && (irq_status & MASK_ISP_INT_EOF)) {
		complete(&isp->frame_eof);
		isp->wait_eof = false;
		ISP_PRINT(ISP_INFO,"[0x%02x] frame_eof[0x%02x]!!\n", cmd, isp_reg_readb(isp, REG_ISP_INT_EN_C1));
	}
	if (irq_status & MASK_INT_MAC) {
		/* Drop. */
		if (mac_irq_status & (MASK_INT_DROP0 | MASK_INT_DROP1)) {
			if (mac_irq_status & MASK_INT_DROP0) {
				notify |= ISP_NOTIFY_DROP_FRAME | ISP_NOTIFY_DROP_FRAME0;
				ISP_PRINT(ISP_INFO,"[0x%02x] drop 0 !!\n", cmd);
			} else {
				notify |= ISP_NOTIFY_DROP_FRAME | ISP_NOTIFY_DROP_FRAME1;
				ISP_PRINT(ISP_INFO,"[0x%02x] drop 1 !!\n", cmd);
			}
			if (isp->is_capturing) {
				complete(&isp->capture_start);
			}
		}
		/* Done. */
		if (mac_irq_status & (MASK_INT_WRITE_DONE0 | MASK_INT_WRITE_DONE1)) {
			if (mac_irq_status & MASK_INT_WRITE_DONE0) {
				if (isp->frame_need_drop > 0) {
					notify |= ISP_NOTIFY_OVERFLOW | ISP_NOTIFY_DROP_FRAME0;
					isp->frame_need_drop--;
				} else
					notify |= ISP_NOTIFY_DATA_DONE | ISP_NOTIFY_DATA_DONE0;
				ISP_PRINT(ISP_INFO,"[0x%02x] done - 0!\n", cmd);
			} else {
				if (isp->frame_need_drop > 0) {
					notify |= ISP_NOTIFY_OVERFLOW | ISP_NOTIFY_DROP_FRAME1;
					isp->frame_need_drop--;
				} else
					notify |= ISP_NOTIFY_DATA_DONE | ISP_NOTIFY_DATA_DONE1;
				ISP_PRINT(ISP_INFO,"[0x%02x] done - 1!\n", cmd);
			}
			isp->done_frames++;
			if (isp->done_frames == 1) {
				isp_firmware_writeb(isp, 0x00, 0x1e022);
				isp_reg_writeb(isp, 0x00, 0x65320);
			}
			if (isp->is_capturing) {
				complete(&isp->capture_start);
			}
		}
		/* FIFO overflow */
		if (mac_irq_status & (MASK_INT_OVERFLOW0 | MASK_INT_OVERFLOW1)) {
			if (mac_irq_status & MASK_INT_OVERFLOW0) {
				ISP_PRINT(ISP_WARNING,"[0x%02x] overflow-0\n",cmd);
				notify |= ISP_NOTIFY_OVERFLOW | ISP_NOTIFY_DROP_FRAME0;
			} else {
				ISP_PRINT(ISP_WARNING,"[0x%02x] overflow-1\n",cmd);
				notify |= ISP_NOTIFY_OVERFLOW | ISP_NOTIFY_DROP_FRAME1;
			}
			if (isp->is_capturing) {
				complete(&isp->capture_start);
			}
		}
		/* Start */
		if (mac_irq_status & (MASK_INT_WRITE_START0 | MASK_INT_WRITE_START1)) {
#ifdef CONFIG_BOARD_NETEASE
			/*sgm31324 led control exposure*/
			enable_sgm31324(1);
			cur_level = (unsigned int)isp_firmware_readb(isp, 0x1c056);
			tar_level = (unsigned int)isp_firmware_readb(isp, 0x1e014);
			change_sgm31324_current(cur_level, tar_level);
			mod_timer(&led_timer, jiffies + (HZ/200));
#endif
			
			if (mac_irq_status & MASK_INT_WRITE_START0) {
				ISP_PRINT(ISP_INFO,"[0x%02x] start 0\n", cmd);
				notify |= ISP_NOTIFY_DATA_START | ISP_NOTIFY_DATA_START0;
			} else {
				ISP_PRINT(ISP_INFO,"[0x%02x] start 1\n", cmd);
				notify |= ISP_NOTIFY_DATA_START | ISP_NOTIFY_DATA_START1;
			}
		}
	}
	isp_irq_notify(isp, notify);
	return IRQ_HANDLED;
}

static int isp_regs_set(struct isp_device *isp, struct isp_reg_t* isp_setting)
{
	int i;
	for (i = 0; isp_setting[i].reg != OVISP_REG_END; i++) {
		if( isp_setting[i].reg & 0x10000 )
			isp_firmware_writeb(isp, isp_setting[i].value, isp_setting[i].reg);
		else if( isp_setting[i].reg & 0x60000 )
			isp_reg_writeb(isp, isp_setting[i].value, isp_setting[i].reg);
	}
	return 0;
}

static int isp_mipi_init(struct isp_device *isp)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(isp_mipi_regs_init); i++) {
		isp_reg_writeb(isp, isp_mipi_regs_init[i].value,
				isp_mipi_regs_init[i].reg);
	}
	/*Necessary delay, at least 2ms. for aw6120 isp.*/
	/* mdelay(10); */
	return 0;
}

static int isp_dvp_init(struct isp_device *isp)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(isp_dvp_regs_init); i++) {
		isp_reg_writeb(isp, isp_dvp_regs_init[i].value,
				isp_dvp_regs_init[i].reg);
	}

	return 0;
}

static int isp_int_init(struct isp_device *isp)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&isp->lock, flags);

	isp->intr.intc = 0;
	isp->mac_intr.intc = 0;
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C3);
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C2);
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C1);
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C0);
	isp_reg_writeb(isp, 0x00, REG_ISP_MAC_INT_EN_L);
	isp_reg_writeb(isp, 0x00, REG_ISP_MAC_INT_EN_H);

	spin_unlock_irqrestore(&isp->lock, flags);

	ret = request_irq(isp->irq, isp_irq, IRQF_SHARED,
			"isp", isp);

	return ret;
}

static int isp_i2c_init(struct isp_device *isp)
{
	int ret;
	if (isp->pdata->flags & CAMERA_USE_ISP_I2C) {
		ISP_PRINT(ISP_INFO,"CAMERA USE ISP I2C, NOT SOC I2C\n");
		isp->i2c.xfer_cmd = isp_i2c_xfer_cmd_sccb;
		ret = isp_i2c_register(isp);
		if (ret)
			return ret;
	} else {
		ISP_PRINT(ISP_INFO,"CAMERA USE SOC I2C\n");
	}
	return 0;
}

static int isp_i2c_release(struct isp_device *isp)
{
	if (isp->pdata->flags & CAMERA_USE_ISP_I2C) {
		isp_i2c_unregister(isp);
	} else {
		kfree(isp->i2c.pdata);
	}

	return 0;
}

static int isp_mfp_init(struct isp_device *isp)
{
	return 0;
}

static int isp_clk_init(struct isp_device *isp)
{
	int i;
	int ret;

	for (i = 0; i < ISP_CLK_NUM; i++) {
		isp->clk[i] = clk_get(isp->dev, isp_clks[i].name);
		if (IS_ERR(isp->clk[i])) {
			ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to get %s clock %ld\n",
					isp_clks[i].name, PTR_ERR(isp->clk[i]));
			return PTR_ERR(isp->clk[i]);
		}
		if(isp_clks[i].rate != DUMMY_CLOCK_RATE) {
			ret = clk_set_rate(isp->clk[i], isp_clks[i].rate);
			if(ret){
				ISP_PRINT(ISP_ERROR, KERN_ERR "set rate failed ! XXXXXXXXXXXXXXXXXx \n");
			}
		}

		isp->clk_enable[i] = 0;
	}

	return 0;
}

static int isp_clk_release(struct isp_device *isp)
{
	int i;

	for (i = 0; i < ISP_CLK_NUM; i++)
		clk_put(isp->clk[i]);

	return 0;
}

static int isp_clk_enable(struct isp_device *isp, unsigned int type)
{
	int i;

	for (i = 0; i < ISP_CLK_NUM; i++) {
		if (!isp->clk_enable[i] && (isp_clks[i].flags & type)) {
			clk_enable(isp->clk[i]);
			isp->clk_enable[i] = 1;
		}
	}

	return 0;
}

static int isp_clk_disable(struct isp_device *isp, unsigned int type)
{
	int i;

	for (i = 0; i < ISP_CLK_NUM; i++) {
		if (isp->clk_enable[i] && (isp_clks[i].flags & type)) {
			clk_disable(isp->clk[i]);
			isp->clk_enable[i] = 0;
		}
	}

	return 0;
}


static int isp_powerdown(struct isp_device * isp)
{
	isp_clk_disable(isp, ISP_PWC_GATE_ISP);
	return 0;
}

static int isp_powerup(struct isp_device * isp)
{
	isp_clk_enable(isp, ISP_PWC_GATE_ISP);
	return 0;
}

static int isp_normal_capture( struct isp_device * isp, struct isp_input_parm *input,
	struct isp_output_parm *output, unsigned int out_addr, unsigned int vts)
{
	struct ovisp_camera_client *client = isp->client;
	unsigned int addrw, regw;
	/* isp_reg_writeb(isp, 0x00, 0x63b35); */

	/* isp_firmware_writeb(isp, 0x01, 0x1fff9); */
	/*input type, bypass */
	if (client->flags & CAMERA_CLIENT_IF_MIPI)
		input->format |= SENSOR_PRIMARY_MIPI;
	isp_firmware_writeb(isp, (input->format >> 8) & 0xff, ISP_INPUT_FORMAT);
	isp_firmware_writeb(isp, input->format & 0xff, ISP_INPUT_FORMAT + 1);
	isp_firmware_writeb(isp, (input->width >> 8) & 0xff,
			SENSOR_OUTPUT_WIDTH);
	isp_firmware_writeb(isp, input->width & 0xff,
			SENSOR_OUTPUT_WIDTH + 1);

	isp_firmware_writeb(isp, (input->height >> 8) & 0xff,
			SENSOR_OUTPUT_HEIGHT);
	isp_firmware_writeb(isp, input->height & 0xff,
			SENSOR_OUTPUT_HEIGHT + 1);
	isp_firmware_writeb(isp, 0x00, 0x1f006);
	isp_firmware_writeb(isp, 0x00, 0x1f007);
	isp_firmware_writeb(isp, (input->width >> 8) & 0xff,
			ISP_IDI_OUTPUT_WIDTH);
	isp_firmware_writeb(isp, input->width & 0xff,
			ISP_IDI_OUTPUT_WIDTH + 1);
	isp_firmware_writeb(isp, (input->height >> 8) & 0xff,
			ISP_IDI_OUTPUT_HEIGHT);
	isp_firmware_writeb(isp, input->height & 0xff,
			ISP_IDI_OUTPUT_HEIGHT + 1);

	isp_firmware_writeb(isp, 0x00, ISP_IDI_OUTPUT_H_START);
	isp_firmware_writeb(isp, 0x00, ISP_IDI_OUTPUT_H_START + 1);
	isp_firmware_writeb(isp, 0x00, ISP_IDI_OUTPUT_V_START);
	isp_firmware_writeb(isp, 0x00, ISP_IDI_OUTPUT_V_START + 1);
	/*output config*/
	isp_firmware_writeb(isp, (output->format >> 8) & 0xff, 0x1f022);
	isp_firmware_writeb(isp, (output->format & 0xff), 0x1f023);
	/* iamge output */
	isp_firmware_writeb(isp, (output->width >> 8) & 0xff, 0x1f024);
	isp_firmware_writeb(isp, output->width & 0xff, 0x1f025);
	isp_firmware_writeb(isp, (output->height >> 8) & 0xff, 0x1f026);
	isp_firmware_writeb(isp, output->height & 0xff, 0x1f027);/*w:h*/

	/* memory width */
	isp_firmware_writeb(isp, (output->width >> 8) & 0xff, 0x1f028);
	isp_firmware_writeb(isp, output->width & 0xff, 0x1f029);/*m_y*/
	isp_firmware_writeb(isp, (output->width >> 8) & 0xff, 0x1f02a);
	isp_firmware_writeb(isp, output->width & 0xff, 0x1f02b);/*m_uv*/

	/* 3. ISP CONFIGURATION */
	//isp config
#ifdef CONFIG_BOARD_M200S_ZBRZFY
	isp_firmware_writeb(isp, 0x00, ISP_EXPOSURE_RATIO);
	isp_firmware_writeb(isp, 0xd0, ISP_EXPOSURE_RATIO + 1);
#else
	isp_firmware_writeb(isp, 0x01, ISP_EXPOSURE_RATIO);
	isp_firmware_writeb(isp, 0x00, ISP_EXPOSURE_RATIO + 1);
#endif
	isp_firmware_writeb(isp, ((vts - 0x10) >> 8) & 0xff,
						ISP_MAX_EXPOSURE);
	isp_firmware_writeb(isp, (vts - 0x10) & 0xff,
						ISP_MAX_EXPOSURE + 1);

	isp_firmware_writeb(isp, 0x00, ISP_MIN_EXPOSURE);
	isp_firmware_writeb(isp, 0x10, ISP_MIN_EXPOSURE + 1);
	isp_firmware_writeb(isp, 0x00, ISP_MAX_GAIN);
	isp_firmware_writeb(isp, 0xff, ISP_MAX_GAIN + 1);

	isp_firmware_writeb(isp, 0x00, ISP_MIN_GAIN);
	isp_firmware_writeb(isp, 0x10, ISP_MIN_GAIN + 1);
	isp_firmware_writeb(isp, (vts >> 8) & 0xff, ISP_VTS);
	isp_firmware_writeb(isp, vts & 0xff, ISP_VTS + 1);
	/*zoom in*/
	isp_firmware_writeb(isp, 0x01, 0x1f084);
	isp_firmware_writeb(isp, 0x00, 0x1f085);

	/*address config*/
	/*output address*/
	switch(output->addrnums){
		case 3:
			addrw = out_addr + output->addroff[2];
			regw = 0x1f0a8;
			isp_firmware_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_firmware_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_firmware_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_firmware_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
		case 2:
			addrw = out_addr + output->addroff[1];
			regw = 0x1f0a4;
			isp_firmware_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_firmware_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_firmware_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_firmware_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
		case 1:
			addrw = out_addr;
			regw = 0x1f0a0;
			isp_firmware_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_firmware_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_firmware_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_firmware_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
			break;
		default:
			ISP_PRINT(ISP_ERROR, "%s[%d] addrnums is wrong; it should be 1 ~ 3,but it is %d",
					__func__,__LINE__,output->addrnums);
			break;
	}
	/* isp_firmware_writeb(isp, 0x06, 0x1fff9); */

	/* dump_isp_configuration(isp); */
	/* isp_reg_writeb(isp, 0x00, COMMAND_REG1); */
	/* isp_reg_writeb(isp, 0x00, COMMAND_REG2); */
	/* isp_reg_writeb(isp, 0x00, COMMAND_REG3); */
	isp_reg_writeb(isp, 0x01, COMMAND_REG7);
	isp_reg_writeb(isp, 0x00, COMMAND_REG5);
#if defined CAPTURE_INITAL_MODE
	isp_reg_writeb(isp, 0x80, COMMAND_REG5);
#endif
	/* Wait for command set successfully. */
	/* isp_firmware_writeb(isp, 0x00, 0x1e010); */
	if (isp_send_cmd(isp, 0x04, ISP_FORMAT_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait normal capture set done!\n");
		return -EINVAL;
	}

	return 0;
}
static int isp_set_format_stop(struct isp_device * isp)
{
//	if(isp->capture_raw_enable){
		isp_reg_writeb(isp, 0x00, COMMAND_REG7);
		if (isp_send_cmd(isp, 0x02, ISP_FORMAT_TIMEOUT)) {
			ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait capture raw done!\n");
			return -EINVAL;
		}
//		isp->capture_raw_enable = false;
//	}
	return 0;
}
static int isp_capture_raw_stop(struct isp_device * isp)
{
	if(isp->capture_raw_enable){
		isp_reg_writeb(isp, 0x00, COMMAND_REG1);
		if (isp_send_cmd(isp, 0x05, ISP_FORMAT_TIMEOUT)) {
			ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait capture raw done!\n");
			return -EINVAL;
		}
		isp->capture_raw_enable = false;
	}
	return 0;
}
static int isp_offline_process(struct isp_device * isp, struct isp_input_parm *input,
			       struct isp_output_parm *output, unsigned int in_addr, 
			       unsigned int out_addr, unsigned char *info, int zoom)
{
	unsigned int addrw, regw, i = 0;
	int zoom_ratio;
	/* printk("in_addr = %08x, out_addr = %08x\n",in_addr, out_addr); */
	/* isp_firmware_writeb(isp, 0x01, 0x1fff9); */
	/*input type, from memory*/
	input->format |= (ISP_PROCESS | SENSOR_MEMORY);/*isp processing*/
	isp_firmware_writeb(isp, (input->format >> 8) & 0xff, ISP_INPUT_FORMAT);
	isp_firmware_writeb(isp, input->format & 0xff, ISP_INPUT_FORMAT + 1);
	isp_firmware_writeb(isp, (input->width >> 8) & 0xff,
			SENSOR_OUTPUT_WIDTH);
	isp_firmware_writeb(isp, input->width & 0xff,
			SENSOR_OUTPUT_WIDTH + 1);

	isp_firmware_writeb(isp, (input->height >> 8) & 0xff,
			SENSOR_OUTPUT_HEIGHT);
	isp_firmware_writeb(isp, input->height & 0xff,
			SENSOR_OUTPUT_HEIGHT + 1);

	/*output config*/
	isp_firmware_writeb(isp, (output->format >> 8) & 0xff, 0x1f022);
	isp_firmware_writeb(isp, (output->format & 0xff), 0x1f023);
	/* iamge output */
	/* 640*480 */
	isp_firmware_writeb(isp, (output->width >> 8) & 0xff, 0x1f024);
	isp_firmware_writeb(isp, output->width & 0xff, 0x1f025);
	isp_firmware_writeb(isp, (output->height >> 8) & 0xff, 0x1f026);
	isp_firmware_writeb(isp, output->height & 0xff, 0x1f027);/*w:h*/

	/* memory width */
	isp_firmware_writeb(isp, (output->width >> 8) & 0xff, 0x1f028);
	isp_firmware_writeb(isp, output->width & 0xff, 0x1f029);/*m_y*/
	/* isp_firmware_writeb(isp, (0x640 >> 8) & 0xff, 0x1f02a); */
	/* isp_firmware_writeb(isp, 0x640 & 0xff, 0x1f02b);/\*m_uv*\/ */
	isp_firmware_writeb(isp, (output->width >> 8) & 0xff, 0x1f02a);
	isp_firmware_writeb(isp, output->width & 0xff, 0x1f02b);/*m_uv*/
	/*zoom in*/
	zoom_ratio = isp_get_zoom_ratio(isp, zoom);
	/* zoom_ratio = 0x400; */
	isp_firmware_writeb(isp, zoom_ratio >> 8, 0x1f084);
	isp_firmware_writeb(isp, zoom_ratio & 0xFF, 0x1f085);

	/*address config*/
	/*output address*/
	switch(output->addrnums){
		case 3:
			addrw = out_addr + output->addroff[2];
			regw = 0x1f0a8;
			isp_firmware_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_firmware_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_firmware_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_firmware_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
		case 2:
			addrw = out_addr + output->addroff[1];
			regw = 0x1f0a4;
			isp_firmware_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_firmware_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_firmware_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_firmware_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
		case 1:
			addrw = out_addr;
			regw = 0x1f0a0;
			isp_firmware_writeb(isp, (addrw >> 24) & 0xff, regw);
			isp_firmware_writeb(isp, (addrw >> 16) & 0xff, regw + 1);
			isp_firmware_writeb(isp, (addrw >> 8) & 0xff, regw + 2);
			isp_firmware_writeb(isp, (addrw >> 0) & 0xff, regw + 3);
			break;
		default:
			ISP_PRINT(ISP_ERROR, "%s[%d] addrnums is wrong; it should be 1 ~ 3,but it is %d",
					__func__,__LINE__,output->addrnums);
			break;
	}
	/*input address*/
	isp_firmware_writeb(isp, (in_addr >> 24) & 0xff, 0x1f0c4);
	isp_firmware_writeb(isp, (in_addr >> 16) & 0xff, 0x1f0c5);
	isp_firmware_writeb(isp, (in_addr >> 8) & 0xff, 0x1f0c6);
	isp_firmware_writeb(isp, (in_addr >> 0) & 0xff, 0x1f0c7);

	for(i = 0; i < ISP_OUTPUT_INFO_LENS; i++)
		isp_firmware_writeb(isp, info[i], 0x1f300 + i);

	/* isp_firmware_writeb(isp, 0x06, 0x1fff9); */

//	for(i = 0; i < ISP_OUTPUT_INFO_LENS; i++)
//		printk("0x%x [0x%02x]\n", 0x1f300 + i, isp_reg_readb(isp, 0x1f300 + i));
	/* dump_isp_configuration(isp); */

	/* isp_i2c_fill_buffer(isp); */
	isp_reg_writeb(isp, 0x01, COMMAND_REG2);
	/* Wait for command set successfully. */
	/* isp_reg_writeb(isp, 0x04, 0x63b35); */
	if (isp_send_cmd(isp, 0x06, ISP_FORMAT_TIMEOUT)) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Failed to wait process raw set done!\n");
		return -EINVAL;
	}

	/* dump_isp_offline_process(isp); */
	/* dump_mac(isp); */
	/* __dump_isp_regs(isp, 0x63b00, 0x63b80); */
	return 0;
}
/*
* isp ops
*/
static int isp_init(struct isp_device *isp, void *data)
{
	isp->boot = 0;
	isp->poweron = 1;
	isp->snapshot = 0;
	isp->bypass = false;
	isp->running = 0;
	isp->format_active = 0;
	isp->bracket_end = 0;
	isp->is_capturing = false;
	memset(&isp->parm, 0, sizeof(isp->parm));
	isp->parm.out_videos = 1; // its default value is 1.
	isp->parm.c_video = 0; // the first video.
	isp->light = LIGHT_OFF;
	isp->face_exposure = -1;

	isp_mfp_init(isp);

	return 0;
}

static int isp_release(struct isp_device *isp, void *data)
{
	isp->boot = 0;
	isp->poweron = 0;
	isp->snapshot = 0;
	isp->bypass = false;
	isp->running = 0;
	isp->format_active = 0;
	isp->light = LIGHT_OFF;

	return 0;
}

static int isp_open(struct isp_device *isp, struct isp_prop *prop)
{
	struct ovisp_camera_client *client = &isp->pdata->client[prop->index];
	unsigned long flags;
	int ret = 0;

	isp_clk_enable(isp, ISP_CLK_CGU_ISP | ISP_CLK_GATE_ISP);

	if (!isp->poweron)
		return -ENODEV;
	isp_powerup(isp);

#ifdef CONFIG_BOARD_NETEASE
	init_timer(&led_timer);
	led_timer.function = sgm31324_timer_expire;
	led_timer.data = 0;
#endif

	/*check if clk ok!*/
	ISP_PRINT(ISP_INFO,"cpm cpccr:%x\n", *((volatile unsigned int *)0xB0000000));
	ISP_PRINT(ISP_INFO,"isp cpm regs:%08x\n", *((volatile unsigned int *)0xB0000080));

	if (!isp->boot) {
		ret = isp_boot(isp);
		if (ret)
			return ret;
		isp->boot = 1;
	}

	if (client->flags & CAMERA_CLIENT_IF_MIPI) {
		isp_clk_enable(isp, ISP_CLK_GATE_CSI);
		ret = isp_mipi_init(isp);
	} else if (client->flags & CAMERA_CLIENT_IF_DVP) {
		ret = isp_dvp_init(isp);
	}else{
		ISP_PRINT(ISP_ERROR,"we cann't support the sensor because its interface neither DVP nor MIPI!\n");
		ret = -1;
	}

	isp->debug.status = 1;
	isp->input = prop->index;
//	isp->bypass = prop->bypass;
	isp->snapshot = 0;
	isp->format_active = 0;
	memset(&isp->fmt_data, 0, sizeof(isp->fmt_data));
	/* init the parameters of isp */
	memset(&(isp->parm), 0 , sizeof(struct isp_parm));
	isp->parm.out_videos = 1; // its default value is 1.
	isp->parm.c_video = 0; // the first video.

	isp->first_init = true;
	isp->done_frames = 0;
	isp->frame_need_drop = 0;
	spin_lock_irqsave(&isp->lock, flags);
	isp->wait_eof = false;
	isp->capture_raw_enable = false;
	spin_unlock_irqrestore(&isp->lock, flags);

	//isp_dump_firmware(isp);
	//isp_dump_configuration(isp);

	memset(&isp->debug, 0, sizeof(isp->debug));
	return ret;
}

static int isp_close(struct isp_device *isp, struct isp_prop *prop)
{
	struct ovisp_camera_client *client = &isp->pdata->client[prop->index];

	/*clk should be disabled here, but error happens, add by pzqi*/
	/* wait for mac wirte ram finish */
	msleep(80);

#ifdef CONFIG_BOARD_NETEASE
	del_timer(&led_timer);
#endif

	if (!isp->poweron)
		return -ENODEV;
	if (client->flags & CAMERA_CLIENT_IF_MIPI) {
		csi_phy_stop(prop->index);
		isp_clk_disable(isp, ISP_CLK_GATE_CSI);
	}
	/*disable interrupt*/
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C3);
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C2);
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C1);
	isp_reg_writeb(isp, 0x00, REG_ISP_INT_EN_C0);
	isp_reg_writeb(isp, 0x00, REG_ISP_MAC_INT_EN_H);
	isp_reg_writeb(isp, 0x00, REG_ISP_MAC_INT_EN_L);

	isp_powerdown(isp); //isp should first power down

	isp_clk_disable(isp, ISP_CLK_GATE_ISP);

	/*for fpga test*/
	isp_clk_disable(isp, ISP_CLK_CGU_ISP);

	/*release the subdev index*/
	isp->input = -1;

	return 0;
}

static int isp_config(struct isp_device *isp, void *data)
{
	return 0;
}

static int isp_suspend(struct isp_device *isp, void *data)
{
	return 0;
}

static int isp_resume(struct isp_device *isp, void *data)
{
	return 0;
}

static int isp_mclk_on(struct isp_device *isp, int index)
{
	/*ovisp_mfp_config(MFP_PIN_GPIO72, MFP_GPIO72_ISP_SCLK0);*/
	//isp_clk_enable(isp, ISP_CLK_DEV);
	//isp_clk_enable(isp, ISP_CLK_ALL);

	return 0;
}

static int isp_mclk_off(struct isp_device *isp, int index)
{
//	isp_clk_disable(isp, ISP_CLK_GATE_ISP);
	/*ovisp_mfp_config(MFP_PIN_GPIO72, MFP_PIN_MODE_GPIO);*/

	return 0;
}

static int isp_set_parameters(struct isp_device *isp)
{
	struct isp_parm *parm = &isp->parm;
	if(parm->out_videos == 2 && (!isp->bypass)
			&& ovisp_output_fmt_is_raw(parm->output[1].format))
	{
		isp_set_preview_parameters(isp, 1);
		isp_set_capture_raw_parameters(isp, 1);
	}else if(isp->snapshot){
		isp_set_capture_parameters(isp, 1);
	}else
		isp_set_preview_parameters(isp, parm->out_videos);
	return 0;
}

static int isp_send_start_capture_cmd(struct isp_device *isp)
{
	int ret = 0;
	struct isp_parm *parm = &isp->parm;
	if(parm->out_videos == 2 && (!isp->bypass)
			&& ovisp_output_fmt_is_raw(parm->output[1].format))
	{
		ret = isp_set_format(isp, 1);
		if (ret)
			return ret;
		ret = isp_set_capture_raw(isp);
	} else {
		ret = isp_set_format(isp, parm->out_videos);
	}
	return ret;
}

static int isp_start_capture(struct isp_device *isp, struct isp_capture *cap)
{
	int ret = 0;
	struct ovisp_camera_dev *camera_dev;
	camera_dev = (struct ovisp_camera_dev *)(isp->data);
	isp->snapshot = cap->snapshot;
	isp->client = cap->client;
	isp->sensor_info = cap->si;
	if (isp->format_active) {
		isp_set_parameters(isp);
		if (!isp->snapshot) {
			ret = isp_setting_init(isp);
			if (ret) {
				printk("isp setting init error !!! \n");
				return ret;
			}
			ret = isp_send_start_capture_cmd(isp);
		} else {
			ret = isp_set_capture(isp);
		}
		if (ret)
			return ret;
	}
	if (isp->bypass && isp->snapshot && !isp->format_active) {
		ISP_PRINT(ISP_INFO,"--%s: %d\n", __func__, __LINE__);
		isp_set_parameters(isp);
		ret = isp_set_capture(isp);
	}
//#define OVISP_CSI_TEST
#ifdef OVISP_CSI_TEST
	printk("csi sensor test ! \n");
	while(1) {
		check_csi_error();
		//mdelay(10);
	}
#endif
	/*mask all interrupt*/
#if 1
	isp_intc_disable(isp, 0xffffffff);
	isp_mac_int_mask(isp, 0xffff);
	/*read to clear interrupt*/
	isp_intc_state(isp);
	isp_mac_int_state(isp);

	isp_intc_enable(isp, MASK_INT_MAC);
	isp_intc_enable(isp, MASK_INT_CMDSET);
	isp_intc_enable(isp, MASK_ISP_INT_EOF);
	// isp_intc_enable(isp, MASK_ISP_INT_SOF);
	isp_mac_int_unmask(isp,
			MASK_INT_WRITE_DONE0 | MASK_INT_WRITE_DONE1 |
			MASK_INT_OVERFLOW0 | MASK_INT_OVERFLOW1 |
			MASK_INT_DROP0 | MASK_INT_DROP1 |
			MASK_INT_WRITE_START0 | MASK_INT_WRITE_START1
			);
#endif
	ISP_PRINT(ISP_INFO, "now start capturing , waiting for interrupt\n");
	isp->running = 1;
	/* call the update buffer in drop int */

	isp->hdr_mode = 0;
	isp->bracket_end = 0;
	isp->debug.status = isp->snapshot;
	//__dump_isp_regs(isp, 0x63022, 0x63047);
//	dump_isp_syscontrol(isp);
//	dump_isp_top_register(isp);

	return 0;
}

static int isp_stop_capture(struct isp_device *isp, void *data)
{
	isp->running = 0;
	isp_intc_disable(isp, 0xffffffff);
	isp_mac_int_mask(isp, 0xffff);
#if defined(CONFIG_VIDEO_OV8858)
	isp_intc_enable(isp, MASK_INT_CMDSET);
#endif
	isp_reg_writeb(isp, 0x00, REG_BASE_ADDR_READY);
	isp_intc_state(isp);
	isp_mac_int_state(isp);
	return 0;
}

static int isp_update_buffer(struct isp_device *isp, struct isp_buffer *buf, int index)
{
	if(buf->addr == 0)
		return -1;
	if(index == 0){
		isp_set_address(isp, 0, buf->addr);
		isp_reg_writeb(isp, 0x01, REG_BASE_ADDR_READY);
	}else{
		isp_set_address(isp, 1, buf->addr);
		isp_reg_writeb(isp, 0x02, REG_BASE_ADDR_READY);
	}

	return 0;
}

static int isp_check_output_fmt(struct isp_device *isp, struct ovisp_video_format *vfmt)
{
	int ret = 0;
	if(isp->parm.c_video == 0){
		/* first video */
		switch (vfmt->fourcc) {
			case V4L2_PIX_FMT_YUYV:
			case V4L2_PIX_FMT_NV12:
			case V4L2_PIX_FMT_YUV420:
				if(vfmt->width > 1792)
					ret = -EINVAL;
				break;
			case V4L2_PIX_FMT_SBGGR8:
			case V4L2_PIX_FMT_SBGGR10:
				break;
			default:
				ret = -EINVAL;
		}
	}else if(isp->parm.c_video == 1){
		/* second video */
		switch (vfmt->fourcc) {
			case V4L2_PIX_FMT_YUYV:
				if(vfmt->width > 1792 || isp->bypass)
					ret = -EINVAL;
				break;
			case V4L2_PIX_FMT_SBGGR8:
			case V4L2_PIX_FMT_SBGGR10:
				break;
			default:
				ret = -EINVAL;
		}
	}else{
		ret = -EINVAL;
	}
	if(ret)
		ISP_PRINT(ISP_ERROR, "isp check fmt,video[%d] fourcc:%d pix->width:%d\n",
						isp->parm.c_video, vfmt->fourcc, vfmt->width);
	return ret;
}
static int isp_check_input_format(struct isp_device *isp, struct ovisp_video_format *vfmt)
{
	switch(vfmt->dev_fourcc){
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
		case V4L2_PIX_FMT_YUYV:
			break;
		default:
			return -EINVAL;
	}
	return 0;
}
static int isp_set_output_parm(struct isp_device *isp, struct ovisp_video_format *vfmt, struct isp_output_parm *output)
{
	int ret = 0;
	if (ret)
		return ret;
	switch (vfmt->fourcc) {
		case V4L2_PIX_FMT_YUYV:
			output->format = OFORMAT_YUV422;
			output->depth = 16;
			output->addrnums = 1;
			output->addroff[0] = 0;
			break;
		case V4L2_PIX_FMT_NV12:
			output->format = OFORMAT_NV12;
			output->depth = 12;
			output->addrnums = 2;
			output->addroff[0] = 0;
			output->addroff[1] = vfmt->width * vfmt->height;
			break;
		case V4L2_PIX_FMT_YUV420:
			output->format = OFORMAT_YUV420;
			output->depth = 12;
			output->addrnums = 3;
			output->addroff[0] = 0;
			output->addroff[1] = vfmt->width * vfmt->height;
			output->addroff[2] = output->addroff[1] * 5 / 4;
			break;
		case V4L2_PIX_FMT_SBGGR8:
			output->format = OFORMAT_RAW8;
			output->depth = 8;
			output->addrnums = 1;
			output->addroff[0] = 0;
			break;
		case V4L2_PIX_FMT_SBGGR10:
			output->format = OFORMAT_RAW10;
			output->depth = 16;
			output->addrnums = 1;
			output->addroff[0] = 0;
			break;
		default:
			ret = -EINVAL;
	}
	if(ret)
		ISP_PRINT(ISP_ERROR, "isp check fmt, fourcc:%d pix->width:%d\n", vfmt->fourcc, vfmt->width);
	output->width	= vfmt->width;
	output->height	= vfmt->height;
	ISP_PRINT(ISP_INFO, "isp check fmt, output->width:%d output->height:%d\n", output->width, output->height);
	output->fourcc = vfmt->fourcc;
	output->field = vfmt->field;
	output->colorspace = vfmt->colorspace;
	output->bytesperline = (output->width  * output->depth) >> 3;
	output->sizeimage = output->bytesperline * output->height;
	return ret;
}
static int isp_fill_i2c_group(struct isp_device *isp)
{
	struct ovisp_camera_client *client = isp->client;
	struct sensor_info *info = isp->sensor_info;
	struct sensor_win_setting *wsize = info->trying_win;
	struct regval_list *sensor_regs;
	unsigned char i2c_flag = 0;
	int j =0;

	i2c_flag = SELECT_I2C_PRIMARY | SELECT_I2C_WRITE | SELECT_I2C_16BIT_ADDR;

	if (info->write_mode == ISP_CMD_GROUP_WRITE && (NULL != wsize) && wsize->regs) {
		if (wsize->reg_sum >= 150) {
			return -EINVAL;
		}
		ISP_PRINT(ISP_INFO,"isp_fill_i2c_group : wsize->width = %d, wsize->height = %d,\n", 
			  wsize->width, wsize->height);
		sensor_regs = wsize->regs;
		for (j = 0; j < 150; j++) {
			if (sensor_regs[j].reg_num == SENSOR_REG_END) {
				break;
			}
			isp_firmware_writeb(isp, sensor_regs[j].reg_num >> 8, COMMAND_BUFFER + j * 4);
			isp_firmware_writeb(isp, sensor_regs[j].reg_num & 0xff, COMMAND_BUFFER + j * 4 + 1);
			isp_firmware_writeb(isp, sensor_regs[j].value, COMMAND_BUFFER + j * 4 + 2);
			isp_firmware_writeb(isp, 0xff, COMMAND_BUFFER + j * 4 + 3);
		}
		isp_reg_writeb(isp, i2c_flag, COMMAND_REG1);
		isp_reg_writeb(isp, isp->sensor_info->i2c_addr * 2, COMMAND_REG2);
		isp_reg_writeb(isp, j, COMMAND_REG3);

		info->using_win = wsize;
	} else {
		isp_reg_writeb(isp, 0, COMMAND_REG1);
		isp_reg_writeb(isp, 0, COMMAND_REG2);
		isp_reg_writeb(isp, 0, COMMAND_REG3);
	}

	return 0;
}

static int isp_set_input_parm(struct isp_device *isp, struct ovisp_video_format *vfmt, struct isp_input_parm *input)
{
	int ret = 0;
	ret = isp_check_input_format(isp, vfmt);
	if (ret)
		return ret;
	if(input->width + input->height >= vfmt->dev_width + vfmt->height)
		return 0;

	switch(vfmt->dev_fourcc){
		case V4L2_PIX_FMT_SBGGR8:
			input->format = IFORMAT_RAW8;
			input->sequence = IFORMAT_BGGR;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SGBRG8:
			input->format = IFORMAT_RAW8;
			input->sequence = IFORMAT_GBRG;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SGRBG8:
			input->format = IFORMAT_RAW8;
			input->sequence = IFORMAT_GRBG;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SRGGB8:
			input->format = IFORMAT_RAW8;
			input->sequence = IFORMAT_RGGB;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SBGGR10:
			input->format = IFORMAT_RAW10;
			input->sequence = IFORMAT_BGGR;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SGBRG10:
			input->format = IFORMAT_RAW10;
			input->sequence = IFORMAT_GBRG;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SGRBG10:
			input->format = IFORMAT_RAW10;
			input->sequence = IFORMAT_GRBG;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SRGGB10:
			input->format = IFORMAT_RAW10;
			input->sequence = IFORMAT_RGGB;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SBGGR12:
			input->format = IFORMAT_RAW12;
			input->sequence = IFORMAT_BGGR;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SGBRG12:
			input->format = IFORMAT_RAW12;
			input->sequence = IFORMAT_GBRG;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SGRBG12:
			input->format = IFORMAT_RAW12;
			input->sequence = IFORMAT_GRBG;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_SRGGB12:
			input->format = IFORMAT_RAW12;
			input->sequence = IFORMAT_RGGB;
			input->addrnums = 1;
			break;
		case V4L2_PIX_FMT_YUYV:
			input->format = IFORMAT_YUV422;
			input->sequence = IFORMAT_RGGB;
			input->addrnums = 1;
			break;
		default:
			return -EINVAL;
	}
	if(ret)
		ISP_PRINT(ISP_ERROR, "isp check input format 0x%x\n", vfmt->dev_fourcc);
	isp_reg_writeb(isp, input->sequence, 0x65006);
	input->width = vfmt->dev_width;
	input->height = vfmt->dev_height;
	ISP_PRINT(ISP_INFO, "isp check fmt, input->width:%d input->height:%d\n", input->width, input->height);
	input->idi_in_width = input->width;
	input->idi_in_height = input->height;
	return 0;
}
static int isp_try_fmt(struct isp_device *isp, struct isp_format *f)
{
	int ret;
	ret = isp_check_input_format(isp, &(f->vfmt));
	if (ret)
		return ret;
	if (!isp->bypass){
		ret = isp_check_output_fmt(isp, &(f->vfmt));
	}
	return ret;
}

static int isp_pre_fmt(struct isp_device *isp, struct isp_format *f)
{
	struct ovisp_camera_client *client = &isp->pdata->client[isp->input];
	int ret = 0;

	if (client->flags & CAMERA_CLIENT_IF_MIPI) {
		if ((isp->fmt_data.sensor_sys_pclk != f->fmt_data->sensor_sys_pclk)) {
			csi_phy_init();
			ISP_PRINT(ISP_INFO,"isp pre fmt restart phy!!!, lanes = %d\n", f->fmt_data->lans);
			ISP_PRINT(ISP_INFO,"f->fmt_data->sensor_sys_pclk = %d\n",f->fmt_data->sensor_sys_pclk);
			ret = csi_phy_start(0, f->fmt_data->sensor_sys_pclk, f->fmt_data->lans);
			if (!ret)
				isp->fmt_data.sensor_sys_pclk = f->fmt_data->sensor_sys_pclk;
		}
	}
	isp->parm.vts = f->fmt_data->vts;

	memcpy(&(isp->fmt_data), f->fmt_data, sizeof(*f->fmt_data));
	ISP_PRINT(ISP_INFO,"[%s]isp->fmt_data.vts = %x;isp->fmt_data.sensor_sys_pclk = %d\n",
		  __func__, isp->fmt_data.vts, isp->fmt_data.sensor_sys_pclk);
	return ret;
}

static int isp_s_fmt(struct isp_device *isp, struct isp_format *f)
{
	int ret = 0;
	isp->format_active = 0;
	ret = isp_set_input_parm(isp, &(f->vfmt), &(isp->parm.input));
	if(ret)
		return ret;
	ret = isp_set_output_parm(isp, &(f->vfmt), &(isp->parm.output[isp->parm.c_video]));
	if(ret)
		return ret;
	if(isp->parm.c_video == 0){
		if(f->vfmt.dev_fourcc == f->vfmt.fourcc)
			isp->bypass = true;
		else
			isp->bypass = false;
	}
	isp->format_active = 1;
	return 0;
}
static int isp_g_fmt(struct isp_device *isp, struct v4l2_format *f)
{
	struct isp_output_parm *output = &(isp->parm.output[isp->parm.c_video]);

	f->fmt.pix.width        = output->width;
	f->fmt.pix.height       = output->height;
	f->fmt.pix.field        = output->field;
	f->fmt.pix.pixelformat  = output->fourcc;
	f->fmt.pix.bytesperline = output->bytesperline;
	f->fmt.pix.sizeimage	= output->sizeimage;
	return 0;
}
static int isp_g_size(struct isp_device *isp, unsigned long *size)
{
	int index = 0;
	int videos = isp->parm.out_videos;
	*size = 0;
	for(index = 0; index < videos; index++){
		*size += isp->parm.output[index].sizeimage;
	}
	return 0;
}
static int isp_g_devfmt(struct isp_device *isp, struct isp_format *f)
{
	struct isp_parm *iparm = &isp->parm;
	struct ovisp_video_format *vfmt = &(f->vfmt);

	vfmt->dev_width = iparm->input.width;
	vfmt->dev_height = iparm->input.height;
	return 0;
}
static int isp_g_outinfo(struct isp_device *isp, unsigned char *priv)
{
	int i = 0;
	for(i = 0; i < ISP_OUTPUT_INFO_LENS; i++){
		priv[i] = isp_firmware_readb(isp, 0x1f200 + i);
	}
	return 0;
}
static inline int isp_set_videonum(struct isp_device *isp, unsigned int num)
{
	if(num > ISP_MAX_OUTPUT_VIDEOS)
		return -1;
	isp->parm.out_videos = num;
	return 0;
}
static inline int isp_set_current_videoindex(struct isp_device *isp, unsigned int index)
{
	if(index >= isp->parm.out_videos)
		return -1;
	isp->parm.c_video = index;
	return 0;
}
static int isp_s_ctrl(struct isp_device *isp, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct isp_parm *iparm = &isp->parm;

	switch (ctrl->id) {
	case V4L2_CID_AUTO_WHITE_BALANCE:
		ISP_PRINT(ISP_INFO,"set white_balance %d\n", ctrl->value);
		ret = isp_set_auto_white_balance(isp, ctrl->value);
		if (!ret){
			iparm->white_balance = 0;
			iparm->auto_white_balance = ctrl->value;
		}
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		ISP_PRINT(ISP_INFO,"set white_balance %d\n", ctrl->value);
		ret = isp_set_do_white_balance(isp, ctrl->value);
		if (!ret){
			iparm->white_balance = ctrl->value;
			iparm->auto_white_balance = 0;
		}
		break;
	case V4L2_CID_BRIGHTNESS:
		ISP_PRINT(ISP_INFO,"set brightness %d\n", ctrl->value);
		ret = isp_set_brightness(isp, ctrl->value);
		if (!ret)
			iparm->brightness = ctrl->value;
		break;
	case V4L2_CID_EXPOSURE_AUTO:
		ISP_PRINT(ISP_INFO,"set exposure manual %d\n", ctrl->value);
		if (ctrl->value == V4L2_EXPOSURE_MANUAL){
			ret = isp_set_exposure_manual(isp);
			if (!ret)
				iparm->auto_exposure = 1;
		}
		break;
	case V4L2_CID_EXPOSURE:
		ISP_PRINT(ISP_INFO,"set exposure %d\n", ctrl->value);
		ret = isp_set_exposure(isp, ctrl->value);
		if (!ret){
			iparm->exposure = ctrl->value;
			iparm->auto_exposure = 0;
		}
		break;
	case V4L2_CID_GAIN:
		ISP_PRINT(ISP_INFO,"set exposure gain%d\n", ctrl->value);
		ret = isp_set_gain(isp, ctrl->value);
		if (!ret){
			iparm->gain = ctrl->value;
		}
		break;
	case V4L2_CID_CONTRAST:
		ISP_PRINT(ISP_INFO,"set contrast %d\n", ctrl->value);
		ret = isp_set_contrast(isp, ctrl->value);
		if (!ret)
			iparm->contrast = ctrl->value;
		break;
	case V4L2_CID_SATURATION:
		ISP_PRINT(ISP_INFO,"set saturation %d\n", ctrl->value);
		ret = isp_set_saturation(isp, ctrl->value);
		if (!ret)
			iparm->saturation = ctrl->value;
		break;
	case V4L2_CID_SHARPNESS:
		ISP_PRINT(ISP_INFO,"set sharpness %d\n", ctrl->value);
		ret = isp_set_sharpness(isp, ctrl->value);
		if (!ret)
			iparm->sharpness = ctrl->value;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		ISP_PRINT(ISP_INFO,"set flicker %d\n", ctrl->value);
		ret = isp_set_flicker(isp, ctrl->value);
		if (!ret)
			iparm->flicker = ctrl->value;
		break;
	case V4L2_CID_FOCUS_AUTO:
		break;
	case V4L2_CID_FOCUS_RELATIVE:
		break;
	case V4L2_CID_FOCUS_ABSOLUTE:
		break;
	case V4L2_CID_SET_FOCUS:
	    if(ctrl->value == 0x01){
		isp_firmware_writeb(isp, 0x01, 0x1ec6e);
		isp_firmware_writeb(isp, 0x01, 0x1ec6f);
		isp_firmware_writeb(isp, 0x00, 0x1ed38);
	    }
	    if(ctrl->value == 0x00){
		isp_firmware_writeb(isp, 0x01, 0x1ec6e);
		isp_firmware_writeb(isp, 0x00, 0x1ec6f);
		isp_firmware_writeb(isp, 0x00, 0x1ed38);
	    }
	    if(ctrl->value == 0x02){
		isp_firmware_writeb(isp, 0x00, 0x1ec6e);
		isp_reg_writeb(isp, 0x18, 0x63601);
		isp_reg_writeb(isp, 0x02, 0x63606);
		isp_reg_writeb(isp, 0x19, 0x63604);
		isp_reg_writeb(isp, 0x00, 0x63605);
		isp_reg_writeb(isp, 0x35, 0x63609);
	    }
		break;
	case V4L2_CID_ZOOM_RELATIVE:
		ISP_PRINT(ISP_INFO,"set zoom %d\n", ctrl->value);
		if (isp->running)
			ret = isp_set_zoom(isp, ctrl->value);
		if (!ret)
			iparm->zoom = ctrl->value;
		break;
	case V4L2_CID_HFLIP:
		ISP_PRINT(ISP_INFO,"set hflip %d\n", ctrl->value);
		ret = isp_set_hflip(isp, ctrl->value);
		if (!ret)
			iparm->hflip = ctrl->value;
		break;
	case V4L2_CID_VFLIP:
		ISP_PRINT(ISP_INFO,"set vflip %d\n", ctrl->value);
		ret = isp_set_vflip(isp, ctrl->value);
		if (!ret)
			iparm->vflip = ctrl->value;
		break;
		/* Private. */
	case V4L2_CID_ISO:
		ISP_PRINT(ISP_INFO,"set iso %d\n", ctrl->value);
		ret = isp_set_iso(isp, ctrl->value);
		if (!ret)
			iparm->iso = ctrl->value;
		break;
	case V4L2_CID_EFFECT:
		ISP_PRINT(ISP_INFO,"set effect %d\n", ctrl->value);
		ret = isp_set_effect(isp, ctrl->value);
		if (!ret)
			iparm->effects = ctrl->value;
		break;
	case V4L2_CID_FLASH_MODE:
		break;
	case V4L2_CID_SCENE:
		ISP_PRINT(ISP_INFO,"set scene %d\n", ctrl->value);
		ret = isp_set_scene(isp, ctrl->value);
		if (!ret)
			iparm->scene_mode = ctrl->value;
		break;
	case V4L2_CID_FRAME_RATE:
		iparm->frame_rate = ctrl->value;
		break;
	case V4L2_CID_RED_BALANCE:
		ISP_PRINT(ISP_INFO,"set scene %d\n", ctrl->value);
		ret = isp_set_red_balance(isp, ctrl->value);
		break;
	case V4L2_CID_BLUE_BALANCE:
		ISP_PRINT(ISP_INFO,"set scene %d\n", ctrl->value);
		ret = isp_set_blue_balance(isp, ctrl->value);
		break;
	case V4L2_CID_FLOW_VIDEONUM:
		ISP_PRINT(ISP_INFO,"set videonum %d\n", ctrl->value);
		ret = isp_set_videonum(isp, ctrl->value);
		break;
	case V4L2_CID_FLOW_CFG_VIDEO:
		ISP_PRINT(ISP_INFO,"set videonum %d\n", ctrl->value);
		ret = isp_set_current_videoindex(isp, ctrl->value);
		break;
	case V4L2_CID_ISP_SETTING_SWITCH:
		ISP_PRINT(ISP_INFO, "set isp setting base of light %d\n", ctrl->value);
		if (ctrl->value != isp->light) {
			isp->light = ctrl->value;
			isp_set_color_parameters(isp);
		}
		break;
	case V4L2_CID_ISP_FACE_EXPOSURE:
		ISP_PRINT(ISP_INFO, "set isp setting base of face exposurce %d\n", ctrl->value);
		isp->face_exposure = ctrl->value;
		isp_set_face_exposure_parameters(isp);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int isp_g_ctrl(struct isp_device *isp, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct isp_parm *iparm = &isp->parm;

	switch (ctrl->id) {
	case V4L2_CID_DO_WHITE_BALANCE:
		ctrl->value = iparm->white_balance;
		break;
	case V4L2_CID_BRIGHTNESS:
		ctrl->value = iparm->brightness;
		break;
	case V4L2_CID_EXPOSURE:
		ctrl->value = iparm->exposure;
		break;
	case V4L2_CID_CONTRAST:
		ctrl->value = iparm->contrast;
		break;
	case V4L2_CID_SATURATION:
		ctrl->value = iparm->saturation;
		break;
	case V4L2_CID_SHARPNESS:
		ctrl->value = iparm->sharpness;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		ctrl->value = iparm->flicker;
		break;
	case V4L2_CID_FOCUS_AUTO:
		break;
	case V4L2_CID_FOCUS_RELATIVE:
		break;
	case V4L2_CID_FOCUS_ABSOLUTE:
		break;
	case V4L2_CID_GET_AF_STATE:
		ret = isp_get_af_state(isp, &(ctrl->value));
		break;
	case V4L2_CID_ZOOM_RELATIVE:
		ctrl->value = iparm->zoom;
		break;
	case V4L2_CID_HFLIP:
		ctrl->value = iparm->hflip;
		break;
	case V4L2_CID_VFLIP:
		ctrl->value = iparm->vflip;
		break;
		/* Private. */
	case V4L2_CID_ISO:
		ctrl->value = iparm->iso;
		break;
	case V4L2_CID_EFFECT:
		ctrl->value = iparm->effects;
		break;
	case V4L2_CID_FLASH_MODE:
		break;
	case V4L2_CID_SCENE:
		ctrl->value = iparm->scene_mode;
		break;
	case V4L2_CID_FRAME_RATE:
		ctrl->value = iparm->frame_rate;
		break;
	case V4L2_CID_RED_BALANCE:
		ret = isp_get_red_balance(isp, &(ctrl->value));
		break;
	case V4L2_CID_BLUE_BALANCE:
		ret = isp_get_blue_balance(isp, &(ctrl->value));
		break;
	case V4L2_CID_FLOW_VIDEONUM:
		ctrl->value = iparm->out_videos;
		break;
	case V4L2_CID_FLOW_CFG_VIDEO:
		ctrl->value = iparm->c_video;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int isp_s_parm(struct isp_device *isp, struct v4l2_streamparm *parm)
{
	return 0;
}

static int isp_g_parm(struct isp_device *isp, struct v4l2_streamparm *parm)
{
	return 0;
}

static int isp_save_flags(struct isp_device *isp, struct isp_format *ifmt)
{
	int ret = 0;
	struct isp_parm *iparm = &isp->parm;

	ifmt->vfmt.dev_width = iparm->input.width;
	ifmt->vfmt.dev_height = iparm->input.height;
#if defined CAPTURE_INITAL_MODE
	ret = isp_wait_frame_eof(isp);
	if(ret)
		ISP_PRINT(ISP_ERROR,"%s[%d]\n", __func__, __LINE__);
#endif
#ifdef CONFIG_BOARD_WESEEING
	isp_reg_writeb(isp, 0xa0, 0x65404);
	isp_reg_writeb(isp, 0xff, 0x65001);
#endif
	isp_firmware_writeb(isp, 0x01, 0x1e022);
	/* isp_firmware_writeb(isp, 0x01, 0x1e5dd); */
	return ret;
}

static int isp_restore_flags(struct isp_device *isp, struct isp_format *ifmt)
{
	int ret = 0;
	/* isp_firmware_writeb(isp, 0x00, 0x1e5dd); */
	/* isp->first_init = true; */
#ifdef CONFIG_VIDEO_OV5648
	isp->frame_need_drop = 2;
	isp->done_frames = -1;
#else
	isp_firmware_writeb(isp, 0x00, 0x1e022);
#endif
#ifdef CONFIG_BOARD_WESEEING
	isp_reg_writeb(isp, 0x60, 0x65404);
	isp_reg_writeb(isp, 0x7f, 0x65001);
#endif
	ret = isp_send_start_capture_cmd(isp);
	isp->is_capturing = false;
	return ret;
}

static int isp_process_raw(struct isp_device *isp, struct isp_format *ifmt,
			   unsigned int src_addr, unsigned int dst_addr, unsigned char *info, int zoom)
{
	int ret = 0;
	struct isp_input_parm input = {0};
	struct isp_output_parm output = {0};

	/* printk("@@@@@ width=%d height=%d dev_fourcc=0x%08x fourcc=0x%08x\n", */
				/* ifmt->vfmt.width,ifmt->vfmt.height,ifmt->vfmt.dev_fourcc,ifmt->vfmt.fourcc); */
	/*ifmt->vfmt.width = 1600;
	ifmt->vfmt.height = 1200; */

	ret = isp_set_input_parm(isp, &ifmt->vfmt, &input);
	if(ret)
		return ret;
	ret = isp_set_output_parm(isp, &ifmt->vfmt, &output);
	if(ret)
		return ret;
//	printk("@@@@@ inwidth=%d inheight=%d informat=0x%08x outformat=0x%08x\n",
//			input.width, input.height, input.format, output.format);
//	printk("@@@@@ srcaddr=0x%08x addrnum=%d dstaddr=0x%08x addr[0]=0x%08x addr[1]=0x%08x\n",
//			src_addr, output.addrnums, dst_addr, output.addroff[0], output.addroff[1]);
	/* ret = isp_capture_raw_stop(isp); */
	if(ret)
		ISP_PRINT(ISP_ERROR,"%s[%d]\n", __func__, __LINE__);

	ret = isp_offline_process(isp, &input, &output, src_addr, dst_addr, info, zoom);
	if(ret)
		ISP_PRINT(ISP_ERROR,"%s[%d]\n", __func__, __LINE__);
	/* isp->first_init = true; */
	/* ret = isp_send_start_capture_cmd(isp); */
	return ret;
}
static int isp_bypass_capture(struct isp_device *isp, struct isp_format *ifmt, unsigned int dst_addr)
{
	int ret = 0;
	unsigned long tm = 0;
	struct isp_input_parm input = {0};
	struct isp_output_parm output = {0};

//	printk("@@@@@ width=%d height=%d dev_fourcc=0x%08x fourcc=0x%08x\n",
//				ifmt->vfmt.width,ifmt->vfmt.height,ifmt->vfmt.dev_fourcc,ifmt->vfmt.fourcc);
	ret = isp_set_input_parm(isp, &ifmt->vfmt, &input);
	if(ret)
		return ret;
	ret = isp_set_output_parm(isp, &ifmt->vfmt, &output);
	if(ret)
		return ret;
	isp_set_banding_parameters(isp, false);
//	printk("@@@@@ inwidth=%d inheight=%d informat=0x%08x outformat=0x%08x\n",
//			input.width, input.height, input.format, output.format);
//	printk("@@@@@ addrnum=%d dstaddr=0x%08x addr[0]=0x%08x addr[1]=0x%08x\n",
//			output.addrnums, dst_addr, output.addroff[0], output.addroff[1]);
	ISP_PRINT(ISP_INFO,"++++++++++++ifmt->fmt_data->vts = %08x\n",ifmt->fmt_data->vts);
	ret = isp_fill_i2c_group(isp);
	if(ret){
	    ISP_PRINT(ISP_ERROR, KERN_ERR "isp_cmd_i2c_group faild!\n");
	    return ret;
	}

	/* ret = isp_wait_frame_eof(isp); */
	/* if(ret) */
	/* 	ISP_PRINT(ISP_ERROR,"%s[%d]\n", __func__, __LINE__); */

	isp->is_capturing = true;
	INIT_COMPLETION(isp->capture_start);
	tm = wait_for_completion_timeout(&isp->capture_start, msecs_to_jiffies(1000));
	if (!tm && !isp->capture_start.done) {
		ret = -ETIMEDOUT;
	}
	isp_set_address(isp, 0, dst_addr);
	isp_set_address(isp, 1, dst_addr);

	ret = isp_normal_capture(isp, &input, &output, dst_addr, ifmt->fmt_data->vts);
	if(ret)
		ISP_PRINT(ISP_ERROR,"%s[%d]\n", __func__, __LINE__);
	return ret;
}

static int isp_s_tlb_base(struct isp_device *isp, unsigned int *tlb_base)
{

	int tlb_en = 1;
	int mipi_sel = 1;
	int tlb_invld = 1;
	int tlb_gcn = DMMU_PTE_CHECK_PAGE_VALID;
	int tlb_cnm = DMMU_PTE_CHECK_PAGE_VALID;
	int tlb_ridx = 0;  /*6 bits TLB entry read-index*/
	unsigned int _tlb_base  = *tlb_base;
	ISP_PRINT(ISP_INFO," Read ISP TLB CTRL : 0x%x\n", isp_reg_readl(isp, 0xF0004));
	ISP_PRINT(ISP_INFO,"Read ISP TLB BASE : 0x%x\n", isp_reg_readl(isp, 0xF0008));

	if(tlb_invld)
		isp_reg_writel(isp, 0x00000010, 0xF0000);/*TLB Reset*/
	/*TLB Control*/
	isp_reg_writel(isp,( tlb_en |
				(mipi_sel << 1) |
				(tlb_gcn << 2) |
				(tlb_cnm << 14) |
				(tlb_ridx << 26)
				), 0xF0004);

	/*TLB Table Base Address*/
	isp_reg_writel(isp, _tlb_base, 0xF0008);
	/* dump_tlb_regs(isp); */

	return 0;
}

static int isp_tlb_map_one_vaddr(struct isp_device *isp, unsigned int vaddr, unsigned int size)
{
	int ret = 0;
	struct isp_tlb_pidmanager *p = NULL;
	struct isp_tlb_pidmanager *tmp = NULL;
	struct isp_tlb_vaddrmanager *v = NULL;
	struct isp_tlb_vaddrmanager *prev = NULL;
	struct isp_tlb_vaddrmanager *after = NULL;
	struct isp_tlb_vaddrmanager *tmv = NULL;
	pid_t pid = current->tgid;

	if(list_empty(&(isp->tlb_list))){
		ISP_PRINT(ISP_ERROR,"%s[%d] vaddr can't map because tlb isn't inited!\n",__func__,__LINE__);
		ret = -EINVAL;
		goto out;
	}
	list_for_each_entry(tmp, &(isp->tlb_list), pid_entry){
		if(tmp->pid == pid){
			p = tmp;
			break;
		}
	}
	if(!p){
		ret = -EINVAL;
		goto out;
	}

	list_for_each_entry(tmv, &(p->vaddr_list), vaddr_entry){
		if(tmv->vaddr <= vaddr && vaddr < tmv->vaddr + tmv->size){
			v = tmv;
			break;
		}else if(vaddr < tmv->vaddr){
			after = tmv;
			break;
		}else
			prev = tmv;
	}
	/* after != NULL or prev != NULL */
	if(!v){
		if(prev && (prev->vaddr + prev->size == vaddr)){
			/* prev and current are sequential */
			ISP_PRINT(ISP_INFO,"^^^%s[%d] *prev-current* vaddr = 0x%08x\n",__func__,__LINE__,vaddr);
			prev->size += size;
			v = prev;
		}
		if(after && (vaddr + size == after->vaddr)){
			if(!v){
				ISP_PRINT(ISP_INFO,"^^^%s[%d] *current-after* vaddr = 0x%08x\n",__func__,__LINE__,vaddr);
				/* after and current are sequential */
				after->vaddr = vaddr;
				after->size += size;
				v = after;
			}else{
				ISP_PRINT(ISP_INFO,"^^^%s[%d] *prev-current-after* vaddr = 0x%08x\n",__func__,__LINE__,vaddr);
				/* prev, after and current are sequential */
				prev->size += after->size;
				list_del(&(after->vaddr_entry));
				kfree(after);
			}
		}
		if(!v){
			/* prev ,after and current are discontinuous */
			ISP_PRINT(ISP_INFO,"^^^%s[%d] alloc_vaddrmanager! vaddr = 0x%08x\n",__func__,__LINE__,vaddr);
			v = kzalloc(sizeof(*v), GFP_KERNEL);
			if(!v){
				ISP_PRINT(ISP_ERROR,"%s[%d]\n",__func__,__LINE__);
				ret = -EINVAL;
				goto alloc_fail;
			}
			v->vaddr = vaddr;
			v->size = size;
			if(prev){
				ISP_PRINT(ISP_INFO,"^^^%s[%d] *instand prev* vaddr = 0x%08x\n",__func__,__LINE__,vaddr);
				prev->vaddr_entry.next->prev = &(v->vaddr_entry);
				v->vaddr_entry.prev = &(prev->vaddr_entry);
				v->vaddr_entry.next = prev->vaddr_entry.next;
				prev->vaddr_entry.next = &(v->vaddr_entry);
			}else if(after){
				ISP_PRINT(ISP_INFO,"^^^%s[%d] *instand after* vaddr = 0x%08x\n",__func__,__LINE__,vaddr);
				after->vaddr_entry.prev->next = &(v->vaddr_entry);
				v->vaddr_entry.next = &(after->vaddr_entry);
				v->vaddr_entry.prev = after->vaddr_entry.prev;
				after->vaddr_entry.prev = &(v->vaddr_entry);
			}else
				list_add_tail(&(v->vaddr_entry), &(p->vaddr_list));
		}

		ret = dmmu_match_user_mem_tlb((void*)vaddr, size);
		if(ret < 0)
			goto match_fail;
		ret = dmmu_map_user_mem((void *)vaddr, size);
		if(ret < 0)
			goto map_fail;
	}
	return 0;
map_fail:
match_fail:
	kfree(v);
alloc_fail:
out:
	return ret;
}
static int isp_tlb_unmap_all_vaddr(struct isp_device *isp)
{
	struct isp_tlb_pidmanager *p = NULL;
	struct isp_tlb_pidmanager *tmp = NULL;
	struct isp_tlb_vaddrmanager *v = NULL;
	struct list_head *pos = NULL;
	pid_t pid = current->tgid;

	if(!list_empty(&(isp->tlb_list))){
		list_for_each_entry(tmp, &(isp->tlb_list), pid_entry){
			if(tmp->pid == pid){
				p = tmp;
				break;
			}
		}
		/* if find, release it's vaddr_list */
		if(p){
			ISP_PRINT(ISP_INFO,"^^^%s[%d] unmap!\n",__func__,__LINE__);
			/* if vaddr_list  of the tlb_pidmanager isn't empty, release it */
			if(!list_empty(&(p->vaddr_list))){
				list_for_each(pos, &(p->vaddr_list)){
					v = list_entry(pos, struct isp_tlb_vaddrmanager,vaddr_entry);
					list_del(&(v->vaddr_entry));
					dmmu_unmap_user_mem((void *)(v->vaddr), v->size);
					kfree(v);
					pos = &(p->vaddr_list);
				}
			}
		}
	}
	return 0;
}

static int isp_tlb_init(struct isp_device *isp)
{
	int ret = 0;
	struct isp_tlb_pidmanager *p = NULL;
	struct isp_tlb_pidmanager *tmp = NULL;
	pid_t pid = current->tgid;

	/* first */
	if(list_empty(&(isp->tlb_list))){
		ISP_PRINT(ISP_INFO,"^^^%s[%d] mmu_init! \n",__func__,__LINE__);
		ret = dmmu_init();
		if(ret < 0)
			goto dmmu_fail;
		isp->tlb_flag = 1;
	}

	/* if tlb_list isn't empty , check pid */
	if(!list_empty(&(isp->tlb_list))){
		list_for_each_entry(tmp, &(isp->tlb_list), pid_entry){
			if(tmp->pid == pid){
				p = tmp;
				break;
			}
		}
	}

	/* if current->tgid isn't in tlb_list, add a isp_tlb_pidmanager */
	if(!p){
		p = kzalloc(sizeof(*p), GFP_KERNEL);
		if(!p){
			ISP_PRINT(ISP_ERROR,"%s[%d]\n",__func__,__LINE__);
			ret = -EINVAL;
			goto alloc_fail;
		}
		p->pid = pid;
		INIT_LIST_HEAD(&(p->vaddr_list));
		ret = dmmu_get_page_table_base_phys(&(p->tlbbase));
		ISP_PRINT(ISP_INFO,"^^^%s[%d] alloc pidmanager! tlbbase = 0x%08x\n",__func__,__LINE__,p->tlbbase);
		isp_s_tlb_base(isp,&(p->tlbbase));
		if(ret < 0)
			goto tlbbase_fail;
		list_add_tail(&(p->pid_entry), &(isp->tlb_list));
	}
	return 0;
tlbbase_fail:
	kfree(p);
alloc_fail:
dmmu_fail:
	return ret;
}
static int isp_tlb_deinit(struct isp_device *isp)
{
	int ret = 0;
	struct isp_tlb_pidmanager *p = NULL;
	struct isp_tlb_pidmanager *tmp = NULL;
	struct isp_tlb_vaddrmanager *vaddr = NULL;
	struct list_head *pos = NULL;
	pid_t pid = current->tgid;

	if(isp->tlb_flag == 0)
		return 0;
	/* if tlb_list isn't empty , check pid */
	if(!list_empty(&(isp->tlb_list))){
		list_for_each_entry(tmp, &(isp->tlb_list), pid_entry){
			if(tmp->pid == pid){
				p = tmp;
				break;
			}
		}
	}

	/* if current->tgid is find, release it  */
	if(p){
		list_del(&(p->pid_entry));

		/* if vaddr_list  of the tlb_pidmanager, release it */
		if(!list_empty(&(p->vaddr_list))){
			list_for_each(pos, &(p->vaddr_list)){
				vaddr = list_entry(pos, struct isp_tlb_vaddrmanager,vaddr_entry);
				list_del(&(vaddr->vaddr_entry));
				kfree(vaddr);
				pos = &(p->vaddr_list);
			}
		}
		kfree(p);
		ISP_PRINT(ISP_INFO,"%s[%d] kfree pidmanager! \n",__func__,__LINE__);
	}
	/* last */
	if(list_empty(&(isp->tlb_list))){
		ISP_PRINT(ISP_INFO,"%s[%d] mmu_deinit! \n",__func__,__LINE__);
		ret = dmmu_deinit();
		isp->tlb_flag = 0;
	}
	return ret;
}
static int isp_set_camera_fmt(struct isp_device *isp, struct v4l2_mbus_framefmt *fmt, struct isp_i2c_grp_w *grp)
{
	int i = 0, j = 0, roll = 0;
	unsigned int dev_i2c_id = grp->dev_i2c_id;
	unsigned char i2c_flag = 0;
	struct sensor_info *info = grp->si;
	struct sensor_win_setting *wsize = info->trying_win;
	struct regval_list *sensor_regs;
	i2c_flag |= SELECT_I2C_PRIMARY | SELECT_I2C_WRITE | SELECT_I2C_16BIT_ADDR;

	isp_firmware_writeb(isp, dev_i2c_id * 2, 0x1e056);
	/* for (wsize = info->win; wsize < info->win + info->win_size; wsize++) */
	/* 	if (fmt->width > wsize->width && fmt->height > wsize->height) */
	/* 		break; */
	/* /\* if (wsize >= info->win + info->win_size) *\/ */
	/* 	wsize--;  /\* Take the smallest one *\/ */
	ISP_PRINT(ISP_INFO,"wsize- >width = %d, wsize- >height = %d,\n", wsize->width, wsize->height);
	if ((info->using_win != wsize) && wsize->regs) {
		sensor_regs = wsize->regs;
		roll = (wsize->reg_sum-1) / 150;
		for (i = 0; i <= roll; i++) {
			for (j = 0; j < 150; j++) {
				if (sensor_regs[j].reg_num == SENSOR_REG_END) {
					break;
				}
				isp_firmware_writeb(isp, sensor_regs[j].reg_num >> 8, COMMAND_BUFFER + j * 4);
				isp_firmware_writeb(isp, sensor_regs[j].reg_num & 0xff, COMMAND_BUFFER + j * 4 + 1);
				isp_firmware_writeb(isp, sensor_regs[j].value, COMMAND_BUFFER + j * 4 + 2);
				isp_firmware_writeb(isp, 0xff, COMMAND_BUFFER + j * 4 + 3);
			}

			isp_reg_writeb(isp, i2c_flag, COMMAND_REG1);
			isp_reg_writeb(isp, dev_i2c_id * 2, COMMAND_REG2);
			isp_reg_writeb(isp, j, COMMAND_REG3);

			if (isp_send_cmd(isp, CMD_I2C_GRP_WR, ISP_I2C_TIMEOUT)) {
				ISP_PRINT(ISP_ERROR,"Failed to wait i2c set done (%02x)!\n",
					  isp_reg_readb(isp, REG_ISP_INT_EN_C2));
				return -EINVAL;
			}

			sensor_regs += 150;
		}
		info->using_win = wsize;
	}
	

	return 0;
}

static int isp_set_camera_grouphold_mode(struct isp_device *isp, struct isp_i2c_grp_w *grp)
{
	int i = 0, j = 0, roll = 0;
	unsigned int dev_i2c_id = grp->dev_i2c_id;
	unsigned char i2c_flag = 0;
	struct sensor_info *info = grp->si;
	struct regval_list *sensor_regs;
	if(info->support_group_lunch == false || info->group_hold_regs == NULL)return 0;	

	i2c_flag |= SELECT_I2C_PRIMARY | SELECT_I2C_WRITE | SELECT_I2C_16BIT_ADDR;
	isp_firmware_writeb(isp, dev_i2c_id * 2, 0x1e056);

	sensor_regs = info->group_hold_regs;
	roll = (info->group_hold_regs_num -1) / 150;
	for (i = 0; i <= roll; i++) {
	    for (j = 0; j < 150; j++) {
		if (sensor_regs[j].reg_num == SENSOR_REG_END) {
		    break;
		}
		isp_firmware_writeb(isp, sensor_regs[j].reg_num >> 8, COMMAND_BUFFER + j * 4);
		isp_firmware_writeb(isp, sensor_regs[j].reg_num & 0xff, COMMAND_BUFFER + j * 4 + 1);
		isp_firmware_writeb(isp, sensor_regs[j].value, COMMAND_BUFFER + j * 4 + 2);
		isp_firmware_writeb(isp, 0xff, COMMAND_BUFFER + j * 4 + 3);
	    }
	    isp_reg_writeb(isp, i2c_flag, COMMAND_REG1);
	    isp_reg_writeb(isp, dev_i2c_id * 2, COMMAND_REG2);
	    isp_reg_writeb(isp, j, COMMAND_REG3);
	    
	    if (isp_send_cmd(isp, CMD_I2C_GRP_WR, ISP_I2C_TIMEOUT)) {
		ISP_PRINT(ISP_INFO, KERN_ERR "Failed to wait i2c set done (%02x)!\n",
					  isp_reg_readb(isp, REG_ISP_INT_EN_C2));
		return -EINVAL;
	    }
	    
	    sensor_regs += 150;
	}
	info->write_mode = GROUP_LUNCH;	

	return 0;
}

static int isp_get_camera_info(struct isp_device *isp, struct v4l2_fmt_data *fmt)
{
	memcpy(&(isp->fmt_data), fmt, sizeof(*fmt));
	ISP_PRINT(ISP_INFO,"[%s]isp->fmt_data.vts = %x;isp->fmt_data.sensor_sys_pclk = %d\n",
						__func__,isp->fmt_data.vts,isp->fmt_data.sensor_sys_pclk);
	return 0;
}

static int isp_get_exif_info(struct isp_device *isp, struct v4l2_acquire_photo_parm *parm)
{
	if(parm != NULL){
	    parm->exposure_line = (unsigned int)(isp_firmware_readb(isp,0x1e030)<<24) | (isp_firmware_readb(isp,0x1e031) <<16) | (isp_firmware_readb(isp,0x1e032) << 8) | isp_firmware_readb(isp,0x1e0c3);
	    parm->banding_step_50hz = (unsigned int)(isp_firmware_readb(isp,0x1f07c)<<8) | isp_firmware_readb(isp,0x1f07d);
	    parm->sensor_gain = (unsigned int)(isp_firmware_readb(isp,0x1e034)<<8) | isp_firmware_readb(isp,0x1e035);	
	    parm->exif_info_valid = true;
	}
	return 0;
}

static struct isp_ops isp_ops = {

	.init = isp_init,
	.release = isp_release,
	.open = isp_open,
	.close = isp_close,
	.config = isp_config,
	.suspend = isp_suspend,
	.resume = isp_resume,
	.mclk_on = isp_mclk_on,
	.mclk_off = isp_mclk_off,
	.start_capture = isp_start_capture,
	.stop_capture = isp_stop_capture,
	.update_buffer = isp_update_buffer,
	.try_fmt = isp_try_fmt,
	.pre_fmt = isp_pre_fmt,
	.s_fmt = isp_s_fmt,
	.g_fmt = isp_g_fmt,
	.g_devfmt = isp_g_devfmt,
	.g_outinfo = isp_g_outinfo,
	.g_size = isp_g_size,
	.s_ctrl = isp_s_ctrl,
	.g_ctrl = isp_g_ctrl,
	.s_parm = isp_s_parm,
	.g_parm = isp_g_parm,
	.save_flags = isp_save_flags,
	.restore_flags = isp_restore_flags,
	.process_raw = isp_process_raw,
	.bypass_capture = isp_bypass_capture,
	.s_tlb_base = isp_s_tlb_base,
	.tlb_init = isp_tlb_init,
	.tlb_deinit = isp_tlb_deinit,
	.tlb_map_one_vaddr = isp_tlb_map_one_vaddr,
	.tlb_unmap_all_vaddr = isp_tlb_unmap_all_vaddr,
	.set_camera_fmt = isp_set_camera_fmt,
	.set_camera_grouphold_mode = isp_set_camera_grouphold_mode,
	.get_camera_info = isp_get_camera_info,
	.get_exif_info = isp_get_exif_info,
};

int isp_device_init(struct isp_device* isp)
{
	struct resource *res = isp->res;
	int ret = 0;
	spin_lock_init(&isp->lock);
	init_completion(&isp->completion);
	init_completion(&isp->frame_eof);
	init_completion(&isp->capture_start);

	ISP_PRINT(ISP_INFO,"enter device init %d\n",ret);
	isp->base = ioremap(res->start, res->end - res->start + 1);
	if (!isp->base) {
		ISP_PRINT(ISP_ERROR, KERN_ERR "Unable to ioremap registers.n");
		ret = -ENXIO;
		goto exit;
	}

	isp->boot = 0;
	ret = isp_int_init(isp);
	if (ret)
		goto io_unmap;

	ret = isp_i2c_init(isp);
	if (ret)
		goto irq_free;

	ret = isp_clk_init(isp);
	if (ret)
		goto i2c_release;

	isp->ops = &isp_ops;

	INIT_LIST_HEAD(&(isp->tlb_list)); // add by xhshen
	isp->tlb_flag = 0;

	return 0;

i2c_release:
	isp_i2c_release(isp);
irq_free:
	free_irq(isp->irq, isp);
io_unmap:
	iounmap(isp->base);
exit:
	return ret;
}
EXPORT_SYMBOL(isp_device_init);

int isp_device_release(struct isp_device* isp)
{
	if (!isp)
		return -EINVAL;

//	csi_phy_release();
	isp_clk_release(isp);
	isp_i2c_release(isp);
	free_irq(isp->irq, isp);
	iounmap(isp->base);

	return 0;
}
EXPORT_SYMBOL(isp_device_release);
