/*
  Zyxel 630-11 & Asus AAM6000UG init process
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
  Creation   : 7/7/2003

  Description: This program inits Zxyel 630-11 & Asus AAM6000UG (USB ADSL Modems with Alcatel chipset).

  Log:

  7/7/2003 Josep Comas
  Initial release

  14/7/2003 Josep Comas
  Wait after firmware is sent
  Reduce instructions for config bytes

  21/7/2003 Josep Comas
  Added support for Asus AAM6000UG

  24/9/2003 Josep Comas
  Commented blinking leds
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <usb.h>
#include <usbi.h>
#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include "amedyn.h"


/* translation files */
#define TF_CODE "amload"

/* prepare to send usb firmware */
char inifirm[] = {
0x04, 0xa0, 0x00, 0x47, 0x02, 0x00, 0x00, 0xea,
0x01, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0xea, 0xff, 0xff, 0xff, 0xea, 0xd1, 0x00, 0xa0, 0xe3,
0x00, 0xf0, 0x29, 0xe1, 0x01, 0xd6, 0xa0, 0xe3, 0xd2, 0x00, 0xa0, 0xe3, 0x00, 0xf0, 0x29, 0xe1,
0x01, 0xd6, 0xa0, 0xe3, 0xd3, 0x00, 0xa0, 0xe3, 0x00, 0xf0, 0x29, 0xe1, 0x01, 0xd6, 0xa0, 0xe3,
0xb4, 0x23, 0x9f, 0xe5, 0x00, 0x10, 0x92, 0xe5, 0xff, 0x10, 0x01, 0xe2, 0x7f, 0x20, 0xa0, 0xe3,
0x02, 0x00, 0x51, 0xe1, 0x26, 0x00, 0x00, 0x0a, 0xac, 0x03, 0x9f, 0xe5, 0x08, 0x00, 0x80, 0xe2,
0xb8, 0x13, 0x9f, 0xe5, 0x00, 0x10, 0x80, 0xe5, 0x5f, 0x10, 0xe0, 0xe3, 0x5d, 0x30, 0xe0, 0xe3,
0x5c, 0x50, 0xe0, 0xe3, 0x5b, 0x70, 0xe0, 0xe3, 0xa8, 0x03, 0x9f, 0xe5, 0xa0, 0x23, 0x9f, 0xe5,
0xa4, 0x43, 0x9f, 0xe5, 0xa4, 0x63, 0x9f, 0xe5, 0x00, 0x10, 0x80, 0xe5, 0x00, 0x30, 0x82, 0xe5,
0x00, 0x50, 0x84, 0xe5, 0x00, 0x50, 0x84, 0xe5, 0x00, 0x50, 0x84, 0xe5, 0x00, 0x50, 0x84, 0xe5,
0x00, 0x50, 0x84, 0xe5, 0x00, 0x50, 0x84, 0xe5, 0x00, 0x50, 0x84, 0xe5, 0x00, 0x50, 0x84, 0xe5,
0x00, 0x70, 0x86, 0xe5, 0x50, 0x03, 0x9f, 0xe5, 0x08, 0x00, 0x80, 0xe2, 0x58, 0x13, 0x9f, 0xe5,
0x00, 0x10, 0x80, 0xe5, 0x68, 0x03, 0x9f, 0xe5, 0x68, 0x13, 0x9f, 0xe5, 0x00, 0x10, 0x80, 0xe5,
0x24, 0x23, 0x9f, 0xe5, 0x00, 0x10, 0x92, 0xe5, 0xff, 0x10, 0x01, 0xe2, 0x7f, 0x60, 0xa0, 0xe3,
0x06, 0x00, 0x51, 0xe1, 0x02, 0x00, 0x00, 0x1a, 0x31, 0x1e, 0x8f, 0xe2, 0x00, 0x20, 0xa0, 0xe3,
0x00, 0x20, 0x81, 0xe5, 0x0a, 0x02, 0xa0, 0xe3, 0x02, 0x14, 0xa0, 0xe3, 0x38, 0x63, 0x9f, 0xe5,
0x00, 0x30, 0x91, 0xe5, 0x00, 0x40, 0xa0, 0xe3, 0x00, 0x50, 0xa0, 0xe1, 0x85, 0x40, 0x24, 0xe0,
0x85, 0x4d, 0x24, 0xe0, 0x05, 0x4e, 0x24, 0xe0, 0x02, 0x41, 0x04, 0xe2, 0x00, 0x40, 0x24, 0xe0,
0xe4, 0x0f, 0x23, 0xe0, 0x04, 0x10, 0x81, 0xe2, 0x06, 0x00, 0x51, 0xe1, 0xf3, 0xff, 0xff, 0x1a,
0xc4, 0x22, 0x9f, 0xe5, 0x00, 0x10, 0x92, 0xe5, 0xff, 0x10, 0x01, 0xe2, 0x7a, 0x70, 0xa0, 0xe3,
0x07, 0x00, 0x51, 0xe1, 0x01, 0x00, 0x00, 0x1a, 0xa7, 0x2f, 0x8f, 0xe2, 0x0d, 0x00, 0x00, 0xea,
0x7c, 0x70, 0xa0, 0xe3, 0x07, 0x00, 0x51, 0xe1, 0x01, 0x00, 0x00, 0x1a, 0xa3, 0x2f, 0x8f, 0xe2,
0x08, 0x00, 0x00, 0xea, 0x7f, 0x70, 0xa0, 0xe3, 0x07, 0x00, 0x51, 0xe1, 0x01, 0x00, 0x00, 0x1a,
0x9f, 0x2f, 0x8f, 0xe2, 0x03, 0x00, 0x00, 0xea, 0x04, 0x20, 0xa0, 0xe3, 0x9d, 0x1f, 0x8f, 0xe2,
0x00, 0x20, 0x81, 0xe5, 0x05, 0x00, 0x00, 0xea, 0x00, 0x70, 0x92, 0xe5, 0x00, 0x00, 0x57, 0xe1,
0x02, 0x00, 0x00, 0x0a, 0x02, 0x20, 0xa0, 0xe3, 0x96, 0x1f, 0x8f, 0xe2, 0x00, 0x20, 0x81, 0xe5,
0x98, 0x12, 0x9f, 0xe5, 0x00, 0x20, 0xa0, 0xe3,
0x01, 0x89, 0xa0, 0xe3, 0x9b, 0x6a, 0xa0, 0xe3,
0x04, 0x70, 0xa0, 0xe3, 0x08, 0x00, 0xa0, 0xe1, 0x00, 0x10, 0x80, 0xe5, 0x07, 0x00, 0x80, 0xe0,
0x06, 0x00, 0x50, 0xe1, 0xfb, 0xff, 0xff, 0x1a, 0x03, 0x30, 0xa0, 0xe3, 0x01, 0x40, 0xe0, 0xe1,
0x08, 0x00, 0xa0, 0xe1, 0x00, 0x50, 0x90, 0xe5, 0x01, 0x00, 0x55, 0xe1, 0x0f, 0x00, 0x00, 0x1a,
0x01, 0x00, 0x53, 0xe3, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x40, 0x80, 0xe5, 0x07, 0x00, 0x80, 0xe0,
0x06, 0x00, 0x50, 0xe1, 0xf6, 0xff, 0xff, 0x1a, 0x04, 0x10, 0xa0, 0xe1, 0x01, 0x30, 0x53, 0xe2,
0xf1, 0xff, 0xff, 0x1a, 0x04, 0x00, 0x57, 0xe3, 0x0a, 0x00, 0x00, 0x1a, 0x30, 0x82, 0x9f, 0xe5,
0x30, 0x62, 0x9f, 0xe5, 0x03, 0x70, 0xe0, 0xe3, 0x04, 0x10, 0xe0, 0xe1, 0xe9, 0xff, 0xff, 0xea,
0x01, 0x30, 0xa0, 0xe3, 0x73, 0x1f, 0x8f, 0xe2, 0x00, 0x20, 0x91, 0xe5, 0x03, 0x20, 0x82, 0xe1,
0x00, 0x20, 0x81, 0xe5, 0x21, 0x00, 0x00, 0xea, 0x00, 0x00, 0xa0, 0xe3, 0x06, 0x17, 0xa0, 0xe3,
0x1e, 0x30, 0xa0, 0xe3, 0x03, 0x3b, 0x83, 0xe2, 0x02, 0x40, 0xa0, 0xe3, 0x03, 0x4b, 0x84, 0xe2,
0x00, 0x2a, 0x83, 0xe1, 0x01, 0x00, 0x80, 0xe2, 0x0c, 0x00, 0x50, 0xe3, 0x04, 0x20, 0x81, 0xe4,
0xfa, 0xff, 0xff, 0xba, 0x00, 0x2a, 0x84, 0xe1, 0x01, 0x00, 0x80, 0xe2, 0x18, 0x00, 0x50, 0xe3,
0x04, 0x20, 0x81, 0xe4, 0xfa, 0xff, 0xff, 0xba, 0x00, 0x2a, 0x83, 0xe1, 0x01, 0x00, 0x80, 0xe2,
0x1c, 0x00, 0x50, 0xe3, 0x04, 0x20, 0x81, 0xe4, 0xfa, 0xff, 0xff, 0xba, 0x00, 0x2a, 0x84, 0xe1,
0x01, 0x00, 0x80, 0xe2, 0x01, 0x0a, 0x50, 0xe3, 0x04, 0x20, 0x81, 0xe4, 0xfa, 0xff, 0xff, 0xba,
0x06, 0x07, 0xa0, 0xe3, 0x03, 0x40, 0xa0, 0xe3, 0x10, 0x0f, 0x02, 0xee, 0x10, 0x4f, 0x03, 0xee,
0x10, 0x0f, 0x07, 0xee, 0x10, 0x0f, 0x05, 0xee, 0x3d, 0x00, 0xa0, 0xe3, 0x10, 0x0f, 0x01, 0xee,
0x01, 0x73, 0xa0, 0xe3, 0x00, 0x00, 0xa0, 0xe3, 0xfa, 0x01, 0xc7, 0xe5, 0x05, 0x10, 0xa0, 0xe3,
0x02, 0x0c, 0x51, 0xe3, 0x02, 0x00, 0x00, 0x0a, 0x01, 0x00, 0xc7, 0xe7, 0x01, 0x10, 0x81, 0xe2,
0xfa, 0xff, 0xff, 0xea, 0x02, 0x00, 0xa0, 0xe3, 0x4a, 0x00, 0xc7, 0xe5, 0x08, 0x21, 0x9f, 0xe5,
0x00, 0x10, 0x92, 0xe5, 0xff, 0x10, 0x01, 0xe2, 0x7f, 0x60, 0xa0, 0xe3, 0x06, 0x00, 0x51, 0xe1,
0x04, 0x00, 0x00, 0x0a, 0x1c, 0x00, 0xa0, 0xe3, 0x4b, 0x00, 0xc7, 0xe5, 0x1d, 0x00, 0xa0, 0xe3,
0x4c, 0x00, 0xc7, 0xe5, 0x03, 0x00, 0x00, 0xea, 0x00, 0x00, 0xa0, 0xe3, 0x4b, 0x00, 0xc7, 0xe5,
0x01, 0x00, 0xa0, 0xe3, 0x4c, 0x00, 0xc7, 0xe5, 0x0f, 0x00, 0xa0, 0xe3, 0xba, 0x00, 0xc7, 0xe5,
0x00, 0x00, 0xa0, 0xe3, 0xbb, 0x00, 0xc7, 0xe5, 0x42, 0x00, 0xa0, 0xe3, 0xbc, 0x00, 0xc7, 0xe5,
0x43, 0x00, 0xa0, 0xe3, 0xbd, 0x00, 0xc7, 0xe5,
0x4c, 0x00, 0xa0, 0xe3, 0xbe, 0x00, 0xc7, 0xe5,
0x41, 0x00, 0xa0, 0xe3, 0xbf, 0x00, 0xc7, 0xe5, 0x00, 0x00, 0xa0, 0xe3, 0xc0, 0x00, 0xc7, 0xe5,
0x00, 0x00, 0xa0, 0xe3, 0xc1, 0x00, 0xc7, 0xe5, 0xff, 0x00, 0xa0, 0xe3, 0xc2, 0x00, 0xc7, 0xe5,
0xff, 0x00, 0xa0, 0xe3, 0xc3, 0x00, 0xc7, 0xe5, 0x00, 0x00, 0xa0, 0xe3, 0xfd, 0x01, 0xc7, 0xe5,
0x74, 0x20, 0x9f, 0xe5, 0x00, 0x10, 0x92, 0xe5, 0xff, 0x10, 0x01, 0xe2, 0x7f, 0x60, 0xa0, 0xe3,
0x06, 0x00, 0x51, 0xe1, 0x0b, 0x00, 0x00, 0x1a, 0x40, 0x10, 0xa0, 0xe3, 0x98, 0x10, 0x87, 0xe5,
0x99, 0x10, 0x87, 0xe5, 0x20, 0x10, 0xa0, 0xe3, 0x9a, 0x10, 0x87, 0xe5, 0x9b, 0x10, 0x87, 0xe5,
0x04, 0x10, 0xa0, 0xe3, 0x9c, 0x10, 0x87, 0xe5, 0x02, 0x10, 0xa0, 0xe3, 0x9d, 0x10, 0x87, 0xe5,
0x03, 0x10, 0xa0, 0xe3, 0x9e, 0x10, 0x87, 0xe5, 0x80, 0x00, 0xa0, 0xe3, 0x24, 0x10, 0x8f, 0xe2,
0x00, 0x20, 0x91, 0xe5, 0x02, 0x00, 0x80, 0xe1, 0x01, 0x00, 0xc7, 0xe5, 0x04, 0x10, 0x8f, 0xe2,
0x00, 0x20, 0x91, 0xe5, 0x12, 0xff, 0x2f, 0xe1, 0x17, 0x00, 0x00, 0x02, 0x15, 0x2f, 0x8c, 0x9b,
0xef, 0x63, 0xf5, 0x3f, 0x16, 0xb1, 0x38, 0xea, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x02,
0x00, 0x02, 0xd0, 0x00, 0x0e, 0xf0, 0xa0, 0xe1, 0x0e, 0xf0, 0xa0, 0xe1, 0x00, 0x00, 0x00, 0x8c,
0x20, 0x00, 0x00, 0x88, 0x50, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00,
0x39, 0x00, 0x00, 0x00, 0x10, 0x40, 0x08, 0x00, 0x00, 0x40, 0x00, 0x00, 0x20, 0x40, 0x00, 0x00,
0x30, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x15, 0x80, 0x00, 0x00, 0xac, 0x01, 0x00, 0x02,
0xaa, 0xaa, 0xaa, 0xaa, 0xfc, 0xaf, 0x09, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0x44, 0x79, 0x6e, 0x61,
0x4d, 0x69, 0x54, 0x65, 0x20, 0x6d, 0x6f, 0x64, 0x65, 0x6d, 0x20, 0x53, 0x57, 0x20, 0x49, 0x4e,
0x49, 0x54, 0x20, 0x33, 0x5f, 0x36, 0x5f, 0x37, 0x30, 0x5f, 0x72, 0x65, 0x64, 0x75, 0x63, 0x65,
0x64, 0x42, 0x49, 0x53, 0x54, 0x20, 0x2f, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68,
0x74, 0x20, 0x41, 0x6c, 0x63, 0x61, 0x74, 0x65, 0x6c, 0x20, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x65,
0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x20, 0x57, 0x65, 0x64, 0x20, 0x4f,
0x63, 0x74, 0x20, 0x31, 0x31, 0x20, 0x31, 0x33, 0x3a, 0x35, 0x37, 0x3a, 0x30, 0x30, 0x20, 0x4d,
0x45, 0x54, 0x20, 0x44, 0x53, 0x54, 0x20, 0x32, 0x30, 0x30, 0x30, 0x00, 0xd9, 0xcb, 0x27, 0x34};

