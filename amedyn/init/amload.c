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
#include <popt.h>
#include "amedyn.h"


/* translation files */
#define TF_CODE "amload"

/* modem internal characteristics */
struct usb_modem_char {
  unsigned int vid;  /* VendorID */
  unsigned int pid;  /* ProductID */
  char *firmfile;  /* firmware file name */
  char *initfirmfile;  /* init firmware file name */
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

extern struct options command_options;
extern int check_modem(unsigned int vid, unsigned int pid);
extern int sync_line(usb_dev_handle *adsl_handle, int tmodem);
extern void init_modem(unsigned int tmodem, struct usb_device *adsl_dev, int open_mode);
extern int first_config(usb_dev_handle *adsl_handle, int tmodem);
extern int sync_line(usb_dev_handle *adsl_handle, int tmodem, int max_wait_line_up);
extern int load_firmware(usb_dev_handle *adsl_handle, int tmodem);
extern int send_line_down_signal (usb_dev_handle * adsl_handle, int tmodem);

{
  poptContext optCon; /* context for parsing command-line options */

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

  /* buffer use to check if the modem are init yet.*/  
            "Verbose level.",  "[0..1]"},
        { "linetype",  '\0', POPT_ARG_INT, &command_options.linetype, 0,
            "Set phone line type code. (default: 0x15)",  "0x11 | 0x15"},
  
  /* reset command queries */
  //memset(modem_cmd_state, 0, sizeof(modem_cmd_state));

