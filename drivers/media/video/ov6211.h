#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <mach/ovisp-v4l2.h>
#include <mach/regs-isp.h>
#include "sensor.h"

#define OV6211_MAX_WIDTH		400
#define OV6211_MAX_HEIGHT		400

#define OV6211_CAMERA_RAW_8

#define OV6211_SETTING_PROFILE_2

#define OV6211_DEFAULT_REG_WRTIE_MODE   ISP_CMD_GROUP_WRITE

#if defined OV6211_SETTING_PROFILE_2

#ifdef OV6211_CAMERA_RAW_8

static struct regval_list ov6211_init_default[] = {

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_200_200_50fps[] = {

	/* @@ R200x200 bin RAW8 50fps MIPI150Mbps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x14},
	{0x309a, 0x06},
	{0x309b, 0x02},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x08},
	{0x30b1, 0x04},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x00},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x23},
	{0x3502, 0x40},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x03},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0xe8},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x01},
	{0x3805, 0x9f},
	{0x3806, 0x01},
	{0x3807, 0x9f},
	{0x3808, 0x00},
	{0x3809, 0xc8},
	{0x380a, 0x00},
	{0x380b, 0xc8},
	{0x380c, 0x05},
	{0x380d, 0x78},
	{0x380e, 0x02},
	{0x380f, 0x3c},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x01},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x02},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4837, 0x85},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_200_200_100fps[] = {
	/* @@ R200x200 RAW8 100fps MIPI150Mbps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x14},
	{0x309a, 0x06},
	{0x309b, 0x02},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x08},
	{0x30b1, 0x04},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x00},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x11},
	{0x3502, 0x80},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x03},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0xe8},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x01},
	{0x3805, 0x9f},
	{0x3806, 0x01},
	{0x3807, 0x9f},
	{0x3808, 0x00},
	{0x3809, 0xc8},
	{0x380a, 0x00},
	{0x380b, 0xc8},
	{0x380c, 0x05},
	{0x380d, 0x78},
	{0x380e, 0x01},
	{0x380f, 0x1e},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x01},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x02},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4837, 0x85},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_200_200_120fps[] = {

	/* @@ R200x200 RAW8 120fps MIPI150Mbps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x14},
	{0x309a, 0x06},
	{0x309b, 0x02},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x08},
	{0x30b1, 0x04},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x00},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x0e},
	{0x3502, 0x80},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x03},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0xe8},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x01},
	{0x3805, 0x9f},
	{0x3806, 0x01},
	{0x3807, 0x9f},
	{0x3808, 0x00},
	{0x3809, 0xc8},
	{0x380a, 0x00},
	{0x380b, 0xc8},
	{0x380c, 0x05},
	{0x380d, 0x78},
	{0x380e, 0x00},
	{0x380f, 0xee},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x01},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x02},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4837, 0x85},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_400_400_50fps[] = {

	/* @@ R400x400 RAW8 50fps MIPI300Mbps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x06},
	{0x309b, 0x04},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x08},
	{0x30b1, 0x02},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x00},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x41},
	{0x3502, 0x40},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x03},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x01},
	{0x3805, 0x9b},
	{0x3806, 0x01},
	{0x3807, 0x9b},
	{0x3808, 0x01},
	{0x3809, 0x90},
	{0x380a, 0x01},
	{0x380b, 0x90},
	{0x380c, 0x05},
	{0x380d, 0xf2},
	{0x380e, 0x04},
	{0x380f, 0x1c},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x04},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4825, 0x4a},
	{0x4837, 0x43},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_400_400_100fps[] = {
	/* @@ R400x400 RAW8 100fps MIPI300Mbps */

	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x06},
	{0x309b, 0x04},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x08},
	{0x30b1, 0x02},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x00},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x20},
	{0x3502, 0xa0},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x03},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x01},
	{0x3805, 0x9b},
	{0x3806, 0x01},
	{0x3807, 0x9b},
	{0x3808, 0x01},
	{0x3809, 0x90},
	{0x380a, 0x01},
	{0x380b, 0x90},
	{0x380c, 0x05},
	{0x380d, 0xf2},
	{0x380e, 0x02},
	{0x380f, 0x0e},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x04},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4825, 0x4a},
	{0x4837, 0x43},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},
	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_400_400_120fps[] = {

	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x06},
	{0x309b, 0x04},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x08},
	{0x30b1, 0x02},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x00},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x1b},
	{0x3502, 0x20},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x03},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x01},
	{0x3805, 0x9b},
	{0x3806, 0x01},
	{0x3807, 0x9b},
	{0x3808, 0x01},
	{0x3809, 0x90},
	{0x380a, 0x01},
	{0x380b, 0x90},
	{0x380c, 0x05},
	{0x380d, 0xf2},
	{0x380e, 0x01},
	{0x380f, 0xb6},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x04},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4825, 0x4a},
	{0x4837, 0x43},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},
	{SENSOR_REG_END, 0x00},
};