/* modem internal characteristics */
struct usb_modem_char {
  unsigned int vid;  /* VendorID */
  unsigned int pid;  /* ProductID */
  char *firmfile;  /* firmware file name */
  unsigned int datamax;  /* maximum data that we can send in a block */
};
struct usb_modem_char modem_char;

/* info about modem */
struct usb_modem_info {
  int modem_status;
  char firm_version[5];  /* firmware version */
  char mac[6];  /* MAC address */
  int down_bitrate;  /* download bitrate */
  int up_bitrate;  /* upload bitrate */
  int link_status;  /* link status */
  int line_status;  /* line status */
  int operational_mode;  /* operational mode */
};
struct usb_modem_info modem_info;

/* adsl modes */
const char *adsl_modes[] = {
 "ANSI",
 "G.DMT",
 "G.Lite"
};

/* check if a file exists */
int file_exists(const char *filename)
{
  struct stat info_file;

  return stat(filename, &info_file) == 0;
}

/* show printable char */
void print_char(unsigned char c)
{
  if (c >= ' ' && c < 0x7f)
    printf("%c", c);
  else
    printf(".");
}

/* show buffer */
void dump(unsigned char *buf, int lenbuf, int lenline)
{
  int i, j;  /* counters */

  for (i = 0; i < lenbuf; i+= lenline)
  {
    for (j = i; j < lenbuf && j < i + lenline; j++)
      printf("%02x ", buf[j]);
    for (; j < i + lenline; j++)
      printf("   ");
    for (j = i; j < lenbuf && j < i + lenline; j++)
      print_char(buf[j]);
    printf("\n");
  }
}