	    POPT_ARG_NONE, &no_claim_interface_1, 0,
            "Send line down signal before sync line.",  ""},
        { NULL,     '0',
	    POPT_ARG_NONE, &command_options.no_claim_interface_0, 0,
	    POPT_ARG_NONE, &no_claim_interface_2, 0,
            "Don't claim interface 0. (Debug option)",  ""},
        { NULL,     '1',
	    POPT_ARG_NONE, &command_options.no_claim_interface_1, 0,
            "Don't claim interface 1. (Debug option)",  ""},
        { NULL,     '2',
	    POPT_ARG_NONE, &command_options.no_claim_interface_2, 0,
  printf(" 02/08/2004
");
  printf(gettext("Zyxel 630-11 & Asus AAM6000UG microcode upload program."));
  printf(" 16/07/2006
");
  printf("Josep Comas <jcomas@gna.es>
");
  printf("Sundar <sundar@cynaptix.biz>
");
  printf("Eduardo Espejo <eespejo@users.sourceforge.net>

");

  if (argc <= 1)
    fprintf(stderr, "WARNING: amload must be run with root privileges
");
    poptPrintUsage(optCon, stderr, 0);
	poptFreeContext(optCon);
    return -1;
  }

  r = poptGetNextOpt(optCon);
  if ( r < -1 ) {
    fprintf(stderr, "%s: %s
",
	poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
  /* check parameters */
/*
  if (argc < 1)
  {
    printf(gettext("Usage:
"));
    printf(gettext("   %s [open_mode]
"), argv[0]);
    return -1;
  }
  if (argc > 1) {
    open_mode = atoi(argv[1]);
    if ((open_mode < 0) || (open_mode > 5)) {
      printf(gettext("Error: Incorrect open mode
"));
      return -1;
    }
  }
*/

	poptStrerror(r));
	poptFreeContext(optCon);
    return -1;
    }
    
  poptFreeContext(optCon);

  /*
  * Security stuff
  * 1 - be sure to be root
  * 2 - umask to prevent critical data being read from log file
  */
  if(geteuid() != 0) {
    poptPrintUsage(optCon, stderr, 0);
    fprintf(stderr, "WARNING: amload must be run with root privileges

");
    exit (-1);
  }


  /* init USB bus and find devices */
  usb_init();
  if (usb_find_busses() < 0)
  {
    printf(gettext("Error: I can't find busses
"));
    return -1;
  }
  if (usb_find_devices() < 0)
  {
    printf(gettext("Error: I can't find devices
"));
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
    printf(gettext("Error: I didn't find ADSL modem
"));
    return -1;
  }
  printf(gettext("I found ADSL modem with VendorID = %04x & ProductID = %04x
"), adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);

#if DEBUG
   printf(" bLength: 0x%02x
", adsl_dev->config->bLength);
  if (usb_claim_interface(adsl_handle, 0) < 0)
   printf(" bDescriptorType: 0x%02x
", adsl_dev->config->bDescriptorType);
   printf(" wTotalLength: 0x%04x
", adsl_dev->config->wTotalLength);
   printf(" bNumInterfaces: 0x%02x
", adsl_dev->config->bNumInterfaces);
   printf(" bConfigurationValue: 0x%02x
", adsl_dev->config->bConfigurationValue);
   printf(" iConfiguration: 0x%02x
", adsl_dev->config->iConfiguration);
  if (usb_claim_interface(adsl_handle, 1) < 0)
   printf(" bmAttributes: 0x%02x
", adsl_dev->config->bmAttributes);
   printf(" MaxPower: 0x%02x
", adsl_dev->config->MaxPower);
#endif

  /* connect to ADSL modem */
  if (usb_claim_interface(adsl_handle, 2) < 0)
  r = sync_line(adsl_handle, tmodem);
  r = load_firmware(adsl_handle, tmodem); 
  r = first_config(adsl_handle, tmodem); 
  if ( ! no_claim_interface_0 && usb_claim_interface(adsl_handle, 0) < 0)
  {
  if ( ! no_claim_interface_1 && usb_claim_interface(adsl_handle, 1) < 0)
  /* check if other program is using interfaces 0, 1, 2 */
    r = load_firmware(adsl_handle, tmodem); 
    r = first_config(adsl_handle, tmodem); }
    return -1;
  if ( ! no_claim_interface_2 && usb_claim_interface(adsl_handle, 2) < 0)
  }

  r = sync_line(adsl_handle, tmodem, MAX_WAIT_LINE_UP);
    return -1;
  }
  if ( ! command_options.no_claim_interface_2 && usb_claim_interface(adsl_handle, 2) < 0)
  usb_release_interface(adsl_handle, 0);
  usb_release_interface(adsl_handle, 1);
  usb_release_interface(adsl_handle, 2);
  PDEBUG(gettext("Interface = %d
"), adsl_handle->interface);

  init_modem(tmodem, adsl_dev, open_mode); 

/* Check if the modem is working yet */
    if (firmware)
  if ( command_options.firmware || command_options.config ) {
  if ( ! command_options.no_check_modem_before )
    if (config)
    r = usb_bulk_read(adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
  else
    r=-1;
  if (r < 0) {
    if (command_options.firmware) {
    if (command_options.firmware)
  if (sync)
    r = sync_line(adsl_handle, tmodem, max_wait_line_up);
	if (r < 0)
    if (command_options.config)
        r = first_config(adsl_handle, tmodem); }
  if ( ! no_claim_interface_0 )
	if (r < 0)
  }
  if ( ! no_claim_interface_1 )
  
  if (command_options.unsync_first) {
  if ( ! no_claim_interface_2 )
  if (command_options.unsync_first)
    r = send_line_down_signal (adsl_handle, tmodem);
    if ( r < 0 )
  if ( r < 0 )
    return r;

  if (command_options.sync)

  if ( r < 0 )
    return r;

  PDEBUG(gettext("Releasing interface...
"));
  if ( ! command_options.no_claim_interface_0 )
     usb_release_interface(adsl_handle, 0);
  if ( ! command_options.no_claim_interface_1 )
    usb_release_interface(adsl_handle, 1);
  if ( ! command_options.no_claim_interface_2 )
    usb_release_interface(adsl_handle, 2);
  PDEBUG(gettext("Releasing device...
"));
  usb_close(adsl_handle);

  return r;
   
}

