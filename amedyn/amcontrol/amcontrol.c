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
  WINDOW *infow;  /* Global var..... Is bad? */

/* modem internal characteristics */
struct usb_modem_char
struct usb_modem_char {
  unsigned int vid;  /* VendorID */
  unsigned int pid;  /* ProductID */
  char *firmfile;  /* firmware file name */
  int datamax;  /* maximum data that we can send in a block */
};
struct usb_modem_char modem_char;

/* info about modem */
struct usb_modem_info
struct usb_modem_info {
  int modem_status;
  char firm_version[5];		/* firmware version */
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
 "ANSI",
 "G.DMT",
 "G.Lite"
};

/* show printable char */
void
void print_char(unsigned char c)
{
  if (c >= ' ' && c < 0x7f)
    printf ("%c", c);
    printf("%c", c);
  else
    printf (".");
    printf(".");
}

/* show buffer */
/*
void dump(unsigned char *buf, int lenbuf, int lenline)
{
  int i, j;			// counters 
  int i, j;  /* counters */
    
  printf ("                                                                          \r") ;
  for (i = 0; i < lenbuf; i+= lenline)
  {
    for (j = i; j < lenbuf && j < i + lenline; j++)
      printf ("%02x ", buf[j]);
      printf("%02x ", buf[j]);
    for (; j < i + lenline; j++)
      printf ("   ");
      printf("   ");
    for (j = i; j < lenbuf && j < i + lenline; j++)
      print_char (buf[j]);
      print_char(buf[j]);
    printf("\r");
  }
}
*/