/* transfer a control message to USB bus */
int transfer_ctrl_msg(usb_dev_handle *adsl_handle, int requesttype, int request, int value, int index, char *buf, int size)
{
  int j;  /* counter */
  int n;  /* bytes transfed or error code */
  int tmout = CTRL_TIMEOUT;  /* timeout value */

  n = 0;
  for (j = 0; j < CTRL_MSG_RETRIES; j++) {
#ifdef SIMULATE
    n = size;
#else
    n = usb_control_msg(adsl_handle, requesttype, request, value, index,  buf, size, tmout);
#endif
    if (n >= 0) {
#if DEBUG_TRANSFER
      printf(gettext("%d bytes transferred:\n"), n);
      dump(buf, n, 16);
#endif
      break;
    }
    else {
      printf(gettext("Error: usb_control_msg: %s\n"), usb_strerror());
      if (n == -EPIPE) {
        usb_clear_halt(adsl_handle, 0x00);
        usb_clear_halt(adsl_handle, 0x80);
      }
      else if (n == -ETIMEDOUT) {
	tmout += TIMEOUT_ADD;
      }
    }
  }
  if (n < 0) {
    printf(gettext("Error: usb_control_msg failed after %d retries\n"), CTRL_MSG_RETRIES);
    return -1;
  }
  return n;
}

