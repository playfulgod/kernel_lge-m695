#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>

#include <mach/msm_rpcrouter.h>
#include <mach/rpc_pmapp.h>
#include <mach/board.h>

#include "devices.h"
#include "board-m3mpcs.h"
#include <mach/board_lge.h>
#include <mach/vreg.h>

#ifdef CONFIG_MACH_MSM7X27A_M3MPCS_REV_A
#include <linux/regulator/rt8053.h>
#include <linux/regulator/consumer.h>

/* Sub-PMIC */
static struct regulator_consumer_supply rt8053_vreg_supply[] = {
	REGULATOR_SUPPLY("RT8053_LDO1", NULL),
	REGULATOR_SUPPLY("RT8053_LDO2", NULL),
	REGULATOR_SUPPLY("RT8053_LDO3", NULL),
	REGULATOR_SUPPLY("RT8053_LDO4", NULL),
	REGULATOR_SUPPLY("RT8053_LDO5", NULL),
	REGULATOR_SUPPLY("RT8053_DCDC1", NULL),
};

#define RT8053_VREG_INIT(_id, _min_uV, _max_uV) \
	{ \
		.id = RT8053_##_id, \
		.initdata = { \
			.constraints = { \
				.valid_modes_mask = REGULATOR_MODE_NORMAL, \
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
			}, \
			.num_consumer_supplies = 1, \
			.consumer_supplies = \
				&rt8053_vreg_supply[RT8053_##_id], \
		}, \
	}

static struct rt8053_regulator_subdev rt8053_regulators[] = {
	RT8053_VREG_INIT(LDO1, 1200000, 3300000),
	RT8053_VREG_INIT(LDO2, 1200000, 3300000),
	RT8053_VREG_INIT(LDO3, 1200000, 3300000),
	RT8053_VREG_INIT(LDO4,  800000, 2850000),
	RT8053_VREG_INIT(LDO5, 1200000, 3300000),
	RT8053_VREG_INIT(DCDC1, 800000, 2300000),
};

static struct rt8053_platform_data rt8053_data = {
	.num_regulators = 6,
	.enable_gpio = 76,
	.regulators = rt8053_regulators,
};

/* REV_A workaround for camera flash driver */
static struct led_flash_platform_data lm3559_flash_pdata = {
	.gpio_flen = 13,
};
#endif /* CONFIG_MACH_MSM7X27A_M3MPCS_REV_A */

/* backlight device */
static struct gpio_i2c_pin bl_i2c_pin = {
#ifdef CONFIG_MACH_MSM7X27A_M3MPCS_REV_A
	.sda_pin = 123,
	.scl_pin = 122,
#else
	.sda_pin = 112,
	.scl_pin = 111,
#endif
	.reset_pin = 124,
};

static struct i2c_gpio_platform_data bl_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device bl_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &bl_i2c_pdata,
};

static struct lge_backlight_platform_data lm3530bl_data = {
	.gpio = 124,
	.version = 3530,
};

static struct i2c_board_info bl_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("lm3530bl", 0x38),
		.type = "lm3530bl",
		.platform_data = &lm3530bl_data,
	},
#ifdef CONFIG_MACH_MSM7X27A_M3MPCS_REV_A
	[1] = {
		I2C_BOARD_INFO("rt8053", 0x7D),
		.type = "rt8053",
		.platform_data = &rt8053_data,
	},
	[2] = {
		I2C_BOARD_INFO("lm3559", FLASH_I2C_ADDRESS),
		.platform_data = &lm3559_flash_pdata,
	},
#endif /* CONFIG_MACH_MSM7X27A_M3MPCS_REV_A */
};

static struct platform_device mipi_dsi_r61529_panel_device = {
	.name = "mipi_r61529",
	.id = 0,
};