/* transfer a control message to USB bus */
int
int transfer_ctrl_msg(usb_dev_handle *adsl_handle, int requesttype, int request, int value, int index, char *buf, int size)
{
  int j;			/* counter */
  int j;  /* counter */
  int n;  /* bytes transfed or error code */
  int tmout = CTRL_TIMEOUT;  /* timeout value */

  n = 0;
  for (j = 0; j < CTRL_MSG_RETRIES; j++) {
#ifdef SIMULATE
    n = size;
#else
    n =
    n = usb_control_msg(adsl_handle, requesttype, request, value, index,  buf, size, tmout);
#endif
    if (n >= 0) {
#if DEBUG_TRANSFER
      printf (gettext ("%d bytes transferred:
"), n);
      printf(gettext("%d bytes transferred:
"), n);
      dump(buf, n, 16);
#endif
      break;
    }
    else {
      printf (gettext ("Error: usb_control_msg: %s
"), usb_strerror ());
      printf(gettext("Error: usb_control_msg: %s
"), usb_strerror());
      if (n == -EPIPE) {
	usb_clear_halt (adsl_handle, 0x00);
        usb_clear_halt(adsl_handle, 0x00);
        usb_clear_halt(adsl_handle, 0x80);
      }
      else if (n == -ETIMEDOUT) {
	tmout += TIMEOUT_ADD;
      }
    }
  }
  if (n < 0) {
    printf (gettext ("Error: usb_control_msg failed after %d retries
"),
    printf(gettext("Error: usb_control_msg failed after %d retries
"), CTRL_MSG_RETRIES);
    return -1;
  }
  return n;
}

/* Send sync signals */
int
int send_sync_signal(usb_dev_handle *adsl_handle, int tmodem)
{
  char buf[0x1ff];	/* buffer */
  unsigned char buf[0x1ff];   /* buffer */
  long len;     /* length */
  wprintw (infow, "[Sending SyncS.");

  wprintw (infow,"[Sending SyncS.");
*/

  /* set AFE value, R_Function_Code = 0x15 (adjust Alcatel DSP for our configuration) */
  /* 0x1fd in CTRLE protocol */
  /* 0x15 = analog line, 0x11 ISDN line */
  buf[0] = 0x15;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x1fd, buf, 1);
		       buf, 1);
  if (len < 0)
    return -1;
  wprintw (infow,".");
  wprintw (infow, ".");

  buf[0] = 0x01;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4a, buf, 1);
		       1);
  if (len < 0)
    return -1;
  wprintw (infow,".");
  wprintw (infow, ".");

  buf[0] = 0x00;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4b, buf, 1);
		       1);
  if (len < 0)
    return -1;
  wprintw (infow,".");
  wprintw (infow, ".");

  buf[0] = 0x00;
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x06, 0x03, 0x4c, buf, 1);
		       1);
  if (len < 0)
    return -1;
  wprintw (infow,".");


  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x02, 0x03, 0x00, NULL, 0);
		       NULL, 0);
  if (len < 0)
    return -1;
  wprintw (infow,".");
  wprintw (infow, ".");

  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_IN, 0x0e, 0x03, 0x00, buf, 0x0c);
		       0x0c);
  if (len < 0x0c)
    return -1;
  wprintw (infow,".send]");
  wprintw (infow,"
\r");
  wprintw (infow, "\r");

  return 0;
}

/* Send line down signal */
int send_line_down_signal(usb_dev_handle *adsl_handle, int tmodem)
/* Send signal Request=30 Value=03 Index=00*/
int send_signal_30 (usb_dev_handle * adsl_handle, int tmodem)
  unsigned char buf[0x1ff];   /* buffer */
  long len;     /* length */
  char buf[0x1ff];	/* buffer */
  unsigned char buf[0x1ff];	/* buffer */
  wprintw (infow,"[Sending DLS.");
  len = transfer_ctrl_msg(adsl_handle, VENDOR_REQUEST_OUT, 0x03, 0x03, 0x00, buf, 0);
  wprintw (infow,".");
  if (len != 0)
    { wprintw (infow,"(Len %li)",len); return -1;} 
  wprintw (infow,".send]");
  wprintw (infow,"
\r");
  }
  wprintw (infow, ".send]");
  wprintw (infow, "
\r");
  return 0;
}

int check_modem(unsigned int vid, unsigned int pid)

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

int translate_buf (unsigned char buf[0x1ff],int len)
}

  switch (len)
    {
    case 0x00:
	wprintw (infow,"[{}]");
        wprintw (infow,"
\r");
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
	wprintw (infow,"
\r");
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
        wprintw (infow,"
\r");
	break;
    case 0x0c:
	if (buf[0]==0x01)
	  {
/*	  // Too much output
	  if (buf[1] == MODEM_UP)   wprintw (infow,"[MODEM_UP  ]
\r");
  case 0x01:
    modem_info.ruidor = (float) buf[6] / 2;
    modem_info.potenciar = (float) buf[5] / 2;
    modem_info.ruidot = (float) buf[4] / 2;
    modem_info.potenciat = (float) buf[7] / 2;
	  }
	else
	  wprintw (infow,"[Unknow status %02x]
\r",buf[1]);
	break;	
    default:
        if ( (buf[0] < 0xf1) || (buf[0] > 0xfc) ) 
	    wprintw (infow,"[Other info len: %2d (%02x)]
\r",len,buf[0]);
	break;
    }
    break;
    return 0;
	for ( i = 0x00 ; i < len; i = i + 1 )
	    { wprintw (infow, "%02x ", buf[i]); }
      wprintw (infow, "]
\r" );
int main(int argc, char *argv[])
  }
  return 0;
}

  struct usb_bus * bus;
  struct usb_device * dev;
  struct usb_device * adsl_dev = NULL;
  usb_dev_handle * adsl_handle;
  unsigned char buf[0x1ff];   /* buffer */
  int len; 
    
  struct usb_device *dev;
  struct usb_device *adsl_dev = NULL;
  usb_dev_handle *adsl_handle;
  unsigned char buf[0x1ff];	/* buffer */
  char buf[0x1ff];	/* buffer */
  int r = 0; 
  int len;

  /* boolean value */
  int goon;

  /* result code */
  int r = 0;

  /* open mode */
//  int open_mode = -1;
  int read   = 1;
  int read1  = 0;

  /*
  1 = Raw output, etc.
  2 = Human output, etc.
  3 = Help.
  */
    
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
    
  scrollok (humaninfow, TRUE);
  box (humaninfow, 0, 0);
  wborder (humaninfow, 0, 0, 0, 0, 0, 0, 0, 0);

  helpw = newwin (0x0d, 70, 12, 10);
  setlocale(LC_ALL, "");
  scrollok (helpw, TRUE);
  box (helpw, 0, 0);
  wborder (helpw, 0, 0, 0, 0, 0, 0, 0, 0);

    bindtextdomain(TF_CODE, "/usr/share/locale");  /* set directory for a domain (source code messages) */
  textdomain(TF_CODE);  /* set domain */
  //memset(modem_cmd_state, 0, sizeof(modem_cmd_state));

  /* init locale */
  wprintw(creditw,gettext("Zyxel 630-11 & Asus AAM6000UG line control program."));
  wprintw(creditw," 03/08/2004
\r");
  wprintw(creditw,"Aurelio Arroyo  <sktt@users.sourceforge.net>

\r");
  wprintw(creditw,"Key a: Auto ON/OFF      Key d: Send Down Line signal Key r: Ask on time
\r");
  wprintw(creditw,"Key s: Send Sync signal Key q: Exit program          Key c: Ask 12 times

\r");
  wprintw(creditw,"Status:: '#' MODEM_DOWN '_' MODEM_WAIT '-' MODEM_INIT '@' MODEM_UP

\r");
  
	   gettext ("Zyxel 630-11 & Asus AAM6000UG line control program."));
  wprintw (creditw, " 03/08/2004
\r");
  usb_init();
  if (usb_find_busses() < 0)
  {
    wprintw(creditw,gettext("Error: I can't find busses
\r"));
  wprintw (helpw,  "Key a: Auto ON/OFF      Key d: Send Down Line signal

\r");
  wprintw (helpw,  "Key r: Ask on time      Key c: Ask 12 times

\r");
  wprintw (helpw,  "Key s: Send Sync signal Key q: Exit program

\r");
  if (usb_find_devices() < 0)
  {
    wprintw(creditw,gettext("Error: I can't find devices
\r"));

  /* init USB bus and find devices */
  usb_init ();
  if (usb_find_busses () < 0) {
    wprintw (creditw, gettext ("Error: I can't find busses
\r"));
    return -1;
  }
  while (bus && goon)
  {
  if (usb_find_devices () < 0) {
    wprintw (creditw, gettext ("Error: I can't find devices
\r"));
    while (dev && goon)
    {
      tmodem = check_modem(dev->descriptor.idVendor, dev->descriptor.idProduct);
      if (tmodem > 0)
      {
        goon = 0;
        adsl_dev = dev;
  goon = 1;
  while (bus && goon) {
    dev = bus->devices;
        dev = dev->next;
    while (dev && goon) {
      tmodem =
	check_modem (dev->descriptor.idVendor, dev->descriptor.idProduct);
      if (tmodem > 0) {
	goon = 0;
  if (adsl_dev == NULL)
  {
    wprintw(creditw,gettext("Error: I didn't find ADSL modem
\r"));
      }
      else
	dev = dev->next;
  wprintw(creditw,gettext("I found ADSL modem with VendorID = %04x & ProductID = %04x
\r"), adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);
  }
  if (adsl_dev == NULL) {
    wprintw (creditw, gettext ("Error: I didn't find ADSL modem
\r"));
  adsl_handle = usb_open(adsl_dev);
  if (adsl_handle == NULL)
  {
    wprintw(creditw,gettext("Error: Couldn't get device handle for ADSL modem
\r"));
	   ("I found ADSL modem with VendorID = %04x & ProductID = %04x
\r"),
	   adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);

  /* connect to ADSL modem */
  adsl_handle = usb_open (adsl_dev);
  if (adsl_handle == NULL) {
  r=usb_claim_interface(adsl_handle, 0);
  if ( r < 0)
  {
    wprintw(creditw,"Error: usb_claim_interface 0: %s
\r", usb_strerror());
    return -1;
  }

  /* check if other program is using interfaces 0 */

  r = usb_claim_interface (adsl_handle, 0);
  if (r < 0) {
  PDEBUG(gettext("Interface = %d
"), adsl_handle->interface);
    wprintw (creditw, "Error: usb_claim_interface 0: %s
\r",
	     usb_strerror ());
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
    	    printf(gettext("Error retrieving info!
"));
	else
	    { 
	    translate_buf(buf,len);
	    wrefresh (rawinfow);
	  wprintw (rawinfow, "---------------------------------------\r>");
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
	      wprintw (statusw, "D");
	      send_line_down_signal (adsl_handle, tmodem);
	      if (buf[1] == MODEM_UP) wprintw (statusw,"@");
	      if (buf[1] == MODEM_DOWN) 
		{ 
	        wprintw (statusw,"#"); 
		if (arsync) send_sync_signal(adsl_handle, tmodem); 
		} 
	      if (buf[1] == MODEM_WAIT) wprintw (statusw,"_");
	      if (buf[1] == MODEM_INIT)	wprintw (statusw,"-");
	      wrefresh(statusw);
	      send_sync_signal (adsl_handle, tmodem,line);
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
			wprintw (infow,"[Asking one time]
\r"); 
			read1 = 1; 
			break;
		    case 'C':
		    case 'c':
			wprintw (infow,"[Asking 12 times]
\r"); 
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
    case 3:
      wclear (helpw);
  PDEBUG(gettext("Releasing interface...
"));
  usb_release_interface(adsl_handle, 0);
  PDEBUG(gettext("Releasing device...
"));
  usb_close(adsl_handle);
  wprintw (helpw,  "Key L: Change line type - Analog <==> ISDN

\r");
  wprintw (helpw,  "Key 3: Send signal Request=30 Value=03 Index=00 

\r");
  wprintw (helpw,  "Status:: '#' MODEM_DOWN '_' MODEM_WAIT '-' MODEM_INIT '@' MODEM_UP

\r");
  refresh();
  delwin(statusw);
  delwin(infow);
  delwin(creditw);
  delwin(autow);
  delwin(rawinfow);
  endwin();
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
