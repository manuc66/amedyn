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
  WINDOW *infow;  /* Global var..... Is bad? */

/* modem internal characteristics */
struct usb_modem_char {
  unsigned int vid;  /* VendorID */
  unsigned int pid;  /* ProductID */
  char *firmfile;  /* firmware file name */
  int datamax;  /* maximum data that we can send in a block */
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
    
  printf ("                                                                          \r") ;
  for (i = 0; i < lenbuf; i+= lenline)
  {
    for (j = i; j < lenbuf && j < i + lenline; j++)
      printf("%02x ", buf[j]);
    for (; j < i + lenline; j++)
      printf("   ");
    for (j = i; j < lenbuf && j < i + lenline; j++)
      print_char(buf[j]);
    printf("\r");
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

/* Send sync signals */
int send_sync_signal(usb_dev_handle *adsl_handle, int tmodem)
{
  unsigned char buf[0x1ff];   /* buffer */
  long len;     /* length */

  wprintw (infow,"[Sending SyncS.");

  /* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
  /* 0x1fd in CTRLE protocol */
  /* 0x15 = analog line, 0x11 ISDN line */
  buf[0] = 0x15;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x1fd, buf, 1);
  if (len < 0)
    return -1;
  wprintw (infow,".");

  buf[0] = 0x01;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4a, buf, 1);
  if (len < 0)
    return -1;
  wprintw (infow,".");

  buf[0] = 0x00;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4b, buf, 1);
  if (len < 0)
    return -1;
  wprintw (infow,".");

  buf[0] = 0x00;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4c, buf, 1);
  if (len < 0)
    return -1;
  wprintw (infow,".");

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x02, 0x03, 0x00, NULL, 0);
  if (len < 0)
    return -1;
  wprintw (infow,".");

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, 0x0e, 0x03, 0x00, buf, 0x0c);
  if (len < 0x0c)
    return -1;
  wprintw (infow,".send]");
  wprintw (infow,"\n\r");

  return 0;
}

/* Send line down signal */
int send_line_down_signal(usb_dev_handle *adsl_handle, int tmodem)
{
  unsigned char buf[0x1ff];   /* buffer */
  long len;     /* length */

  wprintw (infow,"[Sending DLS.");
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x03, 0x03, 0x00, buf, 0);
  wprintw (infow,".");
  if (len != 0)
    { wprintw (infow,"(Len %li)",len); return -1;} 
  wprintw (infow,".send]");
  wprintw (infow,"\n\r");
  return 0;
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

