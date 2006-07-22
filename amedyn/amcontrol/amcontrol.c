/*
  Zyxel 630-11 & Asus AAM6000UG line control process
  Copyright (C) 2004 Aurelio Arroyo

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

  Author     : Aurelio Arroyo <sktt@users.sourceforge.net>
  Creation   : 30/07/2004

  Based      : On amload from Josep Comas <jcomas@gna.es> and others (See amload.c)

  Description: This program control a already runing Zxyel 630-11 & Asus AAM6000UG (USB ADSL Modems with Alcatel chipset).

  Log:

  30/07/2004 Aurelio Arroyo
  Initial release

  03/08/2004 Aurelio Arroyo
  Add more ouput

  TO-DO:
  
  Enable debug.

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
#include <syslog.h>
#include <curses.h>


/* translation files */
#define TF_CODE "amcontrol"

/* info window */
WINDOW *infow;			/* Global var..... Is bad? */

/* modem internal characteristics */
struct usb_modem_char modem_char;

/* info about modem */
struct usb_modem_info modem_info;

/* show printable char */
void
print_char (unsigned char c)
{
  if (c >= ' ' && c < 0x7f)
    printf ("%c", c);
  else
    printf (".");
}

/* show buffer */
/*
void
{
  int i, j;			// counters 

  printf
    ("                                                                          \r");
  for (i = 0; i < lenbuf; i += lenline) {
    for (j = i; j < lenbuf && j < i + lenline; j++)
      printf ("%02x ", buf[j]);
    for (; j < i + lenline; j++)
      printf ("   ");
    for (j = i; j < lenbuf && j < i + lenline; j++)
      print_char (buf[j]);
    printf ("\r");
  }
}
*/
/* transfer a control message to USB bus */
int
transfer_ctrl_msg (usb_dev_handle * adsl_handle, int requesttype, int request,
		   int value, int index, char *buf, int size)
{
  int j;			/* counter */
  int n;			/* bytes transfed or error code */
  int tmout = CTRL_TIMEOUT;	/* timeout value */

  n = 0;
  for (j = 0; j < CTRL_MSG_RETRIES; j++) {
#ifdef SIMULATE
    n = size;
#else
    n =
      usb_control_msg (adsl_handle, requesttype, request, value, index, buf,
		       size, tmout);
#endif
    if (n >= 0) {
#if DEBUG_TRANSFER
      printf (gettext ("%d bytes transferred:
"), n);
      dump (buf, n, 16);
#endif
      break;
    }
    else {
      printf (gettext ("Error: usb_control_msg: %s
"), usb_strerror ());
      if (n == -EPIPE) {
	usb_clear_halt (adsl_handle, 0x00);
	usb_clear_halt (adsl_handle, 0x80);
      }
      else if (n == -ETIMEDOUT) {
	tmout += TIMEOUT_ADD;
      }
    }
  }
  if (n < 0) {
    printf (gettext ("Error: usb_control_msg failed after %d retries
"),
	    CTRL_MSG_RETRIES);
    return -1;
  }
  return n;
}

/* Send sync signals */
int
send_sync_signal (usb_dev_handle * adsl_handle, int tmodem, unsigned char line )  
{
  char buf[0x1ff];	/* buffer */
  long len;			/* length */
//  int i;
  
//  sleep (2);
  
  if ( ( line != 0x15 ) && ( line !=0x11 ) )
    {
     wprintw (infow, "[Unknow line type %02x]",line);
    } 
  else
    {
     if ( line == 0x15)	wprintw (infow, "[Config for Analog line]
\r");
     else wprintw (infow, "[Config for ISDN line]
\r");
    }
  wprintw (infow, "[Sending SyncS.");

/* Present in Window USB log. */ 
/* But in amcontrol we haven't the buf[] whit the info. */ 
/*
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x0b, 0x0c, 0x00, NULL, 0);
  if (len < 0)
    return -1;

  for (i = 0xba; i <= 0xc1; i++) {
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, i, &bufconf[i-0xba], 1);
    if (len < 0)
      return -1;

*/

  /* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
  /* 0x1fd in CTRLE protocol */
  /* 0x15 = analog line, 0x11 ISDN line */

  buf[0] = line;
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x1fd,
		       buf, 1);
  if (len < 0)
    return -1;
  wprintw (infow, ".");

  buf[0] = 0x01;
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4a, buf,
		       1);
  if (len < 0)
    return -1;
  wprintw (infow, ".");

  buf[0] = 0x00;
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4b, buf,
		       1);
  if (len < 0)
    return -1;
  wprintw (infow, ".");

  buf[0] = 0x00;
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4c, buf,
		       1);
  if (len < 0)
    return -1;
  wprintw (infow, ".");


/* Only this command seem to be necesary to resync. */
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x02, 0x03, 0x00,
		       NULL, 0);
  if (len < 0)
    return -1;
  wprintw (infow, ".");


  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_IN, 0x0e, 0x03, 0x00, buf,
		       0x0c);
  if (len < 0x0c)
    return -1;


  wprintw (infow, ".send]");
  wprintw (infow, "\r");

  return 0;
}

/* Send line down signal */
int send_line_down_signal (usb_dev_handle * adsl_handle, int tmodem)
{
  char buf[0x1ff];	/* buffer */
  long len;			/* length */

  wprintw (infow, "[Sending DLS.");
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x03, 0x03, 0x00, buf,
		       0);
  wprintw (infow, ".");
  if (len != 0) {
    wprintw (infow, "(Len %li)", len);
    return -1;
  }
  wprintw (infow, ".send]");
  wprintw (infow, "
\r");
  return 0;
}

/* Send signal Request=30 Value=03 Index=00*/
int send_signal_30 (usb_dev_handle * adsl_handle, int tmodem)
{
  char buf[0x1ff];	/* buffer */
  long len;			/* length */

  wprintw (infow, "[Sending 30 03 00.");
  len =
    transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x30, 0x03, 0x00, buf,
		       0);
  wprintw (infow, ".");
  if (len != 0) {
    wprintw (infow, "(Len %li)", len);
    return -1;
  }
  wprintw (infow, ".send]");
  wprintw (infow, "
\r");
  return 0;
}


/* check if it is a known modem */
int check_modem (unsigned int vid, unsigned int pid)
{
  /* Vendor = AME (Alcatel Microelectronics), Product = DynaMiTe USB Modem */
  /* Used in Zyxel 630-11 */
  if (vid == 0x06b9 && pid == 0xa5a5)
    return 1;

  /* Vendor = ASUSTeK Computer Inc., Product = AAM6000UG */
  if (vid == 0x0b05 && pid == 0x6206)
    return 2;

  /* Vendor = Mediacom Europe?, Product = DynaMiTe USB Modem */
//  if (vid == 0x0572 && pid == 0xcafe)
//    return 2;

  return -1;
}

int
translate_buf (char buf[0x1ff], int len)
{

  int i;

  switch (buf[0] & 0xff) {
  case 0x02:
    wprintw (infow, "[Line problem?]
\r");
    break;
  case 0x50:
    wprintw (infow, "[Line up?]
\r");
    break;
  case 0x60:
    wprintw (infow, "[Line yet down?]
\r");
    break;
  case 0x70:
    wprintw (infow, "[Line down OK?]
\r");
    break;
  case 0x40:
    switch (buf[1]) {
    case 0x0f:
      wprintw (infow, "[0f][Error Unknow [40 0f]]\r");
      break;
    case 0x14:
      wprintw (infow, "[14][Error at MODEM_INIT?]\r");
      break;
    case 0x19:
      wprintw (infow, "[19][Error at MODEM_WAIT?]\r");
      break;
    default:
      wprintw (infow, "[%02x %02x]
\r", buf[0], buf[1]);
      break;
    }
  case 0x01:
    modem_info.ruidor = (float) buf[6] / 2;
    modem_info.potenciar = (float) buf[5] / 2;
    modem_info.ruidot = (float) buf[4] / 2;
    modem_info.potenciat = (float) buf[7] / 2;
/*	  if (buf[1] == MODEM_UP)   wprintw (infow,"[MODEM_UP  ]
\r");
	  if (buf[1] == MODEM_DOWN) wprintw (infow,"[MODEM_DOWM]
\r"); 
	  if (buf[1] == MODEM_WAIT) wprintw (infow,"[MODEM_WAIT]
\r");
	  if (buf[1] == MODEM_INIT) wprintw (infow,"[MODEM_INIT]
\r");
*/
//        wprintw (infow,"[Unknow status %02x]
\r",buf[1]);
    break;
  case 0xf1:
    modem_info.down_bitrate_percent =
      (((buf[2] & 0xff) << 8) | (buf[1] & 0xff));
    modem_info.up_bitrate_percent =
      (((buf[5] & 0xff) << 8) | (buf[4] & 0xff));
    modem_info.down_bitrate = (((buf[8] & 0xff) << 8) | (buf[7] & 0xff));
    modem_info.up_bitrate = (((buf[12] & 0xff) << 8) | (buf[11] & 0xff));
    modem_info.down_attenuation = buf[3];
    modem_info.up_attenuation = buf[6];
    break;
  case 0xf2:
    modem_info.NearFecNotInterleaved =
      (((buf[2] & 0xff) << 8) | (buf[1] & 0xff));
    modem_info.NearFecInterleaved =
      (((buf[4] & 0xff) << 8) | (buf[3] & 0xff));
    modem_info.NearCrcNotInterleaved =
      (((buf[6] & 0xff) << 8) | (buf[5] & 0xff));
    modem_info.NearCrcInterleaved =
      (((buf[8] & 0xff) << 8) | (buf[7] & 0xff));
    modem_info.NearHecNotInterleaved =
      (((buf[10] & 0xff) << 8) | (buf[9] & 0xff));
    modem_info.NearHecInterleaved =
      (((buf[12] & 0xff) << 8) | (buf[11] & 0xff));
    break;
  case 0xf3:
    modem_info.FarFecNotInterleaved =
      (((buf[2] & 0xff) << 8) | (buf[1] & 0xff));
    modem_info.FarFecInterleaved = (((buf[4] & 0xff) << 8) | (buf[3] & 0xff));
    modem_info.FarCrcNotInterleaved =
      (((buf[6] & 0xff) << 8) | (buf[5] & 0xff));
    modem_info.FarCrcInterleaved = (((buf[8] & 0xff) << 8) | (buf[7] & 0xff));
    modem_info.FarHecNotInterleaved =
      (((buf[10] & 0xff) << 8) | (buf[9] & 0xff));
    modem_info.FarHecInterleaved =
      (((buf[12] & 0xff) << 8) | (buf[11] & 0xff));
    break;
  case 0xf4:
    for (i = 0x00; i <= 0x0f; i = i + 1) {
      modem_info.tons[0x00 + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0x00 + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xf5:
    for (i = 0x00; i <= 0x0f; i = i + 1) {
      modem_info.tons[(0x1e) + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[(0x1e) + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xf6:
    for (i = 0x00; i <= 0x0f; i = i + 1) {
      modem_info.tons[0x3c + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0x3c + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xf7:
    for (i = 0x00; i <= 0x08; i = i + 1) {
      modem_info.tons[0x5a + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0x5a + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xf8:
    for (i = 0x00; i <= 0x08; i = i + 1) {
      modem_info.tons[0x6a + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0x6a + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xf9:
    for (i = 0x00; i <= 0x08; i = i + 1) {
      modem_info.tons[0x88 + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0x88 + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xfa:
    for (i = 0x00; i <= 0x08; i = i + 1) {
      modem_info.tons[0xa6 + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0xa6 + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xfb:
    for (i = 0x00; i <= 0x08; i = i + 1) {
      modem_info.tons[0xc4 + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0xc4 + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  case 0xfc:
    for (i = 0x00; i <= 0x08; i = i + 1) {
      modem_info.tons[0xe2 + i * 2] = ((buf[i] & 0xf0) >> 4);
      modem_info.tons[0xe2 + i * 2 + 1] = ((buf[i] & 0x0f));
    }
    break;
  default:
      wprintw (infow, "RAW [" );
	for ( i = 0x00 ; i < len; i = i + 1 )
	    { wprintw (infow, "%02x ", buf[i]); }
      wprintw (infow, "]
\r" );
  break;
  }
  return 0;
}

int
main (int argc, char *argv[])
{
  /* bus structures variables */
  int i;
  struct usb_bus *bus;
  struct usb_device *dev;
  struct usb_device *adsl_dev = NULL;
  usb_dev_handle *adsl_handle;
  char buf[0x1ff];	/* buffer */
  int len;

  /* boolean value */
  int goon;

  /* result code */
  int r = 0;

  /* open mode */
//  int open_mode = -1;

  /* type of modem */
  int tmodem = -1;

  /* auto resync */
  int arsync = 1;
  int read = 1;
  int read1 = 0;
//  int translate = 0;

  /* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
  /* 0x1fd in CTRLE protocol */
  /* 0x15 = analog line, 0x11 ISDN line */
  unsigned char line = 0x15;

  /*
  1 = Raw output, etc.
  2 = Human output, etc.
  3 = Help.
  */
  int show = 1;

  WINDOW *statusw;
  WINDOW *autow;
  WINDOW *creditw;
  WINDOW *rawinfow;
  WINDOW *humaninfow;
  WINDOW *helpw;

  /*
  * Security stuff
  * 1 - be sure to be root
  * 2 - umask to prevent critical data being read from log file
  */
  if(geteuid() != 0) {
    fprintf(stderr, "WARNING: amcontrol must be run with root privileges
");
    exit (-1);
  }

  initscr ();
  noecho ();
  timeout (0);
  clear ();

  statusw = newwin (7, 70, 5, 10);
  scrollok (statusw, TRUE);
  box (statusw, 0, 0);
  wborder (statusw, 0, 0, 0, 0, 0, 0, 0, 0);

  autow = newwin (1, 9, 5, 0);
  scrollok (autow, TRUE);
//  box (autow, 0, 0);
//  wborder (autow, 0, 0, 0, 0, 0, 0, 0, 0);

  infow = newwin (0x0d, 26, 12, 0);
  scrollok (infow, TRUE);
  box (infow, 0, 0);
  wborder (infow, 0, 0, 0, 0, 0, 0, 0, 0);

  creditw = newwin (5, 80, 0, 0);
  scrollok (creditw, TRUE);
  box (creditw, 0, 0);
  wborder (creditw, 0, 0, 0, 0, 0, 0, 0, 0);

  rawinfow = newwin (0x0d, 40, 12, 27);
  scrollok (rawinfow, TRUE);
  box (rawinfow, 0, 0);
  wborder (rawinfow, 0, 0, 0, 0, 0, 0, 0, 0);

  humaninfow = newwin (0x0d, 53, 12, 27);
  scrollok (humaninfow, TRUE);
  box (humaninfow, 0, 0);
  wborder (humaninfow, 0, 0, 0, 0, 0, 0, 0, 0);

  helpw = newwin (0x0d, 70, 12, 10);
  scrollok (helpw, TRUE);
  box (helpw, 0, 0);
  wborder (helpw, 0, 0, 0, 0, 0, 0, 0, 0);

  /* reset command queries */
  //memset(modem_cmd_state, 0, sizeof(modem_cmd_state));

  /* init locale */
  setlocale (LC_ALL, "");
  //if (file_exists("./locale")) 
  //  bindtextdomain(TF_CODE, "./locale");  /* set directory for a domain (source code messages) */
  //else 
  bindtextdomain (TF_CODE, "/usr/share/locale");	/* set directory for a domain (source code messages) */
  textdomain (TF_CODE);		/* set domain */

  /* show program information */
  wprintw (creditw,
	   gettext ("Zyxel 630-11 & Asus AAM6000UG line control program."));
  wprintw (creditw, " 03/08/2004
\r");
  wprintw (creditw, "Aurelio Arroyo  <sktt@users.sourceforge.net>

\r");

  wprintw (helpw,  "Key a: Auto ON/OFF      Key d: Send Down Line signal

\r");
  wprintw (helpw,  "Key r: Ask on time      Key c: Ask 12 times

\r");
  wprintw (helpw,  "Key s: Send Sync signal Key q: Exit program

\r");
  wprintw (helpw,  "Status:: '#' MODEM_DOWN '_' MODEM_WAIT '-' MODEM_INIT '@' MODEM_UP

\r");

  /* init USB bus and find devices */
  usb_init ();
  if (usb_find_busses () < 0) {
    wprintw (creditw, gettext ("Error: I can't find busses
\r"));
    return -1;
  }
  if (usb_find_devices () < 0) {
    wprintw (creditw, gettext ("Error: I can't find devices
\r"));
    return -1;
  }

  /* search first ADSL USB modem */
  bus = usb_busses;
  goon = 1;
  while (bus && goon) {
    dev = bus->devices;
    while (dev && goon) {
      tmodem =
	check_modem (dev->descriptor.idVendor, dev->descriptor.idProduct);
      if (tmodem > 0) {
	goon = 0;
	adsl_dev = dev;
      }
      else
	dev = dev->next;
    }
    if (goon)
      bus = bus->next;
  }
  if (adsl_dev == NULL) {
    wprintw (creditw, gettext ("Error: I didn't find ADSL modem
\r"));
    return -1;
  }
  wprintw (creditw,
	   gettext
	   ("I found ADSL modem with VendorID = %04x & ProductID = %04x
\r"),
	   adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);

  /* connect to ADSL modem */
  adsl_handle = usb_open (adsl_dev);
  if (adsl_handle == NULL) {
    wprintw (creditw,
	     gettext
	     ("Error: Couldn't get device handle for ADSL modem
\r"));
    return -1;
  }

  /* check if other program is using interfaces 0 */

  r = usb_claim_interface (adsl_handle, 0);
  if (r < 0) {
    wprintw (creditw, "Error: usb_claim_interface 0: %s
\r",
	     usb_strerror ());
    return -1;
  }
//  else
//    { if (r!=0) printf("Call usb_claim_interface 0  return: %d
",r); }


  PDEBUG (gettext ("Interface = %d
"), adsl_handle->interface);

  if (arsync) {
    wprintw (autow, "\rAuto  ON");
  }
  else {
    wprintw (autow, "\rAuto OFF");
  }

  while (getch () != 'q') {
    wrefresh (creditw);
    wrefresh (autow);
    memset (buf, 0, 0x10);
    if (read1 > 0 || read) {
      len = usb_bulk_read (adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
      read1 = read1 - 1;
      if (len < 0)
	printf (gettext ("Error retrieving info!
"));
      else {
	translate_buf (buf, len);

	if (buf[0] == 0x01) {
	  wprintw (rawinfow, " ");
	  wmove (rawinfow, 0x0c, 0);
	  if ( show == 1 )
	    wrefresh (rawinfow);
	  wprintw (rawinfow, "---------------------------------------\r>");

	  for (i = 0; i <= len; i++) {
	    wprintw (rawinfow, "%02x", buf[i] & 0xff );
	    if ((i % 4 == 0) && (i != len))
	      wprintw (rawinfow, " ");
	  }
	  wmove (rawinfow, 0x0c, 0);
	  if ( show == 1 )
	    wrefresh (rawinfow);
	}

	if ( ( buf[0] & 0xff ) >= 0xf1 && ( buf[0] & 0xff ) <= 0xfc) {
	  wprintw (rawinfow, " ");
	  wmove (rawinfow, ( buf[0] - 0xf1 ) & 0xff, 0);
	  if ( show == 1 )
	    wrefresh (rawinfow);
	  wprintw (rawinfow, "---------------------------------------\r>");
	  for (i = 0; i < len; i++) {
	    wprintw (rawinfow, "%02x", buf[i] & 0xff );
	    if ((i % 4 == 0) && (i != len))
	      wprintw (rawinfow, " ");
//                  printf("%02x", buf[i]);
	  }
	  wmove (rawinfow, ( buf[0] - 0xf1 ) & 0xff, 0);
	  if ( show == 1 )
	    wrefresh (rawinfow);
	}

	if (buf[0] == 0x01) {
	  if (buf[1] == MODEM_UP)
	    wprintw (statusw, "@");
	  if (buf[1] == MODEM_DOWN) {
	    wprintw (statusw, "#");
	    if (arsync)
	      {
	      wprintw (statusw, "D");
	      send_line_down_signal (adsl_handle, tmodem);
	      wprintw (statusw, "S");
	      send_sync_signal (adsl_handle, tmodem,line);
		}
	  }
	  if (buf[1] == MODEM_WAIT)
	    wprintw (statusw, "_");
	  if (buf[1] == MODEM_INIT)
	    wprintw (statusw, "-");
	  wrefresh (statusw);
	}
	if ((buf[0] == 0x02) && (len == 1))
	  if (arsync)
	    {
	    wprintw (statusw, "D");
	    send_line_down_signal (adsl_handle, tmodem);
	    }
      }
    }
    else {
      sleep (1);
    }
    switch (getch ()) {
    case 'D':
    case 'd':
	wprintw (statusw, "D");
      r = send_line_down_signal (adsl_handle, tmodem);
      break;
    case '3':
	wprintw (statusw, "[30]");
      r = send_signal_30 (adsl_handle, tmodem);
      break;
    case 'S':
    case 's':
	    wprintw (statusw, "S");
      r = send_sync_signal (adsl_handle, tmodem,line);
      break;
    case 'R':
    case 'r':
      wprintw (infow, "[Asking one time]
\r");
      read1 = 1;
      break;
    case 'C':
    case 'c':
      wprintw (infow, "[Asking 12 times]
\r");
      read1 = 12;
      break;
    case 'A':
    case 'a':
      if (arsync) {
	wprintw (autow, "\rAuto OFF");
	arsync = 0;
	read = 1;
      }
      else {
	wprintw (autow, "\rAuto  ON");
	arsync = 1;
	read = 1;
      }
      wrefresh (autow);
      break;
    case 'T':
    case 't':
	if ( show == 2 )
	     show = 1;
	else
	 show = 2;
      break;
    case 'H':
    case 'h':
	 show = 3;
      break;
    case 'L':
    case 'l':
      if (line == 0x15) {
	line = 0x11;
        r = send_line_down_signal (adsl_handle, tmodem);
      }
      else {
	line = 0x15;
        r = send_line_down_signal (adsl_handle, tmodem);
      }
      break;
    case 'Q':
    case 'q':
      ungetch ('q');
      break;
    case ERR:
//                      printf ("nada");
      break;
    default:
//                      printf ("hola");
      break;
    }
    switch (show) {
	case 2:
      wclear (humaninfow);
      wborder (humaninfow, 0, 0, 0, 0, 0, 0, 0, 0);

      wprintw (humaninfow,
	       "    Near End      Far End         Transmision Recepcion
\r");
      wprintw (humaninfow, "CESR : %5d ", modem_info.NearFecNotInterleaved);
      wprintw (humaninfow, "CESR : %5d ", modem_info.FarFecNotInterleaved);
      wprintw (humaninfow, "Pot.  %4.1f dBm  %4.1f dBm
\r",
	       modem_info.potenciat, modem_info.potenciar);
      wprintw (humaninfow, "CESF : %5d ", modem_info.NearFecInterleaved);
      wprintw (humaninfow, "CESF : %5d ", modem_info.FarFecInterleaved);
      wprintw (humaninfow, "Rui.  %4.1f dB   %4.1f dB 
\r",
	       modem_info.ruidot, modem_info.ruidor);
      wprintw (humaninfow, "CRC-F: %5d ", modem_info.NearCrcNotInterleaved);
      wprintw (humaninfow, "CRC-F: %5d ", modem_info.FarCrcNotInterleaved);
      wprintw (humaninfow, "Ate.  %4.1f dB   %4.1f dB 
\r",
	       (float) modem_info.up_attenuation / 2,
	       (float) modem_info.down_attenuation / 2);
      wprintw (humaninfow, "CRC-I: %5d ", modem_info.NearCrcInterleaved);
      wprintw (humaninfow, "CRC-I: %5d ", modem_info.FarCrcInterleaved);
      wprintw (humaninfow, "Ocu.  %3d %%     %3d %% 
\r",
	       modem_info.up_bitrate_percent,
	       modem_info.down_bitrate_percent);
      wprintw (humaninfow, "HEC-F: %5d ", modem_info.NearHecNotInterleaved);
      wprintw (humaninfow, "HEC-F: %5d ", modem_info.FarHecNotInterleaved);
      wprintw (humaninfow, "Vel.  %4d kb/s %4d kb/s 
\r",
	       modem_info.up_bitrate, modem_info.down_bitrate);
      wprintw (humaninfow, "HEC-I: %5d ", modem_info.NearHecInterleaved);
      wprintw (humaninfow, "HEC-I: %5d
\r", modem_info.FarHecInterleaved);

      wrefresh (humaninfow);
      break;
    case 1:
      wclear (helpw);
      wclear (humaninfow);
      wrefresh (infow);
      wrefresh (rawinfow);
      break;
    case 3:
      wclear (helpw);
  wprintw (helpw,  "Key A: Auto ON/OFF      Key D: Send Down Line signal

\r");
  wprintw (helpw,  "Key R: Ask on time      Key C: Ask 12 times

\r");
  wprintw (helpw,  "Key S: Send Sync signal Key Q: Exit program

\r");
  wprintw (helpw,  "Key L: Change line type - Analog <==> ISDN

\r");
  wprintw (helpw,  "Key 3: Send signal Request=30 Value=03 Index=00 

\r");
  wprintw (helpw,  "Status:: '#' MODEM_DOWN '_' MODEM_WAIT '-' MODEM_INIT '@' MODEM_UP

\r");

      wrefresh (helpw);
    break;
    
    }
  }
  printf ("
");
  PDEBUG (gettext ("Releasing interface...
"));
  usb_release_interface (adsl_handle, 0);
  PDEBUG (gettext ("Releasing device...
"));
  usb_close (adsl_handle);

  echo ();
  refresh ();
  delwin (statusw);
  delwin (infow);
  delwin (creditw);
  delwin (autow);
  delwin (rawinfow);
  endwin ();

  return r;

}
