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

  Author     : Emmanuel Counasse <manuc66@sourceforge.net> and Aurelio Arroyo
  Creation   : 03/12/2005

  Description: Sync line.

  Log:

  13/11/2005 Emmanuel Counasse
  Initial version
  
  03/12/2005 Sktt (Aurelio)
  Initial release

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

extern int check_modem(unsigned int vid, unsigned int pid);
extern void init_modem(unsigned int tmodem, struct usb_device *adsl_dev, int open_mode);
extern int first_config(usb_dev_handle *adsl_handle, int tmodem);
extern int resync_line(usb_dev_handle *adsl_handle, int tmodem);
extern int sync_line(usb_dev_handle *adsl_handle, int tmodem, int max_wait_line_up);
extern int load_firmware(usb_dev_handle *adsl_handle, int tmodem);

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
  printf(gettext("Zyxel 630-11 & Asus AAM6000UG sync line program."));
  printf(" 03/12/2005
");
  printf("EmmanuelCounasse <manuc66@sourceforge.net>
");
  printf("Aurelio Arroyo

");

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
  /* check if other program is using interfaces 0 */
  if (usb_claim_interface(adsl_handle, 0) < 0)
  {
    printf("Error: usb_claim_interface 0: %s
", usb_strerror());
    return -1;
  }
  PDEBUG(gettext("Interface = %d
"), adsl_handle->interface);

  r = resync_line(adsl_handle, tmodem);
  r = sync_line(adsl_handle, tmodem, -1);

  PDEBUG(gettext("Releasing interface...
"));
  usb_release_interface(adsl_handle, 0);
  PDEBUG(gettext("Releasing device...
"));
  usb_close(adsl_handle);

  return r;
   
}

