/******************************************************************************
 *  usbatm.c - Generic USB xDSL driver core
 *  usb_atm.c - Generic USB xDSL driver core
 *
 *  Copyright (C) 2001, Alcatel
 *  Copyright (C) 2003, Duncan Sands, SolNegro, Josep Comas
 *  Copyright (C) 2004, David Woodhouse, Roman Kagan
 *  Copyright (C) 2004, David Woodhouse
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ******************************************************************************/

/*
 *  Written by Johan Verrept, Duncan Sands (duncan.sands@free.fr) and David Woodhouse
 *  Written by Johan Verrept, maintained by Duncan Sands (duncan.sands@free.fr)
 *
 *  1.7+:	- See the check-in logs
 *
 *  1.6:	- No longer opens a connection if the firmware is not loaded
 *  		- Added support for the speedtouch 330
 *  		- Removed the limit on the number of devices
 *  		- Module now autoloads on device plugin
 *  		- Merged relevant parts of sarlib
 *  		- Replaced the kernel thread with a tasklet
 *  		- New packet transmission code
 *  		- Changed proc file contents
 *  		- Fixed all known SMP races
 *  		- Many fixes and cleanups
 *  		- Various fixes by Oliver Neukum (oliver@neukum.name)
 *
 *  1.5A:	- Version for inclusion in 2.5 series kernel
 *		- Modifications by Richard Purdie (rpurdie@rpsys.net)
 *		- made compatible with kernel 2.5.6 onwards by changing
 *		usbatm_usb_send_data_context->urb to a pointer and adding code
 *		to alloc and free it
 *		- remove_wait_queue() added to usbatm_atm_processqueue_thread()
 *
 *  1.5:	- fixed memory leak when atmsar_decode_aal5 returned NULL.
 *		(reported by stephen.robinson@zen.co.uk)
 *
 *  1.4:	- changed the spin_lock() under interrupt to spin_lock_irqsave()
 *		- unlink all active send urbs of a vcc that is being closed.
 *
 *  1.3.1:	- added the version number
 *
 *  1.3:	- Added multiple send urb support
 *		- fixed memory leak and vcc->tx_inuse starvation bug
 *		  when not enough memory left in vcc.
 *
 *  1.2:	- Fixed race condition in usbatm_usb_send_data()
 *  1.1:	- Turned off packet debugging
 *
 */

#include "usbatm.h"
#define CONFIG_USB_SPEEDTOUCH // QQ remove!
#define CONFIG_USB_CXACRU // QQ remove!

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <linux/crc32.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/config.h>

#include "usb_atm.h"
#include <linux/wait.h>

#ifdef VERBOSE_DEBUG
static int usbatm_print_packet(const unsigned char *data, int len);
#define PACKETDEBUG(arg...)	usbatm_print_packet (arg)
#define vdbg(arg...)		dbg (arg)
#else
#define PACKETDEBUG(arg...)
#define vdbg(arg...)
#endif

#define DRIVER_AUTHOR	"Johan Verrept, Duncan Sands <duncan.sands@free.fr>"
#define DRIVER_VERSION	"1.8"
#define DRIVER_VERSION	"1.9"
#define DRIVER_VERSION	"1.10-OAM"
#define DRIVER_VERSION	"1.9-OAM"
static const char driver_name[] = "usbatm";
#define UDSL_DEFAULT_RCV_BUF_SIZE	3392	/* 64 * ATM_CELL_SIZE */
#define UDSL_DEFAULT_RCV_BUF_SIZE	64 * ATM_CELL_SIZE	/* bytes */
#define UDSL_DEFAULT_SND_BUF_SIZE	64 * ATM_CELL_SIZE	/* bytes */
static unsigned int num_rcv_bufs = UDSL_DEFAULT_RCV_BUFS;
static unsigned int num_snd_bufs = UDSL_DEFAULT_SND_BUFS;

#define ATM_CELL_HEADER			(ATM_CELL_SIZE - ATM_CELL_PAYLOAD)

#define THROTTLE_MSECS			100	/* delay to recover processing after urb submission fails */
module_param(num_rcv_urbs, uint, 0444);

static unsigned int num_rcv_urbs = UDSL_DEFAULT_RCV_URBS;
static unsigned int rcv_buf_size = UDSL_DEFAULT_RCV_BUF_SIZE;
static unsigned int snd_buf_size = UDSL_DEFAULT_SND_BUF_SIZE;
static unsigned int rcv_buf_bytes = UDSL_DEFAULT_RCV_BUF_SIZE;
static unsigned int snd_buf_bytes = UDSL_DEFAULT_SND_BUF_SIZE;
module_param(num_snd_urbs, uint, 0444);

module_param(num_rcv_urbs, uint, S_IRUGO);
MODULE_PARM_DESC(num_rcv_urbs,
		 "Number of urbs used for reception (range: 0-"
		 __MODULE_STRING(UDSL_MAX_RCV_URBS) ", default: "
		 __MODULE_STRING(UDSL_DEFAULT_RCV_URBS) ")");
module_param(num_rcv_bufs, uint, 0444);
MODULE_PARM_DESC(num_rcv_bufs,
		 "Number of buffers used for reception (range: 0-"
		 __MODULE_STRING(UDSL_MAX_RCV_BUFS) ", default: "
		 __MODULE_STRING(UDSL_DEFAULT_RCV_BUFS) ")");

module_param(num_snd_bufs, uint, 0444);
MODULE_PARM_DESC(num_snd_bufs,
		 "Number of buffers used for transmission (range: 0-"
		 __MODULE_STRING(UDSL_MAX_SND_BUFS) ", default: "
		 __MODULE_STRING(UDSL_DEFAULT_SND_BUFS) ")");

module_param(rcv_buf_size, uint, 0444);

module_param(num_snd_urbs, uint, S_IRUGO);
		 "Size of the buffers used for reception (range: 0-"
MODULE_PARM_DESC(num_snd_urbs,
		 "Number of urbs used for transmission (range: 0-"
		 __MODULE_STRING(UDSL_MAX_SND_URBS) ", default: "
		 __MODULE_STRING(UDSL_DEFAULT_SND_URBS) ")");
module_param(snd_buf_size, uint, 0444);
		 "Size of the buffers used for reception in ATM cells (range: 1-"
		 __MODULE_STRING(UDSL_MAX_RCV_BUF_SIZE) ", default: "
		 "Size of the buffers used for transmission (range: 0-"