int translate_buf (unsigned char buf[0x1ff],int len)
{
  switch (len)
    {
    case 0x00:
	wprintw (infow,"[{}]");
        wprintw (infow,"\n\r");
	break;
    case 0x01:
	switch (buf[0])
	    {
	    case 0x02:
		wprintw (infow,"[Line problem?]");
		break;
	    case 0x50:
		wprintw (infow,"[Line up?]");
		break;
	    case 0x60:
		wprintw (infow,"[Line yet down?]");
		break;
	    case 0x70:
		wprintw (infow,"[Line down OK?]");
		break;
	    default:
		wprintw (infow,"[%02x]",buf[0]);
		break;
	    }
	wprintw (infow,"\n\r");
	break;
    case 0x02:
	switch (buf[0])
	    {
	    case 0x40:
		switch (buf[1])
		    {
		    case 0x14:
			wprintw (infow,"[Error at MODEM_INIT?]");
			break;
		    case 0x19:
			wprintw (infow,"[Error at MODEM_WAIT?]");
			break;
		    default:
			wprintw (infow,"[%02x %02x]",buf[0],buf[1]);
		    break;
		    }
		break;
	    default:
		wprintw (infow,"[%02x %02x]",buf[0],buf[1]);
		break;
	    }
        wprintw (infow,"\n\r");
	break;
    case 0x0c:
	if (buf[0]==0x01)
	  {
/*	  // Too much output
	  if (buf[1] == MODEM_UP)   wprintw (infow,"[MODEM_UP  ]\n\r");
	  if (buf[1] == MODEM_DOWN) wprintw (infow,"[MODEM_DOWM]\n\r"); 
	  if (buf[1] == MODEM_WAIT) wprintw (infow,"[MODEM_WAIT]\n\r");
	  if (buf[1] == MODEM_INIT) wprintw (infow,"[MODEM_INIT]\n\r");
*/
	  }
	else
	  wprintw (infow,"[Unknow status %02x]\n\r",buf[1]);
	break;	
    default:
        if ( (buf[0] < 0xf1) || (buf[0] > 0xfc) ) 
	    wprintw (infow,"[Other info len: %2d (%02x)]\n\r",len,buf[0]);
	break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
  /* bus structures variables */
  int i;
  struct usb_bus * bus;
  struct usb_device * dev;
  struct usb_device * adsl_dev = NULL;
  usb_dev_handle * adsl_handle;
  unsigned char buf[0x1ff];   /* buffer */
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
  int read   = 1;
  int read1  = 0;

  WINDOW *statusw;
  WINDOW *autow;
  WINDOW *creditw;
  WINDOW *rawinfow;
    
    initscr();
    noecho();
    timeout (0);
    clear();

    statusw=newwin (2,70,10,5);    
    scrollok(statusw,TRUE);
    box(statusw,0,0);
    wborder(statusw,0,0,0,0,0,0,0,0);

    autow=newwin (2,10,13,0);    
    scrollok(autow,TRUE);
    box(autow,0,0);
    wborder(autow,0,0,0,0,0,0,0,0);

    infow=newwin (10,30,13,10);    
    scrollok(infow,TRUE);
    box(infow,0,0);
    wborder(infow,0,0,0,0,0,0,0,0);

    creditw=newwin (10,80,0,0);    
    scrollok(creditw,TRUE);
    box(creditw,0,0);
    wborder(creditw,0,0,0,0,0,0,0,0);
        
    rawinfow=newwin (0xc,40,12,40);    
    scrollok(rawinfow,TRUE);
    box(rawinfow,0,0);
    wborder(rawinfow,0,0,0,0,0,0,0,0);
    
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
  wprintw(creditw,gettext("Zyxel 630-11 & Asus AAM6000UG line control program."));
  wprintw(creditw," 03/08/2004\n\r");
  wprintw(creditw,"Aurelio Arroyo  <sktt@users.sourceforge.net>\n\n\r");
  wprintw(creditw,"Key a: Auto ON/OFF      Key d: Send Down Line signal Key r: Ask on time\n\r");
  wprintw(creditw,"Key s: Send Sync signal Key q: Exit program          Key c: Ask 12 times\n\n\r");
  wprintw(creditw,"Status:: '#' MODEM_DOWN '_' MODEM_WAIT '-' MODEM_INIT '@' MODEM_UP\n\n\r");
  
  /* init USB bus and find devices */
  usb_init();
  if (usb_find_busses() < 0)
  {
    wprintw(creditw,gettext("Error: I can't find busses\n\r"));
    return -1;
  }
  if (usb_find_devices() < 0)
  {
    wprintw(creditw,gettext("Error: I can't find devices\n\r"));
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
    wprintw(creditw,gettext("Error: I didn't find ADSL modem\n\r"));
    return -1;
  }
  wprintw(creditw,gettext("I found ADSL modem with VendorID = %04x & ProductID = %04x\n\r"), adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);

  /* connect to ADSL modem */
  adsl_handle = usb_open(adsl_dev);
  if (adsl_handle == NULL)
  {
    wprintw(creditw,gettext("Error: Couldn't get device handle for ADSL modem\n\r"));
    return -1;
  }

  /* check if other program is using interfaces 0 */

  r=usb_claim_interface(adsl_handle, 0);
  if ( r < 0)
  {
    wprintw(creditw,"Error: usb_claim_interface 0: %s\n\r", usb_strerror());
    return -1;
  }
//  else
//    { if (r!=0) printf("Call usb_claim_interface 0  return: %d\n",r); }


  PDEBUG(gettext("Interface = %d\n"), adsl_handle->interface);

  if (arsync) 
    {wprintw (autow,"[Auto  ON]");} 
  else 
    {wprintw (autow,"[Auto OFF]");}

    while ( getch() != 'q' )
	{
	wrefresh(creditw);
        wrefresh(autow);
	memset(buf, 0, 0x10);
	if ( read1 > 0 || read ) 
	{
	 len = usb_bulk_read(adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
	 read1=read1-1;
	if (len < 0   )
    	    printf(gettext("Error retrieving info!\n"));
	else
	    { 
	    translate_buf(buf,len);

	    if ( buf[0] >= 0xf1 && buf[0] <= 0xfc )
		{
		wmove(rawinfow,buf[0]-0xf1,0);
    		wrefresh(rawinfow);
    		wprintw(rawinfow,"--------------------");
    		wprintw(rawinfow,"-------------------\r");
    		wprintw(rawinfow,">");
		for (i=0;i<=len;i++)
		    {
    		    wprintw(rawinfow,"%02x", buf[i]);
		    if ( ( i%4 == 0 ) && ( i != len ) ) wprintw(rawinfow," ");
//    		    printf("%02x", buf[i]);
		    }
		wmove(rawinfow,buf[0]-0xf1,0);
    		wrefresh(rawinfow);
    		wprintw(rawinfow," ");
		} 
	    
	    if (buf[0]==0x01)
	    {
	      if (buf[1] == MODEM_UP) wprintw (statusw,"@");
	      if (buf[1] == MODEM_DOWN) 
		{ 
	        wprintw (statusw,"#"); 
		if (arsync) send_sync_signal(adsl_handle, tmodem); 
		} 
	      if (buf[1] == MODEM_WAIT) wprintw (statusw,"_");
	      if (buf[1] == MODEM_INIT)	wprintw (statusw,"-");
	      wrefresh(statusw);
	    }
	    if ( (buf[0] == 0x02) && (len == 1) )
		if (arsync) send_line_down_signal(adsl_handle, tmodem); 
	 }
	}
	     switch (getch())
		    {
		    case 'D':
		    case 'd':
			r = send_line_down_signal(adsl_handle, tmodem); 
			break;
		    case 'S':
		    case 's':
			r = send_sync_signal(adsl_handle, tmodem); 
			break;
		    case 'R':
		    case 'r':
			wprintw (infow,"[Asking one time]\n\r"); 
			read1 = 1; 
			break;
		    case 'C':
		    case 'c':
			wprintw (infow,"[Asking 12 times]\n\r"); 
			read1 = 12; 
			break;
		    case 'A': 
		    case 'a':
			if (arsync) 
			    {wprintw (autow,"[Auto OFF]"); arsync=0; read=0;} 
			else 
			    {wprintw (autow,"[Auto  ON]"); arsync=1; read=1;}
	    		wrefresh(autow);
			break;
		    case 'Q':
		    case 'q':
			ungetch('q');
			break;
		    case ERR :
//			printf ("nada");
			break;
		    default :
//			printf ("hola");
			break;
		    }	    
	     wrefresh(infow);
	    
	}
  printf ("\n");
  PDEBUG(gettext("Releasing interface...\n"));
  usb_release_interface(adsl_handle, 0);
  PDEBUG(gettext("Releasing device...\n"));
  usb_close(adsl_handle);

  echo ();
  refresh();
  delwin(statusw);
  delwin(infow);
  delwin(creditw);
  delwin(autow);
  delwin(rawinfow);
  endwin();

  return r;
   
}