/* receive a packet from USB bus by bulk transfer */
int read_bulk(usb_dev_handle *adsl_handle, int ep, char *buf, int size)
{
  int n;  /* bytes readed or error code */
  int i;  /* counter */
  int tmout = DATA_TIMEOUT;  /* timeout value */

  memset(buf, 0, sizeof(buf));
  n = 0;
  for (i = 0; i < READ_BULK_RETRIES; i++) {
#ifdef SIMULATE
    n = size;
#else
    n = usb_bulk_read(adsl_handle, ep, buf, size, tmout);
#endif
    if (n >= 0) {
#if DEBUG_TRANSFER
      printf(gettext("%d bytes downloaded:\n"), n);
      dump(buf, n, 16);
#endif
      break;
    }
    else {
      printf(gettext("Error: usb_bulk_read: %s\n"), usb_strerror());
      if (n == -EPIPE) {
        usb_clear_halt(adsl_handle, ep);
      }
      else if (n == -ETIMEDOUT) {
	tmout += TIMEOUT_ADD;
      }
    }
  }
  if (n < 0) {
    printf(gettext("Error: usb_bulk_read failed after %d retries\n"), READ_BULK_RETRIES);
    return -1;
  }
  return 0;
}

/* send one or more packets to USB bus by bulk transfer */
int send_bulk(usb_dev_handle *adsl_handle, int ep, char *buf, int nfil, int ncol)
{
  int i, j;  /* counters */
  int n;  /* bytes sent or error code */
  int tmout = DATA_TIMEOUT;  /* timeout value */

  n = 0;
  for (i = 0; i < nfil; i++)
  {
    for (j = 0; j < SEND_BULK_RETRIES; j++) {
#ifdef SIMULATE
      n = ncol;
#else
      n = usb_bulk_write(adsl_handle, ep, buf+(i*ncol), ncol, tmout);
#endif
      if (n >= 0) {
#if DEBUG_TRANSFER
        printf(gettext("%d bytes uploaded:\n"), n);
        dump(buf+(i*ncol), ncol, 16);
#endif
        break;
      }
      else {
        printf(gettext("Error: usb_bulk_write: %s\n"), usb_strerror());
	if (n == -EPIPE) {
	  usb_clear_halt(adsl_handle, ep);
	}
	else if (n == -ETIMEDOUT) {
	  tmout += TIMEOUT_ADD;
	}
      }
    }
    if (n < 0) {
      printf(gettext("Error: usb_bulk_write failed after %d retries\n"), SEND_BULK_RETRIES);
      return -1;
    }
  }
  return 0;
}

