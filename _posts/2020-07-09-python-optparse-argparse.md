---
layout: post
title: 'Python: optparse vs. argparse'
date: '2020-07-09T13:59:43+02:00'
categories: python
---

Be careful when you switch from [optparse.OptionParser](https://docs.python.org/2.7/library/optparse.html) to [argparse.ArgumentParser](https://docs.python.org/2.7/library/argparse.html#module-argparse) as they use different defaults for `action="store_true"` and `action="store_false"`: The former initializes the value to `None` while the later one to the inverse of the desired action, that is to `False` if `action="store_true"` is used and vis-versa.

Even more alternatives for parsing command line arguments:

- [getopt](https://docs.python.org/2.7/library/getopt.html)
- [click](https://click.palletsprojects.com/)
- [docopt](http://docopt.org/)

PS: Our former college "Janek" would call `argparse` an "atrocity" anyway and you should stay with `optparse`.

{% include abbreviations.md %}
