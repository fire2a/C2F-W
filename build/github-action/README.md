# Github Actions

This document describes the workflows located in `.github/workflows`, its purpose is to build, test, document and create releases of Cell2Fire using GitHub Actions, GitHub's CI/CD platform.
What triggers them and what repo-level actions variables are used to control these.

Workflows are located in `.github/workflows`
- build-debian.yml
- build-manylinux.yml
- build-macos.yml
- build-windows.yml
- release.yml
- ci.yml
- doxygen-gh-pages.yml

## Workflows overview

- `ci.yml` (Continuous Integration)
  - Runs on every push (all branches) and on pull requests to `main`.
  - Use this for quick verification (build + unit tests).
  - Can be disabled manually (see below).

- `release.yml` (Release orchestration)
  - Triggered by tag pushes (semantic version tags) and manually via the
    "Run workflow" button in the Actions UI (`workflow_dispatch`).
  - Orchestrates platform builds by calling the build workflows.

- `build-*.yml` (Build & test per platfrom workflows)
  - Can also be invoked manually in the Actions UI (`workflow_dispatch`) then its binaries/artifacts
    downloaded from there.
  - Each workflow builds Cell2Fire on its respective platform (linux, macOS or Windows)
     - Dependencies statically and dynamically linked (where applicable)
     - For macOS archs x86_64 & arm64 are created 
     - For windows an `.exe` file and its dependencies `.dll` files are zipped together (must be kept together to run unless running inside QGIS which handles that automatically)
     
- `doxygen-gh-pages.yml` (Documentation deployment)
  - Triggered by pushes to `doc`
  - Builds Doxygen documentation and deploys it to the `gh-pages` branch.

## Configuration

### Configuring repository-level Actions variables
- Go to: https://github.com/fire2a/C2F-W/settings/variables/actions
- The repository variables used by `release.yml` and related `build-*.yml` workflows
  include (typical names and example values):
  - `BUILD_DEBIAN` : `true` or `false`
  - `BUILD_MANYLINUX` : `true` or `false`
  - `BUILD_MACOS` : `true` or `false`
  - `BUILD_WINDOWS` : `true` or `false`
  - `CACHE_EPOCH` : `1` (integer/string; bump to invalidate caches hence update/recreate them)
  - `RUN_REGRESSION_TESTS` : `true` or `false`
  - `RUN_UNIT_TESTS` : `true` or `false`
- Notes:
  - Variables are stored as strings. In workflows they are typically checked
    like `if: ${{ vars.BUILD_DEBIAN == 'true' }}`.
  - These variables control whether platform builds run when `release.yml` is
    evaluated. They are useful for toggling expensive builds from the UI.

### Manual disabling of `ci.yml`
- Quick (UI) method: Go to Actions → Workflows → select "CI" → click
  "Disable workflow". This prevents the workflow from running until re-enabled.

## Other references

### 1. Creating a release
1. Merge a PR into `main` that contains **all** the changes you want to release.
   - Ensure that the `ci.yml` workflow passes on `main`.
   - Ensure that the `release.yml` workflow passes on the PR branch before merging!

2. Create a new tag based on `main`
   - As other branches are normally deleted as PR's are merged, so the tags are orphaned.
   ```bash
   git checkout main
   git pull origin main
   # locally && remotely
   git tag -a v1.2.3 -m 'A meaningful sentence' && git push origin v1.2.3
   # if you fucked up the tag:
   git tag --delete v1.2.3 && git push --delete origin v1.2.3
   ```

3. Wait for the `release.yml` workflow to complete (successfully).
   - https://github.com/fire2a/C2F-W/actions/workflows/release.yml

4. Go to the releases section, (verify) and edit the release to Publish it!
   - https://github.com/fire2a/c2f-w/releases

### 2. How to run workflows manually
- From the GitHub web UI: Actions → select the workflow (for example "Release") →
  click "Run workflow" (if the workflow defines `workflow_dispatch`).
- Create a tag and push it to trigger `release.yml` automatically.
- For `ci.yml`, push a commit or open a PR against `main`.

### 3. Workflow changes must reach the `main` branch to take effect on the GitHub web UI