/* format a message */
void format_message(int cmd, int ldata, int address, char *bufin)
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

/* clear endpoints that we use */
void clear_endpoints(usb_dev_handle *adsl_handle, int op) {
  if (op == 1) {
    usb_resetep(adsl_handle, USB_OUT_FIRM);
    usb_resetep(adsl_handle, USB_IN_FIRM);
  }
  else {
    usb_resetep(adsl_handle, USB_OUT_DATA);
    usb_resetep(adsl_handle, USB_IN_DATA);
  }
}

/* send a block of data */
int send_block(usb_dev_handle *adsl_handle, int place, char *bufin, int len)
{
  char buf[0x1ff];  /* = modem_char.datamax + 8 */

  if ((bufin == NULL) || (len > modem_char.datamax))
    return -1;
  memset(buf, 0, sizeof(buf));
  format_message(0x88, len, place, buf);
  memcpy(buf+8, bufin, len);
  PDEBUG(gettext("Sending block at address = 0x%04x...\n"), place);
  if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, len+8))
    return -1;
  return 0;
}

/* start code execution at specified address */
int jump_to_address(usb_dev_handle *adsl_handle, unsigned int place)
{
  unsigned char buf[6];  /* buffer */

  buf[0] = 0x08; // Command (= set base address)
  buf[1] = 0x04; // Length (= 4 bytes)
  // Value (base address = place)
  buf[2] = (place >> 24) & 0xff;
  buf[3] = (place >> 16) & 0xff;
  buf[4] = (place >> 8) & 0xff;
  buf[5] = place & 0xff;
  if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, 6))
    return -1;
  buf[0] = 0x00;  // Command (= jump?)
  buf[1] = 0x01;  // Length (= 1 byte)
  buf[2] = 0x14;  // Value (= jump to base address)
  if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, 3))
    return -1;
  return 0;
}

