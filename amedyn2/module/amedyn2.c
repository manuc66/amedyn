/******************************************************************************
 *  amedyn.c  -  Zyxel 630-11/13 and Asus AAM600UG ALC USB xDSL modem driver
 *
 *  Copyright (C) 2001, Alcatel
 *  Copyright (C) 2003, Duncan Sands
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
#include <asm/page.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/firmware.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/usb_ch9.h>
#include <linux/workqueue.h>

#include "usbatm.h"

#define DRIVER_AUTHOR	"Aurelio Arroyo, Josep Comas, David Woodhouse, Duncan Sands"
#define DRIVER_VERSION	"0.9.2"
#define DRIVER_DESC	"Zyxel 630-11/13 and Asus AAM600UG ALC USB driver version " DRIVER_VERSION

static const char amedyn_driver_name[] = "amedyn";
static const char amedyn_driver_name[] = "amedyn2";

/* Alcatel Microelectronics new reference design */
#define AME_VENDORID2		        0x06b9  /* Vendor = Zyxel */
#define AME_PRODUCTID2		        0xa5a5  /* Product = 630-11 & 630-13 */
// /* Modem types */
// #define UDSL_MODEM_TYPE1                0  /* Alcatel reference design */
// #define UDSL_MODEM_TYPE2                1  /* Conexant reference design */
// #define UDSL_MODEM_TYPE3                2  /* 3Com reference design (Alcatel DSP) */

#define AME_VENDORID3		        0x0b05  /* Vendor = Asustek */
#define AME_PRODUCTID3		        0x6206  /* Product = AAM6000UG with Alcatel chipset */
#define AME_PRODUCTID4		        0x0005  /* Product = */

#define CTRL_TIMEOUT 2000	/* milliseconds */
#define DATA_TIMEOUT 2000	/* milliseconds */

#define OFFSET_7	1		/* size 1 */

#define MIN_POLL_DELAY		5000	/* milliseconds */
#define MAX_POLL_DELAY		60000	/* milliseconds */
// #define DEFAULT_DL_512_FIRST	0
// #define DEFAULT_SW_BUFFERING	0

#define DEFAULT_ALTSETTING	1
// static int dl_512_first = DEFAULT_DL_512_FIRST;
// static int sw_buffering = DEFAULT_SW_BUFFERING;

#define DEFAULT_BULK_ALTSETTING	1
static int altsetting = DEFAULT_ALTSETTING;

