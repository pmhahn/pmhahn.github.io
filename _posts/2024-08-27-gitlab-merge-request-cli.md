---
layout: post
title: "GitLab merge requests from CLI"
date: 2024-08-27 17:40:00  +0200
categories: git
excerpt_separator: <!--more-->
---

I work with [GitLab](https://about.gitlab.com/), where I often have to create [Merge Requests](https://docs.gitlab.com/ee/user/project/merge_requests/).
You can do this using the browser based graphical user interface, or from the command line when doing a `git push`:
You can specify several [push options for merge requests](https://docs.gitlab.com/ee/topics/git/commit.html#push-options-for-merge-requests) via the argument `-o`:

- `merge_request.create`: Create a new merge request for the pushed branch.
- `merge_request.target=<branch_name>`: Set the target of the merge request to a particular branch.
- `merge_request.merge_when_pipeline_succeeds`: Set the merge request to merge when its pipeline succeeds.
- `merge_request.remove_source_branch`: Set the merge request to remove the source branch when it’s merged.

There are many more.

<!--more-->

I have a `git alias` to push my branch and create a merge request at the same time:

```ini
[alias]
	mwp = "!f() { git push
	-o merge_request.create
	-o merge_request.remove_source_branch
	-o merge_request.target=\"$(git rev-parse --abbrev-ref '@{u}' | cut -d/ -f2)\";};f"
```

With that my workflow looks like this:

```console
$ git fetch origin
$ git checkout -b feature/foo --track=inherit main
$ # hack away
$ git add …
$ git commit …
$ git mwp origin
```

The last command will push my local branch `feature/foo` to the remote named `origin` as a new branch with the same name .
It will also create a merge request (using the first commit as description), which will target `main`.

{% include abbreviations.md %}