/* load firmware */
int load_firmware(usb_dev_handle *adsl_handle, unsigned int tmodem)
{
  unsigned char buf[0x1ff];   /* buffer */
  FILE *soft;   /* file handle */
  long len;     /* length */
  int place;    /* initial target address */
  char value;  /* returned byte */
  int i;  /* counter */
  time_t first, last, before;  /* to wait */
  unsigned char bufconf[8];  /* buffer to save config bytes */
  unsigned char *pbuf;  /* pointer to buffer */

  /* clear endpoints */
  clear_endpoints(adsl_handle, 1);

  /* load firmware */
  printf(gettext("Loading and sending %s...\n"), modem_char.firmfile); 
  soft = fopen(modem_char.firmfile, "rb");
  if (soft == NULL)
  {
    printf(gettext("Error: I can't open file %s\n"), modem_char.firmfile);
    return -1;
  }
  fseek(soft, 0L, SEEK_END);
  len = ftell(soft);
  PDEBUG(gettext("Length of file %s = %ld bytes\n"), modem_char.firmfile, len);
/*
  fseek(soft, 0L, SEEK_SET);
  len = fread(buf, 1, 5, soft);
  if (len <= 0)
  {
    printf(gettext("Error: No bytes to read from file %s\n"), modem_char.firmfile);
    return -1;
  }
  if (len != 5)
  {
    printf(gettext("Error: I can't read initial 5 bytes from file %s\n"), filename);
    return -1;
  }
*/
  /* check initial bytes */
/*
  PDEBUG(gettext("Initial bytes from file %s:\n"), filename);
#if DEBUG
  dump(buf, 5, 5);
#endif
  if (buf[0] != FIRMBYTE1 || buf[1] != FIRMBYTE2 || buf[2] != FIRMBYTE3 || buf[3] != FIRMBYTE4 || buf[4] != FIRMBYTE5) {
    printf(gettext("Error: Maybe file %s isn't Conexant firmware, contact with author of this program\n"), filename);
    return -1;
  }
*/

  /**************/
  /* initialize */
  /**************/

  PDEBUG(gettext("PreInit...\n"));

  /* clear, reset */
  i = sizeof(inifirm);
  place = 0x0000;
  pbuf = inifirm; 
  while (i > 0) {
    if (i > modem_char.datamax)
      len = modem_char.datamax;
    else
      len = i;
    if (send_block(adsl_handle, place, pbuf, len))
      return -1;
    i -= len;
    place += len;
    pbuf += len;
    buf[0] = 0x40; buf[1] = 0x01; buf[2] = 0x12;
    if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, 3))
      return -1;
  }
  if (jump_to_address(adsl_handle, 0x00000000))
    return -1;

  /* read something needed */
  if (read_bulk(adsl_handle, USB_IN_FIRM, buf, 0x1ff))
    return -1;
  memcpy(bufconf, buf+0xb9, 8);

  /* clear endpoints */
  clear_endpoints(adsl_handle, 1);


  /*****************/
  /* send firmware */
  /*****************/

  PDEBUG(gettext("Firmware...\n"));
  fseek(soft, 0L, SEEK_SET);
  place = 0x0000; 
  while ((len = fread(buf, 1, modem_char.datamax, soft)) > 0)
  {
    PDEBUG(gettext("%ld bytes readed from file %s\n"), len, modem_char.firmfile);
    if (send_block(adsl_handle, place, buf, len))
      return -1;
    place += len;
    buf[0] = 0x40; buf[1] = 0x01; buf[2] = 0x12;
    if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, 3))
      return -1;
  } 
  fclose(soft);

  if (jump_to_address(adsl_handle, 0x00000000))
    return -1;
  printf(gettext("Firmware is sent!\n"));

  /* wait until firmware is ready */
  sleep(1);


  /*************/
  /* post load */
  /*************/

  PDEBUG(gettext("PostInit...\n"));

  /* configure something */

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, 0x0a, 0x0c, 0x08, buf, 0x1);
  if (len < 1)
    return -1;
  value = buf[0];

  usb_resetep(adsl_handle, 0x81);

  // send (0x40)
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x40, 0x03, 0x00, NULL, 0);
  if (len < 0)
    return -1;

  // read (0xC0)
  for (i = 0xc2; i <= 0xcd; i++) {
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, value, 0x03, i, buf, 3);
    if (len < 3)
      return -1;
  }

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x0b, 0x0c, 0x00, NULL, 0);
  if (len < 0)
    return -1;

  for (i = 0xba; i <= 0xc1; i++) {
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, i, &bufconf[i-0xba], 1);
    if (len < 0)
      return -1;
  }

  /* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
  /* 0x1fd in CTRLE protocol */
  buf[0] = 0x15;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x1fd, buf, 1);
  if (len < 0)
    return -1;

  buf[0] = 0x01;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4a, buf, 1);
  if (len < 0)
    return -1;

  buf[0] = 0x00;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4b, buf, 1);
  if (len < 0)
    return -1;

  buf[0] = 0x00;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4c, buf, 1);
  if (len < 0)
    return -1;

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x02, 0x03, 0x00, NULL, 0);
  if (len < 0)
    return -1;

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, 0x0e, 0x03, 0x00, buf, 0x0c);
  if (len < 0x0c)
    return -1;

