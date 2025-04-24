---
layout: post
title: "Python XML parsing"
date: 2020-12-19 12:50:00  +0200
categories: python documentation
excerpt_separator: <!--more-->
---

At work we're using [DocBook](https://tdg.docbook.org/) for our product documentation.
We have a tool for spell-checking our documents.

1. It uses [Document Type Definition (DTD)](https://wiki.selfhtml.org/wiki/XML/DTD) for validation and entity declarations.
2. The DTD and other referenced files should be cached locally by using an [XML catalog](http://www.sagehill.net/docbookxsl/WriteCatalog.html).
2. For misspelled words the tool should prints the *line* and *column* number.

Solving this with Python is not easy for obscure reasons.

<!--more-->

1. Is you use [xml.sax](https://docs.python.org/3/library/xml.sax.html) you get *line* and *column* numbers from the `Locator`.
   But the default parser [expat](https://docs.python.org/3/library/pyexpat.html) has no support for *XML Catalog*.

2. [libxml2](http://xmlsoft.org/catalog.html) has support for *XML catalog*, but is not installable from [PyPI](https://pypi.org/project/libxml2-python3/):
   It requires `libxml2-dev` to be installed.
   At least you can use it to setup the catalog and then use it with a custom resolver like show below.

3. [lxml](https://lxml.de/) is based on `libxml2` itself, but only provides an interface to get the *line* via [sourceline](https://lxml.de/apidoc/lxml.etree.html#lxml.etree._Element.sourceline).
   The column number is not exposed.

If you find a solution, please tell me.

My old solution was to use `libxml2` with the above mentioned PyPI drawback like this:

```python
from os import environ
from os.path import dirname, join
from xml.sax import handler, make_parser
import libxml2  # type: ignore

class DocBookResolver(handler.EntityResolver):
   CATALOG = join(dirname(__file__), "catalog.xml")

    def __init__(self) -> None:
        catalog = environ.setdefault("XML_CATALOG_FILES", self.CATALOG)
        libxml2.loadCatalog(catalog)

    def resolveEntity(self, publicId: str, systemId: str) -> str:
        return libxml2.catalogResolve(publicId, systemId) or systemId


parser = make_parser()
resolver = DocBookResolver()
parser.setEntityResolver(resolver)
```

{% include abbreviations.md %}