module_param(altsetting, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(altsetting,
// module_param(dl_512_first, bool, S_IRUGO | S_IWUSR);
// MODULE_PARM_DESC(dl_512_first,
// 		 "Read 512 bytes before sending firmware (default: "
// 		 __MODULE_STRING(DEFAULT_DL_512_FIRST) ")");
// 
// module_param(sw_buffering, bool, S_IRUGO | S_IWUSR);
// MODULE_PARM_DESC(sw_buffering,
// 		 "Enable software buffering (default: "
// 		 __MODULE_STRING(DEFAULT_SW_BUFFERING) ")");

		 "Alternative setting for data interface (default: "
		 __MODULE_STRING(DEFAULT_ALTSETTING) ")");
#define DEFAULT_ISOC_ALTSETTING	1 /* This modem don't have iso*/
static int altsetting = DEFAULT_BULK_ALTSETTING;

static unsigned int altsetting = DEFAULT_BULK_ALTSETTING;

#define ANALOG 0x15
#define ISDN 0x11

static int linetype = 0;


module_param(altsetting, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(altsetting,
    		"Alternative setting for data interface (bulk_default: "
#define ENDPOINT_DATA_IN		0x87
#define ENDPOINT_DATA_OUT		0x07

#define UDSL_IOCTL_LINE_UP		1
#define UDSL_IOCTL_LINE_DOWN		2

#define INTERFACE_DATA 1
#define ENDPOINT_BULK_DATA		0x07
#define ENDPOINT_FIRMWARE		0x05

// from the userspace tool
#define AMEDYN_USB_IN_INFO  0x81    // IN endpoint address, read modem status /
#define ENDPOINT_FIRMWARE_IN  0x85    // IN endpoint address, read config /
#define ENDPOINT_FIRMWARE_IN  0x85  /* IN endpoint address, read config */

#define hex2int(c) ( (c >= '0') && (c <= '9') ? (c - '0') : ((c & 0xf) + 9) )

	int altsetting;
struct amedyn_instance_data {
	struct usbatm_data *usbatm;

	struct work_struct status_checker;
	unsigned int altsetting;

	struct delayed_work status_checker;
	
	unsigned char last_status;

	int poll_delay; /* milliseconds */
	
	int datamax;  /* maximum data that we can send in a block */

	char *initfirmfile;  /* init firmware file name */
	char *firmfile;  /* firmware file name */

	char linetype; /* 0x15 = analog line, 0x11 ISDN line */

	unsigned char bufconf[8];  /* buffer to save config bytes */
	int  havebufconf; /* 1 after read bufconf */ 
	
	unsigned char scratch_buffer[0x10];
};

static int amedyn_line_down_signal (struct amedyn_instance_data *instance)
{
	struct usbatm_data *usbatm = instance->usbatm;
	struct usb_device *usb_dev = usbatm->usb_dev;
//	unsigned char buf[0x1ff];
	struct atm_dev *atm_dev = usbatm->atm_dev;
	int ret;

	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x03, 0x40, 0x03, 0x00,
			      NULL, 0x00, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s: Failed sync down command: %d
", __func__, ret);
		return ret; }
	atm_dbg(usbatm, "%s: OK sync down command
", __func__);
	atm_dbg(usbatm, "%s: OK sync dowm command
", __func__);

	atm_dev->signal = ATM_PHY_SIG_UNKNOWN;
	
	return 0;
}

int jump_to_address(struct amedyn_instance_data *instance, unsigned int place)
{
	struct usb_device *dev = instance->usbatm->usb_dev;
	unsigned char buf[6];  /* buffer */

	buf[0] = 0x08; // Command (= set base address)
	buf[1] = 0x04; // Length (= 4 bytes)
	// Value (base address = place)
	buf[2] = (place >> 24) & 0xff;
	buf[3] = (place >> 16) & 0xff;
	buf[4] = (place >> 8) & 0xff;
	buf[5] = place & 0xff;
	if (usb_bulk_msg (dev, usb_sndbulkpipe(dev, ENDPOINT_FIRMWARE), buf, 6, NULL,  DATA_TIMEOUT))
		return -1;
	buf[0] = 0x00;  // Command (= jump?)
	buf[1] = 0x01;  // Length (= 1 byte)
	buf[2] = 0x14;  // Value (= jump to base address)
	if (usb_bulk_msg (dev, usb_sndbulkpipe(dev, ENDPOINT_FIRMWARE), buf, 3, NULL,  DATA_TIMEOUT))
		return -1;
	return 0;
}

/* From userspace tool */
/***************
**  firmware  **
***************/

/* From userspace tool */
void format_message(int cmd, int ldata, int address, char *bufin)
/* format a message */
static void format_message(int cmd, int ldata, int address, char *bufin)
{
	char buf[8];  /* initial bytes of a message */

	memset(buf, 0, sizeof(buf));
	buf[0] = cmd & 0xff;    /* usb command */
	/* address */
	buf[2] = address & 0xff;
	buf[3] = (address >> 8) & 0xff;
	buf[4] = (address >> 16) & 0xff;
	buf[5] = (address >> 24) & 0xff;
	/* data length */
	buf[6] = ldata & 0xff;
	buf[7] = (ldata >> 8) & 0xff;
	buf[1] = buf[6] + 6;
	memcpy(bufin, buf, sizeof(buf));
}

int send_block(struct amedyn_instance_data *instance, int place, char *bufin, int len)
/* From userspace tool */
static int send_block(struct amedyn_instance_data *instance, int place, char *bufin, int len)
{
	struct usb_device *dev = instance->usbatm->usb_dev;
	char buf[0x1ff];  /* = modem_char.datamax + 8 */

	if ((bufin == NULL) || (len > instance->datamax))
		return -1;
	memset(buf, 0, sizeof(buf));
	format_message(0x88, len, place, buf);
	memcpy(buf+8, bufin, len);

	if (usb_bulk_msg (dev, usb_sndbulkpipe(dev, ENDPOINT_FIRMWARE), buf, len+8, NULL,  DATA_TIMEOUT))
		return -1;

	return 0;
}

static int amedyn_upload_firmware(struct amedyn_instance_data *instance,
				     const struct firmware *fw1,
				     const struct firmware *fw2)
{
	unsigned char buf[0x1ff];
	unsigned char *buffer;
	struct usbatm_data *usbatm = instance->usbatm;
	struct usb_interface *intf;
	struct usb_device *usb_dev = usbatm->usb_dev;
	unsigned char *buf = instance->scratch_buffer;
	single transfer. Probably it can be send with the firmware too, as the
	speedtch.c driver do.
	*/

	char jump_to_address_0x00000000[9] =
	    {0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14} ;

	usb_dbg(usbatm, "%s entered
", __func__);

	if (!(buffer = (unsigned char *)__get_free_page(GFP_KERNEL))) {
		ret = -ENOMEM;
		usb_dbg(usbatm, "%s: no memory for buffer!
", __func__);
		goto out;
	}

	if (!(intf = usb_ifnum_to_if(usb_dev, 2))) {
		ret = -ENODEV;
		usb_dbg(usbatm, "%s: interface not found!
", __func__);
		goto out_free;
	}
		memcpy(buf, fw1->data + offset, thislen);
		if (send_block(instance, offset, buf, thislen))
	usb_clear_halt(usb_dev, usb_sndbulkpipe(usb_dev, ENDPOINT_FIRMWARE));
	usb_clear_halt(usb_dev, usb_rcvbulkpipe(usb_dev, ENDPOINT_FIRMWARE_IN));

	/* Init firmware upload*/
	offset = 0;
	do {
		int thislen = min_t(int, instance->datamax, fw1->size - offset);
		memcpy(buffer, fw1->data + offset, thislen);
		if (send_block(instance, offset, buffer, thislen))
			goto out_free;
		offset+=thislen;
		buf[0] = 0x40; buf[1] = 0x01; buf[2] = 0x12;
		ret = usb_bulk_msg (usb_dev, usb_sndbulkpipe(usb_dev, ENDPOINT_FIRMWARE), buf, 3, NULL,  DATA_TIMEOUT);
		if (ret < 0) {
			dbg("amedyn_upload_firmware: write Init firmware to modem failed (%d)!", ret);
	if (jump_to_address(instance, 0x00000000))
			dbg("amedyn_upload_firmware: write Init firmware to modem failed (%d)!", ret);
	ret = usb_bulk_msg (usb_dev, usb_rcvbulkpipe(usb_dev, ENDPOINT_FIRMWARE_IN), buf, 0x1ff, NULL,  DATA_TIMEOUT);
		dbg("amedyn_upload_firmware: read bufconf failed (%d)!", ret);
		}
	} while (offset < fw1->size );

	dbg("amedyn_upload_firmware: Init load");

	memcpy(instance->bufconf, buf+0xb9, 8);
	if (usb_bulk_msg (usb_dev, usb_sndbulkpipe(usb_dev, ENDPOINT_FIRMWARE),
		    jump_to_address_0x00000000, 9, NULL,  DATA_TIMEOUT))
		goto out_free;

	/* read something needed */
	ret = usb_bulk_msg (usb_dev, usb_rcvbulkpipe(usb_dev, ENDPOINT_FIRMWARE_IN), buffer, 0x1ff, NULL,  DATA_TIMEOUT);
	if (ret < 0) {
		dbg("amedyn_upload_firmware: read bufconf failed (%d)!", ret);
		goto out_free;
		memcpy(buf, fw2->data + offset, thislen);
		if (send_block(instance, offset, buf, thislen))
	memcpy(instance->bufconf, buffer+0xb9, 8);

	dbg("amedyn_upload_firmware: Read bufconf OK");
	instance->havebufconf=1;
	
	/* Firmware upload */
	offset = 0;
	do {
		int thislen = min_t(int, instance->datamax, fw2->size - offset);
		memcpy(buffer, fw2->data + offset, thislen);
		if (send_block(instance, offset, buffer, thislen))
			goto out_free;
		offset+=thislen;
		buf[0] = 0x40; buf[1] = 0x01; buf[2] = 0x12;
		ret = usb_bulk_msg (usb_dev, usb_sndbulkpipe(usb_dev, ENDPOINT_FIRMWARE), buf, 3, NULL,  DATA_TIMEOUT);
		if (ret < 0) {
	if (jump_to_address(instance, 0x00000000))
			goto out_free;
		}
	} while (offset < fw2->size );
	msleep(2000);

	dbg("amedyn_upload_firmware: Firmware load");

	if (usb_bulk_msg (usb_dev, usb_sndbulkpipe(usb_dev, ENDPOINT_FIRMWARE),
		    jump_to_address_0x00000000, 9, NULL,  DATA_TIMEOUT))
		goto out_free;

	msleep(10000);

	dbg("PostInit...");

	/* configure something */

	ret = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
			      0x0a, 0xc0, 0x0c, 0x08,
			      buf, 0x01, CTRL_TIMEOUT);
	if (ret < 1) {
		dbg("amedyn_upload_firmware: PostInit fail at local urb 1: %d
", ret);
		goto out_free; }
	value = buf[0];

	usb_clear_halt(usb_dev, usb_rcvbulkpipe(usb_dev, AMEDYN_USB_IN_INFO));

	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
	
		goto out_free; }

	for (i = 0xc2; i <= 0xcd; i++) {
		ret = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
			 	      value, 0xc0, 0x03, i,
			    	      buf, 0x03, CTRL_TIMEOUT);
		if (ret < 3) {
			dbg("amedyn_upload_firmware: PostInit fail at local urb 3: %d
", ret);
			goto out_free; }
static int amedyn_find_firmware(struct usb_interface *intf, char* phase,
				const struct firmware **fw_p)
   	}

	/* success */
	usb_dbg(usbatm, "%s: firmware upload OK
", __func__);

	/* Delay to allow firmware to start up. We can do this here
	dev_dbg(dev, "%s: looking for %s
", __func__, phase);
	   because we're in our own kernel thread anyway. */
	msleep_interruptible(1000);

		dev_warn(dev, "no stage %s firmware found
", phase);
	if ((ret = usb_set_interface(usb_dev, INTERFACE_DATA, instance->altsetting)) < 0) {
		usb_err(usbatm, "%s: setting interface to %d failed (%d)!
", __func__, instance->altsetting, ret);
		goto out_free;
	}
	dev_info(dev, "found firmware %s
", phase);
	ret = 0;

out_free:
	free_page((unsigned long)buffer);
out:
	return ret;
}

static int amedyn_find_firmware(struct usbatm_data *usbatm, struct usb_interface *intf,
				char* phase, const struct firmware **fw_p)
{
	if ((ret = amedyn_find_firmware(intf, instance->initfirmfile, &fw1)) < 0)
			return ret;
	char buf[16];

	if ((ret = amedyn_find_firmware(intf, instance->firmfile, &fw2)) < 0) {
	sprintf(buf, "%s", phase);
	usb_dbg(usbatm, "%s: looking for %s
", __func__, phase);

	if (request_firmware(fw_p, buf, dev)) {
		usb_dbg(usbatm, "no stage %s firmware found
", phase);
	ret = amedyn_upload_firmware(instance, fw1, fw2);
	}

	usb_info(usbatm, "found firmware %s
", phase);

	return 0;
}

static int amedyn_heavy_init(struct usbatm_data *usbatm, struct usb_interface *intf)
{
	const struct firmware *fw1, *fw2;
	struct amedyn_instance_data *instance = usbatm->driver_data;
	int ret;

	if ((ret = amedyn_find_firmware(usbatm, intf, instance->initfirmfile, &fw1)) < 0)
		return ret;

	if ((ret = amedyn_find_firmware(usbatm, intf, instance->firmfile, &fw2)) < 0) {
		release_firmware(fw1);
		return ret;
	}

	if ((ret = amedyn_upload_firmware(instance, fw1, fw2)) < 0)
		usb_err(usbatm, "%s: firmware upload failed (%d)!
", __func__, ret);

	release_firmware(fw2);
	release_firmware(fw1);

	return ret;
}


/**********
**  ATM  **
**********/

static int amedyn_read_status(struct amedyn_instance_data *instance)
{
	struct usbatm_data *usbatm = instance->usbatm;
	struct usb_device *usb_dev = usbatm->usb_dev;
	unsigned char *buf = instance->scratch_buffer;
	int ret;

	do {
		memset(buf, 0, 0x10);
		ret= usb_bulk_msg (usb_dev, usb_rcvbulkpipe(usb_dev, AMEDYN_USB_IN_INFO),
				   buf, 0x10, NULL,  DATA_TIMEOUT);
/* Do while buf[] isn't line status info, buf[0]=0x01,or a error code, ret < 0. */
	} while ( buf[0]  != 0x01 && ret >= 0 );

	if (ret < 0) {
//	atm_dbg(usbatm, "%s: Info type - %02x
", __func__, buf[0]);
		atm_warn(usbatm, "Error retrieving info!
");
		return ret;
	}

/*	atm_dbg(usbatm, "%s: Info type - %02x
", __func__, buf[0]); */

	return 0;
}

static int amedyn_start_synchro(struct amedyn_instance_data *instance)
{
	struct usbatm_data *usbatm = instance->usbatm;
	struct usb_device *usb_dev = usbatm->usb_dev;
	unsigned char buf[0x1ff];   /* buffer */
	int ret, i;

	atm_dbg(usbatm, "%s entered
", __func__);
	
	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x0b, 0x40, 0x0c, 0x00,
			      NULL, 0x00, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 1: %d
", __func__, ret);
		return ret;
	}

	if ( instance->havebufconf == 1 ) {
	    for (i = 0xba; i <= 0xc1; i++) {
		ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x06, 0x40, 0x03, i,
			      &(instance->bufconf[i-0xba]), 0x01, CTRL_TIMEOUT);
		if (ret < 0) {
		    atm_warn(usbatm, "%s failed on local urb 2: %d
", __func__, ret);
		    return ret; }
	    }
	    atm_dbg(usbatm, "%s: bufconf send.
", __func__);
	}
	else
	    atm_dbg(usbatm, "%s: bufconf not send.
", __func__);
	

/* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
/* 0x1fd in CTRLE protocol */
/* 0x15 = analog line, 0x11 ISDN line */
	buf[0] = instance->linetype;
	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x06, 0x40, 0x03, 0x1fd,
			      buf, 0x01, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 3: %d
", __func__, ret);
		return ret;
	}

	buf[0] = 0x01;
	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x06, 0x40, 0x03, 0x4a,
			      buf, 0x01, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 4: %d
", __func__, ret);
		return ret;
	}

	buf[0] = 0x00;
	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x06, 0x40, 0x03, 0x4b,
			      buf, 0x01, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 5: %d
", __func__, ret);
		return ret;
	}

	buf[0] = 0x00;
	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x06, 0x40, 0x03, 0x4c,
			      buf, 0x01, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 6: %d
", __func__, ret);
		return ret;
	}

/* Only this command seem to be necesary to resync. */
	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x02, 0x40, 0x03, 0x00,
			      NULL, 0x00, CTRL_TIMEOUT);
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 7: %d
", __func__, ret);
		return ret;
	}

	ret = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
			      0x0e, 0xc0, 0x03, 0x00,
			      buf, 0x0c, CTRL_TIMEOUT);
		if (instance->poll_delay < MAX_POLL_DELAY)
			instance->poll_delay *= 2;
	if (ret < 0) {
		atm_warn(usbatm, "%s failed on local urb 8: %d
", __func__, ret);
static void amedyn_check_status(struct amedyn_instance_data *instance)
		return ret;
	if (instance->poll_delay > MIN_POLL_DELAY)
		instance->poll_delay /= 2;
	}
}

static void amedyn_check_status(struct work_struct *work)
{

	struct amedyn_instance_data *instance =
		container_of(work, struct amedyn_instance_data,
//	    printk(KERN_NOTICE "amedyn_get_status return useless info
");
			     status_checker.work); 
	struct usbatm_data *usbatm = instance->usbatm;
	struct atm_dev *atm_dev = usbatm->atm_dev;
	unsigned char *buf = instance->scratch_buffer;
	int ret;
	unsigned char status;

#ifdef VERBOSE_DEBUG
	atm_dbg(usbatm, "%s entered
", __func__);
#endif

	ret = amedyn_read_status(instance);
	if (ret < 0) {
		atm_warn(usbatm, "error %d fetching device status
", ret);
		instance->poll_delay = min(2 * instance->poll_delay, MAX_POLL_DELAY);
		return;
	}

	instance->poll_delay = max(instance->poll_delay / 2, MIN_POLL_DELAY);

	if ( buf[0] == 0x01 )
		atm_dbg(usbatm, "%s: line state %02x
", __func__, buf[OFFSET_7]);
	else {
		if (atm_dev->signal != ATM_PHY_SIG_LOST) {
			atm_dev->signal = ATM_PHY_SIG_LOST;
	    return ;
	}
	}

	switch (buf[OFFSET_7]) {
	case 0x00:
		atm_dev->signal = ATM_PHY_SIG_LOST;
		atm_info(usbatm, "ADSL line is down
");
		/* It'll never resync again unless we ask it to... */
		amedyn_line_down_signal (instance);
		ret = amedyn_start_synchro(instance);
		break;
//		atm_dbg(usbatm, "%s: line state %02x
", __func__, buf[OFFSET_7]);
//	else {
	case 0x08:
		if (buf[OFFSET_7] != instance->last_status) {
//	    atm_dbg(usbatm, "amedyn_get_status return useless info
");
//	    return ;
//	}
		}
		break;
		atm_dbg(usbatm, "%s: line state %02x
", __func__, buf[OFFSET_7]);
	else {
	case 0x10:
		if (buf[OFFSET_7] != instance->last_status) {
	    atm_dbg(usbatm, "amedyn_get_status return useless info
");
	    return ;
	} */
		}
		break;

	status = buf[OFFSET_7];	
	case 0x20:
		if (buf[OFFSET_7] != instance->last_status) {

	if ((status != instance->last_status) || !status) {
			atm_info(usbatm,
				 "ADSL line is up
");
		}
		break;

		switch (status) {
	default:
		if (buf[OFFSET_7] != instance->last_status) {
		case 0x00:
			atm_dev->signal = ATM_PHY_SIG_LOST;
			atm_info(usbatm, "Unknown line state %02x
", buf[OFFSET_7]);
				atm_info(usbatm, "ADSL line is down
");
			/* It may never resync again unless we ask it to... */
		break;
			amedyn_line_down_signal (instance);
	instance->last_status = buf[OFFSET_7];
			ret = amedyn_start_synchro(instance);
			break;

		case 0x08:
			atm_dev->signal = ATM_PHY_SIG_UNKNOWN;
			atm_info(usbatm, "ADSL line is blocked?
");
			break;
	if ((ret = usb_set_interface(usb_dev, 1, altsetting)) < 0) {
		atm_dbg(usbatm, "%s: usb_set_interface returned %d!
", __func__, ret);
		return ret;
	}


	atm_warn(usbatm, "%s entered
", __func__);
		case 0x10:
			atm_dev->signal = ATM_PHY_SIG_UNKNOWN;
			atm_info(usbatm, "ADSL line is synchronising
");
			break;

		case 0x20:
			atm_dev->signal = ATM_PHY_SIG_FOUND;
			atm_info(usbatm, "ADSL line is up
");
			break;

		default:
			atm_dev->signal = ATM_PHY_SIG_UNKNOWN;
			atm_info(usbatm, "Unknown line state %02x
", status);
			break;
		}
		instance->last_status = status;
	schedule_work(&instance->status_checker);
		return;
	}
}

static void amedyn_status_poll(unsigned long data)
{
	struct amedyn_instance_data *instance = (void *)data;

	schedule_delayed_work(&instance->status_checker, 0);

	/* The following check is racy, but the race is harmless */
	if (instance->poll_delay < MAX_POLL_DELAY)
		mod_timer(&instance->status_checker.timer, jiffies + msecs_to_jiffies(instance->poll_delay));
	else
		atm_warn(instance->usbatm, "Too many failures - disabling line status polling
");
}

static int amedyn_atm_start(struct usbatm_data *usbatm, struct atm_dev *atm_dev)
{
	struct usb_device *usb_dev = usbatm->usb_dev;
	struct amedyn_instance_data *instance = usbatm->driver_data;
	int i, ret;
	unsigned char mac_str[13];

	atm_dbg(usbatm, "%s entered
", __func__);

	/* Set MAC address, it is stored in the serial number */
	memset(atm_dev->esi, 0, sizeof(atm_dev->esi));
	if (usb_string(usb_dev, usb_dev->descriptor.iSerialNumber, mac_str, sizeof(mac_str)) == 12) {
		for (i = 0; i < 6; i++)
			atm_dev->esi[i] = (hex2int(mac_str[i * 2]) * 16) + (hex2int(mac_str[i * 2 + 1]));
	.owner		= THIS_MODULE,
	}

	/* Start modem synchronisation */
	ret = amedyn_start_synchro(instance);

	/* Start status polling */
	mod_timer(&instance->status_checker.timer, jiffies + msecs_to_jiffies(1000));

	return 0;
}

static void amedyn_atm_stop(struct usbatm_data *usbatm, struct atm_dev *atm_dev)
{
	struct amedyn_instance_data *instance = usbatm->driver_data;
	
	mb(); // Delete?
			 const struct usb_device_id *id,
			 int *need_heavy_init)
	atm_dbg(usbatm, "%s entered
", __func__);

	del_timer_sync(&instance->status_checker.timer);
	struct usb_interface *cur_intf;

	mb(); /* Delete? */

	flush_scheduled_work();
}


/**********
**  USB  **

static struct usb_device_id amedyn_usb_ids[] = {
		usb_dbg(usbatm, "%s: wrong device class %d
", __func__, usb_dev->descriptor.bDeviceClass);

MODULE_DEVICE_TABLE(usb, amedyn_usb_ids);

static int amedyn_usb_probe(struct usb_interface *, const struct usb_device_id *);

static struct usb_driver amedyn_usb_driver = {
	.name		= amedyn_driver_name,
	.probe		= amedyn_usb_probe,
	.disconnect	= usbatm_usb_disconnect,
	.id_table	= amedyn_usb_ids
};

static void amedyn_release_interfaces(struct usb_device *usb_dev, int num_interfaces) {
				usb_dbg(usbatm, "%s: failed to claim interface %d (%d)
", __func__, i, ret);
	struct usb_interface *cur_intf;
	int i;

	for(i = 0; i < num_interfaces; i++)
		if ((cur_intf = usb_ifnum_to_if(usb_dev, i))) {
			usb_set_intfdata(cur_intf, NULL);
			usb_driver_release_interface(&amedyn_usb_driver, cur_intf);
		}
}

		usb_dbg(usbatm, "%s: no memory for instance data!
", __func__);
static int amedyn_bind(struct usbatm_data *usbatm,
			 struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct usb_interface *cur_intf, *data_intf;
	struct amedyn_instance_data *instance;
	int ifnum = intf->altsetting->desc.bInterfaceNumber;
	int use_isoc; /* This modem don't have iso*/
	use_isoc = 0;
	/* altsetting may change at any moment, so take a snapshot */
	instance->altsetting = altsetting;
	usbatm->flags |= use_isoc ? UDSL_USE_ISOC : 0;

	if (instance->altsetting)
	instance->last_status = 0xff;
		if ((ret = usb_set_interface(usb_dev, INTERFACE_DATA, instance->altsetting)) < 0) {
			usb_err(usbatm, "%s: setting interface to %2d failed (%d)!
", __func__, instance->altsetting, ret);
			instance->altsetting = 0; /* fall back to default */
		}
	if (!instance->altsetting && use_isoc)
		if ((ret = usb_set_interface(usb_dev, INTERFACE_DATA, DEFAULT_ISOC_ALTSETTING)) < 0) {
			usb_dbg(usbatm, "%s: setting interface to %2d failed (%d)!
", __func__, DEFAULT_ISOC_ALTSETTING, ret);
			use_isoc = 0; /* fall back to bulk */
		}

	if (use_isoc) {
		const struct usb_host_interface *desc = data_intf->cur_altsetting;
		const __u8 target_address = USB_DIR_IN | usbatm->driver->isoc_in;
		int i;

		use_isoc = 0; /* fall back to bulk if endpoint not found */

		for (i=0; i<desc->desc.bNumEndpoints; i++) {
			const struct usb_endpoint_descriptor *endpoint_desc = &desc->endpoint[i].desc;

			if ((endpoint_desc->bEndpointAddress == target_address)) {
				use_isoc = (endpoint_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
					USB_ENDPOINT_XFER_ISOC;
				break;
			}
	usb_warn(usbatm, "%s entered
", __func__);
		}

		*need_heavy_init = 0;

		if (!use_isoc)
			usb_info(usbatm, "isochronous transfer not supported - using bulk
");
		*need_heavy_init = 1;
	

	if (!use_isoc && !instance->altsetting)
	usb_dbg(usbatm, "%s: firmware %s loaded
", __func__, need_heavy_init ? "not" : "already");

	INIT_WORK(&instance->status_checker, (void *)amedyn_check_status, instance);
	if (*need_heavy_init)
		if ((ret = usb_reset_device(usb_dev)) < 0)
			usb_err(usbatm, "%s: setting interface to %2d failed (%d)!
", __func__, DEFAULT_BULK_ALTSETTING, ret);
			}
	if (usb_dev->descriptor.idProduct == AME_PRODUCTID2) {
	if (!instance->altsetting)
		instance->altsetting = use_isoc ? DEFAULT_ISOC_ALTSETTING : DEFAULT_BULK_ALTSETTING;

	usbatm->flags |= (use_isoc ? UDSL_USE_ISOC : 0);

		}

//-------------------------------------------------------------------
	INIT_DELAYED_WORK(&instance->status_checker, amedyn_check_status);

	instance->status_checker.timer.function = amedyn_status_poll;
	instance->status_checker.timer.data = (unsigned long)instance;
	instance->last_status = 0xff;
	instance->poll_delay = MIN_POLL_DELAY;

/* ------------------------------------------------------------------- */
	
	if ( linetype == ANALOG ||  linetype == ISDN)
		instance->linetype = linetype;
	
	else {
		dbg("using default line type - 0x15 (analog)");
		instance->linetype = ANALOG;
	}

	instance->havebufconf = 0;
	instance->initfirmfile = "Init-usb.bin";
	if (usb_dev->descriptor.idProduct == AME_PRODUCTID2
	    || usb_dev->descriptor.idProduct == AME_PRODUCTID4) {
//----------------------------------------------------------------------
	    dbg("Config for modem type 2 (Zyxel)");
	    instance->datamax = 0x1a0;
	    instance->firmfile = "fw-usb.bin"; }
	if (usb_dev->descriptor.idProduct == AME_PRODUCTID3) {
	    dbg("Config for modem type 3 (AAM600UG)");
	.owner		= THIS_MODULE,
	    instance->datamax = 0x1f2;
	    instance->firmfile = "Fw-usb_A.bin"; }
	.in		= ENDPOINT_DATA_IN,
	.out		= ENDPOINT_DATA_OUT
	
	/* check whether the modem already seems to be alive */
	ret = usb_bulk_msg(usb_dev, usb_rcvbulkpipe(usb_dev, AMEDYN_USB_IN_INFO),
			   NULL, 0x10, NULL,  DATA_TIMEOUT);

	if (ret >= 0)
		usbatm->flags |= UDSL_SKIP_HEAVY_INIT;
	else
		usbatm->flags = 0;
	    	
	
	usb_dbg(usbatm, "%s: firmware %s loaded
", __func__, usbatm->flags & UDSL_SKIP_HEAVY_INIT ? "already" : "not");

	if (!(usbatm->flags & UDSL_SKIP_HEAVY_INIT))
		if ((ret = usb_reset_device(usb_dev)) < 0) {
			usb_err(usbatm, "%s: device reset failed (%d)!
", __func__, ret);
			goto fail_free;
		}

        usbatm->driver_data = instance;

	return 0;

fail_free:
	kfree(instance);
fail_release:
	amedyn_release_interfaces(usb_dev, num_interfaces);
	return ret;
}

static void amedyn_unbind(struct usbatm_data *usbatm, struct usb_interface *intf)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct amedyn_instance_data *instance = usbatm->driver_data;

	usb_dbg(usbatm, "%s entered
", __func__);

	amedyn_release_interfaces(usb_dev, usb_dev->actconfig->desc.bNumInterfaces);
	kfree(instance);
}


/***********
**  init  **
***********/

static struct usbatm_driver amedyn_usbatm_driver = {
	.driver_name	= amedyn_driver_name,
	.bind		= amedyn_bind,
	.heavy_init	= amedyn_heavy_init,
	.unbind		= amedyn_unbind,
	.atm_start	= amedyn_atm_start,
	.atm_stop	= amedyn_atm_stop,
	.bulk_in	= ENDPOINT_BULK_DATA,
	.bulk_out	= ENDPOINT_BULK_DATA,
};

static int amedyn_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	return usbatm_usb_probe(intf, id, &amedyn_usbatm_driver);
}

static int __init amedyn_usb_init(void)
{
	dbg("%s: driver version %s", __func__, DRIVER_VERSION);

	return usb_register(&amedyn_usb_driver);
}

static void __exit amedyn_usb_cleanup(void)
{
	dbg("%s", __func__);

	usb_deregister(&amedyn_usb_driver);
}

module_init(amedyn_usb_init);
module_exit(amedyn_usb_cleanup);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
