---
layout: post
title: "Sign-off multiple previous GIT commits"
date: 2020-07-22 13:45:00  +0200
categories: git
---

Working for `libvirt` I had to add the [Developer Certificate of Origin](http://developercertificate.org/) to several previous commits, where I forgot to directly use `git commit --signoff`.
[StackOverflow](https://stackoverflow.com/questions/13043357/git-sign-off-previous-commits) has that question, but it started mising `-s` for `--signoff` with `-S` for `--gpg-sign`.

At the end I used the following so sign the last 89 commits:

```bash
git config trailer.sign.key 'Signed-off-by'
SIGNOFF="sign: $(git config --get user.name) <$(git config --get user.email)>"
git filter-branch \
  --force \
  --msg-filter "git interpret-trailers --trailer '$SIGNOFF'" \
  @~89..@
```

See [GIT filter-branch]({% post_url 2019-02-28-git-filter-branch %}) for other use cases of `git filter-branch`.

Only to find out that is as simple as

```bash
git rebase --signoff @~89
```
