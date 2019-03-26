---
layout: post
title: "GIT filter-branch"
date: 2019-02-28 07:59:00  +0100
categories: git
---

`git-filter-branch` can be used to rewrite the history of one or more branches.
As a [Debian Developer](https://www.debian.org/) working for [Univention GmbH](https://www.univention.de/) I often have to work with Debian packages.
Here are some more examples from my daily work.

Removing files
==============
The manual page for [git filter-branch](https://git-scm.com/docs/git-filter-branch) contains several examples already.

The following two commands can be used to remove the file `filename` from all commits in the current branch:
```bash
git filter-branch --tree-filter \
	'rm -f filename' HEAD
git filter-branch --index-filter \
	'git rm --cached --ignore-unmatch filename' HEAD
```

The second command is faster (for large repositories) as it does not need to checkout each revision to the working space.
You can use the `-d` option to specify an alternative working directory like a `tmpfs` file system or some other local file system on a fast SSD.

Change files
============
For removing or reverting changes to files working with the index is fine and faster.
But as soon as you want to apply a change to all commit, it is easier to use `--tree-filter`:
The following example changes all occurrences of `foo` to `bar` in all revisions of the file `filename`:

```bash
git filter-branch -d /tmp --tree-filter \
	`sed -i "s/foo/bar/g" filename' HEAD
```

It is important to apply the same transformation to each commit:
Otherwise the next commit after the one which got changed will accumulate all previous changes!

Dropping changes to files
=========================
Sometimes I have to apply the changes of one feature branch to multiple upstream branches.

This often leads to conflicts with `debian/changelog` as different branches have different versions of this file.
So I often just strip all changes to those files, apply the feature-branch to my target-branch and generate a new entry for `debian/changelog` (suing `gbp dch` or similar).

This can be done using `--index-filter`:
for each revision I tell `git` to reset the file back to the content from the preceding commit.

```bash
git filter-branch --prune-empty --index-filter \
	'git reset $(map $(git rev-parse $GIT_COMMIT~1)) -- "**/debian/changelog"' \
	@{u}..HEAD
```

Some notes on that:

1. `@{u}..HEAD}` is my way to iterate over all commits starting at the branch point up to the current HEAD.
2. You cannot use `HEAD` inside the filter as it always points to your last commit.
   It is **not** updated while the loop is iterating.
   It only gets updated once **after** the rewrite has been done.
3. Instead you can use `$GIT_COMMIT`, which points to the commit currently being processed.
4. `$GIT_COMMIT~1` references the **original** previous commit.
   Using `map $REV` this gets mapped to the **rewritten** previous commit.

With some more thinking this can be simplified to:

```bash
base="$(git merge-base @{u} HEAD)"
git filter-branch --prune-empty --index-filter \
	"git reset '$base' -- '**/debian/changelog'" \
	@{u}..HEAD
```

which basically tells git to reset all `changelog` files back to the revision at the branch point.

Dropping hunks
==============
The previous example is somehow easy, as we drop all changes to a single file.
It gets more complicated if you only want to drop **some** changes (hunks), but keep others.
Then the previous technique fails because `git` works **tree** based and not **diff** based:

    a -> b -> c  -> d  -> HEAD
          \
           -> c' -> d' -> HEAD'

If for example you only do the filtering for `c'`, but not `d'`, then the diff from `c'` to `d'` will include the *dropped* change from `c` to `c'`.

TBC...