/* input platform device */
static struct platform_device *m3mpcs_panel_devices[] __initdata = {
	&mipi_dsi_r61529_panel_device,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,						/*LCD_VSYNC_P*/
	.mdp_rev = MDP_REV_303,
};

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	int rc = DSI_TWO_LANES;
#if 0
	if (cpu_is_msm7x25a() || cpu_is_msm7x25aa()) {
		rc = DSI_SINGLE_LANE;
		pr_info("DSI Single Lane\n");
	} else {
		pr_info("DSI Two Lanes\n");
	}
#endif
	return rc;
}

#define GPIO_LCD_RESET 125
static int dsi_gpio_initialized;
static int Isfirstbootend;

static int mipi_dsi_panel_power(int on)
{
	int rc = 0;
	struct vreg *vreg_mipi_dsi_v28;

	printk("mipi_dsi_panel_power : %d \n", on);

	if (!dsi_gpio_initialized) {

		/*Resetting LCD Panel*/
		rc = gpio_request(GPIO_LCD_RESET, "lcd_reset");
		if (rc) {
			pr_err("%s: gpio_request GPIO_LCD_RESET failed\n", __func__);
		}
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_LCD_RESET, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: Failed to configure GPIO %d\n",
					__func__, rc);
		}

		dsi_gpio_initialized = 1;
	}

	vreg_mipi_dsi_v28 = vreg_get(0, "emmc");
	if (IS_ERR(vreg_mipi_dsi_v28)) {
		pr_err("%s: vreg_get for emmc failed\n", __func__);
		return PTR_ERR(vreg_mipi_dsi_v28);
	}

	if (on) {
		rc = vreg_set_level(vreg_mipi_dsi_v28, 2800);
		if (rc) {
			pr_err("%s: vreg_set_level failed for mipi_dsi_v28\n", __func__);
			goto vreg_put_dsi_v28;
		}
		rc = vreg_enable(vreg_mipi_dsi_v28);
		if (rc) {
			pr_err("%s: vreg_enable failed for mipi_dsi_v28\n", __func__);
			goto vreg_put_dsi_v28;
		}

		rc = gpio_direction_output(GPIO_LCD_RESET, 1);
		if (rc) {
			pr_err("%s: gpio_direction_output failed for lcd_reset\n", __func__);
			goto vreg_put_dsi_v28;
		}
		if (Isfirstbootend) {
			printk("gpio lcd reset on...\n");
			msleep(10);
			gpio_set_value(GPIO_LCD_RESET, 0);
			msleep(10);
			gpio_set_value(GPIO_LCD_RESET, 1);
		} else {
			Isfirstbootend = 1;
		}
		msleep(10);
	} else {
		rc = vreg_disable(vreg_mipi_dsi_v28);
		if (rc) {
			pr_err("%s: vreg_disable failed for mipi_dsi_v28\n", __func__);
			goto vreg_put_dsi_v28;
		}
	}

vreg_put_dsi_v28:
	vreg_put(vreg_mipi_dsi_v28);

	return rc;
}

#define MDP_303_VSYNC_GPIO 97

#ifdef CONFIG_FB_MSM_MDP303
static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_303_VSYNC_GPIO,
	.dsi_power_save   = mipi_dsi_panel_power,
#ifndef CONFIG_MACH_LGE
	.dsi_client_reset = msm_fb_dsi_client_reset,
#endif
	.get_lane_config = msm_fb_get_lane_config,
};
#endif

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("lcdc", 0);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
}

void __init msm7x27a_m3mpcs_init_i2c_backlight(int bus_num)
{
	bl_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(&bl_i2c_pdata, bl_i2c_pin, &bl_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, bl_i2c_bdinfo, ARRAY_SIZE(bl_i2c_bdinfo));
	platform_device_register(&bl_i2c_device);
}

void __init lge_add_lcd_devices(void)
{
	platform_add_devices(m3mpcs_panel_devices, ARRAY_SIZE(m3mpcs_panel_devices));
	msm_fb_add_devices();
	lge_add_gpio_i2c_device(msm7x27a_m3mpcs_init_i2c_backlight);
}