MODULE_PARM_DESC(rcv_buf_size,
module_param(rcv_buf_bytes, uint, S_IRUGO);
MODULE_PARM_DESC(rcv_buf_bytes,
		 "Size of the buffers used for reception, in bytes (range: 1-"


/* send */

struct usbatm_control {
	struct atm_skb_data atm;
	u32 len;
	u32 crc;
};
static void usbatm_atm_dev_close(struct atm_dev *dev);

#define UDSL_SKB(x)		((struct usbatm_control *)(x)->cb)

static int usbatm_atm_ioctl(struct atm_dev *dev, unsigned int cmd, void __user * arg);

/* ATM */

static void usbatm_atm_dev_close(struct atm_dev *atm_dev);
static int usbatm_atm_open(struct atm_vcc *vcc);
/* DRIVER */

struct driver_info {
	char	*description;
};

/***************
**  hardware  **
***************/

#ifdef	CONFIG_USB_CXACRU
#define	HAVE_HARDWARE
static const struct driver_info cxacru_info = {
	.description =  "Conexant AccessRunner",
};
#endif

#ifdef	CONFIG_USB_SPEEDTOUCH
#define	HAVE_HARDWARE
static const struct driver_info speedtouch_info = {
	.description =  "SpeedTouch USB",
};
#endif
static void usbatm_atm_close(struct atm_vcc *vcc);
static int usbatm_atm_ioctl(struct atm_dev *atm_dev, unsigned int cmd, void __user * arg);
static inline struct usbatm_vcc_data *usbatm_find_vcc(struct usbatm_instance_data *instance,
static int usbatm_atm_send(struct atm_vcc *vcc, struct sk_buff *skb);
static int usbatm_atm_proc_read(struct atm_dev *atm_dev, loff_t * pos, char *page);

static struct atmdev_ops usbatm_atm_devops = {
	.send		= usbatm_atm_send,
	.proc_read	= usbatm_atm_proc_read,
	.owner		= THIS_MODULE,
};

/***********
**  misc  **
static void usbatm_extract_cells(struct usbatm_instance_data *instance,
***********/

	unsigned long flags;

			__func__, source[ATM_CELL_HEADER], source[ATM_CELL_HEADER + 1]);
		return -EPROTO;
		atm_dbg(instance, "%s: OAM CRC10 error!
", __func__);
		return -EIO;
	}

	urb = usbatm_pop_urb(&instance->tx_channel);
	if (!urb)
		return -ENOMEM;
	     i++, source += ATM_CELL_SIZE + instance->rcv_padding) {
	
	buffer = urb->transfer_buffer;
	memcpy(buffer, source, ATM_CELL_SIZE);

 
	buffer[ATM_CELL_HEADER + 1] = 0;	/* update the direction field */
			       unsigned char *source, unsigned int howmany)
	
	memset(buffer + ATM_CELL_SIZE - 2, 0, 2);
	
	crc = crc10(0, buffer + ATM_CELL_HEADER, ATM_CELL_PAYLOAD);
	buffer[ATM_CELL_SIZE - 2] = (crc >> 8) & 0x3;
	struct usbatm_vcc_data *vcc_data;
	int cached_vci = 0;
	unsigned int i;
	int pti;
	int vci;
	short cached_vpi = 0;
	short vpi;
	urb->transfer_buffer_length = instance->tx_channel.stride;
	return usbatm_submit_urb(urb);
	for (i = 0; i < howmany;
	     i++, source += ATM_CELL_SIZE + instance->rx_padding) {
		vpi = ((source[0] & 0x0f) << 4) | (source[1] >> 4);
		if ((vcc->vci == vci) && (vcc->vpi == vpi))
			return vcc;
		pti = (source[3] & 0x2) != 0;
/*************
**  decode  **
		vdbg("usbatm_extract_cells: vpi %hd, vci %d, pti %d", vpi, vci, pti);
*************/

		if (cached_vcc && (vci == cached_vci) && (vpi == cached_vpi))
			vcc_data = cached_vcc;
		else if ((vcc_data = usbatm_find_vcc(instance, vpi, vci))) {
			cached_vcc = vcc_data;
static void usbatm_extract_cells(struct usbatm_data *instance,
			       unsigned char *source, unsigned int avail_data)
static inline struct usbatm_vcc_data *usbatm_find_vcc(struct usbatm_data *instance,
		} else {
			dbg("usbatm_extract_cells: unknown vpi/vci (%hd/%d)!", vpi, vci);
}
		vdbg("%s: vpi %hd, vci %d, pti %d", __func__, vpi, vci, pti);
{
			atm_warn(instance, "%s: OAM not supported (vpi %d, vci %d)!
", __func__, vpi, vci);
		vcc = vcc_data->vcc;
		sarb = vcc_data->sarb;
			atomic_inc(&vcc->stats->rx_err);
			cached_vci = vci;
	struct sk_buff *sarb;
			dbg("usbatm_extract_cells: buffer overrun (sarb->len %u, vcc: 0x%p)!", sarb->len, vcc);
			cached_vcc = usbatm_find_vcc(instance, vpi, vci);

	vdbg("%s: vpi %hd, vci %d, pti %d", __func__, vpi, vci, pti);
			if (!cached_vcc)
				atm_dbg(instance, "%s: unknown vpi/vci (%hd/%d)!
", __func__, vpi, vci);
//		atomic_inc(&vcc->stats->rx_err);
//		return;

	/* OAM F5 end-to-end */
		if (pti) {
		if (!cached_vcc)
			continue;

		vcc = cached_vcc->vcc;
	if (pti == ATM_PTI_E2EF5) {
		if (printk_ratelimit())
		/* OAM F5 end-to-end */
		if (pti == ATM_PTI_E2EF5) {
			if (usbatm_oam_reply(instance, source)) {
				dbg("usbatm_extract_cells: bogus length %u (vcc: 0x%p)!", length, vcc);
				atomic_inc(&vcc->stats->rx_err);
			}
			continue;
	memcpy(skb_tail_pointer(sarb), source + ATM_CELL_HEADER, ATM_CELL_PAYLOAD);
	__skb_put(sarb, ATM_CELL_PAYLOAD);
			pdu_length = UDSL_NUM_CELLS(length) * ATM_CELL_PAYLOAD;

		sarb = cached_vcc->sarb;
	if (pti & 1) {
				dbg("usbatm_extract_cells: bogus pdu_length %u (sarb->len: %u, vcc: 0x%p)!", pdu_length, sarb->len, vcc);
		if (sarb->tail + ATM_CELL_PAYLOAD > sarb->end) {
			atm_dbg(instance, "%s: buffer overrun (sarb->len %u, vcc: 0x%p)!
",
					__func__, sarb->len, vcc);
			/* discard cells already received */
			skb_trim(sarb, 0);
			UDSL_ASSERT(sarb->tail + ATM_CELL_PAYLOAD <= sarb->end);
				dbg("usbatm_extract_cells: packet failed crc check (vcc: 0x%p)!", vcc);
		/* guard against overflow */
		if (length > ATM_MAX_AAL5_PDU) {
		memcpy(sarb->tail, source + ATM_CELL_HEADER, ATM_CELL_PAYLOAD);
		__skb_put(sarb, ATM_CELL_PAYLOAD);

			vdbg("usbatm_extract_cells: got packet (length: %u, pdu_length: %u, vcc: 0x%p)", length, pdu_length, vcc);
		pdu_length = usbatm_pdu_length(length);
		if (pti & 1) {
			struct sk_buff *skb;
				dbg("usbatm_extract_cells: no memory for skb (length: %u)!", length);
			unsigned int length;
			unsigned int pdu_length;

			length = (source[ATM_CELL_SIZE - 6] << 8) + source[ATM_CELL_SIZE - 5];

			vdbg("usbatm_extract_cells: allocated new sk_buff (skb: 0x%p, skb->truesize: %u)", skb, skb->truesize);
			/* guard against overflow */
			if (length > ATM_MAX_AAL5_PDU) {
				atm_dbg(instance, "%s: bogus length %u (vcc: 0x%p)!
",
				dbg("usbatm_extract_cells: failed atm_charge (skb->truesize: %u)!", skb->truesize);
						__func__, length, vcc);
				atomic_inc(&vcc->stats->rx_err);
				goto out;
			}

		if (sarb->len < pdu_length) {
			pdu_length = usbatm_pdu_length(length);
		if (crc32_be(~0, sarb->tail - pdu_length, pdu_length) != 0xc704dd7b) {
			vdbg("usbatm_extract_cells: sending skb 0x%p, skb->len %u, skb->truesize %u", skb, skb->len, skb->truesize);
			if (sarb->len < pdu_length) {
				atm_dbg(instance, "%s: bogus pdu_length %u (sarb->len: %u, vcc: 0x%p)!
",
						__func__, pdu_length, sarb->len, vcc);
				atomic_inc(&vcc->stats->rx_err);
				goto out;
			}
			atm_rldbg(instance, "%s: packet failed crc check (vcc: 0x%p)!
",
				  __func__, vcc);
			if (crc32_be(~0, sarb->tail - pdu_length, pdu_length) != 0xc704dd7b) {
				atm_dbg(instance, "%s: packet failed crc check (vcc: 0x%p)!
",
						__func__, vcc);
				atomic_inc(&vcc->stats->rx_err);
				goto out;
			}

		if (!(skb = dev_alloc_skb(length))) {
			vdbg("%s: got packet (length: %u, pdu_length: %u, vcc: 0x%p)", __func__, length, pdu_length, vcc);
				atm_err(instance, "%s: no memory for skb (length: %u)!
",
static inline void usbatm_fill_cell_header(unsigned char *target, struct atm_vcc *vcc)
{
	target[0] = vcc->vpi >> 4;
	target[1] = (vcc->vpi << 4) | (vcc->vci >> 12);
	target[2] = vcc->vci >> 4;
	target[3] = vcc->vci << 4;
static unsigned int usbatm_write_cells(struct usbatm_instance_data *instance,
	target[4] = 0xec;
}

static const unsigned char zeros[ATM_CELL_PAYLOAD];

static void usbatm_groom_skb(struct atm_vcc *vcc, struct sk_buff *skb)
				atm_dbg(instance, "%s: no memory for skb (length: %u)!
", __func__, length);
				atomic_inc(&vcc->stats->rx_drop);
				goto out;
	unsigned int zero_padding;
	u32 crc;
		}
			vdbg("%s: allocated new sk_buff (skb: 0x%p, skb->truesize: %u)", __func__, skb, skb->truesize);
	ctrl->atm_data.vcc = vcc;
		vdbg("%s: allocated new sk_buff (skb: 0x%p, skb->truesize: %u)", __func__, skb, skb->truesize);
			if (!atm_charge(vcc, skb->truesize)) {
	ctrl->num_cells = UDSL_NUM_CELLS(skb->len);
	ctrl->num_entire = skb->len / ATM_CELL_PAYLOAD;

		PACKETDEBUG(skb->data, skb->len);
	zero_padding = ctrl->num_cells * ATM_CELL_PAYLOAD - skb->len - ATM_AAL5_TRAILER;
		UDSL_ASSERT(buf_usage <= stride);

	if (ctrl->num_entire + 1 < ctrl->num_cells)
		ctrl->pdu_padding = zero_padding - (ATM_CELL_PAYLOAD - ATM_AAL5_TRAILER);
	else
		ctrl->pdu_padding = zero_padding;
			atomic_inc(&vcc->stats->rx);
		out:
	ctrl->aal5_trailer[0] = 0;	/* UU = 0 */
	ctrl->aal5_trailer[1] = 0;	/* CPI = 0 */
	ctrl->aal5_trailer[2] = skb->len >> 8;
	ctrl->aal5_trailer[3] = skb->len;

	crc = crc32_be(~0, skb->data, skb->len);
	crc = crc32_be(crc, zeros, zero_padding);
	crc = crc32_be(crc, ctrl->aal5_trailer, 4);
	crc = ~crc;

	ctrl->aal5_trailer[4] = crc >> 24;
	ctrl->aal5_trailer[5] = crc >> 16;
	ctrl->aal5_trailer[6] = crc >> 8;
	ctrl->aal5_trailer[7] = crc;
}
		} else {
			/* not enough data to fill the cell */
static unsigned int usbatm_write_cells(struct usbatm_data *instance,
				     unsigned int howmany, struct sk_buff *skb,
				     unsigned char **target_p)
{
	struct usbatm_control *ctrl = UDSL_SKB(skb);
	unsigned char *target = *target_p;
	unsigned int nc, ne, i;
			memcpy(cell_buf + buf_usage, source, avail_data);
			instance->buf_usage = buf_usage + avail_data;
	vdbg("usbatm_write_cells: howmany=%u, skb->len=%d, num_cells=%u, num_entire=%u, pdu_padding=%u", howmany, skb->len, ctrl->num_cells, ctrl->num_entire, ctrl->pdu_padding);

	for (; avail_data >= stride; avail_data -= stride, source += stride)
	nc = ctrl->num_cells;
	ne = min(howmany, ctrl->num_entire);
		usbatm_extract_one_cell(instance, source);

	for (i = 0; i < ne; i++) {
		usbatm_fill_cell_header(target, ctrl->atm_data.vcc);
		target += ATM_CELL_HEADER;
		memcpy(target, skb->data, ATM_CELL_PAYLOAD);
		target += ATM_CELL_PAYLOAD;
		if (instance->tx_padding) {
			memset(target, 0, instance->tx_padding);
			target += instance->tx_padding;
	if (avail_data > 0) {
		/* length was not a multiple of stride -
		__skb_pull(skb, ATM_CELL_PAYLOAD);
		memcpy(instance->cell_buf, source, avail_data);
		instance->buf_usage = avail_data;
	}
	ctrl->num_entire -= ne;
	struct usbatm_instance_data *instance;
}
	if (!(ctrl->num_cells -= ne) || !(howmany -= ne))
		goto out;

	usbatm_fill_cell_header(target, ctrl->atm_data.vcc);
	target += ATM_CELL_HEADER;
	memcpy(target, skb->data, skb->len);
	target += skb->len;
	buf->filled_cells = urb->actual_length / (ATM_CELL_SIZE + instance->rcv_padding);
	__skb_pull(skb, skb->len);
	memset(target, 0, ctrl->pdu_padding);
	target += ctrl->pdu_padding;

	if (--ctrl->num_cells) {
		if (!--howmany) {
			ctrl->pdu_padding = ATM_CELL_PAYLOAD - ATM_AAL5_TRAILER;
			goto out;
		}

		if (instance->tx_padding) {
			memset(target, 0, instance->tx_padding);
			target += instance->tx_padding;
		}
		usbatm_fill_cell_header(target, ctrl->atm_data.vcc);
		target += ATM_CELL_HEADER;
		memset(target, 0, ATM_CELL_PAYLOAD - ATM_AAL5_TRAILER);
		target += ATM_CELL_PAYLOAD - ATM_AAL5_TRAILER;

		--ctrl->num_cells;
		UDSL_ASSERT(!ctrl->num_cells);
	}
	struct usbatm_instance_data *instance = (struct usbatm_instance_data *)data;

	memcpy(target, ctrl->aal5_trailer, ATM_AAL5_TRAILER);
	target += ATM_AAL5_TRAILER;
	/* set pti bit in last cell */
	*(target + 3 - ATM_CELL_SIZE) |= 0x2;
	if (instance->tx_padding) {
		memset(target, 0, instance->tx_padding);
		target += instance->tx_padding;
	}
 out:
	*target_p = target;
	return nc - ctrl->num_cells;
}


/*************
**  encode  **
*************/
	for (num_written = 0; num_written < avail_space && ctrl->len;
static void usbatm_complete_receive(struct urb *urb, struct pt_regs *regs)
				  rcv_buf_size * (ATM_CELL_SIZE + instance->rcv_padding),
	     num_written += stride, target += stride) {
static unsigned int usbatm_write_cells(struct usbatm_data *instance,
	struct usbatm_receive_buffer *buf;
	struct usbatm_data *instance;
	struct usbatm_receiver *rcv;
	unsigned long flags;
				       u8 *target, unsigned int avail_space)
{
	if (!urb || !(rcv = urb->context)) {
		dbg("usbatm_complete_receive: bad urb!");
		return;
	}
	struct atm_vcc *vcc = ctrl->atm.vcc;
	unsigned int bytes_written;
	instance = rcv->instance;
	buf = rcv->buffer;

		ptr[0] = vcc->vpi >> 4;
	buf->filled_cells = urb->actual_length / (ATM_CELL_SIZE + instance->rx_padding);

	vdbg("usbatm_complete_receive: urb 0x%p, status %d, actual_length %d, filled_cells %u, rcv 0x%p, buf 0x%p", urb, urb->status, urb->actual_length, buf->filled_cells, rcv, buf);

	UDSL_ASSERT(buf->filled_cells <= rcv_buf_size);

	/* may not be in_interrupt() */
	spin_lock_irqsave(&instance->receive_lock, flags);
	list_add(&rcv->list, &instance->spare_receivers);
	list_add_tail(&buf->list, &instance->filled_receive_buffers);
	if (likely(!urb->status))
		tasklet_schedule(&instance->receive_tasklet);
	spin_unlock_irqrestore(&instance->receive_lock, flags);
		ptr[3] = vcc->vci << 4;
		ptr[4] = 0xec;
static void usbatm_process_receive(unsigned long data)
{
	struct usbatm_receive_buffer *buf;
	struct usbatm_data *instance = (struct usbatm_data *)data;
	struct usbatm_receiver *rcv;
	int err;

 made_progress:
	while (!list_empty(&instance->spare_receive_buffers)) {
		spin_lock_irq(&instance->receive_lock);
	struct usbatm_instance_data *instance;
		if (list_empty(&instance->spare_receivers)) {
			spin_unlock_irq(&instance->receive_lock);
			break;
		}
		rcv = list_entry(instance->spare_receivers.next,
				 struct usbatm_receiver, list);
		list_del(&rcv->list);
		spin_unlock_irq(&instance->receive_lock);

		buf = list_entry(instance->spare_receive_buffers.next,
				 struct usbatm_receive_buffer, list);
		list_del(&buf->list);

		rcv->buffer = buf;

		usb_fill_bulk_urb(rcv->urb, instance->usb_dev,
				  usb_rcvbulkpipe(instance->usb_dev, instance->data_endpoint),
				  buf->base,
				  rcv_buf_size * (ATM_CELL_SIZE + instance->rx_padding),
				  usbatm_complete_receive, rcv);

		vdbg("usbatm_process_receive: sending urb 0x%p, rcv 0x%p, buf 0x%p",
		     rcv->urb, rcv, buf);
	struct usbatm_instance_data *instance = (struct usbatm_instance_data *)data;

		if ((err = usb_submit_urb(rcv->urb, GFP_ATOMIC)) < 0) {
			dbg("usbatm_process_receive: urb submission failed (%d)!", err);
			list_add(&buf->list, &instance->spare_receive_buffers);
			spin_lock_irq(&instance->receive_lock);
			list_add(&rcv->list, &instance->spare_receivers);
			spin_unlock_irq(&instance->receive_lock);
			break;
		}
	}

	spin_lock_irq(&instance->receive_lock);
	if (list_empty(&instance->filled_receive_buffers)) {
		spin_unlock_irq(&instance->receive_lock);
		return;		/* done - no more buffers */
	}
	buf = list_entry(instance->filled_receive_buffers.next,
			 struct usbatm_receive_buffer, list);
	list_del(&buf->list);
	spin_unlock_irq(&instance->receive_lock);

	vdbg("usbatm_process_receive: processing buf 0x%p", buf);
	usbatm_extract_cells(instance, buf->base, buf->filled_cells);
	list_add(&buf->list, &instance->spare_receive_buffers);
	goto made_progress;
}
		ptr += ATM_CELL_HEADER;
		memcpy(ptr, skb->data, data_len);

		skb_copy_from_linear_data(skb, ptr, data_len);
		ptr += data_len;
		__skb_pull(skb, data_len);
static void usbatm_complete_send(struct urb *urb, struct pt_regs *regs)
{
	struct usbatm_data *instance;
	struct usbatm_sender *snd;
	unsigned long flags;

	if (!urb || !(snd = urb->context) || !(instance = snd->instance)) {
		dbg("usbatm_complete_send: bad urb!");
		return;
	}

	vdbg("usbatm_complete_send: urb 0x%p, status %d, snd 0x%p, buf 0x%p", urb,
	     urb->status, snd, snd->buffer);

	/* may not be in_interrupt() */
	spin_lock_irqsave(&instance->send_lock, flags);
	list_add(&snd->list, &instance->spare_senders);
	list_add(&snd->buffer->list, &instance->spare_send_buffers);
	tasklet_schedule(&instance->send_tasklet);
	spin_unlock_irqrestore(&instance->send_lock, flags);
}

static void usbatm_process_send(unsigned long data)

	struct usbatm_send_buffer *buf;
		if(!left)
			continue;
	struct sk_buff *skb;
	struct usbatm_sender *snd;
	int err;
	unsigned int num_written;

 made_progress:
	spin_lock_irq(&instance->send_lock);
	while (!list_empty(&instance->spare_senders)) {
		if (!list_empty(&instance->filled_send_buffers)) {
			buf = list_entry(instance->filled_send_buffers.next,
					 struct usbatm_send_buffer, list);
			list_del(&buf->list);
		} else if ((buf = instance->current_buffer)) {
			instance->current_buffer = NULL;
		} else		/* all buffers empty */
			break;

		snd = list_entry(instance->spare_senders.next,
				 struct usbatm_sender, list);
		list_del(&snd->list);
		spin_unlock_irq(&instance->send_lock);

		snd->buffer = buf;
		usb_fill_bulk_urb(snd->urb, instance->usb_dev,
				  usb_sndbulkpipe(instance->usb_dev, instance->data_endpoint),
				  buf->base,
				  (snd_buf_size - buf->free_cells) * (ATM_CELL_SIZE + instance->tx_padding),
				  usbatm_complete_send, snd);

		vdbg("usbatm_process_send: submitting urb 0x%p (%d cells), snd 0x%p, buf 0x%p",
		     snd->urb, snd_buf_size - buf->free_cells, snd, buf);

		if ((err = usb_submit_urb(snd->urb, GFP_ATOMIC)) < 0) {
			dbg("usbatm_process_send: urb submission failed (%d)!", err);
			spin_lock_irq(&instance->send_lock);
			list_add(&snd->list, &instance->spare_senders);
			spin_unlock_irq(&instance->send_lock);
static void usbatm_cancel_send(struct usbatm_instance_data *instance,
			list_add(&buf->list, &instance->filled_send_buffers);
			return;	/* bail out */
			trailer[7] = ctrl->crc;

			target[3] |= 0x2;	/* adjust PTI */
		spin_lock_irq(&instance->send_lock);
	}			/* while */
	spin_unlock_irq(&instance->send_lock);

	if (!instance->current_skb)
		instance->current_skb = skb_dequeue(&instance->sndqueue);
	if (!instance->current_skb)
		return;		/* done - no more skbs */

	skb = instance->current_skb;

	if (!(buf = instance->current_buffer)) {
		spin_lock_irq(&instance->send_lock);
		if (list_empty(&instance->spare_send_buffers)) {
			instance->current_buffer = NULL;
			spin_unlock_irq(&instance->send_lock);
			return;	/* done - no more buffers */
		}
		buf = list_entry(instance->spare_send_buffers.next,
			       struct usbatm_send_buffer, list);
		list_del(&buf->list);
		spin_unlock_irq(&instance->send_lock);
			ctrl->crc = crc32_be(ctrl->crc, ptr, left);
	}
	struct usbatm_instance_data *instance = vcc->dev->dev_data;
		buf->free_start = buf->base;
		buf->free_cells = snd_buf_size;
				if (!urb->iso_frame_desc[i].status)
					usbatm_extract_cells(instance,
		instance->current_buffer = buf;
	}
							     urb->iso_frame_desc[i].actual_length);
		}
	num_written = usbatm_write_cells(instance, buf->free_cells, skb, &buf->free_start);
					unsigned int actual_length = urb->iso_frame_desc[i].actual_length;

	vdbg("usbatm_process_send: wrote %u cells from skb 0x%p to buffer 0x%p",
	     num_written, skb, buf);
					if (!merge_length)
						merge_start = (unsigned char *)urb->transfer_buffer + urb->iso_frame_desc[i].offset;
	if (!(buf->free_cells -= num_written)) {
		list_add_tail(&buf->list, &instance->filled_send_buffers);
		instance->current_buffer = NULL;
	}
					if (merge_length && (actual_length < packet_size)) {
						usbatm_extract_cells(instance, merge_start, merge_length);
	vdbg("usbatm_process_send: buffer contains %d cells, %d left",
	     snd_buf_size - buf->free_cells, buf->free_cells);

	if (!UDSL_SKB(skb)->num_cells) {
		struct atm_vcc *vcc = UDSL_SKB(skb)->atm_data.vcc;

		usbatm_pop(vcc, skb);
		instance->current_skb = NULL;

		atomic_inc(&vcc->stats->tx);
					atm_rldbg(instance, "%s: status %d in frame %d!
", __func__, urb->status, i);
					if (merge_length)
						usbatm_extract_cells(instance, merge_start, merge_length);
	goto made_progress;
					merge_length = 0;
					instance->buf_usage = 0;
				}
			}
			     struct atm_vcc *vcc)

			if (merge_length)
				usbatm_extract_cells(instance, merge_start, merge_length);
	struct usbatm_instance_data *instance =
	    container_of(kref, struct usbatm_instance_data, refcount);
	dbg("usbatm_cancel_send entered");
			if (!urb->status)
				usbatm_extract_cells(instance, urb->transfer_buffer, urb->actual_length);
			else
				instance->buf_usage = 0;

		if (UDSL_SKB(skb)->atm_data.vcc == vcc) {
			dbg("usbatm_cancel_send: popping skb 0x%p", skb);
void usbatm_get_instance(struct usbatm_instance_data *instance)
			return;
	unsigned int num_written = 0;
	}
}

void usbatm_put_instance(struct usbatm_instance_data *instance)

	tasklet_disable(&instance->send_tasklet);
	if ((skb = instance->current_skb) && (UDSL_SKB(skb)->atm_data.vcc == vcc)) {
		dbg("usbatm_cancel_send: popping current skb (0x%p)", skb);
***********/

static void usbatm_tx_process(unsigned long data)
{
	tasklet_enable(&instance->send_tasklet);
	dbg("usbatm_cancel_send done");
	struct sk_buff *skb = instance->current_skb;
	struct usbatm_instance_data *instance = dev->dev_data;
			num_written = (urb->status == -EAGAIN) ?
	struct urb *urb = NULL;
	const unsigned int buf_size = instance->tx_channel.buf_size;
	unsigned int bytes_written = 0;
	u8 *buffer = NULL;
		num_written += usbatm_write_cells(instance, skb,
						  buffer + num_written,
						  buf_size - num_written);
	struct usbatm_instance_data *instance = atm_dev->dev_data;
	vdbg("usbatm_atm_send called (skb 0x%p, len %u)", skb, skb->len);
		skb = skb_dequeue(&instance->sndqueue);

	while (skb) {
		dbg("usbatm_atm_send: NULL data!");
		     __func__, num_written, skb, urb);
		if (!urb) {
			urb = usbatm_pop_urb(&instance->tx_channel);
			if (!urb)
				break;		/* no more senders */
			buffer = urb->transfer_buffer;
		dbg("usbatm_atm_send: unsupported ATM type %d!", vcc->qos.aal);
			bytes_written = (urb->status == -EAGAIN) ?
				urb->transfer_buffer_length : 0;
		}

		bytes_written += usbatm_write_cells(instance, skb,
						  buffer + bytes_written,
		dbg("usbatm_atm_send: packet too long (%d vs %d)!", skb->len,
		    ATM_MAX_AAL5_PDU);
			urb->transfer_buffer_length = num_written;

		vdbg("%s: wrote %u bytes from skb 0x%p to urb 0x%p",
		     __func__, bytes_written, skb, urb);

		if (!UDSL_SKB(skb)->len) {
			struct atm_vcc *vcc = UDSL_SKB(skb)->atm.vcc;
	usbatm_groom_skb(vcc, skb);
			skb = skb_dequeue(&instance->sndqueue);
		}
	tasklet_schedule(&instance->send_tasklet);

		if (bytes_written == buf_size || (!skb && bytes_written)) {
			urb->transfer_buffer_length = bytes_written;

			if (usbatm_submit_urb(urb))
				break;
			urb = NULL;
		}
	}

	instance->current_skb = skb;
}

static void usbatm_cancel_send(struct usbatm_data *instance,
			       struct atm_vcc *vcc)
{
	struct usbatm_data *instance =
	    container_of(kref, struct usbatm_data, refcount);
	atm_dbg(instance, "%s entered
", __func__);
	spin_lock_irq(&instance->sndqueue.lock);
	tasklet_kill(&instance->receive_tasklet);
	tasklet_kill(&instance->send_tasklet);
	if (!instance) {
		dbg("%s: NULL data!", __func__);
			usbatm_pop(vcc, skb);
		}
	spin_unlock_irq(&instance->sndqueue.lock);
	struct usbatm_instance_data *instance = vcc->dev->dev_data;

	tasklet_disable(&instance->tx_channel.tasklet);
		atm_dbg(instance, "%s: unsupported ATM type %d!
", __func__, vcc->qos.aal);
		atm_dbg(instance, "%s: popping current skb (0x%p)
", __func__, skb);
		instance->current_skb = NULL;
		usbatm_pop(vcc, skb);
	}
	tasklet_enable(&instance->tx_channel.tasklet);
		atm_dbg(instance, "%s: packet too long (%d vs %d)!
",
				__func__, skb->len, ATM_MAX_AAL5_PDU);

static int usbatm_atm_send(struct atm_vcc *vcc, struct sk_buff *skb)
{
	struct usbatm_data *instance = vcc->dev->dev_data;
	struct usbatm_control *ctrl = UDSL_SKB(skb);
	int err;

	vdbg("%s called (skb 0x%p, len %u)", __func__, skb, skb->len);

	/* racy disconnection check - fine */
	if (!instance || instance->disconnected) {
#endif
		err = -ENODEV;
	usbatm_put_instance(instance);
		goto fail;
	}

	if (vcc->qos.aal != ATM_AAL5) {
		atm_rldbg(instance, "%s: unsupported ATM type %d!
", __func__, vcc->qos.aal);
		err = -EINVAL;
		goto fail;
	}

		dbg("usbatm_atm_proc_read: NULL instance!");
	if (skb->len > ATM_MAX_AAL5_PDU) {
		atm_rldbg(instance, "%s: packet too long (%d vs %d)!
",
				__func__, skb->len, ATM_MAX_AAL5_PDU);
		err = -EINVAL;
		goto fail;
	}

	PACKETDEBUG(skb->data, skb->len);

	/* initialize the control block */
	ctrl->atm.vcc = vcc;
	ctrl->len = skb->len;
	ctrl->crc = crc32_be(~0, skb->data, skb->len);

	skb_queue_tail(&instance->sndqueue, skb);
	tasklet_schedule(&instance->tx_channel.tasklet);

void usbatm_get_instance(struct usbatm_data *instance)
	return 0;

 fail:
	usbatm_pop(vcc, skb);
	if (!left--) {
	return err;
}

			sprintf(page, "Line up");
			break;
void usbatm_put_instance(struct usbatm_data *instance)

			sprintf(page, "Line down");
			break;
/********************
**  bean counting  **
			sprintf(page, "Line state unknown");
			break;
********************/

		if (instance->usb_dev->state == USB_STATE_NOTATTACHED)
			strcat(page, ", disconnected
");
		else {
			if (instance->status == UDSL_LOADED_FIRMWARE)
	struct usbatm_instance_data *instance = vcc->dev->dev_data;
				strcat(page, ", firmware loaded
");
			else if (instance->status == UDSL_LOADING_FIRMWARE)
				strcat(page, ", firmware loading
");
			else
				strcat(page, ", no firmware
");
		}

		return strlen(page);
	}

static void usbatm_destroy_instance(struct kref *kref)
static void usbatm_atm_dev_close(struct atm_dev *dev)
{
	struct usbatm_data *instance = container_of(kref, struct usbatm_data, refcount);
	struct usbatm_data *instance = dev->dev_data;

	dbg("%s", __func__);
	struct usbatm_vcc_data *new;
	unsigned int max_pdu;
	tasklet_kill(&instance->rx_channel.tasklet);
	tasklet_kill(&instance->tx_channel.tasklet);
	int err;

	dbg("usbatm_atm_open: vpi %hd, vci %d", vpi, vci);
	usb_put_dev(instance->usb_dev);
	kfree(instance);
	dev->dev_data = NULL;
		dbg("usbatm_atm_open: NULL data!");
}

static void usbatm_get_instance(struct usbatm_data *instance)
{

	kref_get(&instance->refcount);
}

		dbg("usbatm_atm_open: unsupported ATM type %d!", vcc->qos.aal);
static void usbatm_put_instance(struct usbatm_data *instance)
{
	dbg("%s", __func__);
	if (instance->firmware_wait &&
	    (err = instance->firmware_wait(instance)) < 0) {
		dbg("usbatm_atm_open: firmware not loaded (%d)!", err);
		return err;
	}


	kref_put(&instance->refcount, usbatm_destroy_instance);
}

		dbg("usbatm_atm_open: %hd/%d already in use!", vpi, vci);
		up(&instance->serialize);
		return -EADDRINUSE;
**  ATM  **
			struct usbatm_instance_data *instance)
**********/

static void usbatm_atm_dev_close(struct atm_dev *atm_dev)
		dbg("usbatm_atm_open: no memory for vcc_data!");
		up(&instance->serialize);
		return -ENOMEM;

	dbg("%s", __func__);

	if (!instance)
		return;

	atm_dev->dev_data = NULL; /* catch bugs */
	usbatm_put_instance(instance);	/* taken in usbatm_atm_init */
	/* usbatm_extract_cells requires at least one cell */
	max_pdu = max(1, UDSL_NUM_CELLS(vcc->qos.rxtp.max_sdu)) * ATM_CELL_PAYLOAD;
	if (!(new->sarb = alloc_skb(max_pdu, GFP_KERNEL))) {
		dbg("usbatm_atm_open: no memory for SAR buffer!");
		kfree(new);
		up(&instance->serialize);
		return -ENOMEM;
		case ATM_PHY_SIG_LOST:
			return sprintf(page, "Line down
");
		default:
			return sprintf(page, "Line state unknown
");
		}
	tasklet_disable(&instance->receive_tasklet);
	if (!left--)
		return sprintf(page, "%s
", instance->description);
	tasklet_enable(&instance->receive_tasklet);

	if (!left--)
		return sprintf(page, "MAC: %02x:%02x:%02x:%02x:%02x:%02x
",
			       atm_dev->esi[0], atm_dev->esi[1],
			       atm_dev->esi[2], atm_dev->esi[3],
		buf->base = kmalloc(rcv_buf_size * (ATM_CELL_SIZE + instance->rcv_padding),
			       atm_dev->esi[4], atm_dev->esi[5]);

	if (!left--)
	tasklet_schedule(&instance->receive_tasklet);
		return sprintf(page,
			       "AAL5: tx %d ( %d err ), rx %d ( %d err, %d drop )
",
	dbg("usbatm_atm_open: allocated vcc data 0x%p (max_pdu: %u)", new, max_pdu);
			       atomic_read(&atm_dev->stats.aal5.tx),
			       atomic_read(&atm_dev->stats.aal5.tx_err),
	return 0;

	if (!left--) {
		if (instance->disconnected)
			return sprintf(page, "Disconnected
");
	if ((vcc->qos.aal != ATM_AAL5) || (vcc->qos.rxtp.max_sdu < 0)
	    || (vcc->qos.rxtp.max_sdu > ATM_MAX_AAL5_PDU)) {
		atm_dbg(instance, "%s: unsupported ATM type %d!
", __func__, vcc->qos.aal);
	dbg("usbatm_atm_close called");

				return sprintf(page, "Line state unknown
");
			}
		dbg("usbatm_atm_close: NULL data!");
	}

	down(&instance->serialize);	/* vs self, usbatm_atm_close */
	struct usbatm_vcc_data *new = NULL;
	dbg("usbatm_atm_close: deallocating vcc 0x%p with vpi %d vci %d",
	    vcc_data, vcc_data->vpi, vcc_data->vci);

	if (!instance) {
		dbg("%s: NULL data!", __func__);
		return -ENODEV;
	}
		atm_dbg(instance, "%s: no memory for vcc_data!
", __func__);
	tasklet_disable(&instance->receive_tasklet);

	atm_dbg(instance, "%s: vpi %hd, vci %d
", __func__, vpi, vci);
	tasklet_enable(&instance->receive_tasklet);

	/* only support AAL5 */
	memset(new, 0, sizeof(struct usbatm_vcc_data));
	down(&instance->serialize);	/* vs self, usbatm_atm_close, usbatm_usb_disconnect */
	if ((vcc->qos.aal != ATM_AAL5)) {
		atm_warn(instance, "%s: unsupported ATM type %d!
", __func__, vcc->qos.aal);
		return -EINVAL;
	}

	/* sanity checks */
		atm_dbg(instance, "%s: no memory for SAR buffer!
", __func__);
	if ((vcc->qos.rxtp.max_sdu < 0) || (vcc->qos.rxtp.max_sdu > ATM_MAX_AAL5_PDU)) {
		atm_dbg(instance, "%s: max_sdu %d out of range!
", __func__, vcc->qos.rxtp.max_sdu);
		return -EINVAL;
	}

	dbg("usbatm_atm_close successful");
	mutex_lock(&instance->serialize);	/* vs self, usbatm_atm_close, usbatm_usb_disconnect */

	if (instance->disconnected) {
		goto fail;
	if (!(new = kmalloc(sizeof(struct usbatm_vcc_data), GFP_KERNEL))) {
	}

	if (usbatm_find_vcc(instance, vpi, vci)) {
		atm_dbg(instance, "%s: %hd/%d already in use!
", __func__, vpi, vci);
		ret = -EADDRINUSE;
		goto fail;
	}

	if (!(new = kzalloc(sizeof(struct usbatm_vcc_data), GFP_KERNEL))) {
/**********
**  USB  **
**********/
	tasklet_enable(&instance->rx_channel.tasklet);

int usbatm_instance_setup(struct usb_device *dev,
			struct usbatm_data *instance)
{
	char *buf;
	int i, length;
	set_bit(ATM_VF_PARTIAL, &vcc->flags);
	atm_dbg(instance, "%s: allocated vcc data 0x%p
", __func__, new);
	kref_init(&instance->refcount);	/* one for USB */
	usbatm_get_instance(instance);	/* one for ATM */

fail:
	init_MUTEX(&instance->serialize);
	kfree(new);
	mutex_unlock(&instance->serialize);
	instance->usb_dev = dev;
{
	INIT_LIST_HEAD(&instance->vcc_list);
	struct usbatm_data *instance = vcc->dev->dev_data;
	struct usbatm_vcc_data *vcc_data = vcc->dev_data;
	instance->status = UDSL_NO_FIRMWARE;
	init_waitqueue_head(&instance->firmware_waiters);
	if (!instance || !vcc_data) {
		dbg("%s: NULL data!", __func__);
	spin_lock_init(&instance->receive_lock);
	INIT_LIST_HEAD(&instance->spare_receivers);
	INIT_LIST_HEAD(&instance->filled_receive_buffers);
static int usbatm_atm_ioctl(struct atm_dev *dev, unsigned int cmd,
	atm_dbg(instance, "%s entered
", __func__);
	tasklet_init(&instance->receive_tasklet, usbatm_process_receive, (unsigned long)instance);
	INIT_LIST_HEAD(&instance->spare_receive_buffers);
	atm_dbg(instance, "%s: deallocating vcc 0x%p with vpi %d vci %d
",
	tasklet_disable(&instance->rx_channel.tasklet);
	skb_queue_head_init(&instance->sndqueue);
	if (instance->cached_vcc == vcc_data) {
		instance->cached_vcc = NULL;
	spin_lock_init(&instance->send_lock);
	INIT_LIST_HEAD(&instance->spare_senders);
	INIT_LIST_HEAD(&instance->spare_send_buffers);

	tasklet_init(&instance->send_tasklet, usbatm_process_send,
		     (unsigned long)instance);
	INIT_LIST_HEAD(&instance->filled_send_buffers);
void usbatm_instance_disconnect(struct usbatm_instance_data *instance)

	/* receive init */
	for (i = 0; i < num_rcv_urbs; i++) {
		struct usbatm_receiver *rcv = &(instance->receivers[i]);

		if (!(rcv->urb = usb_alloc_urb(0, GFP_KERNEL))) {
			dbg("usbatm_usb_probe: no memory for receive urb %d!", i);
			goto fail;
		}
		instance->cached_vpi = ATM_VPI_UNSPEC;
		instance->cached_vci = ATM_VCI_UNSPEC;
		rcv->instance = instance;
	list_del(&vcc_data->list);
	tasklet_enable(&instance->rx_channel.tasklet);
		list_add(&rcv->list, &instance->spare_receivers);
	}
	kfree_skb(vcc_data->sarb);
	vcc_data->sarb = NULL;
	for (i = 0; i < num_rcv_bufs; i++) {
		struct usbatm_receive_buffer *buf =
		    &(instance->receive_buffers[i]);
	kfree(vcc_data);
	/* ATM init */
		buf->base = kmalloc(rcv_buf_size * (ATM_CELL_SIZE + instance->rx_padding),
				    GFP_KERNEL);
		if (!buf->base) {
			dbg("usbatm_usb_probe: no memory for receive buffer %d!", i);
			goto fail;
		}
	clear_bit(ATM_VF_PARTIAL, &vcc->flags);
		usb_dbg(instance, "%s: failed to register ATM device!
", __func__);
		list_add(&buf->list, &instance->spare_receive_buffers);
	mutex_unlock(&instance->serialize);

	atm_dbg(instance, "%s successful
", __func__);
	/* send init */
	for (i = 0; i < num_snd_urbs; i++) {
		struct usbatm_sender *snd = &(instance->senders[i]);
static int usbatm_atm_ioctl(struct atm_dev *atm_dev, unsigned int cmd,
			  void __user * arg)
		if (!(snd->urb = usb_alloc_urb(0, GFP_KERNEL))) {
			dbg("usbatm_usb_probe: no memory for send urb %d!", i);
			goto fail;
		}
{
	struct usbatm_data *instance = atm_dev->dev_data;
		snd->instance = instance;
	if (!instance || instance->disconnected) {
		dbg("%s: %s!", __func__, instance ? "disconnected" : "NULL instance");
		list_add(&snd->list, &instance->spare_senders);
	}

	switch (cmd) {
	for (i = 0; i < num_snd_bufs; i++) {
		struct usbatm_send_buffer *buf = &(instance->send_buffers[i]);
static int usbatm_atm_init(struct usbatm_data *instance)
{
		buf->base = kmalloc(snd_buf_size * (ATM_CELL_SIZE + instance->tx_padding),
				    GFP_KERNEL);
		if (!buf->base) {
			dbg("usbatm_usb_probe: no memory for send buffer %d!", i);
			goto fail;
		}

	atm_dev->ci_range.vpi_bits = ATM_CI_MAX;
		list_add(&buf->list, &instance->spare_send_buffers);
	atm_dev->link_rate = 128 * 1000 / 424;

	if (instance->driver->atm_start && ((ret = instance->driver->atm_start(instance, atm_dev)) < 0)) {
	/* ATM init */
	instance->atm_dev = atm_dev_register(instance->driver_name,
					     &usbatm_atm_devops, -1, NULL);
	if (!instance->atm_dev) {
		dbg("usbatm_usb_probe: failed to register ATM device!");
		goto fail;
	}
		atm_err(instance, "%s: atm_start failed: %d!
", __func__, ret);
		goto fail;
	instance->atm_dev->ci_range.vpi_bits = ATM_CI_MAX;
	instance->atm_dev->ci_range.vci_bits = ATM_CI_MAX;
	instance->atm_dev->signal = ATM_PHY_SIG_UNKNOWN;

	usbatm_get_instance(instance);	/* dropped in usbatm_atm_dev_close */
	/* temp init ATM device, set to 128kbit */
	instance->atm_dev->link_rate = 128 * 1000 / 424;
	/* ready for ATM callbacks */
	/* device description */
	mb();
	atm_dev->dev_data = instance;

	/* submit all rx URBs */
	if ((i = usb_string(dev, dev->descriptor.iProduct, buf, length)) < 0)
		goto finish;
		usbatm_submit_urb(instance->urbs[i]);

	return 0;

 fail:
		usb_dbg(instance, "%s: failed to create kernel_thread (%d)!
", __func__, ret);
	instance->atm_dev = NULL;
	atm_dev_deregister(atm_dev); /* usbatm_atm_dev_close will eventually be called */
	return ret;
	if (length <= 0 || (i = usb_make_path(dev, buf, length)) < 0)
		goto finish;


/**********
**  USB  **
**********/

	instance->thread_pid = get_current()->pid;
 finish:
	/* ready for ATM callbacks */
	wmb();
	instance->atm_dev->dev_data = instance;
			le16_to_cpu(usb_dev->descriptor.idProduct),
			intf->altsetting->desc.bInterfaceNumber);
	usb_get_dev(dev);
		return -ENOMEM;
	}
	return 0;
	/* public fields */

 fail:
	for (i = 0; i < num_snd_bufs; i++)
		kfree(instance->send_buffers[i].base);
	instance->rx_channel.endpoint = usb_rcvbulkpipe(usb_dev, driver->in);
	instance->tx_channel.endpoint = usb_sndbulkpipe(usb_dev, driver->out);
	for (i = 0; i < num_snd_urbs; i++)
		usb_free_urb(instance->senders[i].urb);
	snprintf(instance->driver_name, sizeof(instance->driver_name), driver->driver_name);

	for (i = 0; i < num_rcv_bufs; i++)
		kfree(instance->receive_buffers[i].base);
		unsigned int maxpacket = usb_maxpacket(usb_dev, channel->endpoint, i);
		struct urb *urb;
	for (i = 0; i < num_rcv_urbs; i++)
		usb_free_urb(instance->receivers[i].urb);
		unsigned int num_packets;
		unsigned int iso_packets = 0, iso_size = 0;
	return -ENOMEM;

		if ((maxpacket < 1) || (maxpacket > UDSL_MAX_BUF_SIZE)) {
			dev_err(dev, "%s: invalid endpoint %02x!
", __func__, usb_pipeendpoint(channel->endpoint));
			goto fail_unbind;
void usbatm_instance_disconnect(struct usbatm_data *instance)
		}
		if (usb_pipeisoc(channel->endpoint)) {
			iso_size = usb_maxpacket(instance->usb_dev, channel->endpoint, 0);
			iso_size -= iso_size % channel->stride;	/* alignment */
			BUG_ON(!iso_size);
	dbg("usbatm_instance_disconnect entered");
			iso_packets = (channel->buf_size - 1) / iso_size + 1;
		}
	else
		dbg("usbatm_instance_disconnect: NULL instance!");
		instance->rx_channel.endpoint = usb_rcvbulkpipe(usb_dev, driver->bulk_in);
		num_packets = max (1U, (channel->buf_size + maxpacket / 2) / maxpacket); /* round */

			dev_dbg(dev, "%s: no memory for urb %d!
", __func__, i);
	/* receive finalize */
	tasklet_disable(&instance->receive_tasklet);
			num_packets--;

	for (i = 0; i < num_rcv_urbs; i++)
		usb_kill_urb(instance->receivers[i].urb);

			dev_dbg(dev, "%s: no memory for buffer %d!
", __func__, i);
	/* no need to take the spinlock */
	INIT_LIST_HEAD(&instance->filled_receive_buffers);
	INIT_LIST_HEAD(&instance->spare_receive_buffers);
			snd_buf_bytes - (snd_buf_bytes % instance->tx_channel.stride));
		memset(buffer, 0, channel->buf_size);
	tasklet_enable(&instance->receive_tasklet);
	/* rx buffer size must be a positive multiple of the endpoint maxpacket */
	maxpacket = usb_maxpacket(usb_dev, instance->rx_channel.endpoint, 0);
	for (i = 0; i < num_rcv_urbs; i++)
		usb_free_urb(instance->receivers[i].urb);

	if ((maxpacket < 1) || (maxpacket > UDSL_MAX_BUF_SIZE)) {
	for (i = 0; i < num_rcv_bufs; i++)
		kfree(instance->receive_buffers[i].base);
				usb_pipeendpoint(instance->rx_channel.endpoint));
		error = -EINVAL;
	/* send finalize */
	tasklet_disable(&instance->send_tasklet);
	}
				urb->iso_frame_desc[j].offset = iso_size * j;
	for (i = 0; i < num_snd_urbs; i++)
		usb_kill_urb(instance->senders[i].urb);
				urb->iso_frame_desc[j].length = min_t(int, iso_size,
								      channel->buf_size - urb->iso_frame_desc[j].offset);
				  buffer, channel->buf_size, usbatm_complete, channel);//QQ is this OK for iso urbs; also - looks like should use iso_size * iso_packets rather than channel->buf_size
	num_packets = max (1U, (rcv_buf_bytes + maxpacket / 2) / maxpacket); /* round */
	INIT_LIST_HEAD(&instance->spare_senders);
	INIT_LIST_HEAD(&instance->spare_send_buffers);
	instance->current_buffer = NULL;
	if (num_packets * maxpacket > UDSL_MAX_BUF_SIZE)
		num_packets--;
	tasklet_enable(&instance->send_tasklet);
	instance->rx_channel.buf_size = num_packets * maxpacket;
	instance->rx_channel.packet_size = maxpacket;
	for (i = 0; i < num_snd_urbs; i++)
		usb_free_urb(instance->senders[i].urb);
		struct usbatm_channel *channel = i ?
	if (need_heavy && driver->heavy_init) {
	for (i = 0; i < num_snd_bufs; i++)
		kfree(instance->send_buffers[i].base);
		struct urb *urb;
		unsigned int iso_packets = usb_pipeisoc(channel->endpoint) ? channel->buf_size / channel->packet_size : 0;
	/* ATM finalize */
	shutdown_atm_dev(instance->atm_dev);

		UDSL_ASSERT(!usb_pipeisoc(channel->endpoint) || usb_pipein(channel->endpoint));

EXPORT_SYMBOL_GPL(usbatm_get_instance);
EXPORT_SYMBOL_GPL(usbatm_put_instance);
EXPORT_SYMBOL_GPL(usbatm_instance_setup);
EXPORT_SYMBOL_GPL(usbatm_instance_disconnect);
		buffer = kmalloc(channel->buf_size, GFP_KERNEL);
		urb = usb_alloc_urb(iso_packets, GFP_KERNEL);
		if (!urb) {
			dev_err(dev, "%s: no memory for urb %d!
", __func__, i);
			error = -ENOMEM;
#ifndef HAVE_HARDWARE
#error You need to configure some hardware for this driver
#endif

static const struct usb_device_id products [] = {
#ifdef	CONFIG_USB_CXACRU
{
	USB_DEVICE(0x0572, 0xcafe),	/* V = Conexant				P = ADSL modem (Euphrates project)	*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x0572, 0xcb00),	/* V = Conexant				P = ADSL modem (Hasbani project)	*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x0572, 0xcb01),	/* V = Conexant				P = ADSL modem				*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x0572, 0xcb06),	/* V = Conexant				P = ADSL modem				*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x08e3, 0x0100),	/* V = Olitec				P = ADSL modem version 2		*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x08e3, 0x0102),	/* V = Olitec				P = ADSL modem version 3		*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x0eb0, 0x3457),	/* V = Trust/Amigo Technology Co.	P = AMX-CA86U				*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x1803, 0x5510),	/* V = Zoom				P = 5510				*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x0675, 0x0200),	/* V = Draytek				P = Vigor 318				*/
	.driver_info = (unsigned long) &cxacru_info,
},
{
	USB_DEVICE(0x0586, 0x330a),	/* V = Zyxel				P = 630-C1 aka OMNI ADSL USB modem	*/
	.driver_info = (unsigned long) &cxacru_info,
},
#endif

#ifdef	CONFIG_USB_SPEEDTOUCH
{
	USB_DEVICE(0x06b9, 0x4061),
	.driver_info = (unsigned long) &speedtouch_info
},
#endif
	{}	/* END */
};

MODULE_DEVICE_TABLE (usb, products);

static struct usb_driver usbatm_driver = {
        .owner          = THIS_MODULE,
        .name           = driver_name,
//QQ        .probe          = usbatm_probe,
//QQ        .disconnect     = usbatm_disconnect,
        .id_table       = products,
};

			goto fail_unbind;
		}

	dbg("usbatm_usb_init: driver version " DRIVER_VERSION);
		instance->urbs[i] = urb;

		/* zero the tx padding to avoid leaking information */
		printk(KERN_ERR __FILE__ ": unusable with this kernel!
");
		buffer = kzalloc(channel->buf_size, GFP_KERNEL);
			dev_err(dev, "%s: no memory for buffer %d!
", __func__, i);
			error = -ENOMEM;
			goto fail_unbind;
		}

	    || (num_rcv_bufs > UDSL_MAX_RCV_BUFS)
	    || (num_snd_bufs > UDSL_MAX_SND_BUFS)
		usb_fill_bulk_urb(urb, instance->usb_dev, channel->endpoint,
				  buffer, channel->buf_size, usbatm_complete, channel);
		if (iso_packets) {
			int j;
			urb->interval = 1;
			urb->transfer_flags = URB_ISO_ASAP;
	return usb_register(&usbatm_driver);;
			urb->number_of_packets = iso_packets;
			for (j = 0; j < iso_packets; j++) {
				urb->iso_frame_desc[j].offset = channel->packet_size * j;
				urb->iso_frame_desc[j].length = channel->packet_size;
			}
		}
	dbg("usbatm_usb_exit");

	usb_deregister (&usbatm_driver);

		/* put all tx URBs on the list of spares */
		if (i >= num_rcv_urbs)
			list_add_tail(&urb->urb_list, &channel->list);

		vdbg("%s: alloced buffer 0x%p buf size %u urb 0x%p",
		     __func__, urb->transfer_buffer, urb->transfer_buffer_length, urb);
	}

	instance->cached_vpi = ATM_VPI_UNSPEC;
	instance->cached_vci = ATM_VCI_UNSPEC;
	instance->cell_buf = kmalloc(instance->rx_channel.stride, GFP_KERNEL);

	if (!instance->cell_buf) {
		dev_err(dev, "%s: no memory for cell buffer!
", __func__);
		error = -ENOMEM;
		goto fail_unbind;
	}

	if (!(instance->flags & UDSL_SKIP_HEAVY_INIT) && driver->heavy_init) {
	}

	if (error < 0)
		goto fail_unbind;

	usb_get_dev(usb_dev);
	usb_set_intfdata(intf, instance);

	return 0;

		if (instance->urbs[i])
			kfree(instance->urbs[i]->transfer_buffer);
		usb_free_urb(instance->urbs[i]);
	}

	kfree (instance);

	return error;
	/* turn usbatm_[rt]x_process into noop */
	/* no need to take the spinlock */
	INIT_LIST_HEAD(&instance->rx_channel.list);
	INIT_LIST_HEAD(&instance->tx_channel.list);

	tasklet_enable(&instance->rx_channel.tasklet);
	tasklet_enable(&instance->tx_channel.tasklet);

}
	down(&instance->serialize);
EXPORT_SYMBOL_GPL(usbatm_usb_probe);

void usbatm_usb_disconnect(struct usb_interface *intf)
{
	struct device *dev = &intf->dev;
	struct usbatm_data *instance = usb_get_intfdata(intf);
	struct usbatm_vcc_data *vcc_data;
	int i;
	down(&instance->serialize);

	dev_dbg(dev, "%s entered
", __func__);

	up(&instance->serialize);
	if (!instance) {
		dev_dbg(dev, "%s: NULL instance!
", __func__);
		return;
		shutdown_atm_dev(instance->atm_dev);
	}

	usb_set_intfdata(intf, NULL);

	mutex_lock(&instance->serialize);
	instance->disconnected = 1;
	if (instance->thread_pid >= 0)
		kill_proc(instance->thread_pid, SIGTERM, 1);
	mutex_unlock(&instance->serialize);

	wait_for_completion(&instance->thread_exited);

	mutex_lock(&instance->serialize);
	    || (rcv_buf_size > UDSL_MAX_RCV_BUF_SIZE)
	list_for_each_entry(vcc_data, &instance->vcc_list, list)
		vcc_release_async(vcc_data->vcc, -EPIPE);
	    || (snd_buf_size > UDSL_MAX_SND_BUF_SIZE))
	mutex_unlock(&instance->serialize);

	tasklet_disable(&instance->rx_channel.tasklet);
	tasklet_disable(&instance->tx_channel.tasklet);

	for (i = 0; i < num_rcv_urbs + num_snd_urbs; i++)
		usb_kill_urb(instance->urbs[i]);
	    || (rcv_buf_size < 1)
	    || (rcv_buf_size > UDSL_MAX_BUF_SIZE)
	    || (snd_buf_size < 1)
	    || (snd_buf_size > UDSL_MAX_BUF_SIZE))

	/* turn usbatm_[rt]x_process into something close to a no-op */
	/* no need to take the spinlock */
	INIT_LIST_HEAD(&instance->rx_channel.list);
	INIT_LIST_HEAD(&instance->tx_channel.list);

	tasklet_enable(&instance->rx_channel.tasklet);
	tasklet_enable(&instance->tx_channel.tasklet);

	if (instance->atm_dev && instance->driver->atm_stop)
		instance->driver->atm_stop(instance, instance->atm_dev);

	if (instance->driver->unbind)
		instance->driver->unbind(instance, intf);

	instance->driver_data = NULL;

	for (i = 0; i < num_rcv_urbs + num_snd_urbs; i++) {
		kfree(instance->urbs[i]->transfer_buffer);
		usb_free_urb(instance->urbs[i]);
	}

	kfree(instance->cell_buf);

	/* ATM finalize */
	if (instance->atm_dev)
		atm_dev_deregister(instance->atm_dev);

	if (sizeof(struct usbatm_control) > sizeof(((struct sk_buff *) 0)->cb)) {
		printk(KERN_ERR "%s unusable with this kernel!
", usbatm_driver_name);
		return -EIO;
	}
	usbatm_put_instance(instance);	/* taken in usbatm_usb_probe */
}
EXPORT_SYMBOL_GPL(usbatm_usb_disconnect);


/***********
**  init  **
***********/

static int __init usbatm_usb_init(void)
{
	dbg("%s: driver version %s", __func__, DRIVER_VERSION);

	BUILD_BUG_ON(sizeof(struct usbatm_control) > sizeof(((struct sk_buff *) 0)->cb));

	if ((num_rcv_urbs > UDSL_MAX_RCV_URBS)
	    || (num_snd_urbs > UDSL_MAX_SND_URBS)
	    || (rcv_buf_bytes < 1)
	    || (rcv_buf_bytes > UDSL_MAX_BUF_SIZE)
	    || (snd_buf_bytes < 1)
	    || (snd_buf_bytes > UDSL_MAX_BUF_SIZE))
		return -EINVAL;

	return 0;
}
module_init(usbatm_usb_init);

static void __exit usbatm_usb_exit(void)
{
	dbg("%s", __func__);
}
module_exit(usbatm_usb_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

/************
**  debug  **
************/

#ifdef VERBOSE_DEBUG
static int usbatm_print_packet(const unsigned char *data, int len)
{
	unsigned char buffer[256];
	int i = 0, j = 0;

	for (i = 0; i < len;) {
		buffer[0] = '\0';
		sprintf(buffer, "%.3d :", i);
		for (j = 0; (j < 16) && (i < len); j++, i++) {
			sprintf(buffer, "%s %2.2x", buffer, data[i]);
		}
		dbg("%s", buffer);
	}
	return i;
}
#endif
