---
layout: post
title: "Stange GitLab shell eval behaviour"
date: 2025-06-05 13:35:00  +0200
categories: gitlab
excerpt_separator: <!--more-->
---

Two colleges my mine contacted me this week with a strange GitLab runner behaviour.
The following pipeline did not succeed:

```yaml
original:
  script:
    - "sh -c 'exit 42'"
  allow_failure:
    exit_codes:
      - 42
```

<!--more-->

The `sh -c 'exit 42'` launches a sub-process, which exits with return value `42`.
In my case I was launching some linter, which indicated a special condition by returning that code.

GitLab executed commands within a shell, where `set -e` is used.
This terminates the sequence of commands as soon as a command does **not** succeed, e.g. returns `0`.
But instead of getting `42` as the result, GitLab reported `1` as an exit status for the job.
As that code is not listed in `exit_codes`, the jobs failed with an error instead of `okay with warnings`.

## Working alternatives

In contrast to that the following two jobs did work as expected:

```yaml
direct:
  script:
    - "exit 42"
fail:
  script:
    - "sh -c 'exit 42' || exit $?"
```

## Debugging job failures

Using `variables: CI_DEBUG_TRACE: true` did not show anything strange:
GitLab sets up a trap handler called `runner_script_trap` to collect the return code of the failing command and converts it into a JSON output, which is then consumed by GitLab.
There the wrong return value `1` was also visible.

But actually it showed another hint:
GitLab executed the job by [generating a shell script][1] in a temporary file, which then gets executed.
By using `cp "$0" ./debug.txt` combined with `artifacts: paths: [debug.txt]` you can get a hold on it.
Downloading that artifact shows the following (abbreviated) content:

```sh
runner_script_trap() { exit_code=$?; echo JSON… }
trap runner_script_trap EXIT
set -x -e -o pipefail +o noclobber
: | eval $'export=CI… CODE'
```

1. It sets up a trap handler to record the exit status as JSON.
2. It sets up the environment to fail on error.
3. It executes the 'script' code as part of a _shell pipeline_ using `eval`.

## Several experiments

The last part is the relevant thing here and shows very some strange behaviour.
Modifying the command only slightly changes the return value:

```console
$ bash -e -c ':|eval "sh -e -c \"exit 12\""'; echo $?
1
$ bash --posix -e -c ':|eval "sh -e -c \"exit 12\""'; echo $?
1
$ sh -e -c ':|eval "sh -e -c \"exit 12\""'; echo $?
12
$ bash -e -c 'eval "exec sh -e -c \"exit 12\""'; echo $?
12
$ bash -e -c 'eval "sh -e -c \"exit 12\" || exit \$?"'; echo $?
12
$ bash -e -c 'eval "sh -e -c \"exit 12\""'; echo $?
12
$ bash -e -c ':|sh -e -c "exit 12"'; echo $?
12
$ bash -e -c ':|eval "exit 12"'; echo $?
12
$ bash -e -c ' : |(eval "sh -e -c \"exit 12\"")'; echo $?
12
```

So this is some strange behaviour when `|` is combined with `eval` in `bash`.
Looks like to be [bash bug 109840](https://savannah.gnu.org/support/index.php?109840).

## The fix

We fixed this strange GitLab behaviour by enabling the [Runner Feature Flag](https://docs.gitlab.com/runner/configuration/feature-flags/#available-feature-flags) `FF_USE_NEW_BASH_EVAL_STRATEGY`.
This puts one level of parenthesis around the `eval` command to run it in a sub-shell – see last example from above.

This can be done in the pipeline itself by setting the variable to `true` or some other value:
```yaml
fixed:
  variables:
    FF_USE_NEW_BASH_EVAL_STRATEGY: true
  script:
    - "sh -c 'exit 42'"
  allow_failure:
    exit_codes:
      - 42
```

## Closing words

So be careful when using `allow_failure` with `exit_codes` and calling _external_ programs.
Make sure to either enable the feature flag or use `exec …` or `… || exit $?` to really **exit** the shell.

The original [GitLab issue 27668](https://gitlab.com/gitlab-org/gitlab-runner/-/issues/27668) has some more details.
Sadly the feature flag is still not enabled by default as of today, so please vote for [issue 27909][2]

[1]: https://gitlab.com/gitlab-org/gitlab-runner/-/blob/main/shells/bash.go?ref_type=heads#L394-398
[2]: https://gitlab.com/gitlab-org/gitlab-runner/-/issues/27909
