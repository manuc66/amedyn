/*
  Zyxel 630-11 & Asus AAM6000UG ioctl call
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
  Creation   : 14/7/2003

  Description: This program does ioctl calls to Zyxel 630-11 & Asus AAM6000UG driver.

  Log:

  14/7/2003 Josep Comas
  Initial release

  21/7/2003 Josep Comas
  Added support for Asus AAM6000UG

  24/9/2003 Eduardo Espejo
  Changed u_ioctl.ifno = 0 by u_ioctl.ifno = 1

  27/10/2003 Josep Comas
  Credits update
*/


#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <usbi.h>
#include <libintl.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "amedyn.h"


/* translation files */
#define TF_CODE "amioctl"

/* struct to call to kernel driver (module) */
struct usbdevfs_ioctl {
  int ifno;
  int ioctl_code;
  void *data;
};

/* USBDEVFS IOCTL to call to kernel driver (module) */
#define USBDEVFS_IOCTL _IOWR('U', 18, struct usbdevfs_ioctl)

/* IOCTL codes when call to kernel driver (module) */
#define IOCTL_START                1
#define IOCTL_STOP                 2

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

  /* Vendor = Mediacom Europe?, Product = DynaMiTe USB Modem */
  return -1;
}

int main(int argc, char *argv[])
{
  /* struct to call to kernel driver (module) */
  struct usbdevfs_ioctl u_ioctl;

  /* bus structures variables */
  struct usb_bus * bus;
  struct usb_device * dev;
  struct usb_device * adsl_dev = NULL;
  usb_dev_handle * adsl_handle;

  /* boolean value */
  int goon;

  /* result code */
  int r;


  /* init locale */
  setlocale(LC_ALL, "");
  //if (file_exists("./locale"))
  //  bindtextdomain(TF_CODE, "./locale");  /* set directory for a domain (source code messages) */
  //else
    bindtextdomain(TF_CODE, "/usr/share/locale");  /* set directory for a domain (source code messages) */
  textdomain(TF_CODE);  /* set domain */

  /* show program information */
  printf(gettext("Zxyel 630-11 & Asus AAM6000UG ioctl call."));
  printf(" 24/9/2003
");
  printf("Josep Comas <jcomas@gna.es>
");
  printf("Sundar <sundar@cynaptix.biz>
");
  printf("Eduardo Espejo <eespejo@users.sourceforge.net>

");

  /*
  * Security stuff
  * 1 - be sure to be root
  * 2 - umask to prevent critical data being read from log file
  */
  if(geteuid() != 0) {
    fprintf(stderr, "WARNING: amioctl must be run with root privileges
");
    exit (-1);
  }

  /* check parameters */
  if (argc < 2)
  {
    printf(gettext("Usage: %s ioctl_command
"), argv[0]);
    printf(gettext("
IOCTL commands:
"));
    printf(gettext("  1 (start, enable receive data in driver)
"));
    printf(gettext("  2 (stop, disable receive data in driver)
"));
    printf(gettext("
Examples:
"));
    printf(gettext("   Enable receive data in driver: %s 1
"), argv[0]);
    printf(gettext("   Disable receive data in driver: %s 2
"), argv[0]);
    return -1;
  }
  if (strcmp(argv[1], "1") && strcmp(argv[1], "2"))
  {
    printf(gettext("Error: invalid command: %s
"), argv[1]);
    return -1;
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

  /* search first ADSL modem */
  bus = usb_busses;
  goon = 1;
  while (bus && goon)
  {
    dev = bus->devices;
    while (dev && goon)
    {
      if (check_modem(dev->descriptor.idVendor, dev->descriptor.idProduct) > 0)
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
"),
         adsl_dev->descriptor.idVendor, adsl_dev->descriptor.idProduct);

  /* connect to ADSL modem */
  adsl_handle = usb_open(adsl_dev);
  if (adsl_handle == NULL)
  {
    printf(gettext("Error: Couldn't get device handle for ADSL modem
"));
    return -1;
  if (usb_set_configuration(adsl_handle, 1) < 0)
  {
    printf("Error: usb_set_configuration: %s
", usb_strerror());
    return -1;
  }
  }

  u_ioctl.ifno = 1;
  if (argv[1][0] == '1') { 

    u_ioctl.ioctl_code = IOCTL_START;
    u_ioctl.data  = NULL;

  } else if (argv[1][0] == '2') { 

    u_ioctl.ioctl_code = IOCTL_STOP;
    u_ioctl.data  = NULL;

  }

  r = ioctl(adsl_handle->fd, USBDEVFS_IOCTL, &u_ioctl);
  if (r < 0) {
    printf(gettext("Error in ioctl call, status = %d
"), r);
    usb_close(adsl_handle);
    return -1;
  }
  usb_close(adsl_handle);

  return 0;
}