#ifdef SIMULATE
  exit(0);
#endif

  memset(&modem_info, 0, sizeof(struct usb_modem_info));

  /* waiting until line is up (a maximum time) */
  printf(gettext("Waiting ADSL line is up (until %d seconds)...\n"), MAX_WAIT_LINE_UP);
  time(&first); before = first;
  do {
    PDEBUG(gettext("Sending retrieve info...\n"));
    memset(buf, 0, 0x10);
    len = usb_bulk_read(adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
    if (len < 0)
      printf(gettext("Error retrieving info!\n"));
    else {
      PDEBUG(gettext("%li bytes readed:\n"), len);
#if DEBUG_TRANSFER
      if (len > 0)
        dump(buf, len, 16);
#endif
      if (len == 12)
        if ((buf[0] & 0xff) == 0x01) {
          modem_info.modem_status = buf[1] & 0xff;
          PDEBUG(gettext("Modem status = %02x\n"), modem_info.modem_status);
        }
    }
    if (difftime(time(&last), before) > 1) {
#ifndef DEBUG
      printf(".");
      fflush(stdout);
#endif
      before = last;
    }
  }
  while ((modem_info.modem_status != MODEM_UP) && (difftime(last, first) < MAX_WAIT_LINE_UP));
  printf("\n");

  if (modem_info.modem_status == MODEM_UP)
  {
    printf(gettext("ADSL line is up\n"));
/* these lines blink leds:
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x30, 0x03, 0x00, NULL, 0);
    if (len < 0)
      return -1;
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, 0x0e, 0x03, 0x00, buf, 0x0c);
    if (len < 0x0c)
      return -1;
*/
/*
    printf(gettext("ADSL line is up (Downstream %u Kbits/s, Upstream %u Kbits/s)\n"), modem_info.down_bitrate, modem_info.up_bitrate);
#ifdef DEBUG
    printf(gettext("ADSL mode = %i"), modem_info.operational_mode);
    if ((modem_info.operational_mode > 0) && (modem_info.operational_mode < 4))
      printf(" (%s)", adsl_modes[modem_info.operational_mode-1]);
    printf("\n");
#endif
*/
  }
  else
    printf(gettext("ADSL line is down\n"));

/*
#ifdef DEBUG
  printf(gettext("Waiting to receive first ATM cells...\n"));
  time(&first); before = first;
  do {
    len = usb_bulk_read(adsl_handle, USB_IN_DATA, buf, sizeof(buf), DATA_TIMEOUT);
    if (len > 0)  {
      printf("\n");
      printf(gettext("ATM cells received:\n"));
      dump(buf, len, 16);
      return 0;
    }
    if (difftime(time(&last), before) > 1) {
      printf(".");
      fflush(stdout);
      before = last;
    }
  } 
  while (difftime(last, first) < 60);
  printf("\n");
#endif
*/

  return 0;
}

/* it inits modem according modem type */
void init_modem(unsigned int tmodem, struct usb_device *adsl_dev, int open_mode) {

  memset(&modem_char, 0, sizeof(modem_char));
  if (adsl_dev != NULL) {
    modem_char.vid = adsl_dev->descriptor.idVendor;
    modem_char.pid = adsl_dev->descriptor.idProduct;
  }

  switch (tmodem) {

    /* AME Dynamite USB Modem */
    case 1:
      modem_char.datamax = 0x1a0;
      modem_char.firmfile = "/usr/sbin/fw-usb.bin";
      break;

   /* Asus AAM6000UG */
    case 2:
      modem_char.datamax = 0x1f2;
      modem_char.firmfile = "/usr/sbin/Fw-usb_A.bin";
      break;
  }
}

/* check if it is a known modem */
int check_modem(unsigned int vid, unsigned int pid)
{
  /* Vendor = AME (Alcatel Microelectronics), Product = DynaMiTe USB Modem */
  /* Used in Zyxel 630-11 */
  if (vid == 0x06b9 && pid == 0xa5a5)
    return 1;

  /* Vendor = ASUSTeK Computer Inc., Product = AAM6000UG */
  if (vid == 0x0b05 && pid == 0x6206)
    return 2;

//  if (vid == 0x0572 && pid == 0xcafe)
//    return 2;

  return -1;
}

