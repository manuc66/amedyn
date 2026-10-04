# Firmware

This directory is intentionally empty.

`fw-usb.bin`, `Fw-usb_A.bin` and `Init-usb.bin` are **not** free software:
they are proprietary firmware blobs shipped by the modem manufacturer. The
project README says as much:

> You need take it form your Windows drivers. If you don't have this file in
> your Windows system look for the correct name at Hardware control panel of
> the modem.

They must therefore not be redistributed, and they never were part of the
upstream CVS repository: they only ever appeared in the tarballs published by
the older SourceForge project `aam6000ug`, and were dropped during the
conversion to git.

## Where to get them

Extract them from the Windows driver installation for your modem, rename them
to the expected names, and install them under `/lib/firmware/`:

| File            | Role                                |
|-----------------|-------------------------------------|
| `Init-usb.bin`  | first-stage firmware                |
| `fw-usb.bin`    | second-stage firmware               |
| `Fw-usb_A.bin`  | second-stage firmware (variant)     |

If your files are named differently, rename them accordingly. Loading is
normally handled by the kernel's hotplug firmware loader, so a reboot after
installing them is enough.
