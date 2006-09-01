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

  Author     : Sktt (Aurelio)
  Creation   : 03/12/2005

  Description: Split from amload.c . Commons funtion for amdeyn user space tools.

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

  12/10/2003 Mathias Gug
  Fix claim interfaces 0, 1

  27/10/2003 Josep Comas
  Credits update
  Ajust sign int types values

  22/06/2004 Counasse Emmanuel (manuc66[a]yahoo.fr)
  Don't clear_endpoints' before firmware send
  micro sleep added in post load

  11/07/2004  Sktt (Aurelio)
  Fix synchronization problem

  02/08/2004 Sktt (Aurelio)
  Remove my stats and debug code
  Add send_cmds_sync function

  07/12/2004 Sktt (Aurelio)
  Remove init firmware
  
  03/12/2005 Sktt (Aurelio)
  Split amload.c. Now all funtions are in amfunctions.c  
  Split load_firmware.

  11/12/2005 Emmanuel Counasse
  Add resync function
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

/* modem internal characteristics */
struct usb_modem_char modem_char;

/* info about modem */
struct usb_modem_info modem_info;

/* command-line options */
struct options command_options;

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
    printf("
");
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
      printf(gettext("%d bytes transferred:
"), n);
      dump(buf, n, 16);
#endif
      break;
    }
    else {
      printf(gettext("Error: usb_control_msg: %s
"), usb_strerror());
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
    printf(gettext("Error: usb_control_msg failed after %d retries
"), CTRL_MSG_RETRIES);
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
      printf(gettext("%d bytes downloaded:
"), n);
      dump(buf, n, 16);
#endif
      break;
    }
    else {
      printf(gettext("Error: usb_bulk_read: %s
"), usb_strerror());
      if (n == -EPIPE) {
        usb_clear_halt(adsl_handle, ep);
      }
      else if (n == -ETIMEDOUT) {
	tmout += TIMEOUT_ADD;
      }
    }
  }
  if (n < 0) {
    printf(gettext("Error: usb_bulk_read failed after %d retries
"), READ_BULK_RETRIES);
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
        printf(gettext("%d bytes uploaded:
"), n);
        dump(buf+(i*ncol), ncol, 16);
#endif
        break;
      }
      else {
        printf(gettext("Error: usb_bulk_write: %s
"), usb_strerror());
	if (n == -EPIPE) {
	  usb_clear_halt(adsl_handle, ep);
	}
	else if (n == -ETIMEDOUT) {
	  tmout += TIMEOUT_ADD;
	}
      }
    }
    if (n < 0) {
      printf(gettext("Error: usb_bulk_write failed after %d retries
"), SEND_BULK_RETRIES);
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
  PDEBUG(gettext("Sending block at address = 0x%04x...
"), place);
  if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, len+8))
    return -1;
  return 0;
}

/* start code execution at specified address */
int jump_to_address(usb_dev_handle *adsl_handle, unsigned int place)
{
  char buf[6];  /* buffer */

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

/* Say modem sync line */
int send_cmds_sync (usb_dev_handle *adsl_handle, int tmodem)
  {
  char buf[0x1ff];   /* buffer */
  long len;     /* length */

  if (command_options.verbose == 1)
    printf ("S");

  /* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
  /* 0x1fd in CTRLE protocol */
  /* 0x15 = analog line, 0x11 ISDN line */
  buf[0] = command_options.linetype & 0xff;
  if (command_options.verbose == 1)
    printf ("[0x%x]", buf[0]&0xff);
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

  return 0;
  }

/* Send line down signal */
int send_line_down_signal (usb_dev_handle * adsl_handle, int tmodem)
{
  char buf[0x1ff];		/* buffer */
  long len;			/* length */

  len = transfer_ctrl_msg (adsl_handle, VENDOR_REQUEST_OUT, 0x03, 0x03, 0x00, buf, 0);
  if (len != 0)
      return -1;

  return 0;
}

/* resync line */
int resync_line(usb_dev_handle * adsl_handle, int tmodem) {
  char buf[0x1ff];	/* buffer */
  int len;
  int line_up;
  int error;

  int read = 1;
  int read1 = 0;


  line_up = 0;
  error = 0;
  while ((line_up < 4) && (error < 3)) {
    memset(buf, 0, 0x10);
    if (read1 > 0 || read) {
      len = usb_bulk_read(adsl_handle, USB_IN_INFO, (char*) buf, 0x10, DATA_TIMEOUT);
      read1 = read1 - 1;
      if (len < 0) {
	printf (gettext("Error retrieving info!
"));
	++error;
      }
      else if (buf[0] == 0x01) {
	error = 0;
	if (buf[1] == MODEM_UP) {
	  printf("up|");
	  fflush(stdout);
	  ++line_up;
	}
	else if (buf[1] == MODEM_DOWN) {
	  printf("#|");
	  fflush(stdout);
	  if (send_line_down_signal (adsl_handle, tmodem) == -1)
	    ++error;
	  else if (send_cmds_sync (adsl_handle, tmodem) == -1) /* Sync line */
	    ++error;
	} else if (buf[1] == MODEM_WAIT) {
	  printf("_");
	  fflush(stdout);
	}
	else if (buf[1] == MODEM_INIT) {
	  printf("-");
	  fflush(stdout);
	}
      }
      else if ((buf[0] == 0x02) && (len == 1)) {
	error = 0;
	printf("#|");
	fflush(stdout);
	if (send_line_down_signal (adsl_handle, tmodem) == -1)
	  ++error;
      }
    }
    else {
      sleep (1);
    }
  }

  if (error == 3) {
    printf("Too many errors !");
    return -1;
  }

  return 0;
}


/* load firmware */
int load_firmware(usb_dev_handle *adsl_handle, int tmodem)
{
  char buf[0x1ff];   /* buffer */
  FILE *soft;   /* file handle */
  long len;     /* length */
  int place;    /* initial target address */
  char bufconf[8];  /* buffer to save config bytes */

  /*

  /* clear endpoints */
  clear_endpoints(adsl_handle, 1);
/*
  fseek(soft, 0L, SEEK_SET);
  len = fread(buf, 1, 5, soft);
  if (len <= 0)
  {
    printf(gettext("Error: No bytes to read from file %s
"), modem_char.firmfile);
    return -1;
  }
  if (len != 5)
  {
    printf(gettext("Error: I can't read initial 5 bytes from file %s
"), filename);
    return -1;
  }
*/
  /* check initial bytes */
/*
  PDEBUG(gettext("Initial bytes from file %s:
"), filename);
#if DEBUG
  dump(buf, 5, 5);
#endif
  if (buf[0] != FIRMBYTE1 || buf[1] != FIRMBYTE2 || buf[2] != FIRMBYTE3 || buf[3] != FIRMBYTE4 || buf[4] != FIRMBYTE5) {
    printf(gettext("Error: Maybe file %s isn't Conexant firmware, contact with author of this program
"), filename);
    return -1;
  }
*/


  /**************/
  /* initialize */
  /**************/

  PDEBUG(gettext("PreInit...
"));

  /* clear, reset */

  printf(gettext("Loading and sending %s...
"), modem_char.initfirmfile); 
  soft = fopen(modem_char.initfirmfile, "rb");
  if (soft == NULL)
  {
    printf(gettext("Error: I can't open file %s
"), modem_char.initfirmfile);
    return -1;
  }
  fseek(soft, 0L, SEEK_END);
  len = ftell(soft);
  PDEBUG(gettext("Length of file %s = %ld bytes
"), modem_char.initfirmfile, len);

  PDEBUG(gettext("Init Firmware...
"));
  fseek(soft, 0L, SEEK_SET);
  place = 0x0000; 
  while ((len = fread(buf, 1, modem_char.datamax, soft)) > 0)
  {
    PDEBUG(gettext("%ld bytes readed from file %s
"), len, modem_char.initfirmfile);
    if (send_block(adsl_handle, place, buf, len))
      return -1;
    place += len;
    buf[0] = 0x40; buf[1] = 0x01; buf[2] = 0x12;
    if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, 3))
      return -1;
  } 
  fclose(soft);

  if (send_bulk(adsl_handle, USB_OUT_FIRM, jump_to_address_0x0000, 1, 9))
  if (jump_to_address(adsl_handle, 0x00000000))
    return -1;

  printf(gettext("Init firmware is sent!
"));

  /* read something needed */
  if (read_bulk(adsl_handle, USB_IN_FIRM, buf, 0x1ff))
    return -1;
  memcpy(bufconf, buf+0xb9, 8);


  /*****************/
  /* send firmware */
  /*****************/

  printf(gettext("Loading and sending %s...
"), modem_char.firmfile); 
  soft = fopen(modem_char.firmfile, "rb");
  if (soft == NULL)
  {
    printf(gettext("Error: I can't open file %s
"), modem_char.firmfile);
    return -1;
  }
  fseek(soft, 0L, SEEK_END);
  len = ftell(soft);
  PDEBUG(gettext("Length of file %s = %ld bytes
"), modem_char.firmfile, len);

  PDEBUG(gettext("Firmware...
"));
  fseek(soft, 0L, SEEK_SET);
  place = 0x0000; 
  while ((len = fread(buf, 1, modem_char.datamax, soft)) > 0)
  {
    PDEBUG(gettext("%ld bytes readed from file %s
"), len, modem_char.firmfile);
    if (send_block(adsl_handle, place, buf, len))
      return -1;
    place += len;
    buf[0] = 0x40; buf[1] = 0x01; buf[2] = 0x12;
    if (send_bulk(adsl_handle, USB_OUT_FIRM, buf, 1, 3))
      return -1;
  } 
  fclose(soft);

  if (send_bulk(adsl_handle, USB_OUT_FIRM, jump_to_address_0x0000, 1, 9))
  if (jump_to_address(adsl_handle, 0x00000000))
    return -1;

  printf(gettext("Firmware is sent!
"));

  /* wait until firmware is ready */
  sleep(1);

  return 0;    
  }

int first_config(usb_dev_handle *adsl_handle, int tmodem)
  {
  char buf[0x1ff];   /* buffer */
  long len;     /* length */
  char value;  /* returned byte */
  int i;  /* counter */

  /*************/
  /* post load */
  /*************/

  PDEBUG(gettext("PostInit...
"));

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
    usleep(10000);
    if (len < 3)
      return -1;
  }

  return 0;
  }

int sync_line(usb_dev_handle *adsl_handle, int tmodem, int max_wait_line_up)
  {
  char buf[0x1ff];   /* buffer */
  long len;     /* length */
  time_t first, last, before;  /* to wait */

  /* waiting until line is up (a maximum time) */
  printf (gettext ("Waiting ADSL line is up (until %d seconds)...
"),
	  max_wait_line_up);
  time (&first);
  before = first;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x0b, 0x0c, 0x00, NULL, 0);
  if (len < 0)
    return -1;

/*At this point the Vendor driver write at CTRLE memory. From offset 0xba to*/
/*offset 0xc1. We don't change default values from this offset so we can*/
/*ignore this step.*/
/*
  for (i = 0xba; i <= 0xc1; i++) {
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, i, &bufconf[i-0xba], 1);
    if (len < 0)
      return -1;
  }
*/ 

do
  {

  len = send_cmds_sync (adsl_handle, tmodem); /* Sync line */

  if (len < 0)
	{
	printf(gettext("Error at sync line!
"));
	return -1;
	}

#ifdef SIMULATE
  exit(0);
#endif

  memset(&modem_info, 0, sizeof(struct usb_modem_info));

  do {
    PDEBUG(gettext("Sending retrieve info...
"));
    memset(buf, 0, 0x10);
    len = usb_bulk_read(adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
    if (len < 0)
      printf(gettext("Error retrieving info!
"));
    else {
      PDEBUG(gettext("%li bytes readed:
"), len);
#if DEBUG_TRANSFER
      if (command_options.verbose == 3 )
        if (len > 0)
            dump(buf, len, 16);
#endif

		  if ((buf[0] & 0xff) == 0x01)
		    {
		      modem_info.modem_status = buf[1] & 0xff;
              if (command_options.verbose == 1 ) {
		        switch (modem_info.modem_status) {
                    case MODEM_UP:
			            printf ("@");
                        continue;
		            case MODEM_DOWN:
			            printf ("#");
                        continue;
		            case MODEM_WAIT:
			            printf ("_");
                        continue;
		            case MODEM_INIT:
                        printf ("-");
                        continue;
                    }
              }
		      PDEBUG (gettext ("Modem status = %02x
"),
			      modem_info.modem_status);
		    }
	      fflush (stdout);
 


  if (difftime (time (&last), before) > 1)
	    {
#ifndef DEBUG
      if (command_options.verbose == 0 ) {
        printf(".");
        fflush(stdout);
        }
#endif
	      before = last;
	    }
	  }
	}
      while ((len != 2 && (buf[0] & 0xff) != 0x40)
         && (modem_info.modem_status != MODEM_UP)   
	     && ! (len == 1 && (buf[0] & 0xff) == LINE_UP) 
	     && ((difftime (last, first) < max_wait_line_up)
		 || max_wait_line_up == -1));
    }
  while (! (len == 1 && (buf[0] & 0xff) == LINE_UP)
     && (modem_info.modem_status != MODEM_UP)   
	 && ((difftime (last, first) < max_wait_line_up)
	     || max_wait_line_up == -1));

  printf ("
");


  if ( (modem_info.modem_status == MODEM_UP ) 
    ||(len == 1 && (buf[0] & 0xff) == 0x50) )
  {
    printf(gettext("ADSL line is up
"));
/* these lines blink leds:
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x30, 0x03, 0x00, NULL, 0);
    if (len < 0)
      return -1;
    len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, 0x0e, 0x03, 0x00, buf, 0x0c);
    if (len < 0x0c)
      return -1;
*/
/*
    printf(gettext("ADSL line is up (Downstream %u Kbits/s, Upstream %u Kbits/s)
"), modem_info.down_bitrate, modem_info.up_bitrate);
#ifdef DEBUG
    printf(gettext("ADSL mode = %i"), modem_info.operational_mode);
    if ((modem_info.operational_mode > 0) && (modem_info.operational_mode < 4))
      printf(" (%s)", adsl_modes[modem_info.operational_mode-1]);
    printf("
");
#endif
*/
  }
  else
  {
    printf(gettext("ADSL line is down
"));
    return -1;
  }

  return 0;
}

int wait_while_line_is_up(usb_dev_handle *adsl_handle, int tmodem)
  {
  char buf[0x1ff];   /* buffer */
  long len;     /* length */

modem_info.modem_status = 0xff;
do
  {
    PDEBUG(gettext("Sending retrieve info...
"));
    memset(buf, 0, 0x10);
    len = usb_bulk_read(adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
    if (len < 0)
      printf(gettext("Error retrieving info!
"));
    else {
      PDEBUG(gettext("%li bytes readed:
"), len);
#if DEBUG_TRANSFER
      if (len > 0)
        dump(buf, len, 16);
#endif
		  if ((buf[0] & 0xff) == 0x01)
		    {
		      modem_info.modem_status = buf[1] & 0xff;
/*		      if (modem_info.modem_status == MODEM_UP)
			printf ("@");
		      if (modem_info.modem_status == MODEM_DOWN)
			printf ("#");
		      if (modem_info.modem_status == MODEM_WAIT)
			printf ("_");
		      if (modem_info.modem_status == MODEM_INIT)
			printf ("-");
 */
		      PDEBUG (gettext ("Modem status = %02x
"),
			      modem_info.modem_status);
		    }
	      fflush (stdout);
	  }
    }
  while ((len != 1 && (buf[0] & 0xff) != 0x02)
	&& (modem_info.modem_status != MODEM_DOWN) );

  send_line_down_signal (adsl_handle, tmodem);

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
      modem_char.firmfile = "/lib/firmware/fw-usb.bin";
      modem_char.initfirmfile = "/lib/firmware/Init-usb.bin";
      break;

   /* Asus AAM6000UG */
    case 2:
      modem_char.datamax = 0x1f2;
      modem_char.firmfile = "/lib/firmware/Fw-usb_A.bin";
      modem_char.initfirmfile = "/lib/firmware/Init-usb.bin";
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
  /* Vendor = Mediacom Europe?, Product = DynaMiTe USB Modem */
  if (vid == 0x1767 && pid == 0x0005)
    return 1;

//  if (vid == 0x0572 && pid == 0xcafe)
//    return 2;

  return -1;
}


