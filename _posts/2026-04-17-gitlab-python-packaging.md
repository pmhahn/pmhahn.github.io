---
title: 'Building Python packages via GitLab pipeline with type checking'
date: '2026-04-18T09:13:00+02:00'
layout: post
categories: gitlab python
excerpt_separator: <!--more-->
---

My main programming language is [Python][python] and I'm a huge fan of [static type hinting](https://docs.python.org/3/library/typing.html).
As of 2026 there are four type checkers:

- Dropbox [`mypy`][mypy] (Python)
- Microsoft [`pyright`][pyright] (TypeScript)
- Facebook [`pyrefly`][pyrefly] (rust)
- Astral [`ty`][ty] (rust)

I like to integrate them into my [GitLab][gitlab] workflow, which includes generatin a [Code Quality][gitlab-cc] report.

On top of this my pipeline also runs [Astral `ruff`][ruff] as a linter and code formatter and uses [Astral `uv`][uv] to build and publish the Python package to GitLabs's [PyPI package repository][gitlab-pypi].

<!--more-->

The complete example is available on [gitlab.com](https://gitlab.com/pmhahn/python-packaging/).

## Basic GitLab pipeline

```yaml
workflow:
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"  # MR
    - if: $CI_COMMIT_BRANCH && $CI_OPEN_MERGE_REQUESTS
      when: never
    - if: $CI_COMMIT_BRANCH  # Branch,Schedule,Web,CLI
    - if: $CI_COMMIT_TAG  # Tag
```

My workflow required pipelines for merge-requests, stable/default/protected branches, tags and manually and time triggered pipelines.

```yaml
stages:
  - lint
  - build
  - publish
  - release

default:
  interruptible: true
  artifacts:
    expire_in: 1 day

variables:
  FF_SCRIPT_SECTIONS: true
  FF_TIMESTAMPS: true
  FF_USE_NEW_BASH_EVAL_STRATEGY: true

.py:
  rules:
    - changes:
        paths:
          - pyproject.toml
          - uv.lock
          - "**/*.py"
  variables:
    GIT_DEPTH: 1
```

I set several [GitLab Runner feature flags](https://docs.gitlab.com/runner/configuration/feature-flags/) to get some better experience.

## Prepare `uv`

```yaml
.uv:
  variables:
    UV_VERSION: "0.11"
    PYTHON_VERSION: "3.13"
    BASE_LAYER: trixie
    # GitLab CI creates a separate mountpoint for the build directory,
    # so we need to copy instead of using hard links.
    UV_LINK_MODE: copy
    UV_CACHE_DIR: .uv-cache
  image: ghcr.io/astral-sh/uv:$UV_VERSION-python$PYTHON_VERSION-$BASE_LAYER
  cache:
    - key:
        files:
          - uv.lock
      paths:
        - $UV_CACHE_DIR
  after_script:
    - uv cache prune --ci
```

I'm using [`uv`][uv] here with some specific version matching Debian 13 "Trixie".
This sets up caching as documented in [`uv`'s GitLab integration][uv-gitlab].

## Running `ruff`

```yaml
.ruff:
  extends: [.uv, .py]
  stage: lint

ruff check:
  extends: [.ruff]
  script:
    - uvx ruff check --output-format=gitlab --output-file=code-quality-report.json
  artifacts:
    reports:
      codequality: $CI_PROJECT_DIR/code-quality-report.json

ruff format:
  extends: [.ruff]
  script:
    - uvx ruff format --diff
```
This runs [`ruff`][ruff-gitlab] twice:
1. Once as a linter to check form [common issues](https://docs.astral.sh/ruff/rules/)
2. Once again as a code formatter to check, if the formatting does not follow the configured style.

## Running the type checkers

```yaml
.lint:
  stage: lint
  extends: [.uv, .py]
  artifacts:
    reports:
      codequality: $CI_PROJECT_DIR/gl-code-quality-report.json

mypy:
  extends: [.lint]
  script:
    - uvx mypy --no-error-summary >mypy-out.txt
  after_script:
    - uvx mypy-gitlab-code-quality <mypy-out.txt >gl-code-quality-report.json
    - !reference [.uv, after_script]

pyright:
  extends: [.lint]
  script:
    - uvx --from=pyright[nodejs] pyright --outputjson >pyright-raw.json
  after_script:
    - uvx pyright-to-gitlab -i pyright-raw.json -o gl-code-quality-report.json
    - !reference [.uv, after_script]

pyrefly:
  extends: [.uv]  # .lint
  stage: lint  # TEMPORARY
  script:
    - uvx pyrefly check  # --output-format CodeQuality --output gl-code-quality-report.json

ty:
  extends: [.lint]
  script:
    - uvx ty check  --output-format gitlab >gl-code-quality-report.json
```

`mypy` and `pyright` do not generate the [Code Quality JSON][gitlab-cc-json] themselves.
They require running some converter, which transforms their output format.
This is done in `after_script` to always run them, even when the type checkers abort with an exit code other than 0.
This overwrites the `after_script` from the template job `.uv`, which calls `uv cache prune --ci` to maintain its cache.
As such we have to restore that functionality and use [`!reference`][gitlab-ref] to do that.

`ty` already generated the required JSON.
For `pyrefly` there is [issue 3049](https://github.com/facebook/pyrefly/issues/3049), where I asked to add native support for it.

## Build and publish the Python package

```yaml
build python package:
  stage: build
  extends: [.uv]
  rules:
    - if: $CI_COMMIT_BRANCH
    - if: $CI_COMMIT_TAG
  variables:
    GIT_DEPTH: 0
    GIT_FETCH_EXTRA_FLAGS: --prune --quiet --tags --filter=tree:0
  script:
    # - |
    #   VERSION=$(git describe --exact-match --tags) &&
    #   uvx --from=toml-cli toml set --toml-path=pyproject.toml project.version "$VERSION"
    - uv build
  artifacts:
    paths:
      - dist/
```
This builds the Python package.

Alter `uv publish` will fail when you try to upload a Python package with an already existing version.
This happens mostly because I forget to update `[project]version` in the `pyproject.toml`.
The out-commented code above would poke the version into the file before each build.

My alternative was to switch to [`setuptools-scm`][setuptools-scm]:
This will use `git desctibe` to generate the version from `git tags`, which requires two things:

1.  The image must contain the `git` binary. Debian's `-slim`-images do not.
    Switch to the non-`-slim` versions.
2.  Fetch enough history:
    `GIT_DEPTH: 1` may not be enough to walk the git commits from `HEAD` to any previous `git tag`.
    Therefor I use `GIT_DEPTH: 0` to fetch the complete history, but combine it with `GIT_FETCH_EXTRA_FLAGS: --filter=tree:0`:
    That way I use [`git`'s partial-clone][git-partial] feature:
    It fetch all commit object, but only the tree and blob objects required for HEAD.

```yaml
publish python package:
  stage: publish
  extends: [.uv]
  rules:
    - if: $CI_COMMIT_TAG
  needs:
    - job: build python package
  variables:
    GIT_STRATEGY: none
    UV_PUBLISH_URL: ${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/pypi
    UV_PUBLISH_USERNAME: gitlab-ci-token
    UV_PUBLISH_PASSWORD: ${CI_JOB_TOKEN}
  script:
    - uv publish dist/*.whl
```
This published the Python package to GitLab's [PyPI package registry][gitlab-pypi].

## Creating a GitLab release

```yaml
release_job:
  stage: release
  image: registry.gitlab.com/gitlab-org/cli:latest
  rules:
    - if: '$CI_COMMIT_TAG =~ /^v?\d+\.\d+\.\d+$/'
  variables:
    # GIT_STRATEGY: none
    GIT_DEPTH: 1
    GIT_CHECKOUT: false
    GLAB_CONFIG_DIR: ${CI_PROJECT_DIR}/.glab-config.${CI_PIPELINE_ID}
    GLAB_ENABLE_CI_AUTOLOGIN: true
  dependencies: []
  script:
    - >
      glab changelog generate >changelog.md
      --version "$CI_COMMIT_TAG"
      --to "$CI_COMMIT_BRANCH"
  release:
    name: 'Release $CI_COMMIT_TAG'
    description: changelog.md
    tag_name: $CI_COMMIT_TAG
    assets:
      links:
        - name: 'PyPi package $CI_COMMIT_TAG'
          url: $CI_PROJECT_URL/-/packages/
          link_type: package
```
The final part creates a [GitLab release][gitlab-release].
It uses [GitLab's changelog API][gitlab-changelog] to automatically create a changelog in Markdown format from the git commits having a `Changelog:` trailer.

For my environment I have to tell `glab` to use configuration file in a writeable directory.
Without that it will try to write to `/.glab/`, which will fail.

`glab` also requires a local `git` repository to work with.
Therefore I use `GIT_DEPTH: 1` to reduce the number of commits to fetch combined with `GIT_CHECKOUT: false` to disable creating a work-space.

That `assets:links:` part creates a link to the PyPI package registry.
[GitLab 18.11](https://docs.gitlab.com/releases/18/gitlab-18-11-released/) just received a feature, where [packages are included as release evidence](https://docs.gitlab.com/user/project/releases/release_evidence/#include-packages-as-release-evidence), which might make this optional.


[gitlab-cc]: https://docs.gitlab.com/ci/testing/code_quality/
[gitlab-cc-json]: https://docs.gitlab.com/ci/testing/code_quality/#code-quality-report-format
[gitlab]: https://gitlab.com/
[gitlab-pypi]: https://docs.gitlab.com/user/packages/pypi_repository/
[gitlab-ref]: https://docs.gitlab.com/ci/yaml/yaml_optimization/#reference-tags
[gitlab-release]: https://docs.gitlab.com/user/project/releases/#create-a-release
[gitlab-changelog]: https://docs.gitlab.com/user/project/changelogs/
[glab]: https://gitlab.com/gitlab-org/cli/
[mypy]: https://mypy-lang.org/
[pyproject]: https://packaging.python.org/en/latest/guides/writing-pyproject-toml/
[pyrefly]: https://pyrefly.org/
[pyright]: https://github.com/microsoft/pyright
[python]: https://python.org/
[setuptools-scm]: https://pypi.org/project/setuptools-scm/
[ruff-gitlab]: https://docs.astral.sh/ruff/integrations/#gitlab-cicd
[ruff]: https://docs.astral.sh/ruff/
[ty]: https://docs.astral.sh/ty/
[uv-gitlab]: https://docs.astral.sh/uv/guides/integration/gitlab/
[uv]: https://docs.astral.sh/uv/
[git-partial]: https://git-scm.com/docs/partial-clone

{% include abbreviations.md %}
