/* arch/arm/mach-msm/qdsp5/snd.c
 *
 * interface to "snd" service on the baseband cpu
 *
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/msm_audio.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <asm/ioctls.h>
#include <mach/board.h>
#include <mach/msm_rpcrouter.h>
#include <mach/debug_mm.h>

#if defined (CONFIG_MACH_MSM7X27A_M3EU)
#include "../lge/board-m3eu.h"

static int fm_enable;
#endif

#if defined (CONFIG_MACH_MSM7X25A_E0EU)
#include "../lge/board-e0eu.h"

static int fm_enable;
#endif

#if 0 //tamedwolph unnecessary
//#if defined (CONFIG_MACH_MSM7X27A_M3EU) || defined (CONFIG_MACH_MSM7X27A_M3MPCS)
extern int headset_state;
#endif

struct snd_ctxt {
	struct mutex lock;
	int opened;
	struct msm_rpc_endpoint *ept;
	struct msm_snd_endpoints *snd_epts;
};

struct snd_sys_ctxt {
	struct mutex lock;
	struct msm_rpc_endpoint *ept;
};

static struct snd_sys_ctxt the_snd_sys;

static struct snd_ctxt the_snd;

#define RPC_SND_PROG	0x30000002
#define RPC_SND_CB_PROG	0x31000002

#define RPC_SND_VERS                    0x00020001
#define RPC_SND_VERS2                    0x00030001

#define SND_SET_DEVICE_PROC 2
#define SND_SET_VOLUME_PROC 3
#define SND_AVC_CTL_PROC 29
#define SND_AGC_CTL_PROC 30

#if defined (CONFIG_MACH_LGE)
#define SND_SET_LOOPBACK_MODE_PROC 61
#define SND_SET_RX_VOLUME_PROC 65
#define SND_SET_DTMF_VOLUME_PROC 66
#endif

struct rpc_snd_set_device_args {
	uint32_t device;
	uint32_t ear_mute;
	uint32_t mic_mute;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_set_volume_args {
	uint32_t device;
	uint32_t method;
	uint32_t volume;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_avc_ctl_args {
	uint32_t avc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_agc_ctl_args {
	uint32_t agc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_device_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_device_args args;
};

struct snd_set_volume_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_volume_args args;
};

struct snd_avc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_avc_ctl_args args;
};

struct snd_agc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_agc_ctl_args args;
};

#if defined (CONFIG_MACH_LGE)
struct snd_set_loopback_param_rep {
	struct rpc_reply_hdr hdr;
	uint32_t get_mode;
} lrep;

struct rpc_snd_set_loopback_mode_args {
	uint32_t mode;
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_loopback_mode_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_loopback_mode_args args;
};

struct snd_set_rxvol_param_rep {
	struct rpc_reply_hdr hdr;
	uint32_t get_rxvol;
} rrep;

struct rpc_snd_set_rx_volume_param_args {
	uint32_t device;
	uint32_t method;
	uint32_t idx;
	int get_flag;	/* get_flag = 0 for set, get_flag = 1 for get */
	int32_t param_val;

	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_rx_volume_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_rx_volume_param_args args;
};

struct snd_set_dtmfvol_param_rep {
	struct rpc_reply_hdr hdr;
	uint32_t get_dtmfvol;
} frep;

struct rpc_snd_set_dtmf_volume_param_args {
	uint32_t device;
	uint32_t method;
	uint32_t idx;
	int get_flag;	/* get_flag = 0 for set, get_flag = 1 for get */
	int32_t param_val;

	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_dtmf_volume_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_dtmf_volume_param_args args;
};
#endif

struct snd_endpoint *get_snd_endpoints(int *size);

static inline int check_mute(int mute)
{
	return (mute == SND_MUTE_MUTED ||
		mute == SND_MUTE_UNMUTED) ? 0 : -EINVAL;
}