#else

static struct regval_list ov6211_init_default[] = {

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_200_200_50fps[] = {

	/* @@ R200x200 RAW10 50fps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x14},
	{0x309a, 0x06},
	{0x309b, 0x02},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x0a},
	{0x30b1, 0x04},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x05},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x0e},
	{0x3502, 0x80},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x01},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0xe8},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x01},
	{0x3805, 0x9f},
	{0x3806, 0x01},
	{0x3807, 0x9f},
	{0x3808, 0x00},
	{0x3809, 0xc8},
	{0x380a, 0x00},
	{0x380b, 0xc8},
	{0x380c, 0x05},
	{0x380d, 0x78},
	{0x380e, 0x02},
	{0x380f, 0x3c},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x01},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x02},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4837, 0x85},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_200_200_100fps[] = {

	/* @@ R200x200 RAW10 100fps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x14},
	{0x309a, 0x06},
	{0x309b, 0x02},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x0a},
	{0x30b1, 0x04},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x05},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x0e},
	{0x3502, 0x80},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x01},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0xe8},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x01},
	{0x3805, 0x9f},
	{0x3806, 0x01},
	{0x3807, 0x9f},
	{0x3808, 0x00},
	{0x3809, 0xc8},
	{0x380a, 0x00},
	{0x380b, 0xc8},
	{0x380c, 0x05},
	{0x380d, 0x78},
	{0x380e, 0x01},
	{0x380f, 0x1e},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x01},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x02},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4837, 0x85},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_200_200_120fps[] = {

	/* @@ R200x200 RAW10 120fps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x14},
	{0x309a, 0x06},
	{0x309b, 0x02},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x0a},
	{0x30b1, 0x04},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x05},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x0e},
	{0x3502, 0x80},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x01},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0xe8},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x01},
	{0x3805, 0x9f},
	{0x3806, 0x01},
	{0x3807, 0x9f},
	{0x3808, 0x00},
	{0x3809, 0xc8},
	{0x380a, 0x00},
	{0x380b, 0xc8},
	{0x380c, 0x05},
	{0x380d, 0x78},
	{0x380e, 0x00},
	{0x380f, 0xee},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x01},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x02},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4837, 0x85},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},

	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_400_400_50fps[] = {

	/* @@ R400x400 RAW10 50fps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x06},
	{0x309b, 0x04},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x0a},
	{0x30b1, 0x02},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x05},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x1b},
	{0x3502, 0x20},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x01},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x01},
	{0x3805, 0x9b},
	{0x3806, 0x01},
	{0x3807, 0x9b},
	{0x3808, 0x01},
	{0x3809, 0x90},
	{0x380a, 0x01},
	{0x380b, 0x90},
	{0x380c, 0x05},
	{0x380d, 0xf2},
	{0x380e, 0x04},
	{0x380f, 0x1c},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x04},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4825, 0x4a},
	{0x4837, 0x43},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},
	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_400_400_100fps[] = {

	/* @@ R400x400 RAW10 100fps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x06},
	{0x309b, 0x04},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x02},
	{0x309f, 0x0e},
	{0x30b0, 0x0a},
	{0x30b1, 0x02},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x05},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x1b},
	{0x3502, 0x20},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x01},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x01},
	{0x3805, 0x9b},
	{0x3806, 0x01},
	{0x3807, 0x9b},
	{0x3808, 0x01},
	{0x3809, 0x90},
	{0x380a, 0x01},
	{0x380b, 0x90},
	{0x380c, 0x05},
	{0x380d, 0xf2},
	{0x380e, 0x01},
	{0x380f, 0xb6},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x04},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4825, 0x4a},
	{0x4837, 0x43},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},
	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

static struct regval_list ov6211_400_400_120fps[] = {

	/* @@ R400x400 RAW10 120fps */
	{0x3005, 0x00},
	{0x3013, 0x12},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3037, 0xf0},
	{0x3080, 0x01},
	{0x3081, 0x00},
	{0x3082, 0x01},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x06},
	{0x309b, 0x04},
	{0x309c, 0x00},
	{0x309d, 0x00},
	{0x309e, 0x01},
	{0x309f, 0x00},
	{0x30b0, 0x0a},
	{0x30b1, 0x02},
	{0x30b2, 0x00},
	{0x30b3, 0x32},
	{0x30b4, 0x02},
	{0x30b5, 0x05},
	{0x3106, 0xd9},
	{0x3500, 0x00},
	{0x3501, 0x1b},
	{0x3502, 0x20},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0xfc},
	{0x3620, 0xb7},
	{0x3621, 0x05},
	{0x3626, 0x31},
	{0x3627, 0x40},
	{0x3632, 0xa3},
	{0x3633, 0x34},
	{0x3634, 0x40},
	{0x3636, 0x00},
	{0x3660, 0x80},
	{0x3662, 0x01},
	{0x3664, 0xf0},
	{0x366a, 0x10},
	{0x366b, 0x06},
	{0x3680, 0xf4},
	{0x3681, 0x50},
	{0x3682, 0x00},
	{0x3708, 0x20},
	{0x3709, 0x40},
	{0x370d, 0x03},
	{0x373b, 0x02},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3744, 0x16},
	{0x3745, 0x08},
	{0x3781, 0xfc},
	{0x3788, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x01},
	{0x3805, 0x9b},
	{0x3806, 0x01},
	{0x3807, 0x9b},
	{0x3808, 0x01},
	{0x3809, 0x90},
	{0x380a, 0x01},
	{0x380b, 0x90},
	{0x380c, 0x05},
	{0x380d, 0xf2},
	{0x380e, 0x01},
	{0x380f, 0xb6},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x382b, 0xfa},
	{0x382f, 0x04},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x05},
	{0x3882, 0x04},
	{0x3883, 0x00},
	{0x38a4, 0x10},
	{0x38a5, 0x00},
	{0x38b1, 0x03},
	{0x3b80, 0x00},
	{0x3b81, 0xa5},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x05},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x00},
	{0x3b8f, 0x1a},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xf0},
	{0x4004, 0x04},
	{0x404e, 0x01},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4825, 0x4a},
	{0x4837, 0x43},
	{0x5a08, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x10},
	{0x5a05, 0xa0},
	{0x5a06, 0x0c},
	{0x5a07, 0x78},
	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},
};

