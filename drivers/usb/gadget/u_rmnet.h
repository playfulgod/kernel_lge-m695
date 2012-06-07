/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __U_RMNET_H
#define __U_RMNET_H

#include <linux/usb/composite.h>
#include <linux/platform_device.h>
#include <linux/usb/cdc.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

struct rmnet_ctrl_pkt {
	void			*buf;
	int			len;
	struct list_head	list;
};

struct grmnet {
	struct usb_function		func;

	struct usb_ep			*in;
	struct usb_ep			*out;
	struct usb_endpoint_descriptor	*in_desc;
	struct usb_endpoint_descriptor	*out_desc;

	/* to usb host, aka laptop, windows pc etc. Will
	 * be filled by usb driver of rmnet functionality
	 */
	int (*send_cpkt_response)(struct grmnet *g,
				struct rmnet_ctrl_pkt *pkt);

	/* to modem, and to be filled by driver implementing
	 * control function
	 */
	int (*send_cpkt_request)(struct grmnet *g,
				u8 port_num,
				struct rmnet_ctrl_pkt *pkt);

	void (*send_cbits_tomodem)(struct grmnet *g,
				u8 port_num,
				int cbits);
};

#ifdef CONFIG_USB_ANDROID_RMNET_BAM
int gbam_setup(unsigned int count);
int gbam_connect(struct grmnet *, u8 port_num);
void gbam_disconnect(struct grmnet *, u8 port_num);
#else
static inline int
gbam_setup(unsigned int count) { return 0; }
static inline int
gbam_connect(struct grmnet *, u8 port_num) { return 0; }
static inline void
gbam_disconnect(struct grmnet *, u8 port_num) { };
#endif

#ifdef CONFIG_USB_ANDROID_RMNET_CTRL_SMD
int gsmd_ctrl_connect(struct grmnet *gr, int port_num);
void gsmd_ctrl_disconnect(struct grmnet *gr, u8 port_num);
int gsmd_ctrl_setup(unsigned int count);
#else
static inline int
gsmd_ctrl_connect(struct grmnet *gr, int port_num) { return 0; }
static inline void
gsmd_ctrl_disconnect(struct grmnet *gr, u8 port_num) { }
static inline int
gsmd_ctrl_setup(unsigned int count) { return 0; }
#endif

#endif /* __U_RMNET_H*/
