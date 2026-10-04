# amedyn / amedyn2

Linux drivers for USB ADSL modems built on the Alcatel chipset: the **ZyXEL
Prestige 630-11**, the **Asus AAM6000UG**, the Digicom MichelAngelo USB A, the
Topcom Webracer 851 and the PROLiNK Hurricane 8000.

Two modules live side by side:

| Directory   | Purpose |
|-------------|---------|
| `amedyn/`   | the historical `xdslusb` module for Linux **2.4 and 2.6**, together with the userspace tools (`amload`, `amstart`, PPPoE/PPPoA connection scripts) |
| `amedyn2/`  | a rewrite (`amedyn2.c`) built on top of the generic `usbatm` driver, for recent 2.6 kernels |

## History

This repository is the result of converting the old SourceForge **CVS**
repository (`zyxel630-11`), completed with the four releases of the earlier
`aam6000ug` project that preceded it:

| Period       | Origin |
|-------------|--------|
| 2003-10 to 2004-08 | SourceForge project `aam6000ug` (`-src.tgz` tarballs) |
| 2004-12 to 2007-08 | CVS repository `zyxel630-11` (the `amedyn` and `amedyn2` modules) |

The CVS history was rebuilt from the `,v` files: the dates, authors and commit
messages are the original ones. The first day of each module is folded into a
single import commit, because CVS timestamps the individual file revisions of
one import a few seconds apart.

The `amedyn-<date>` and `amedyn2-<date>` tags correspond to the releases that
were actually published on SourceForge. The original CVS tags were deleted from
the repository afterwards; their names were recovered from `CVSROOT/history`
and are recorded in the tag annotation wherever the mapping could be
established.

### Excluded files

Kernel build leftovers (`module/.tmp_versions/*.mod`) and the proprietary
firmware blobs (`*.bin`) are not versioned here — see
[`amedyn/firmware/README.md`](amedyn/firmware/README.md). The `CVSROOT`
directory, which held a password hash, was dropped as well.

## License

GPL v2 — see [`amedyn/COPYING`](amedyn/COPYING) and its translation
[`amedyn/COPIANDO`](amedyn/COPIANDO).

## Authors

* **Josep Comas** (`jcomas`) — original author of the driver
* **sktt** (Aurelio) — main maintainer
* **Emmanuel Counasse** (`manuc66`) — maintainer, translations

## Installing

This code dates from 2003-2007 and targets the 2.4 and 2.6 kernels. It will
not build on a modern kernel without substantial rework. For current-day use,
prefer an in-kernel `usbatm` driver.