#endif
#endif

static struct regval_list ov6211_softreset[] = {
	{0x0103, 0x01},

	{0x0100, 0x00},
	{0x0100, 0x00},
	{0x0100, 0x00},
	{0x0100, 0x00},

	{SENSOR_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov6211_stream_on[] = {
	{0x0100, 0x01},

	{SENSOR_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov6211_stream_off[] = {
	{0x0100, 0x00},

	{SENSOR_REG_END, 0x00},	/* END MARKER */
};

static struct sensor_win_setting ov6211_win_sizes[] = {
#if defined  OV6211_SETTING_PROFILE_2
	{
		.width		= 400,
		.height		= 400,
#ifdef OV6211_CAMERA_RAW_8
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
#else
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
#endif
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov6211_400_400_120fps,
		.lunch_regs 	= NULL,
		.reg_sum        = ARRAY_SIZE(ov6211_400_400_120fps),
		.sysclk         = 80,
		.low_fps_win    = NULL,
		.vts            = 0x41c,
		.hts            = 0x5f2,
	},

	{
		.width		= 200,
		.height		= 200,
#ifdef OV6211_CAMERA_RAW_8
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
#else
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
#endif
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov6211_200_200_50fps,
		.lunch_regs 	= NULL,
		.reg_sum        = ARRAY_SIZE(ov6211_200_200_50fps),
		.sysclk         = 40,
		.low_fps_win    = NULL,
		.vts            = 0x23c,
		.hts            = 0x578,
	}

#endif
};

#define OV6211_N_WIN_SIZES (ARRAY_SIZE(ov6211_win_sizes))

static struct sensor_format_struct ov6211_formats[] = {
	{
#ifdef OV6211_CAMERA_RAW_8
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
#else
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
#endif
		.colorspace	= V4L2_COLORSPACE_SRGB,
	},
	{
#ifdef OV6211_CAMERA_RAW_8
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
#else
		.mbus_code	= V4L2_MBUS_FMT_YUYV10_2X10,
#endif
		.colorspace	 = V4L2_COLORSPACE_BT878,
	}
};

#define N_OV6211_FMTS ARRAY_SIZE(ov6211_formats)

int ov6211_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};
	int ret = -1;
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0) {
		ret = 0;
	}
	return ret;
}

int ov6211_write(struct v4l2_subdev *sd, unsigned short reg, unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};
	int ret = 0;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	return ret;
}

