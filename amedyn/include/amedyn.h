/*
  Zyxel 630-11 & Asus AAM6000UG definitions
  Copyright (C) 2003 Josep Comas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Author     : Josep Comas <jcomas@gna.es>
  Creation   : 05/7/2003

  Description: Definitions for Zyxel 630-11 & Asus AAM6000UG with Alcatel chipset.

  Log:

  21/7/2003 Josep Comas
  Added support for Asus AAM6000UG

*/

#ifndef _AMEDYN_H_
#define _AMEDYN_H_


/* uncomment following line to print debug info or define in compiler parameters: */
/* #define DEBUG 1 */
/* uncomment following line to print debug transfers or define in compiler parameters: */
/* #define DEBUG_TRANSFER 1 */

#ifdef DEBUG
#define PDEBUG(arg...)  printf(arg)
#else
#define PDEBUG(arg...)
#endif

/* usb input and ouput address against modem */
#define USB_IN_INFO  0x81    /* IN endpoint address, read modem status */
#define USB_OUT_INFO 0x01    /* OUT endpoint address */
#define USB_IN_FIRM  0x85    /* IN endpoint address, read config */
#define USB_OUT_FIRM 0x05    /* OUT endpoint address, send firmware */
#define USB_IN_DATA  0x87    /* IN endpoint address, receive ATM cells */
#define USB_OUT_DATA 0x07    /* OUT endpoint address, send ATM cells */

/* timeouts & retries */
#define DATA_TIMEOUT 5000     /* Timeout (ms) for in/out data packets */ 
#define CTRL_TIMEOUT 1000     /* Timeout (ms) for in/out control packets */
#define TIMEOUT_ADD  1000     /* We get timeout then add additional time */
#define SEND_BULK_RETRIES 4   /* Max retries when you send a bulk packet */
#define READ_BULK_RETRIES 4   /* Max retries when you wait a bulk packet */
#define CTRL_MSG_RETRIES  4   /* Max retries when you transfer a control message */
#define MAX_WAIT_LINE_UP 90   /* Seconds to wait until ADSL line is up */

/* usb errors */
#define EPIPE      32        /* When we receive a STALL */
#define ETIMEDOUT 110        /* When we receive a NAK */

/* messages */
#define MAX_DATA_TRANSFER 4096       /* Maximum bytes that we can transfer through USB bus */

/* status of modem */
#define MODEM_DOWN  0x00
#define MODEM_WAIT  0x08
#define MODEM_INIT  0x10
#define MODEM_UP    0x20

#define LINE_UP 0x50
/* usb codes */
#define VENDOR_REQUEST_OUT 0x40      /* Vendor specific requests, OUT direction */
#define VENDOR_REQUEST_IN 0xC0       /* Vendor specific requets, IN direction */

/* modem internal characteristics */
struct usb_modem_char {
  unsigned int vid;  /* VendorID */
  unsigned int pid;  /* ProductID */
  char *firmfile;  /* firmware file name */
  char *initfirmfile;  /* init firmware file name */
  int datamax;  /* maximum data that we can send in a block */
};

/* info about modem */
struct usb_modem_info
{
  int modem_status;
  char firm_version[5];		/* firmware version */
  char mac[6];			/* MAC address */
  int down_bitrate;		/* download bitrate */
  int up_bitrate;		/* upload bitrate */
  int link_status;		/* link status */
  int line_status;		/* line status */
  int operational_mode;		/* operational mode */

  int down_bitrate_percent;
  int up_bitrate_percent;
  unsigned int down_attenuation;
  unsigned int up_attenuation;

  char tons[256];		/* bits/simbolo */
  float potenciat, potenciar;
  float ruidot, ruidor;

  unsigned int NearFecNotInterleaved;
  unsigned int NearFecInterleaved;
  unsigned int NearCrcNotInterleaved;
  unsigned int NearCrcInterleaved;
  unsigned int NearHecNotInterleaved;
  unsigned int NearHecInterleaved;

  unsigned int FarFecNotInterleaved;
  unsigned int FarFecInterleaved;
  unsigned int FarCrcNotInterleaved;
  unsigned int FarCrcInterleaved;
  unsigned int FarHecNotInterleaved;
  unsigned int FarHecInterleaved;

// From cxacru
/*
  // version
  unsigned char mod_major;
  unsigned char mod_minor;
  unsigned char mod_release;
  unsigned char mod_speed;
  int hard_version;  // controller version
  char firm_version[5];
  unsigned char mac[6];

  // transmit/configuration
  unsigned int atm_cell_tx;
  unsigned int atm_cell_rx;
  unsigned int oam_loopback;
  short vpi;
  int vci;

  //state
  int link_status;
  int line_status;
  unsigned int open_mode;
  unsigned int operational_mode;  //modulation
  unsigned int data_path;
  unsigned int startup_attempts;
  unsigned int line_startable;
  unsigned int link_lost;
  int down_bitrate; // *
  int up_bitrate; // *
  unsigned int adsl_headend;
  unsigned int adsl_headend_env;

  //advanced
  unsigned char extended_info;
  int up_snr_margin;
  int down_snr_margin;
  unsigned int up_attenuation; // *
  unsigned int down_attenuation; // *
  int transmitter_power;
  unsigned int up_bits_frame;
  unsigned int down_bits_frame;
  unsigned int up_crc_errors;
  unsigned int down_crc_errors;
  unsigned int up_fec_errors;
  unsigned int down_fec_errors;
  unsigned int up_hec_errors;
  unsigned int down_hec_errors;
*/

};

/* Command-line options */ 

#endif /* _AMEDYN_H_ */