int main(int argc, char *argv[])
{
  /* bus structures variables */
  struct usb_bus * bus;
  struct usb_device * dev;
  struct usb_device * adsl_dev = NULL;
  usb_dev_handle * adsl_handle;

  /* boolean value */
  int goon;

  /* result code */
  int r = 0; 

  /* open mode */
  int open_mode = -1;

  /* type of modem */
  int tmodem = -1;


  /* reset command queries */
  //memset(modem_cmd_state, 0, sizeof(modem_cmd_state));

  /* init locale */
  setlocale(LC_ALL, "");
  //if (file_exists("./locale")) 
  //  bindtextdomain(TF_CODE, "./locale");  /* set directory for a domain (source code messages) */
  //else 
    bindtextdomain(TF_CODE, "/usr/share/locale");  /* set directory for a domain (source code messages) */
  textdomain(TF_CODE);  /* set domain */

  /* show program information */
  printf(gettext("Zyxel 630-11 & Asus AAM6000UG microcode upload program."));
  printf(" 24/9/2003\n");
  printf("Josep Comas <jcomas@gna.es>\n");
  printf("Sundar <sundar@cynaptix.biz>\n\n");

  /* check parameters */
/*
  if (argc < 1)
  {
    printf(gettext("Usage:\n"));
    printf(gettext("   %s [open_mode]\n"), argv[0]);
    return -1;
  }
  if (argc > 1) {
    open_mode = atoi(argv[1]);
    if ((open_mode < 0) || (open_mode > 5)) {
      printf(gettext("Error: Incorrect open mode\n"));
      return -1;
    }
  }
*/

  /* init USB bus and find devices */
  usb_init();
  if (usb_find_busses() < 0)
  {
    printf(gettext("Error: I can't find busses\n"));
    return -1;
  }
  if (usb_find_devices() < 0)
  {
    printf(gettext("Error: I can't find devices\n"));
    return -1;
  }

  /* search first ADSL USB modem */
  bus = usb_busses;
  goon = 1;
  while (bus && goon)
  {
    dev = bus->devices;
    while (dev && goon)
    {
      tmodem = check_modem(dev->descriptor.idVendor, dev->descriptor.idProduct);
      if (tmodem > 0)
      {
        goon = 0;
        adsl_dev = dev;
      }
      else
        dev = dev->next;
    }
    if (goon)
      bus = bus->next;
  }
  if (adsl_dev == NULL)
  {
    printf(gettext("Error: I didn't find ADSL modem\n"));
    return -1;
  }
  printf(gettext("I found ADSL modem with VendorID = %04x & ProductID = %04x\n"), adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);

#if DEBUG
   printf(" bLength: 0x%02x\n", adsl_dev->config->bLength);
   printf(" bDescriptorType: 0x%02x\n", adsl_dev->config->bDescriptorType);
   printf(" wTotalLength: 0x%04x\n", adsl_dev->config->wTotalLength);
   printf(" bNumInterfaces: 0x%02x\n", adsl_dev->config->bNumInterfaces);
   printf(" bConfigurationValue: 0x%02x\n", adsl_dev->config->bConfigurationValue);
   printf(" iConfiguration: 0x%02x\n", adsl_dev->config->iConfiguration);
   printf(" bmAttributes: 0x%02x\n", adsl_dev->config->bmAttributes);
   printf(" MaxPower: 0x%02x\n", adsl_dev->config->MaxPower);
#endif

  /* connect to ADSL modem */
  adsl_handle = usb_open(adsl_dev);
  if (adsl_handle == NULL)
  {
    printf(gettext("Error: Couldn't get device handle for ADSL modem\n"));
    return -1;
  }
  /* set configuration */
  if (usb_set_configuration(adsl_handle, 1) < 0)
  {
    printf("Error: usb_set_configuration: %s\n", usb_strerror());
    return -1;
  }
  /* check if other program is using interface 2 */
//  if (usb_claim_interface(adsl_handle, 0) < 0)
  if (usb_claim_interface(adsl_handle, 2) < 0)
  {
    printf("Error: usb_claim_interface: %s\n", usb_strerror());
    return -1;
  }
  PDEBUG(gettext("Interface = %d\n"), adsl_handle->interface);

  init_modem(tmodem, adsl_dev, open_mode); 
  r = load_firmware(adsl_handle, tmodem); 

  PDEBUG(gettext("Releasing interface...\n"));
  usb_release_interface(adsl_handle, 0);
  PDEBUG(gettext("Releasing device...\n"));
  usb_close(adsl_handle);

  return r;
   
}