static int get_endpoint(struct snd_ctxt *snd, unsigned long arg)
{
	int rc = 0, index;
	struct msm_snd_endpoint ept;

	if (copy_from_user(&ept, (void __user *)arg, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid read pointer\n");
		return -EFAULT;
	}

	index = ept.id;
	if (index < 0 || index >= snd->snd_epts->num) {
		MM_ERR("snd_ioctl get endpoint: invalid index!\n");
		return -EINVAL;
	}

	ept.id = snd->snd_epts->endpoints[index].id;
	strncpy(ept.name,
		snd->snd_epts->endpoints[index].name,
		sizeof(ept.name));

	if (copy_to_user((void __user *)arg, &ept, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid write pointer\n");
		rc = -EFAULT;
	}

	return rc;
}

static long snd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct snd_set_device_msg dmsg;
	struct snd_set_volume_msg vmsg;
	struct snd_avc_ctl_msg avc_msg;
	struct snd_agc_ctl_msg agc_msg;

	struct msm_snd_device_config dev;
	struct msm_snd_volume_config vol;
	struct snd_ctxt *snd = file->private_data;

#if defined (CONFIG_MACH_LGE)
	struct msm_snd_set_loopback_mode_param loopback;
	struct snd_set_loopback_mode_msg lbmsg;
	struct msm_snd_set_rx_volume_param rxvol;
	struct snd_set_rx_volume_param_msg rmsg;
	struct msm_snd_set_dtmf_volume_param dtmfvol;
	struct snd_set_dtmf_volume_param_msg fmsg;
#endif

	int rc = 0;

	uint32_t avc, agc;

	mutex_lock(&snd->lock);
	switch (cmd) {
	case SND_SET_DEVICE:
		if (copy_from_user(&dev, (void __user *) arg, sizeof(dev))) {
			MM_ERR("set device: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

#if 0 //tamedwolph unnecessary

//#if defined (CONFIG_MACH_MSM7X27A_M3EU) || defined (CONFIG_MACH_MSM7X27A_M3MPCS)
		if (headset_state == SW_HEADPHONE_INSERT) {
			if (dev.device == 21)
				dev.device = 22;
		}
#endif
		dmsg.args.device = cpu_to_be32(dev.device);
		dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
		dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
		if (check_mute(dev.ear_mute) < 0 ||
				check_mute(dev.mic_mute) < 0) {
			MM_ERR("set device: invalid mute status\n");
			rc = -EINVAL;
			break;
		}
		dmsg.args.cb_func = -1;
		dmsg.args.client_data = 0;

		MM_INFO("snd_set_device %d %d %d\n", dev.device,
				dev.ear_mute, dev.mic_mute);

#if defined (CONFIG_MACH_MSM7X27A_M3EU) || defined (CONFIG_MACH_MSM7X25A_E0EU)
		if (dev.device == 10 || dev.device == 11)
			fm_enable = 1;
		else
			fm_enable = 0;
#endif

		rc = msm_rpc_call(snd->ept,
			SND_SET_DEVICE_PROC,
			&dmsg, sizeof(dmsg), 5 * HZ);
		break;

	case SND_SET_VOLUME:
		if (copy_from_user(&vol, (void __user *) arg, sizeof(vol))) {
			MM_ERR("set volume: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		vmsg.args.device = cpu_to_be32(vol.device);
		vmsg.args.method = cpu_to_be32(vol.method);
		if (vol.method != SND_METHOD_VOICE) {
			MM_ERR("set volume: invalid method\n");
			rc = -EINVAL;
			break;
		}

		vmsg.args.volume = cpu_to_be32(vol.volume);
		vmsg.args.cb_func = -1;
		vmsg.args.client_data = 0;

		MM_INFO("snd_set_volume %d %d %d\n", vol.device,
				vol.method, vol.volume);

		rc = msm_rpc_call(snd->ept,
			SND_SET_VOLUME_PROC,
			&vmsg, sizeof(vmsg), 5 * HZ);
		break;

	case SND_AVC_CTL:
		if (get_user(avc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((avc != 1) && (avc != 0)) {
			rc = -EINVAL;
			break;
		}

		avc_msg.args.avc_ctl = cpu_to_be32(avc);
		avc_msg.args.cb_func = -1;
		avc_msg.args.client_data = 0;

		MM_INFO("snd_avc_ctl %d\n", avc);

		rc = msm_rpc_call(snd->ept,
			SND_AVC_CTL_PROC,
			&avc_msg, sizeof(avc_msg), 5 * HZ);
		break;

	case SND_AGC_CTL:
		if (get_user(agc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((agc != 1) && (agc != 0)) {
			rc = -EINVAL;
			break;
		}
		agc_msg.args.agc_ctl = cpu_to_be32(agc);
		agc_msg.args.cb_func = -1;
		agc_msg.args.client_data = 0;

		MM_INFO("snd_agc_ctl %d\n", agc);

		rc = msm_rpc_call(snd->ept,
			SND_AGC_CTL_PROC,
			&agc_msg, sizeof(agc_msg), 5 * HZ);
		break;

	case SND_GET_NUM_ENDPOINTS:
		if (copy_to_user((void __user *)arg,
				&snd->snd_epts->num, sizeof(unsigned))) {
			MM_ERR("get endpoint: invalid pointer\n");
			rc = -EFAULT;
		}
		break;

	case SND_GET_ENDPOINT:
		rc = get_endpoint(snd, arg);
		break;

#if defined (CONFIG_MACH_LGE)
	case SND_SET_LOOPBACK_MODE:
		if (copy_from_user(&loopback, (void __user *) arg, sizeof(loopback))) {
			pr_err("snd_ioctl set_loopback_mode: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}

		lbmsg.args.mode = cpu_to_be32(loopback.mode);
		lbmsg.args.cb_func = -1;
		lbmsg.args.client_data = 0;


		pr_info("set_loopback_mode %d \n", loopback.mode);

		rc = msm_rpc_call(snd->ept,
			SND_SET_LOOPBACK_MODE_PROC,
			&lbmsg, sizeof(lbmsg), 5 * HZ);

		if (rc < 0) {
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		} else {
			loopback.get_param = be32_to_cpu(lrep.get_mode);
			printk(KERN_INFO "%s:loopback mode ->%d\n", __func__, loopback.get_param);
			if (copy_to_user((void __user *)arg, &loopback, sizeof(loopback))) {
				pr_err("snd_ioctl get loopback mode: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
	break;

	case SND_SET_RX_VOLUME:
		if (copy_from_user(&rxvol, (void __user *) arg, sizeof(rxvol))) {
			pr_err("snd_ioctl set_rx_volume: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		rmsg.args.device = cpu_to_be32(rxvol.device);
		rmsg.args.method = cpu_to_be32(rxvol.method);
		rmsg.args.idx = cpu_to_be32(rxvol.idx);
		rmsg.args.get_flag = cpu_to_be32(rxvol.get_flag);
		rmsg.args.param_val = cpu_to_be32(rxvol.param_val);
		rmsg.args.cb_func = -1;
		rmsg.args.client_data = 0;
		pr_info("set_rx_volume %d %d %d %d\n", rxvol.device,
						 rxvol.method, rxvol.idx, rxvol.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_RX_VOLUME_PROC,
			&rmsg, sizeof(rmsg), &rrep, sizeof(rrep), 5 * HZ);
		if (rc < 0)
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		else {
			rxvol.get_param = be32_to_cpu(rrep.get_rxvol);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, rxvol.get_param);
			if (copy_to_user((void __user *)arg, &rxvol, sizeof(rxvol))) {
				pr_err("snd_ioctl get rx vol: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;

	case SND_SET_DTMF_VOLUME:
		if (copy_from_user(&dtmfvol, (void __user *) arg, sizeof(dtmfvol))) {
			pr_err("snd_ioctl set_dtmf_volume: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		fmsg.args.device = cpu_to_be32(dtmfvol.device);
		fmsg.args.method = cpu_to_be32(dtmfvol.method);
		fmsg.args.idx = cpu_to_be32(dtmfvol.idx);
		fmsg.args.get_flag = cpu_to_be32(dtmfvol.get_flag);
		fmsg.args.param_val = cpu_to_be32(dtmfvol.param_val);
		fmsg.args.cb_func = -1;
		fmsg.args.client_data = 0;
		pr_info("set_dtmf_volume %d %d %d %d\n", dtmfvol.device,
				dtmfvol.method, dtmfvol.idx, dtmfvol.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_DTMF_VOLUME_PROC,
			&fmsg, sizeof(fmsg), &frep, sizeof(frep), 5 * HZ);
		if (rc < 0)
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		else {
			dtmfvol.get_param = be32_to_cpu(frep.get_dtmfvol);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, dtmfvol.get_param);
			if (copy_to_user((void __user *)arg, &dtmfvol, sizeof(dtmfvol))) {
				pr_err("snd_ioctl get dtmf vol: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
#endif

	default:
		MM_ERR("unknown command\n");
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&snd->lock);

	return rc;
}

static int snd_release(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = file->private_data;
	int rc;

	mutex_lock(&snd->lock);
	rc = msm_rpc_close(snd->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd->ept = NULL;
	snd->opened = 0;
	mutex_unlock(&snd->lock);
	return 0;
}
static int snd_sys_release(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	rc = msm_rpc_close(snd_sys->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd_sys->ept = NULL;
	mutex_unlock(&snd_sys->lock);
	return rc;
}
static int snd_open(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = &the_snd;
	int rc = 0;

	mutex_lock(&snd->lock);
	if (snd->opened == 0) {
		if (snd->ept == NULL) {
			snd->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS, 0);
			if (IS_ERR(snd->ept)) {
				MM_DBG("connect failed with current VERS \
					= %x, trying again with another API\n",
					RPC_SND_VERS2);
				snd->ept =
					msm_rpc_connect_compatible(RPC_SND_PROG,
							RPC_SND_VERS2, 0);
			}
			if (IS_ERR(snd->ept)) {
				rc = PTR_ERR(snd->ept);
				snd->ept = NULL;
				MM_ERR("failed to connect snd svc\n");
				goto err;
			}
		}
		file->private_data = snd;
		snd->opened = 1;
	} else {
		MM_ERR("snd already opened\n");
		rc = -EBUSY;
	}

err:
	mutex_unlock(&snd->lock);
	return rc;
}
static int snd_sys_open(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	if (snd_sys->ept == NULL) {
		snd_sys->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
			RPC_SND_VERS, 0);
		if (IS_ERR(snd_sys->ept)) {
			MM_DBG("connect failed with current VERS \
				= %x, trying again with another API\n",
				RPC_SND_VERS2);
			snd_sys->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS2, 0);
		}
		if (IS_ERR(snd_sys->ept)) {
			rc = PTR_ERR(snd_sys->ept);
			snd_sys->ept = NULL;
			MM_ERR("failed to connect snd svc\n");
			goto err;
		}
	} else
		MM_DBG("snd already opened\n");

err:
	mutex_unlock(&snd_sys->lock);
	return rc;
}

static struct file_operations snd_fops = {
	.owner		= THIS_MODULE,
	.open		= snd_open,
	.release	= snd_release,
	.unlocked_ioctl	= snd_ioctl,
};

struct miscdevice snd_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_snd",
	.fops	= &snd_fops,
};

static long snd_agc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_agc_ctl_msg agc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	agc_msg.args.agc_ctl = cpu_to_be32(arg);
	agc_msg.args.cb_func = -1;
	agc_msg.args.client_data = 0;

	MM_DBG("snd_agc_ctl %ld,%d\n", arg, agc_msg.args.agc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AGC_CTL_PROC,
		&agc_msg, sizeof(agc_msg), 5 * HZ);
	return rc;
}

static long snd_avc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_avc_ctl_msg avc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	avc_msg.args.avc_ctl = cpu_to_be32(arg);

	avc_msg.args.cb_func = -1;
	avc_msg.args.client_data = 0;

	MM_DBG("snd_avc_ctl %ld,%d\n", arg, avc_msg.args.avc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AVC_CTL_PROC,
		&avc_msg, sizeof(avc_msg), 5 * HZ);
	return rc;
}

static ssize_t snd_agc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_agc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_agc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_avc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_avc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_avc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static long snd_vol_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_volume_msg vmsg;
	struct msm_snd_volume_config vol;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &vol.device, &vol.method, &vol.volume);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <method> \
				<volume>\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.device = cpu_to_be32(vol.device);
	vmsg.args.method = cpu_to_be32(vol.method);
	if (vol.method != SND_METHOD_VOICE) {
		MM_ERR("snd_ioctl set volume: invalid method\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.volume = cpu_to_be32(vol.volume);
	vmsg.args.cb_func = -1;
	vmsg.args.client_data = 0;

	MM_DBG("snd_set_volume %d %d %d\n", vol.device, vol.method,
			vol.volume);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_VOLUME_PROC,
		&vmsg, sizeof(vmsg), 5 * HZ);
	return rc;
}

static long snd_dev_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_device_msg dmsg;
	struct msm_snd_device_config dev;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &dev.device, &dev.ear_mute, &dev.mic_mute);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <ear_mute> \
				<mic_mute>\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.device = cpu_to_be32(dev.device);
	dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
	dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
	if (check_mute(dev.ear_mute) < 0 ||
			check_mute(dev.mic_mute) < 0) {
		MM_ERR("snd_ioctl set device: invalid mute status\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.cb_func = -1;
	dmsg.args.client_data = 0;

	MM_INFO("snd_set_device %d %d %d\n", dev.device, dev.ear_mute,
			dev.mic_mute);

#if defined (CONFIG_MACH_MSM7X27A_M3EU) || defined (CONFIG_MACH_MSM7X25A_E0EU)
	if (dev.device == 10 || dev.device == 11)
		fm_enable = 1;
	else
		fm_enable = 0;
#endif
	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_DEVICE_PROC,
		&dmsg, sizeof(dmsg), 5 * HZ);
	return rc;
}

#if defined (CONFIG_MACH_MSM7X27A_M3EU) || defined (CONFIG_MACH_MSM7X25A_E0EU)
void snd_fm_vol_mute()
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return;

	mutex_lock(&snd_sys->lock);

	if (!fm_enable) {
		mutex_unlock(&snd_sys->lock);
		return;
	}

	snd_vol_enable("31 0 0");
	mutex_unlock(&snd_sys->lock);

	snd_sys_release();
}
#endif

static ssize_t snd_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_dev_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_vol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_vol_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static DEVICE_ATTR(agc, S_IWUSR | S_IRUGO,
		NULL, snd_agc_store);

static DEVICE_ATTR(avc, S_IWUSR | S_IRUGO,
		NULL, snd_avc_store);

static DEVICE_ATTR(device, S_IWUSR | S_IRUGO,
		NULL, snd_dev_store);

static DEVICE_ATTR(volume, S_IWUSR | S_IRUGO,
		NULL, snd_vol_store);

static int snd_probe(struct platform_device *pdev)
{
	struct snd_ctxt *snd = &the_snd;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

#if defined (CONFIG_MACH_MSM7X27A_M3EU) || defined (CONFIG_MACH_MSM7X25A_E0EU)
	fm_enable = 0;
#endif
	mutex_init(&snd->lock);
	mutex_init(&snd_sys->lock);
	snd_sys->ept = NULL;
	snd->snd_epts = (struct msm_snd_endpoints *)pdev->dev.platform_data;
	rc = misc_register(&snd_misc);
	if (rc)
		return rc;

	rc = device_create_file(snd_misc.this_device, &dev_attr_agc);
	if (rc) {
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_avc);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_device);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_volume);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_device);
		misc_deregister(&snd_misc);
	}

	return rc;
}

static struct platform_driver snd_plat_driver = {
	.probe = snd_probe,
	.driver = {
		.name = "msm_snd",
		.owner = THIS_MODULE,
	},
};

static int __init snd_init(void)
{
	return platform_driver_register(&snd_plat_driver);
}

module_init(snd_init);