static int ov6211_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SENSOR_REG_END) {
		if (vals->reg_num == SENSOR_REG_DELAY) {
			if (vals->value >= (1000 / HZ)) {
				msleep(vals->value);
			} else {
				mdelay(vals->value);
			}
		} else {
			ret = ov6211_read(sd, vals->reg_num, &val);
			if (ret < 0) {
				return ret;
			}
		}
		vals++;
	}
	ov6211_write(sd, vals->reg_num, vals->value);
	return 0;
}

static int ov6211_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SENSOR_REG_END) {
		if (vals->reg_num == SENSOR_REG_DELAY) {
			if (vals->value >= (1000 / HZ)) {
				msleep(vals->value);
			} else {
				mdelay(vals->value);
			}
		} else {
			ret = ov6211_write(sd, vals->reg_num, vals->value);
			if (ret < 0) {
				return ret;
			}
		}
		vals++;
	}
	return 0;
}


static int ov6211_s_power(struct v4l2_subdev *sd, int on)
{
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);
	info->write_mode = OV6211_DEFAULT_REG_WRTIE_MODE;

	return 0;
}

static int ov6211_reset(struct v4l2_subdev *sd, u32 val)
{
#if 0
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);

	ret = ov6211_write_array(sd, ov6211_softreset);
	if (ret < 0) {
		SENSOR_PRINT(CAMERA_WARNING,"ov6211 failed to reset, return %d\n",ret);
		return ret;
	}
#endif
	return 0;
}

static int ov6211_init(struct v4l2_subdev *sd, u32 val)
{
	int ret = -1;

	ret = ov6211_write_array(sd, ov6211_softreset);
	if (ret < 0) {
		SENSOR_PRINT(CAMERA_ERROR, "ov6211 failed to reset, return %d\n",ret);
		return ret;
	}
	/* chose init setting */
	ret = ov6211_write_array(sd, ov6211_init_default);
	if (ret < 0) {
		SENSOR_PRINT(CAMERA_ERROR, "ov6211 failed to set common reg value, return %d\n",ret);
		return ret;
	}
	return 0;
}

static int ov6211_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index, enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_OV6211_FMTS)
		return -EINVAL;

	*code = ov6211_formats[index].mbus_code;
	return 0;
}

static int ov6211_try_fmt_internal(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt,
				   struct sensor_win_setting **ret_wsize)
{
	struct sensor_win_setting *wsize;

	if (fmt->width > OV6211_MAX_WIDTH || fmt->height > OV6211_MAX_HEIGHT)
		return -EINVAL;
	for (wsize = ov6211_win_sizes; wsize < ov6211_win_sizes + OV6211_N_WIN_SIZES; wsize++)
		if (fmt->width > wsize->width && fmt->height > wsize->height)
			break;
	wsize--;   /* Take the smallest one */
	if (fmt->priv == V4L2_CAP_LOWFPS_MODE && wsize->low_fps_win != NULL) {
		wsize = wsize->low_fps_win;
	}
	if ((!(fmt->priv & V4L2_CAP_QUITAK_MODE)) && (wsize->width == 800 && wsize->height == 600)
	    && (fmt->width != 0 && fmt->height != 0)) {
		wsize--;
	}
	if (ret_wsize != NULL) {
		*ret_wsize = wsize;
	}
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	fmt->code = wsize->mbus_code;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = wsize->colorspace;
	return 0;
}

static int ov6211_g_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(fmt);
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);

	data->i2cflags = V4L2_I2C_ADDR_16BIT;
	data->sensor_sys_pclk = info->trying_win->sysclk;
	data->vts = info->trying_win->vts;
	data->hts = info->trying_win->hts;
	data->lans = 1;

	return 0;
}

static int ov6211_try_mbus_fmt(struct v4l2_subdev *sd,
			       struct v4l2_mbus_framefmt *fmt)
{
	int ret = 0;
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);
	ret = ov6211_try_fmt_internal(sd, fmt, &info->trying_win);
	SENSOR_PRINT(CAMERA_INFO, "info->trying_win->width = %d, info->trying_win->height = %d,\n", 
		     info->trying_win->width, info->trying_win->height);
	return ret;
}

