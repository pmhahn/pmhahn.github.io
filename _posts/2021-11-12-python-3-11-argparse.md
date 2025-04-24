---
title: 'Python 3.11: argparse'
date: '2021-11-12T17:00:12+01:00'
layout: post
categories: python
---

[argparse](https://docs.python.org/3/library/argparse.html) has superseded [optparse](https://docs.python.org/3/library/optparse.html), which is deprecated since Python 3.2 and might get removed in the future. Many of our scripts have already been [migrated](https://docs.python.org/3/library/argparse.html#upgrading-optparse-code), but `argparse` and the migration has several pitfalls. This was already mentioned in one of my previous posts [Python: optparse vs. argparse]({% post_url 2020-07-09-python-optparse-argparse %}).

Alternatives:

- [sys.argv](https://docs.python.org/3/library/sys.html#sys.argv) if you want do parse it yourself. Please do not do this as your colleges will have to maintain this as well.
- [getop](https://docs.python.org/3/library/getopt.html) if you come from C and still have not switched to the Pythonic way.
- [click](https://click.palletsprojects.com/) for a Python decorator based approach.
- [Typer](https://typer.tiangolo.com/) builds on top of <q>click</q>, but also uses Python type hints.

## Usage

`argparse.ArgumentParser(usage=…)` should be removed in almost all cases as the usage description is built automatically by `ArgumentParser`. In most cases it is detailed enough.

## Help

`argparse` uses %-formatting by default; make sure to replace `%default` by `%(defaults)s` for `help` and `%prog` with `%(prog)s` in `usage`.

## Type

`argparse.ArgumentParser.add_argument(type=…)` can be used to add some trivial type and value checking. For example to accept only even numbers the following can be used:

```python
from argparse import ArgumentParser

def even(text: str) -> int:
    val = int(text)
    if val % 2:
        raise ValueError()
    return val

parser = ArgumentParser()
parser.add_argument("val", type=even)
args = parser.parse_args("2")
```

## Default

`argparse.ArgumentParser.add_argument(default=…)` can be removed in may cases as actions provide their specific defaults, e.g. `action="store_true"` implies `default=False` and vis-versa.

This becomes a problem when you have multiple options, which store their values in the same variable.

```python
from argparse import ArgumentParser
parser = ArgumentParser()
parser.add_argument("--foo", action="store_true", dest="var")
parser.add_argument("--bar", action="store_false", dest="var")
args = parser.parse_args()
```

The trick is to also add `default=argparse.SUPPRESS` to both calls, which leaves `args.val` undefined when neither option is given. This can be extended with `parser.set_defaults(var=None)` to at least define the variable and to prevent an `AttributeError`.

This has proven to be useful for handling the optional `-c` for `ucs-kvm-create`: Both `ucs-kvm-create CONF.cfg` and `ucs-kvm-create -c CONF.cfg` can be used to specify the configuration file. Here’s the code:

```python
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument(
    "-c", "--conf",
    default=SUPPRESS,
    help="Config file",
)
group.add_argument(
    "conf",
    nargs="?",
    default=SUPPRESS,
    help="Config file",
)
```

- The `nargs="?"` looks strange, but this is required as `add_mutually_exclusive_groups()` only allows **optional** arguments. Due to the `required=True` **one** of those arguments is required.
- This trick cannot be combined with `argparse.FileType()` as that will try to open the file names `==SUPPRESS==`, which does not exist.

## Sub-commands

[`argparse.ArgumentParser.add_subparsers()`](https://docs.python.org/3/library/argparse.html#sub-commands) can be used to group several commands into one super-command, which is probably best known from `svn` or `git`.
This becomes tricky as soon as you have *global* options and options per *sub-command*, as they cannot be intermixed: The later must come after the sub-command name! This for example prevents the migration of `ucr` from `getopt` to `argparse` as the former allows this intermixing.

Be aware that Python 3.7 introduced a backward incompatible change in behavior: Previously when `argparse.ArgumentParser.add_subparsers()` was used and no sub-command was specified, the parser terminated the process with showing the `--help` output. Python 3.7 introduced the option `required`, which defaults to `False`. To get the old behavior you have to explicitly specify `required=True`.
