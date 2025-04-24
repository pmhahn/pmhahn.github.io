---
title: 'GitLab 104: Merge Request Pipelines for UCS'
date: '2022-08-04T17:39:17+02:00'
layout: post
categories: git
---

GitLab [pipeline](https://docs.gitlab.com/ee/ci/pipelines/) can run for many reasons, which are indicated via the [CI/CD variable](https://docs.gitlab.com/ee/ci/variables/predefined_variables.html) [`CI_PIPELINE_SOURCE`](https://docs.gitlab.com/ee/ci/jobs/job_control.html#common-if-clauses-for-rules):

- `push`
  when new branch is pushed or an existing branch is updated.
  These are called *branch pipelines*.
- `merge_request_event`
  when a merge-request ist created or updated.
  These are called [merge-request pipelines](https://docs.gitlab.com/ee/ci/pipelines/merge_request_pipelines.html).
- `schedule`
  when triggered via the cron like schedule.
  These are called [scheduled pipelines](https://docs.gitlab.com/ee/ci/pipelines/schedules.html).
- `web`
  via the Web user interface.

Previously UCS was using *Branch pipelines*, but [switched to merge-request pipelines](https://github.com/univention/univention-corporate-server/commit/4a51c61c6af9d641b8bb72b2a70a424192fb8bcd) this week and [improved](https://github.com/univention/univention-corporate-server/commit/e6ea0f59b6e6c336cf6ad049a688e7e3ae13528e) later on.
This was done to get automatic UCS package builds per branch, which is part of our ongoing work to get Jenkins branch test jobs.

## Branch pipelines

They get created when a branch is initially pushed to GitLab but also when an already existing branch is updated.
This has some drawback:

On the initial push the branch has no ancestor branch and just carries all commits, including all those commits which were already there when the branch was first forked.
To GitLab this looks like all files just appeared from void and so all paths are considered changed.
This renders the use of [rules:changes:](https://docs.gitlab.com/ee/ci/jobs/job_control.html#variables-in-ruleschanges) void, which limits the number of packages to be built to those touched:
this still builds all packages on the initial push as all paths are new.
Sometimes building all packages is desired, for example when a low-level package gets changed and you want to make sure, that all packages using it still can be built.
But most often this is not wanted, as it is slow and most branches just change a single package or just a few and not all 160 ones.

If one of these packages contained an error — you might have inherited that error already from the branch you forked from — the build stopped there.
All depending packages were never built unless you later on did some dummy change to trigger them via their `rules:changes:` filter.

The initial failure could be easily hidden by just pushing another change to the same branch:
As that follow-up commit mostly changed a single package, only that singe package was rebuilt in the next pipeline and all other errors from the previous pipelines were hidden.
This often led to a broken *main* branch:
while the last pipeline succeeded as it only built a sub-set of packages, previous pipelines were no longer investigated and errors detected by them ignored.

## Merge-request pipelines

They get created as soon as one *Merge Request* (MR) is opened for a git branch.
In addition to the branch name a MR also includes the *target branch*, into which the changes are supposed to be merged into.
With that additional information GitLab can now better detect the delta between your source feature branch and the targeted main branch:
Only the actually changed files are known which simplifies the task of detecting, which packages need to be re-build.
Only those are initially built and re-build on every new push.

For most changes touching only a few packages this drastically improves the built time as only those packages have to be built.
But it might be slower if you touch many packages in the same MR as each commit will rebuilt all packages again, even when your last commit only touched a single one.

But that way you know that all packages touched by your MR do still build and will continue to do so when the MR is merged into main.

## Merge Requests

Creating a MR might look like an extra step, but is quiet easy:
When you push a new branch on the command line `git push` will already show you the URL to create a New merge request with your feature branch name already filled in.
Just open it an your Web browser and fill in the other fields.
Most importantly select the right *Target branch*.
You can still update it later on when you click on *Edit* in the top-right corner or use the `/target_branch` [quick action](https://docs.gitlab.com/ee/user/project/quick_actions.html).

You can also create a MR via the command line by giving [extra options to `git push`](https://docs.gitlab.com/ee/user/project/push_options.html#push-options-for-merge-requests):
```bash
git push \
 -o merge_request.create \
 -o merge_request.target=… \
 -o merge_request.remove_source_branch \
 -o merge_request.merge_when_pipeline_succeeds \
 …
```
GitLab is clever enough to check for an already existing MR first and will only create a new MR when no previous MR is found.

In my personal `~/.config/git/config` I carry two [git aliases](https://git-scm.com/book/en/v2/Git-Basics-Git-Aliases) to simplify my life:

```ini
[alias]
    mwp = "!f() { git push -o merge_request.create -o merge_request.target=\"$(git rev-parse --abbrev-ref \"@{u}\"|cut -d/ -f2-)\" -o merge_request.remove_source_branch \"$@\";};f"
    mwps = "!f() { git push -o merge_request.create -o merge_request.target=\"$(git rev-parse --abbrev-ref \"@{u}\"|cut -d/ -f2-)\" -o merge_request.remove_source_branch -o merge_request.merge_when_pipeline_succeeds \"$@\";};f"
```

*Merge With Pull-request* (mwp) will simply do a push and create a MR.
The branch is marked for automatic deletion when the MR is merged.

*Merge With Pull-request Succeed* (mwps) does the same but will do the automatic merge as soon as the pipeline succeeds.
`repo-ng` has a very good test suite, so I'm very confident that it will detect all errors.
If my change passes the test suite it will automatically get merged into main and deployed to our servers.

## Closing remarks

*Branch pipelines* are the traditional pipelines and *Merge-request pipelines* only got added later on.
There is one annoying problem:
As soon as you enable *Merge Requests* for a project you will get into the [duplicate pipeline problem](https://docs.gitlab.com/ee/ci/jobs/job_control.html#avoid-duplicate-pipelines):
Your single `.gitlab-ci.yml` file gets used for both types of pipelines, leading to all jobs run twice.
You can prevent with by adding [workflow:rules:](https://docs.gitlab.com/ee/ci/yaml/#workflow) to prevent one kind or the other.

For UCS both pipeline types are used at the same time:
For your feature branch the *Merge request pipeline* gets used (as soon as you create your MR), while for our main branches `4.4-9` resp.
`5.0-2` a *Branch pipeline* will get scheduled.
The later are detected by their *protected* status:
All branched named `?.?-?` are protected and cannot be removed.

If you wonder where your packages get stored:
Currently this still is using [Aptly](https://www.aptly.info/) at omar.knut.univention.de/build2 as the [GitLab Debian package Registry](https://docs.gitlab.com/ee/user/packages/debian_repository/) still has too many issues.
You can include the repository associated with your branch by using the following line in `/etc/apt/sources.list`:

```
deb [trusted=yes] http://omar.knut.univention.de/build2/git/$CI_COMMIT_REF_SLUG git main
```

`$CI_COMMIT_REF_SLUG` gets calculated from the name of your git branch by [slugify](https://gitlab.com/gitlab-org/gitlab/-/blob/master/lib/gitlab/utils.rb#L92):
```ruby
# A slugified version of the string, suitable for inclusion in URLs and
# domain names. Rules:
#
# * Lowercased
# * Anything not matching [a-z0-9-] is replaced with a –
# * Maximum length is 63 bytes
# * First/Last Character is not a hyphen
def slugify(str)
 str.downcase
 .gsub(/[^a-z0-9]/, '-')[0..62]
 .gsub(/(\A-+|-+\z)/, '')
end
```

{% include abbreviations.md %}
