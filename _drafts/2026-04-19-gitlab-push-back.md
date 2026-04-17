---
title: 'Releasing Debian packages with updates debian/changelog from GitLab pipeline'
date: '2026-04-17T14:28:00+02:00'
layout: post
categories: gitlab debian
excerpt_separator: <!--more-->
---

I'm using GitLab on a daily basis.
For some time I have been thinking about, how to automate doing a release.
As a [Debian Developer](https://qa.debian.org/developer.php?login=pmhahn%40debian.org) I often have to package software as a [Debian package](https://www.debian.org/doc/manuals/developers-reference/).
That involves maintaining the file [`debian/changelog`](https://www.debian.org/doc/debian-policy/ch-source.html#debian-changelog-debian-changelog):
For each release a new entry must be added, which at least contains the version number.

1. create branch, commit merge
2. update `debian/changelog`, commit
3. create tag

If you have a merge-request based workflow, you can do 1. and 3. using the GitLab GUI, but 2. requires going back to the terminal to do manual work.
How can this be automated?

<!--more-->

My idea is like this:
1. I will have two pipeline types: one for regular work and one to prepare a release.
2. I will use the [`Changelog: …`](https://docs.gitlab.com/user/project/changelogs/) trailer in each git commit to categorize the commits, which should be mentioned in the `changelog`.
3. I will use [GitLabs Repository API][GL-API-Repo-Changelog] to generate the `changelog`.
4. I will commit the generated changelog to the repository and use the [`CI_JOB_TOKEN` to `git push` requests back to the project repository](https://docs.gitlab.com/ci/jobs/ci_job_token/#allow-git-push-requests-to-your-project-repository)
5. I will [use the `CI_JOB_TOKEN` to create the `git tag` and GitLab release](https://docs.gitlab.com/user/project/releases/#create-a-release).


## The main pipeline

I need some event to trigger the release process.
Creating a [git tag](https://git-scm.com/book/en/v2/Git-Basics-Tagging) (or [GitLab release](https://docs.gitlab.com/user/project/releases/)) would be the obvious event, but I'd like to update the changelog **before** that tag is created.
As such I decided to have two kinds of pipelines:
-   Type `development` is the default one, which is triggered on every push and merge request.
    It runs my linters and builds the package each time to get early feedback.
-   Type `release` can be selected manually to cut a release.
    It will ask me for the next version number and will then create that release.

The GitLab documentation already contains an example for such [more dynamic pipelines](https://docs.gitlab.com/ci/inputs/examples/#use-inputs-with-include-for-more-dynamic-pipelines).
The main GitLab CI/CD pipeline file `.gitlab-ci.yml` looks like this:

```yaml
spec:
  inputs:
    pipeline-type:
      type: string
      default: development
      options:
        - development
        - release
      description: "The pipeline type, which determines which set of jobs to include"
    version:
      type: string
      default: ""
      regex: ^c?\d+\.\d+\.\d+$|^$
      description: "The version to create"
---
include:
  - local: .gitlab/ci/$[[ inputs.pipeline-type ]].gitlab-ci.yml
    inputs:
      version: $[[ inputs.version ]]
```

One caveat here is, that as of GitLab 18.10 I cannot make `version` depend on `pipeline-type`:
That input is only required for a pipeline of type `release`.
The input will also be shown for type `development`.
As such it also allows the empty value, which is also the default.

## Shared pipeline

Both modes need some common configuration, which I have put into a shared file `.gitlab/ci/common.yml`.
The file is included by both types:

```yaml
default:
  interruptible: true
  artifacts:
    expire_in: 1 week

variables:
  FF_SCRIPT_SECTIONS: true
  FF_TIMESTAMPS: true
  FF_USE_NEW_BASH_EVAL_STRATEGY: true
  GIT_DEPTH: 1
  GLAB_CONFIG_DIR: "${CI_PROJECT_DIR}/.glab-config.${CI_PIPELINE_ID}"

include:
  - component: "$CI_SERVER_FQDN/ci/py-ruff/uv-python@master"
    inputs:
      job_name: ".uv"
      uv_version: "0.11"
      python_version: "3.13"
      base_layer: "trixie"
      changes: []
```

1. I set several [GitLab Runner feature flags](https://docs.gitlab.com/runner/configuration/feature-flags/) to get some better experience.
2. For my environment I have to tell `glab` to use configuration file in a writeable directory. Without that it will try to write to `/.glab/`, which will fail.
3. For this example I'm using a Python package, which is build via [Astral's `uv`](https://docs.astral.sh/uv/). I have a custom component for that, which boils down to create a template job called `.uv` using the docker image `ghcr.io/astral-sh/uv:0.11-python3.13-trixie`.

## The development pipeline

The file `.gitlab/ci/development.gitlab-ci.yml` contains the definition for the regular development work:
It runs the linters and builds the package.

```yaml
spec:
  inputs:
    version:
      type: string
```

For the development pipeline `version` is not needed.
As such I just declare it as `type: string` with no additional constraints.

```yaml
---
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

include:
  - local: .gitlab/ci/common.yml
  - component: "$CI_SERVER_FQDN/ci/py-ruff/ruff@master"
    inputs:
      job_name: "ruff check"
      stage: lint

ruff format:
  extends: ["ruff check"]
  script:
    - uvx ruff format --diff
  artifacts: {}
```

I'm using my own component here to run [Astral's `ruff`](https://docs.astral.sh/ruff/rules/) two times:
1. Once as a linter via `ruff check`.
2. Once as a formatter via `ruff format`.

```yaml
.lint:
  stage: lint
  extends: [.uv]
  artifacts:
    reports:
      codequality: $CI_PROJECT_DIR/gl-code-quality-report.json

mypy:
  extends: [.lint]
  script:
    - uvx mypy --no-error-summary >mypy-out.txt
  after_script:
    - uvx mypy-gitlab-code-quality <mypy-out.txt >gl-code-quality-report.json
    - !reference [.uv, after_scipt]

pyright:
  stage: lint
  extends: [.lint]
  script:
    - uvx --from=pyright[nodejs] pyright --outputjson >pyright-raw.json
  after_script:
    - uvx pyright-to-gitlab -i pyright-raw.json -o gl-code-quality-report.json
    - !reference [.uv, after_scipt]

pyrefly:
  # <https://github.com/facebook/pyrefly/issues/3049>
  extends: [.uv]  # .lint
  stage: lint  # FIXME
  script:
    - uvx pyrefly check  # --output-format CodeQuality --output gl-code-quality-report.json

ty:
  extends: [.lint]
  script:
    - uvx ty check  --output-format gitlab >gl-code-quality-report.json
```
I'm experimenting with different Python type checks:
- Dropbox `mypy`
- Microsoft `pyright`
- Facebook `pyrefly`
- Astral `ty`

```yaml
check version numbers:
  stage: lint
  image: $CI_REGISTRY/ci/helpers/gitlab-release:latest
  rules:
    - if: '$CI_COMMIT_TAG =~ /^v?\d+\.\d+\.\d+$/'
  script: |
    deb=$(sed -rne '1s/^.+[(]([^)]+)[)].+$/\1/p' debian/changelog)
    py=$(sed -rne '/^\[project\]/,/^\[/s/^version\s*=\s*"([^"]+)"$/\1/p' pyproject.toml)
    printf -v vers '%s\n' "Python ${py:?}" "Debian ${deb:?}" "GitLab ${CI_COMMIT_TAG#v}"
    if [ "$(sort -k2V <<<"${vers%$'\n'}" | uniq --skip-fields=1 | wc -l)" -ne 1 ]
    then
      printf 'Inconsistent versions:\n%s' "$vers" >&2
      exit 1
    fi
```
The version number is contained in multiple files
- `pyproject.toml` for the Python package
- `debian/changelog` for the Debian package

The code here makes sure, that they all match:
When a `git tag` is created, this jobs runs and checks both file to have the same version.

```yaml
build python package:
  stage: build
  extends: [.uv]
  rules:
    - if: $CI_COMMIT_BRANCH
    - if: $CI_COMMIT_TAG
  script:
    - |
      VERSION=$(git describe --exact-match --tags) &&
      uvx --from=toml-cli toml set --toml-path=pyproject.toml project.version "$VERSION"
    - uv build
  artifacts:
    paths:
      - dist/
```
This builds the Python package.
The version number contained in the `pyproject.toml` file is always updated here to have a unique package even for merge requests.

```yaml
build debian package:
  stage: build
  rules:
    - if: $CI_COMMIT_BRANCH
    - if: $CI_COMMIT_TAG
  script:
    - sudo apt-get -qq update
    - sudo apt-get -qq -y build-depends .
    - dpkg-buildpackage --no-sign
    - mkdir dist
    - dcmd mv -t dist/ ../*.changes
  artifacts:
    paths:
      - dist/*.deb
```
This is a simplified version of building the Debian package.
In most cases you have to do more, see
- [apt build-dep .]({% post_url 2018-05-30-build-dep %})
- [Speedup Debian package building]({% post_url 2020-05-02-speedup-debian-package-build %})
- [Build Debian packages out-of-tree]({% post_url 2020-05-03-debian-oot-build %})
<!-- - [How to build Debian packages]( { % post_url 2025-10-12-debian-pkg-build % } ) -->

```yaml
publish python package:
  stage: publish
  extends: [.uv]
  rules:
    - if: $CI_COMMIT_TAG
  needs:
    - job: build python package
      artifacts: true
  variables:
    UV_PUBLISH_URL: "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/pypi"
    UV_PUBLISH_USERNAME: "gitlab-ci-token"
    UV_PUBLISH_PASSWORD: "${CI_JOB_TOKEN}"
  script:
    - uv publish dist/*.whl
```
This published the Python package to [GitLab's PyPI package registry](https://docs.gitlab.com/user/packages/pypi_repository/).

```yaml
publish debian package:
  stage: publish
  rules:
    - if: $CI_COMMIT_TAG
  needs:
    - job: build debian package
      artifacts: true
  script:
    - >
      printf >dput.cf '%s\n'
      '[gitlab]'
      'method = https'
      "fqdn = Job-Token:${CI_JOB_TOKEN}@${CI_SERVER_FQDN}"
      "incoming = /api/v4/projects/${CI_PROJECT_ID}/packages/debian"
    - >
      dput
      --config=dput.cf
      --unchecked 
      --no-upload-log
      gitlab
      dist/*.changes
```
This is a simplified version to publish the Debian package to [GitLab's Debian package registry](https://docs.gitlab.com/user/packages/debian_repository/), which is as of GitLab 18.10 still under development.

```yaml
release_job:
  stage: release
  image: $CI_REGISTRY/ci/helpers/gitlab-release:latest
  rules:
    - if: '$CI_COMMIT_TAG =~ /^v?\d+\.\d+\.\d+$/'
  dependencies: []
  script:
    # Create release notes
    - >
      curl
      --show-error --silent --fail-with-body
      --output release_notes.json
      --header "JOB-TOKEN: $CI_JOB_TOKEN"
      --get
      --data-urlencode version="$CI_COMMIT_TAG"
      --data-urlencode config_file='.gitlab/changelog_config.yml'
      --data-urlencode config_file_ref="$CI_COMMIT_TAG"
      --data-urlencode to="$CI_COMMIT_TAG"
      --url "$CI_API_V4_URL/projects/$CI_PROJECT_ID/repository/changelog"
  - jq -r .notes release_notes.json >release_notes.md
  #- glab changelog generate --version "$CI_COMMIT_TAG" --to "$CI_COMMIT_TAG" >release_notes.md
  - |
    export GITLAB_HOST="$CI_SERVER_URL"
    glab config set ca_cert "" --host "$CI_SERVER_HOST"
    glab auth login --job-token "$CI_JOB_TOKEN" --hostname "$CI_SERVER_HOST" --api-protocol "$CI_SERVER_PROTOCOL"
  - >
    glab release create "$CI_COMMIT_TAG"
    --name "Release $CI_COMMIT_TAG"
    --notes-file release_notes.md
    --repo "$CI_PROJECT_PATH"
    --ref "$CI_COMMIT_SHA"
    --assets-links "$(jq -n --arg url "$CI_PROJECT_URL/-/packages/" --arg name "PyPi package $CI_COMMIT_TAG" --arg link_type package '[$ARGS.named]')"
    --no-close-milestone
```

This job creates the release:
1. It uses `curl` to call GitLab repository API to create a release.
   Using `glab changelog generate …` would be easier, but as of version `1.89` does not allow to specify the reference of the branch, from which the `.gitlab/changelog_config.yml` should be fetched.
   But I want to be able to modify the configuration file in a branch and then use that changed file instead of the old version from the default branch.
   (That's unlikely to happen now as the job is only triggered for `CI_COMMIT_TAG`, but I could tag some other branch than `CI_DEFAULT_BRANCH`.)
2. The `.gitlab/changelog_config.yml` is the default file from GitLab, which generated Markdown.
   `jq` is used to extract the output from the answer and to write it to the file `release_notes.md`, which is later used when the GitLab release is created.
3. The last `glab` command matches, what [`releases:`](https://docs.gitlab.com/ci/yaml/#release) does by default with one difference:
   It skips the option `--no-update`!
   Why this is needed you will see in a minute.

PS: That `--assets-links …` argument will create a link to the PyPI package registry. [GitLab 18.11](https://docs.gitlab.com/releases/18/gitlab-18-11-released/) just received a feature, where [packages are included as release evidence](https://docs.gitlab.com/user/project/releases/release_evidence/#include-packages-as-release-evidence), which might make this optional.

## The release pipeline

The file `.gitlab/ci/release.gitlab-ci.yml` contains the definition for doing a release:

```yaml
spec:
  inputs:
    version:
      type: string
      regex: ^v?\d+\.\d+\.\d+$
```

For a release I need a valid version number.
GitLab will raise an error if you select `pipeline-type: release` but don't provide a valid version matching the regular expression.

```yaml
---
workflow:
  rules:
    - if: $CI_PIPELINE_SOURCE == "api" && $CI_COMMIT_BRANCH
    - if: $CI_PIPELINE_SOURCE == "trigger" && $CI_COMMIT_BRANCH
    - if: $CI_PIPELINE_SOURCE == "web" && $CI_COMMIT_BRANCH
```

I require a valid branch to push back the commit to.
As such only a subset of pipeline sources are supported.

```yaml
stages:
  - release

include:
  - local: .gitlab/ci/common.yml

# <https://gitlab.com/guided-explorations/gitlab-ci-yml-tips-tricks-and-hacks/commit-to-repos-during-ci/commit-to-repos-during-ci>
prepare release:
  stage: release
  extends: [.uv]
  image: $CI_REGISTRY/ci/helpers/gitlab-release:latest
  variables:
    GIT_DEPTH: 10
    tag: $[[ inputs.version ]]
  script:
    - |
      uvx --from=toml-cli toml set --toml-path=pyproject.toml project.version "$tag"
```

This pokes the chosen version number for the Python package into `pyproject.toml`.

```yaml
    - >
      curl
      --show-error --silent --fail-with-body
      --output release_notes.json
      --header "JOB-TOKEN: $CI_JOB_TOKEN"
      --get
      --data-urlencode version="$tag"
      --data-urlencode config_file='.gitlab/changelog_debian.yml'
      --data-urlencode config_file_ref="$CI_COMMIT_BRANCH"
      --data-urlencode date="$CI_COMMIT_TIMESTAMP"
      --data-urlencode to="$CI_COMMIT_BRANCH"
      --url "$CI_API_V4_URL/projects/$CI_PROJECT_ID/repository/changelog"
```

Again I use [GitLabs repository API][GL-API-Repo-Changelog] to generate a changelog, but with a **different configuration**:
It does not create Markdown, but a very simple file format where each git commit summary is on a separate line.

```yaml
    - |
      export DEBFULLNAME="${GITLAB_USER_NAME}" DEBEMAIL="${GITLAB_USER_EMAIL}"
      # glab changelog generate --version "$tag" --to "$CI_COMMIT_BRANCH" --config-file '.gitlab/changelog_debian.yml' |
      jq -r .notes release_notes.json |
        grep -v -e '^#' -e '^\s*$' |
        xargs -d'\n' -r -n1 debchange --no-auto-nmu -v "$tag" --
      debchange --distribution unstable --force-distribution --release ''
```

This converts each changelog entry into a valid entry in the `debian/changelog`:
It uses [`debchange`](https://manpages.debian.org/testing/devscripts/debchange.1.de.html), which is Debians official tool to do that.
The last call is used to finalize the file.

```yaml
    - |
      git config --local user.name "${GITLAB_USER_NAME}"
      git config --local user.email "${GITLAB_USER_EMAIL}"
      git add -- pyproject.toml debian/changelog
      git commit -m "Create release $tag"
      git push -o ci.skip origin HEAD:"$CI_COMMIT_BRANCH"
```

This creates a new commit containing both the changed `pyproject.yaml` and `debian/changelog` file.
As GitLab uses detached heads when checking out the branch, the branch name must be explicitly named with the `git push`.
The `-o ci.skip` is not needed as using `CI_JOB_TOKEN` does that by default, but I want to make sure, that no regular pipeline is triggered at this stage.

```yaml
    # Create release notes
    - >
      curl
      --show-error --silent --fail-with-body
      --output release_notes.json
      --header "JOB-TOKEN: $CI_JOB_TOKEN"
      --get
      --data-urlencode version="$tag"
      --data-urlencode config_file='.gitlab/changelog_config.yml'
      --data-urlencode config_file_ref="$CI_COMMIT_BRANCH"
      --data-urlencode date="$CI_COMMIT_TIMESTAMP"
      --data-urlencode to="$CI_COMMIT_BRANCH"
      --url "$CI_API_V4_URL/projects/$CI_PROJECT_ID/repository/changelog"
    - jq -r .notes release_notes.json >release_notes.md
```

This is the same code is for `pipeline-type: development`, which again created a changelog in format Markdown.
TODO: Check if I can get rid of this as the job there also generated the same Markdown.

```yaml
    #- glab changelog generate --version "$CI_COMMIT_TAG" --to "$CI_COMMIT_TAG" >release_notes.md
  release:
    name: 'Release $[[ inputs.version ]]'
    description: release_notes.md
    tag_name: '$[[ inputs.version ]]'
    ref: '$CI_COMMIT_BRANCH'
```

This is the final part, which creates a GitLab release.
That will also create a `git tag`, which then triggers a _tag pipeline_ (`CI_COMMIT_TAG`) with `pipeline-type: development`.
That pipeline will then run for the commit **include** the just generated _release commit_.
It will lint and build and release the package.

The important things here is that the job `release_job` will also run, which ~~creates~~update_ the **same release again**.
That's why `--no-update` **must not** be used there.
The tag name and referenced commit will not change, but the release will reference the corresponding build artifacts correctly.

If you wonder why two releases are require?

As mentioned above, [using the `CI_JOB_TOKEN` will not trigger another pipeline](https://docs.gitlab.com/ci/jobs/ci_job_token/#allow-git-push-requests-to-your-project-repository) and is equivalent to using the [push option `-o ci.skip`](https://docs.gitlab.com/topics/git/commit/#push-options-for-gitlab-cicd):

> When you use a job token to push to the project, no CI/CD pipelines are triggered.

This also affects pushing tags with `git push origin $CI_COMMIT_TAG`, so no _development_ pipeline would be triggered.

But you can use the `CI_JOB_TOKEN` to create a GitLab release, which then will trigger such a pipeline implicitly.
And as GitLab releases can be updated multiple times, using them solves the problem nicely.

[GL-API-Repo-Changelog]: https://docs.gitlab.com/api/repositories/#generate-changelog-data

{% include abbreviations.md %}
