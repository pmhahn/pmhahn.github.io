---
title: 'shell `trap` and proper quoting'
date: '2025-06-28T07:45:00+02:00'
layout: post
categories: shell
excerpt_separator: <!--more-->
---

What's wrong with
```sh
#!/bin/bash
TMPDIR=$(mktemp -d)
trap 'rm -r $TMPDIR' EXIT
...
```

<!--more-->

Let's ask [shellcheck](https://www.shellcheck.net/):
> No issues detected!

Actually there are multiple issues:

## TMPDIR

Please do not assign to `TMPDIR` as that variable in an *input parameter* to `mktemp` itself:
When you read [man:mktemp](man:mktemp(1)) your will find this for option `-p`:
> if DIR is not specified, use $TMPDIR if set, else /tmp.

The variable is used for example by [pam_tmpdir](man:pam_tmpdir.8) to setup _per user temporary directories_ to improve security on multi-user systems.
By using `TMPDIR` inside your script to store the path of your **specific** temporary directory, you risk chanhing the behavior of other called child-processes also using `mktemp`.
Other [equivalent implementations thereof](https://docs.python.org/3/library/tempfile.html#tempfile.mkstemp)) also use `TEMP` and `TMP`, so better do not use these as well.

So lets use `tmp`:
```sh
tmp=$(mktemp -d)
trap 'rm -r $tmp' EXIT
```

## IFS

By default [man:mktemp](man:mktemp(1)) will only create _safe_ file names, e.g. none containing blanks and characters of `$IFS`.
Remember that `$IFS` is used by the shell to split every argument — which is not quoted — into multiple arguments.
By default it is set to _space_, _tab_ and _newline_.
But you can redefine or extend it, after which _hell breaks loose_:
```console
$ bash -c 'IFS="$IFS/."; . my-trap-script"
rm: cannot remove '': No such file or directory
rm: cannot remove 'tmp': No such file or directory
rm: cannot remove 'user': No such file or directory
rm: cannot remove '1000': No such file or directory
rm: cannot remove 'tmp': No such file or directory
rm: cannot remove 'XyJlR6AHpn': No such file or directory
```

Luckily **`$IFS` is re-set for each shell** to its default value, but do keep that in mind when you fiddle with `$IFS`.
My advise is to do that only in functions and to use `local IFS` there to have the change confined to only inside the function.

## quoting

To prevent `$IFS`-splitting you have to quote arguments.
So let's try with this:
```sh
tmp=$(mktemp -d)
trap 'rm -r "$tmp"' EXIT
```

You may wonder, why I didn't quote `tmp=$(…)` as the spitting occurs after _command substitution_?
For that you have to read [man:bash](man:bash(1)) very carefully.
In section _parameters_ you have this:
> All values undergo tilde expansion, parameter and variable expansion, command substitution, arithmetic expansion, and quote removal.

Compare that to section _expansion_:
> There are seven kinds of expansion performed: brace expansion, tilde expansion, parameter and variable expansion, command substitution, arithmetic expansion, **word splitting**, and pathname expansion.

The important difference here is, that parameter assignment expects a _single argument_ and this _word splitting_ does **not** occurs there.
So no quoting is needed for parameter assignments, but you can do it for consistency — it does not hurt.

## late vs. early evaluation

While `shellcheck` is happy, there is a lingering problem:
The `trap` is executed only later on when the shell exits.
`$tmp` might get changed (by accident) or be used for something else.
In that case the `rm` will delete whatever file `$tmp` points too.

That is because the _outer quotes_ are _single quotes_ while the _inner quotes_ are _double quotes_:
_single quotes_ prevent evaluation of the command when `the trap` statement is executed.
Later on when the trap is executed, the command is evaluated a second time.
That is when the _double quotes_ prevent `$tmp` from being split on `$IFS`.

So lets look at the following variant:
```sh
tmp=$(mktemp -d)
trap "rm -r $tmp" EXIT
tmp+="/subdir"
```

_Double quotes_ are now use when the trap is setup:
`$tmp` gets inserted here as it is currently defined.
If `$tmp` is changed later on, we still delete file temporary file we just created.

But `shellcheck` is unhappy now:
```
trap "rm -r $tmp" EXIT
            ^-- SC2064 (warning): Use single quotes, otherwise this expands now rather than when signalled.
```
Personally I think [SC2064](https://www.shellcheck.net/wiki/SC2064) is a bad advise here as we want to evaluate "$tmp" now and not later.
I want to delete the file `$tmp` is pointing to right now, not where it might point to in the future.
I'm not alone with that opinion and [issue 1945](https://github.com/koalaman/shellcheck/issues/1945) calls SC2064 questionable.

But there is a bigger problem again:
But what will happen, when the trap fires?

## late quoting

Remember that `$tmp` might contain `$IFS` characters!
For example I can set `TMPDIR=/tmp/I like blanks`.
The trap command will be `rm -f /tmp/I like blanks`.
It will fail as there is no file `/tmp/I`, `./like` and `./blanks` — hopefully.

So how do we fix that?
I give you two variants:
1. Nestes double quoting using backslash-escaping:
   ```sh
   tmp=$(mktemp -d)
   trap "rm -r \"$tmp\"" EXIT
   ```

2. Single quotes inside double quotes:
   ```sh
   tmp=$(mktemp -d)
   trap "rm -r '$tmp'" EXIT
   ```

Which one is correct?

The answer is very disappointing:
None!

Variant 1 will fail for `TMPDIR=/tmp/\"` and variante will fail for `TMPDIR=/tmp/\'`.
`$tmp` will then be a path containing a _double quote_ in variant 1 and a _single quote_ in variant 2.
Because of the _early evaluation_ `$tmp` is inserted as-is during the first evaluation when `trap` is setup.
On the 2nd evaluation when the trap is executed, you will have an odd number of quotes!

## correct quoting

So we need a mechanism to quote `$tmp` correctly, so it survives two rounds of evaluation.

Luckily `bash` has such a feature:
```sh
tmp=$(mktemp -d)
# shellcheck disable=SC2064
trap "rm -r ${tmp@Q}" EXIT
```

`@Q` is a _operator_, which is documented like this in [man:bash](man:bash(1)):
> The expansion is a string that is the value of parameter quoted in a format that can be reused as input.

That is exactly what we want:
- the outer quotes prevent `$tmp` from being split when the `trap` is setup.
- the `@Q` adds the necessary escaping to also prevent `$tmp` from being split when the trap executes.

## closing words

Be warned that the operator `@Q` is a `bash`ism:
This is not supported by `ash`, `dash`, or `busybox sh`:
There you have to quote `"` and `'` manually.
I leave that to you.

I will simply accept `bash` and use `@Q` as that is much more readable and — most importantly — correct.
