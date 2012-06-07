#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/err.h>

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/board.h>

#include "board-m3eu.h"

#include <mach/board_lge.h>

//LGE_CHANGE_S[shawn.park@lge.com] 2011-09-22
#define SMS2130_RESET 78
#define SMS2130_1_2V_EN  85
#define SMS2130_1_8V_EN  77
//LGE_CHANGE_E[shawn.park@lge.com] 2011-09-22

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

static unsigned long vreg_sts, gpio_sts;
static struct vreg *vreg_mmc;
static struct vreg *vreg_emmc;

struct sdcc_vreg {
	struct vreg *vreg_data;
	unsigned level;
};

static struct sdcc_vreg sdcc_vreg_data[4];

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};
static void sdcc_gpio_init(void)
{
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	int rc = 0;

	if (gpio_request(GPIO_SD_DETECT_N, "sdc1_status_irq"))
		pr_err("failed to request gpio sdc1_status_irq\n");

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SD_DETECT_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
									GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc)
		printk(KERN_ERR "%s: Failed to configure GPIO %d\n",
					__func__, rc);
/*LGE_CHANGE_S[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */
	if(lge_bd_rev == EVB )
	{
		gpio_tlmm_config(GPIO_CFG(SMS2130_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_10MA), GPIO_CFG_ENABLE ) ;
		gpio_tlmm_config(GPIO_CFG(SMS2130_1_8V_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE ) ;
		gpio_tlmm_config(GPIO_CFG(SMS2130_1_2V_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE ) ;

		gpio_set_value(SMS2130_1_8V_EN,1);
		gpio_set_value(SMS2130_RESET,0);
		gpio_set_value(SMS2130_1_2V_EN,0);
	}
	else if(lge_bd_rev == LGE_REV_A)
	{
		gpio_tlmm_config(GPIO_CFG(77, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_10MA), GPIO_CFG_ENABLE ) ;
		gpio_tlmm_config(GPIO_CFG(SMS2130_1_2V_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE ) ;

		gpio_set_value(77,0);
		gpio_set_value(SMS2130_1_2V_EN,0);		
	}
	else //rev B
	{
		gpio_tlmm_config(GPIO_CFG(77, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_10MA), GPIO_CFG_ENABLE ) ;
		gpio_tlmm_config(GPIO_CFG(SMS2130_1_2V_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE ) ;

		gpio_set_value(77,0);
		gpio_set_value(SMS2130_1_2V_EN,0);		
	}

/*LGE_CHANGE_E[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */

#endif
}

static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc1_clk"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(62, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc2_clk"},
	{GPIO_CFG(63, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_cmd"},
	{GPIO_CFG(64, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_3"},
	{GPIO_CFG(65, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_2"},
	{GPIO_CFG(66, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_1"},
	{GPIO_CFG(67, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc2_dat_0"},
};

static struct msm_gpio sdc2_sleep_cfg_data[] = {
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc2_clk"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc2_cmd"},
	{GPIO_CFG(64, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc2_dat_3"},
	{GPIO_CFG(65, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc2_dat_2"},
	{GPIO_CFG(66, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc2_dat_1"},
	{GPIO_CFG(67, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc2_dat_0"},
};
static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(88, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc3_clk"},
	{GPIO_CFG(89, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_cmd"},
	{GPIO_CFG(90, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_3"},
	{GPIO_CFG(91, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_2"},
	{GPIO_CFG(92, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_1"},
	{GPIO_CFG(93, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_0"},
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	{GPIO_CFG(19, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_7"},
	{GPIO_CFG(20, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_6"},
	{GPIO_CFG(21, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_5"},
	{GPIO_CFG(108, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_4"},
#endif
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(19, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc4_dat_3"},
	{GPIO_CFG(20, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc4_dat_2"},
	{GPIO_CFG(21, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc4_dat_1"},
	{GPIO_CFG(107, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc4_cmd"},
	{GPIO_CFG(108, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc4_dat_0"},
	{GPIO_CFG(109, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc4_clk"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = sdc2_sleep_cfg_data,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
	},
};

static int msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];
	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);	
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if(dev_id == 4 || dev_id == 3)
		{	
			printk("msm_sdcc_setup_vreg, dev_id = %d,enable=%d\n",dev_id,enable);
			return 0;
		}		
		if (rc)
			pr_err("%s: Failed to turn on GPIOs for slot %d\n",
					__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		
		if(dev_id == 4 || dev_id == 3)
		{	
			printk("msm_sdcc_setup_vreg, dev_id = %d,enable=%d\n",dev_id,enable);
			return rc;	
		}
		if (curr->sleep_cfg_data) {
			rc = msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);	
			return rc;
		}
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}
	return rc;
}

static int msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_vreg *curr;

	curr = &sdcc_vreg_data[dev_id - 1];

	if (!(test_bit(dev_id, &vreg_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &vreg_sts);
		rc = vreg_set_level(curr->vreg_data, curr->level);
		if (rc)
			pr_err("%s: vreg_set_level() = %d\n", __func__, rc);

		rc = vreg_enable(curr->vreg_data);
		if (rc)
			pr_err("%s: vreg_enable() = %d\n", __func__, rc);
	} else {
		clear_bit(dev_id, &vreg_sts);
		rc = vreg_disable(curr->vreg_data);
		if (rc)
			pr_err("%s: vreg_disable() = %d\n", __func__, rc);
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;
	static int first_setup = 1;

	pdev = container_of(dv, struct platform_device, dev);

	rc = msm_sdcc_setup_gpio(pdev->id, !!vdd);
	if (rc)
		goto out;

	rc = msm_sdcc_setup_vreg(pdev->id, !!vdd);

			/* if first called related to sdcc1, irq should be registered as wakeup source
	 *      * cleaneye.kim@lge.com, 2010-02-19
	 *           */
	if(vdd && first_setup)
	{
		struct mmc_platform_data *pdata = pdev->dev.platform_data;
		if (pdev->id == 1) {
			first_setup = 0;
			set_irq_wake(pdata->status_irq, 1);
		}
	}
out:
	return rc;
}

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT) \
	&& defined(CONFIG_MMC_MSM_CARD_HW_DETECTION)
static unsigned int msm7x2xa_sdcc_slot_status(struct device *dev)
{
/* FIXME : temporary disable function
	int status;

	status = gpio_tlmm_config(GPIO_CFG(GPIO_SDC1_HW_DET, 2, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (status)
		pr_err("%s:Failed to configure tlmm for GPIO %d\n", __func__,
				GPIO_SDC1_HW_DET);

	status = gpio_request(GPIO_SDC1_HW_DET, "SD_HW_Detect");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n", __func__,
				GPIO_SDC1_HW_DET);
	} else {
		status = gpio_direction_input(GPIO_SDC1_HW_DET);
		if (!status)
			status = gpio_get_value(GPIO_SDC1_HW_DET);
		gpio_free(GPIO_SDC1_HW_DET);
	}
	return status;
*/
	return !gpio_get_value(GPIO_SD_DETECT_N);
}
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct mmc_platform_data sdc1_plat_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7x2xa_sdcc_slot_status,
	.status_irq  = MSM_GPIO_TO_INT(GPIO_SD_DETECT_N),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
/* FIXME : M3EU evb GPIO 66 WLAN SDIO PIN */
static struct mmc_platform_data sdc2_plat_data = {
	/*
	 * SDC2 supports only 1.8V, claim for 2.85V range is just
	 * for allowing buggy cards who advertise 2.8V even though
	 * they can operate at 1.8V supply.
	 */
	.ocr_mask	= MMC_VDD_28_29 | MMC_VDD_165_195,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
	.sdiowakeup_irq = MSM_GPIO_TO_INT(66),
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data sdc3_plat_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
		&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))

/*LGE_CHANGE_S[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */
static unsigned int sms2130_sdcc_slot_status(struct device *dev)
{
  unsigned int ret1;
  	if(lge_bd_rev == EVB)
  		ret1=gpio_get_value(78);
	else if(lge_bd_rev == LGE_REV_A)
		ret1=gpio_get_value(77);
	else
		ret1=gpio_get_value(77);
  printk("[Shawn]%s\tReset[%d], board=%d\n", __FUNCTION__,ret1,lge_bd_rev);
  return ret1;
}

static struct mmc_platform_data sms2130_sdcc_data = {
	.ocr_mask       = MMC_VDD_28_29,/*MMC_VDD_30_31*//*MMC_VDD_28_29*/
	.translate_vdd  = msm_sdcc_setup_power,
	.status         = sms2130_sdcc_slot_status,
	.status_irq	  = MSM_GPIO_TO_INT(SMS2130_RESET),
	.irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	//5MHz
	.msmsdcc_fmid  = 400000,
	.msmsdcc_fmax  = 5000000,
	//16MHz
	//.msmsdcc_fmid  = 400000,
	//.msmsdcc_fmax  = 16000000,
	//17mHz
	//.msmsdcc_fmid  = 400000,
	//.msmsdcc_fmax  = 17000000,	
	//20Mhz
	//.msmsdcc_fmid  = 16000000,
	//.msmsdcc_fmax  = 20000000,	
	//20Mhz
	//.msmsdcc_fmid  = 400000,
	//.msmsdcc_fmax  = 20000000, 
	//25MHz
	//.msmsdcc_fmid	= 16000000,
	//.msmsdcc_fmax	= 24576000,

	.nonremovable	= 0,
};
/*LGE_CHANGE_E[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */
#endif
#endif

static void __init msm7x27a_init_mmc(void)
{
	vreg_emmc = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_emmc)) {
		pr_err("%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_emmc));
		return;
	}

	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		pr_err("%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_mmc));
		return;
	}

	sdcc_gpio_init();

	/* eMMC slot */
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	sdcc_vreg_data[2].vreg_data = vreg_emmc;
	sdcc_vreg_data[2].level = 2850;
	msm_add_sdcc(3, &sdc3_plat_data);
#endif
	/* Micro-SD slot */
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	sdcc_vreg_data[0].vreg_data = vreg_mmc;
	sdcc_vreg_data[0].level = VREG_SD_LEVEL;
	msm_add_sdcc(1, &sdc1_plat_data);
#endif
	/* SDIO WLAN slot */
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	sdcc_vreg_data[1].vreg_data = vreg_mmc;
	sdcc_vreg_data[1].level = 2850;
	msm_add_sdcc(2, &sdc2_plat_data);
#endif

/*LGE_CHANGE_S[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */
#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
			&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))
		sdcc_vreg_data[3].vreg_data = vreg_mmc;
		sdcc_vreg_data[3].level = 2850;

		printk("[shawn]lge_bd_rev=%d\n",lge_bd_rev);
		
		if(lge_bd_rev == EVB)
			sms2130_sdcc_data.status_irq = MSM_GPIO_TO_INT(78);
		else if(lge_bd_rev == LGE_REV_A)
			sms2130_sdcc_data.status_irq = MSM_GPIO_TO_INT(77);
		else //rev B
			sms2130_sdcc_data.status_irq = MSM_GPIO_TO_INT(77);	
		
		msm_add_sdcc(4, &sms2130_sdcc_data);
#endif

/*LGE_CHANGE_E[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */

/*LGE_CHANGE_S[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */
	if(lge_bd_rev == EVB)
		enable_irq(gpio_to_irq(78));
	else if(lge_bd_rev == LGE_REV_A)
		enable_irq(gpio_to_irq(77));
	else //rev B
		enable_irq(gpio_to_irq(77));
/*LGE_CHANGE_E[shawn.park@lge.com] 2011.07.26, SMS2130 For Mobile TV */

}

void __init lge_add_mmc_devices(void)
{
	msm7x27a_init_mmc();
}
