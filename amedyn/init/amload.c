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

  16/07/2006 Sktt (Aurelio)
  Add command-line options.
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

extern struct options command_options;

extern int check_modem(unsigned int vid, unsigned int pid);
extern void init_modem(unsigned int tmodem, struct usb_device *adsl_dev, int open_mode);
extern int first_config(usb_dev_handle *adsl_handle, int tmodem);
extern int sync_line(usb_dev_handle *adsl_handle, int tmodem, int max_wait_line_up);
extern int load_firmware(usb_dev_handle *adsl_handle, int tmodem);
extern int send_line_down_signal (usb_dev_handle * adsl_handle, int tmodem);

int main(int argc, char *argv[])
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
  char buf[0x10];

  /* Command-line options */ 
  command_options.no_check_modem_before=0;
  command_options.firmware=0;
  command_options.config=0;
  command_options.sync=0;
  command_options.max_wait_line_up=MAX_WAIT_LINE_UP;
  command_options.verbose=0;
  command_options.linetype=0x15;
  command_options.unsync_first=0;

  /* Debug command-line optios */
  command_options.no_claim_interface_0=0;
  command_options.no_claim_interface_1=0;
  command_options.no_claim_interface_2=0;

  struct poptOption optionsTable[] = {
        { "nocheck", '\0', POPT_ARG_NONE, &command_options.no_check_modem_before, 0,
            "Don't check modem before try to upload firmware.", ""},
        { "firmware", 'f', POPT_ARG_NONE, &command_options.firmware, 0,
            "Upload firmware.", ""},
        { "config",   'c', POPT_ARG_NONE, &command_options.config, 0,
            "Initial config.",  ""},
        { "sync",     's', POPT_ARG_NONE, &command_options.sync, 0,
            "Sync line.",  ""},
        { "time",     't', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &command_options.max_wait_line_up, 0,
            "Time to wait until sync.",  ""},
        { "verbose",  'v', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &command_options.verbose, 0,
            "Verbose level.",  "[0..3]"},
        { "linetype",  '\0', POPT_ARG_INT, &command_options.linetype, 0,
            "Set phone line type code. (default: 0x15)",  "0x11 | 0x15"},
        { "unsync_first",  '\0', POPT_ARG_NONE, &command_options.unsync_first, 0,
            "Send line down signal before sync line.",  ""},
        { NULL,     '0',
	    POPT_ARG_NONE, &command_options.no_claim_interface_0, 0,
            "Don't claim interface 0. (Debug option)",  ""},
        { NULL,     '1',
	    POPT_ARG_NONE, &command_options.no_claim_interface_1, 0,
            "Don't claim interface 1. (Debug option)",  ""},
        { NULL,     '2',
	    POPT_ARG_NONE, &command_options.no_claim_interface_2, 0,
            "Don't claim interface 2. (Debug option)",  ""},
        POPT_AUTOHELP
        { NULL, 0, 0, NULL, 0 }
    };

  optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);

  /* init locale */
  setlocale(LC_ALL, "");
  //if (file_exists("./locale")) 
  //  bindtextdomain(TF_CODE, "./locale");  /* set directory for a domain (source code messages) */
  //else 
    bindtextdomain(TF_CODE, "/usr/share/locale");  /* set directory for a domain (source code messages) */
  textdomain(TF_CODE);  /* set domain */

  /* show program information */
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
  {
    poptPrintUsage(optCon, stderr, 0);
	poptFreeContext(optCon);
    return -1;
  }

  r = poptGetNextOpt(optCon);
  if ( r < -1 ) {
    fprintf(stderr, "%s: %s
",
	poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
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
   printf(" bmAttributes: 0x%02x
", adsl_dev->config->bmAttributes);
   printf(" MaxPower: 0x%02x
", adsl_dev->config->MaxPower);
#endif

  /* connect to ADSL modem */
  adsl_handle = usb_open(adsl_dev);
  if (adsl_handle == NULL)
  {
    printf(gettext("Error: Couldn't get device handle for ADSL modem
"));
    return -1;
  }
  /* set configuration */
  if (usb_set_configuration(adsl_handle, 1) < 0)
  {
    printf("Error: usb_set_configuration: %s
", usb_strerror());
    return -1;
  }

  /* check if other program is using interfaces 0, 1, 2 */
  if ( ! command_options.no_claim_interface_0 && usb_claim_interface(adsl_handle, 0) < 0)
  {
    printf("Error: usb_claim_interface 0: %s
", usb_strerror());
    return -1;
  }
  if ( ! command_options.no_claim_interface_1 && usb_claim_interface(adsl_handle, 1) < 0)
  {
    printf("Error: usb_claim_interface 1: %s
", usb_strerror());
    return -1;
  }
  if ( ! command_options.no_claim_interface_2 && usb_claim_interface(adsl_handle, 2) < 0)
  {
    printf("Error: usb_claim_interface 2: %s
", usb_strerror());
    return -1;
  }
  PDEBUG(gettext("Interface = %d
"), adsl_handle->interface);

  init_modem(tmodem, adsl_dev, open_mode); 

/* Check if the modem is working yet */
  if ( command_options.firmware || command_options.config ) {
  if ( ! command_options.no_check_modem_before )
    r = usb_bulk_read(adsl_handle, USB_IN_INFO, buf, 0x10, DATA_TIMEOUT);
  else
    r=-1;
  if (r < 0) {
    if (command_options.firmware) {
    if (command_options.firmware)
        r = load_firmware(adsl_handle, tmodem); 
	if (r < 0)
    if (command_options.config)
        r = first_config(adsl_handle, tmodem); }
	if (r < 0)
  else
    printf(gettext("Firmware loaded yet!
"));
  }
  
  if (command_options.unsync_first) {
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