static int ov6211_s_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);
	int ret = -1;
	if (info->using_win != info->trying_win) {
		if (info->write_mode == GROUP_LUNCH) {
			ret = ov6211_write_array(sd, info->trying_win->lunch_regs);
		} else {
			ret = ov6211_write_array(sd, info->trying_win->regs);
		}
		if (ret)
			return ret;
	}
	info->using_win = info->trying_win;
	ov6211_g_mbus_fmt(sd, fmt);

	return 0;
}

static int ov6211_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);

	if (enable) {
		ret = ov6211_write_array(sd, ov6211_stream_on);
		if (ret < 0) {
			SENSOR_PRINT(CAMERA_ERROR,"ov6211 stream on failed ret = %d\n",ret);
		} else {
			SENSOR_PRINT(CAMERA_INFO,"ov6211 stream on\n");
		}
	} else {
		ret = ov6211_write_array(sd, ov6211_stream_off);
		if (ret < 0) {
			SENSOR_PRINT(CAMERA_ERROR,"ov6211 stream off failed ret = %d\n",ret);
		} else {
			SENSOR_PRINT(CAMERA_INFO,"ov6211 stream off\n");
		}
	}
	return ret;
}

static int ov6211_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov6211_enum_frameintervals(struct v4l2_subdev *sd, struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(ov6211_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov6211_frame_rates[interval->index];
	return 0;
}

static int ov6211_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	__u32 index = fsize->index;
	/*
	 * If a minimum width/height was requested, filter out the capture
	 * windows that fall outside that.
	 */
	if (index < OV6211_N_WIN_SIZES) {
		struct sensor_win_setting *win = &ov6211_win_sizes[index];
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = win->width;
		fsize->discrete.height = win->height;
		return 0;
	} else {
		return -EINVAL;
	}

	return -EINVAL;
}

static int ov6211_write_testself_mode(struct v4l2_subdev *sd, struct regval_list *vals,
				      unsigned char config_value)
{
	int ret = -1;
	while (vals->reg_num !=SENSOR_REG_END) {
		if (vals->reg_num == SENSOR_REG_TESTMODE) {
			ret = ov6211_write(sd,vals->reg_num,config_value);
			SENSOR_PRINT(CAMERA_INFO,"config_value is %d\n",config_value);
			if (ret < 0)
				return -1;
			ov6211_read(sd, vals->reg_num, &config_value);
			SENSOR_PRINT(CAMERA_INFO," This is %d\n",config_value);
		}
		vals++;
	}
	return 0;
}

static int ov6211_s_ctrl_1(struct v4l2_subdev *sd, struct v4l2_control *ctrl, unsigned char select_mode)
{
        int ret = -1;
        if (select_mode == TURN_OVSENSOR_TESTMODE) {
		ret = ov6211_write_testself_mode(sd,ov6211_init_default,SENSOR_TESTMODE_VALUE);
		if (ret < 0) {
			SENSOR_PRINT(CAMERA_ERROR,"functiong:%s, line:%d failed\n", __func__, __LINE__);
			return ret;
		}
        }
        if (select_mode == TURN_OVSENSOR_NORMALMODE) {
		ret = ov6211_write_testself_mode(sd,ov6211_init_default,SENSOR_NORMALMODE_VALUE);
		if (ret < 0) {
			SENSOR_PRINT(CAMERA_ERROR, "functiong:%s, line:%d failed\n", __func__, __LINE__);
			return ret;
		}
	}
	return 0;
}

static int ov6211_soft_reset(struct v4l2_subdev *sd, int cmd)
{
	int ret = -1;

	ret = ov6211_write_array(sd, ov6211_softreset);
	if (ret < 0) {
		goto err;
	}
	ret = ov6211_write_array(sd, ov6211_init_default);
	if (ret < 0) {
		goto err;
	}
	ret = ov6211_write_array(sd, ov6211_400_400_50fps);
	if (ret < 0) {
		goto err;
	}
#ifndef OV6211_SETTING_PROFILE_2
	ret = ov6211_write_array(sd, ov6211_stream_on);
#endif
 err:
	if (ret < 0) {
		SENSOR_PRINT(CAMERA_ERROR, "%s : reset Senor failed!!!");
	}

	return ret;
}
