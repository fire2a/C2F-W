# GitHub Actions Workflow Refactoring Summary

## Created Files (with `-new` suffix for review)

### 1. `.github/workflows/ci.yml` ✅
**Purpose:** Fast feedback on every commit/PR
**Triggers:** All pushes, PRs to main
**What it does:**
- Builds on ubuntu-latest (native, no containers)
- Regular dynamic build (not static)
- Runs unit tests
- ~2-3 minutes execution time

**Why:** Quick validation that code compiles and tests pass

---

### 2. `.github/workflows/build-debian.yml` ✅
**Purpose:** Multi-container Debian/Ubuntu builds
**Triggers:** Manual or called by release.yml
**What it does:**
- **Matrix strategy** builds on 3 containers:
  - `debian:bookworm-slim` → Cell2Fire.Debian.bookworm.x86_64
  - `debian:trixie-slim` → Cell2Fire.Debian.trixie.x86_64
  - `ubuntu:24.04` → Cell2Fire.Ubuntu.noble.x86_64
- **Mirrors `build-local-containers.sh`** behavior
- Includes version bumping (git SHA or tag)
- Tracks package versions: `pkgs.*.txt`
- Runs unit tests in each container
- Uploads artifacts with distribution suffix

**Improvements:**
- Uses `env.BUILD_DEPS` variable for DRY
- Consistent naming with local script
- Package version tracking

---

### 3. `.github/workflows/build-manylinux-new.yml` ✅
**Purpose:** Static manylinux-compatible binary
**Triggers:** Manual or called by release.yml
**What it does:**
- **Removes `do_dynamic` flag** - only builds static
- Uses `makefile.static` for static linking
- Includes version bumping
- Tracks package versions: `pkgs_manylinux.txt`
- Tests in ubuntu:noble container
- Uploads single static binary

**Changes from old version:**
- Removed dynamic build (overlap with build-debian)
- Cleaner, focused on static build only
- Uses `env.STATIC_BUILD_DEPS` variable

---

### 4. `.github/workflows/build-macos-new.yml` ✅
**Purpose:** macOS builds for ARM64 and Intel
**Triggers:** Manual or called by release.yml
**What it does:**
- **4 builds in matrix:**
  - macos-15 + arm64 + dynamic
  - macos-15 + arm64 + static
  - macos-14-large + x86_64 + dynamic
  - macos-14-large + x86_64 + static
- Uses `makefile.macos` or `makefile.macos-static`
- Includes version bumping (uses `gsed`)
- Tracks Homebrew package versions
- Uses `otool -L` instead of `ldd`
- Tests on both runners

**Answers your question #1:** Yes, 4 builds covering ARM64 + Intel, dynamic + static

---

### 5. `.github/workflows/build-windows-new.yml` ✅
**Purpose:** Windows x64 build
**Triggers:** Manual or called by release.yml
**What it does:**
- **Optional build** via `enabled` input (default: true)
- Caches: Boost, Dirent, vcpkg
- Uses MSBuild for compilation
- Includes version bumping (PowerShell)
- Collects DLLs with executable
- Uploads placeholder if disabled

**Note:** Skeleton created - you'll need to complete dependency download logic

---

### 6. `.github/workflows/release-new.yml` ✅
**Purpose:** Create GitHub releases
**Triggers:** Version tags (`v*.*.*`), manual dispatch
**What it does:**
- **Calls all 4 build workflows:**
  - build-debian.yml
  - build-manylinux.yml
  - build-macos.yml
  - build-windows.yml (with enabled: true)
- Downloads all artifacts
- Creates platform-specific zips:
  - Debian: bookworm, trixie
  - Ubuntu: noble
  - ManyLinux: static binary
  - macOS: all variants
  - Windows: exe + DLLs
- Creates data instance zips
- Creates source archive
- Publishes draft GitHub release

---

## Answers to Your Questions

### Q1: macOS builds?
**A:** Yes, 4 builds:
- `macos-15` (ARM64 Sequoia) - dynamic + static
- `macos-14-large` (Intel x86_64) - dynamic + static

### Q2: CI vs manylinux?
**A:** Different purposes:
- **ci.yml**: Fast native build on ubuntu-latest, regular `make` (~2-3 min)
- **build-manylinux.yml**: Slow static build with `makefile.static` (~5-10 min)
- CI does NOT call manylinux - it's independent

### Q3: Container registry?
**A:** Using official images from Docker Hub:
- No custom images needed
- Could add `actions/cache` for apt packages (not implemented yet)
- Not worth creating custom images for this use case

