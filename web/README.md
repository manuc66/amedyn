# Personal web page (archived)

This directory preserves the project web page that Emmanuel Counasse
(`manuc66`) hosted on his University of Liège student site:

```
http://www.student.montefiore.ulg.ac.be/~counasse/modem/
```

## Provenance

The original server is long gone. These files were recovered from the
[Wayback Machine](https://web.archive.org/web/20080225101815/http://www.student.montefiore.ulg.ac.be/~counasse/modem/),
captured on **25 February 2008** — several months after the last CVS commit
(August 2007), which makes it an independent witness of the final state of the
documentation.

An earlier capture from **7 December 2004** also exists, predating the CVS
import entirely.

## What is here

| File | Origin |
|------|--------|
| `index.html`          | Wayback capture, 2008-02-25 |
| `en_install.html`     | Wayback capture, 2008-02-25 |
| `fr_install.html`     | Wayback capture, 2008-02-25 |
| `nl_install.html`     | Wayback capture, 2008-02-25 |
| `img/*`               | copied from [`amedyn/doc/img/`](../amedyn/doc/img) |

Notes:

* `es_install.html` is linked from `index.html` but was never archived
  (HTTP 404), and the Italian instructions were listed without a link. The
  Spanish page does exist in the repository proper:
  [`amedyn/doc/es_install.html`](../amedyn/doc/es_install.html).
* The images were not archived either. The personal page used the very same
  files as `amedyn/doc/img/`, so those were copied here to make the page
  render; this directory is therefore a mix of capture and reconstruction.

## Why this matters

`web/index.html` is **byte-for-byte identical** to the reconstructed
[`amedyn/doc/index.html`](../amedyn/doc/index.html). That is the strongest
available check on the CVS reconstruction: an independently hosted copy,
written years after the final commit.

The install pages differ from their `amedyn/doc/` counterparts by two lines
(`popt` and `popt-dev`). Those prerequisites were added to CVS in October 2006,
after this simplified copy of the page had been uploaded.