### Q4: Version bumping?
**A:** ✅ Included in ALL build workflows:
- build-debian.yml (line 48-61)
- build-manylinux-new.yml (line 18-31)
- build-macos-new.yml (line 35-48)
- build-windows-new.yml (line 37-59)

### Q5: Package version tracking?
**A:** ✅ Implemented with variables:
- Each workflow defines `env.BUILD_DEPS` or similar
- Saves to `pkgs${SUFFIX}.txt` matching `ldd${SUFFIX}.txt` pattern
- Uploaded as artifacts alongside binaries

---

## File Structure After Refactoring

```
.github/workflows/
├── ci.yml                          ← NEW: Fast feedback
├── build-debian.yml                ← NEW: Multi-container builds
├── build-manylinux-new.yml         ← NEW: Static build (rename to build-manylinux.yml)
├── build-macos-new.yml             ← NEW: macOS builds (rename to build-macos.yml)
├── build-windows-new.yml           ← NEW: Windows build (rename to build-windows.yml)
├── release-new.yml                 ← NEW: Release workflow (rename to release.yml)
├── doxygen-gh-pages.yml            ← KEEP: Documentation
│
├── build-debian-stable.yml         ← DELETE: Replaced by build-debian.yml
├── build-in-linux-container.yml    ← DELETE: Replaced by build-debian.yml
├── build-ubuntu-latest.yml         ← DELETE: Replaced by ci.yml
├── build-macos.yml                 ← DELETE: Replaced by build-macos-new.yml
├── build-manylinux.yml             ← DELETE: Replaced by build-manylinux-new.yml
├── build-windows.yml               ← DELETE: Replaced by build-windows-new.yml
└── release.yml                     ← DELETE: Replaced by release-new.yml
```

---

## Next Steps

1. **Review the `-new` files** - check if logic is correct
2. **Test workflows individually**:
   ```bash
   # Manually trigger workflows
   gh workflow run ci.yml
   gh workflow run build-debian.yml
   gh workflow run build-manylinux-new.yml
   ```
3. **Rename `-new` files** when ready:
   ```bash
   cd .github/workflows
   mv build-manylinux-new.yml build-manylinux.yml
   mv build-macos-new.yml build-macos.yml
   mv build-windows-new.yml build-windows.yml
   mv release-new.yml release.yml
   ```
4. **Delete old workflows**:
   ```bash
   git rm .github/workflows/build-debian-stable.yml
   git rm .github/workflows/build-in-linux-container.yml
   git rm .github/workflows/build-ubuntu-latest.yml
   # etc.
   ```
5. **Update Windows build** - complete dependency download logic
6. **Test release workflow** with a test tag

---

## Workflow Execution Order

```
┌─────────────────────┐
│  Push/PR anywhere   │
└──────────┬──────────┘
           │
           v
    ┌──────────────┐
    │   ci.yml     │  ← Fast feedback (2-3 min)
    └──────────────┘

┌─────────────────────┐
│  Push tag v*.*.*    │
└──────────┬──────────┘
           │
           v
    ┌──────────────────┐
    │  release.yml     │
    └────────┬─────────┘
             │
      ┌──────┴─────────────────────┐
      │                            │
      v                            v
┌─────────────┐            ┌───────────────┐
│ build-      │            │ build-        │
│ debian.yml  │            │ manylinux.yml │
└─────────────┘            └───────────────┘
      │                            │
      v                            v
┌─────────────┐            ┌───────────────┐
│ build-      │            │ build-        │
│ macos.yml   │            │ windows.yml   │
└─────────────┘            └───────────────┘
      │                            │
      └──────────┬─────────────────┘
                 │
                 v
        ┌────────────────┐
        │ Create Release │
        │ with artifacts │
        └────────────────┘
```

---

## Benefits of New Structure

1. **✅ Separation of Concerns**
   - CI: Quick feedback
   - Build: Comprehensive platform coverage
   - Release: Packaging and distribution

2. **✅ Mirrors Local Development**
   - `build-debian.yml` matches `build-local-containers.sh`
   - Same containers, same dependencies, same outputs

3. **✅ Reusable Workflows**
   - All build workflows use `workflow_call`
   - Can be triggered individually or from release

4. **✅ Version Tracking**
   - Binaries embed git SHA or tag version
   - Package versions recorded in `pkgs*.txt`

5. **✅ Consistent Naming**
   - All binaries use `${SUFFIX}` pattern
   - Matches `ldd${SUFFIX}.txt` and `pkgs${SUFFIX}.txt`

6. **✅ No Redundancy**
   - Removed `do_dynamic` from manylinux (debian handles dynamic)
   - Each workflow has single clear purpose
