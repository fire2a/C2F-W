# Workflow Migration Checklist

## Summary

All new workflow files have been created with `-new` suffix for safe review. This document guides you through testing and migration.

## Current State

```
✅ Created (ready for testing):
   - ci.yml                    (NEW - no old version)
   - build-debian.yml          (NEW - replaces multiple old workflows)
   - build-manylinux-new.yml   (UPDATED - needs rename)
   - build-macos-new.yml       (UPDATED - needs rename)
   - build-windows-new.yml     (UPDATED - needs rename)
   - release-new.yml           (UPDATED - needs rename)

⚠️  Keep as-is:
   - doxygen-gh-pages.yml      (UNCHANGED)

🗑️  To delete after migration:
   - build-debian-stable.yml
   - build-in-linux-container.yml
   - build-ubuntu-latest.yml
   - build-macos.yml (old)
   - build-manylinux.yml (old)
   - build-windows.yml (old)
   - release.yml (old)
```

---

## Migration Steps

### Phase 0: Commit new files

```bash
git add .github/workflows/ci.yml
git add .github/workflows/build-debian.yml
git add .github/workflows/build-manylinux-new.yml
git add .github/workflows/build-macos-new.yml
git add .github/workflows/build-windows-new.yml
git add .github/workflows/release-new.yml
git add docs/WORKFLOW_REFACTORING.md
git add docs/WORKFLOW_MIGRATION.md

git commit -m "feat: add refactored GitHub Actions workflows

- Add ci.yml for fast feedback on every push/PR
- Add build-debian.yml for multi-container builds
- Add build-manylinux-new.yml for static builds
- Add build-macos-new.yml for macOS ARM64 and Intel
- Add build-windows-new.yml for Windows builds
- Add release-new.yml to orchestrate all builds
- Add documentation for migration
"

git push origin 209-upgrade-testing-framework-catch2-to-3-debian-bookworm-to-trixie
```

### Phase 1: Test Individual Workflows (Manual Dispatch)

```bash
# Test CI workflow (should already work - no -new suffix)
gh workflow run ci.yml

# Test Debian builds
gh workflow run build-debian.yml

# Test ManyLinux static build
gh workflow run build-manylinux-new.yml

# Test macOS builds (if you have macOS runners)
gh workflow run build-macos-new.yml

# Test Windows build
gh workflow run build-windows-new.yml --field enabled=true
```

**Check each workflow:**
- ✅ Builds successfully
- ✅ Artifacts are created with correct naming
- ✅ Tests pass
- ✅ Package versions are recorded

---

### Phase 2: Test Release Workflow

```bash
# Option 1: Test with workflow_dispatch
gh workflow run release-new.yml

# Option 2: Create a test tag
git tag v0.0.0-test
git push origin v0.0.0-test
# This should trigger release-new.yml
```

**Check release workflow:**
- ✅ All 4 build workflows are called
- ✅ Artifacts are downloaded
- ✅ Zips are created correctly
- ✅ Draft release is created with all artifacts

---

### Phase 3: Rename Files (Activate New Workflows)

Once testing is complete:

```bash
cd .github/workflows

# Backup old workflows (optional - git already has history)
mkdir -p ../../old-workflows
mv build-manylinux.yml ../../old-workflows/
mv build-macos.yml ../../old-workflows/
mv build-windows.yml ../../old-workflows/
mv release.yml ../../old-workflows/

# Rename new workflows to replace old ones
mv build-manylinux-new.yml build-manylinux.yml
mv build-macos-new.yml build-macos.yml
mv build-windows-new.yml build-windows.yml
mv release-new.yml release.yml

# Commit the changes
git add .
git commit -m "refactor: migrate to new workflow structure

- Split CI and build workflows
- Removed duplicate Linux builds
- Unified naming conventions
- Added package version tracking
- Simplified manylinux (static only)
- Updated macOS to latest runners
"
```

---

### Phase 4: Clean Up Old Workflows

```bash
cd .github/workflows

# Delete replaced workflows
git rm build-debian-stable.yml
git rm build-in-linux-container.yml
git rm build-ubuntu-latest.yml

# Commit cleanup
git add .
git commit -m "chore: remove deprecated workflow files

Replaced by:
- build-debian-stable.yml → build-debian.yml
- build-in-linux-container.yml → build-debian.yml
- build-ubuntu-latest.yml → ci.yml
"
```

---

## Rollback Plan

If something goes wrong:

```bash
# Option 1: Rename files back
cd .github/workflows
mv build-manylinux.yml build-manylinux-new.yml
cp ../../old-workflows/build-manylinux.yml .
# etc.

# Option 2: Git revert
git revert <commit-hash>

# Option 3: Restore from git history
git checkout <old-commit> -- .github/workflows/
```

---

## Verification After Migration

### 1. Check CI triggers on every push
```bash
# Make a trivial change
echo "# test" >> README.md
git add README.md
git commit -m "test: verify ci.yml triggers"
git push
```
Expected: `ci.yml` runs automatically

### 2. Check release on tag
```bash
# Create release tag
git tag v1.0.0-rc1
git push origin v1.0.0-rc1
```
Expected: 
- `release.yml` triggers
- Calls all 4 build workflows
- Creates draft release with all artifacts

### 3. Verify artifact names match expectations
Check that artifacts follow pattern:
- `Cell2Fire.Debian.bookworm.x86_64`
- `Cell2Fire.Debian.trixie.x86_64`
- `Cell2Fire.Ubuntu.noble.x86_64`
- `Cell2Fire` (manylinux static)
- `Cell2Fire_Darwin.macos-15.arm64-dynamic`
- `Cell2Fire_Darwin.macos-15.arm64-static`
- `Cell2Fire_Darwin.macos-14-large.x86_64-dynamic`
- `Cell2Fire_Darwin.macos-14-large.x86_64-static`
- `Cell2Fire.exe` (Windows)

---

## TODO Before Going Live

### Must Do:
- [ ] Complete Windows dependency download logic in `build-windows-new.yml`
- [ ] Test `build-debian.yml` on actual GitHub Actions
- [ ] Test `build-manylinux-new.yml` on actual GitHub Actions
- [ ] Verify `release-new.yml` creates all expected zips
- [ ] Check that `pkgs*.txt` files are included in artifacts

### Nice to Have:
- [ ] Add `actions/cache` for apt packages in `build-debian.yml`
- [ ] Add badge to README.md showing CI status
- [ ] Update documentation to reference new workflows
- [ ] Consider adding workflow status checks to branch protection

### Windows-Specific:
The `build-windows-new.yml` has a skeleton but needs:
1. Complete dependency download script (Boost, Dirent, vcpkg)
2. Verify MSBuild project path
3. Add DLL collection logic
4. Complete test script

You can reference the old `build-windows.yml` (lines 100-194) for the complete implementation.

---

## Questions & Decisions

### Q: Should we keep old workflows during testing?
**A:** Yes, keep them until new workflows are verified working. Use `-new` suffix pattern.

### Q: What if a platform build fails?
**A:** Release workflow will fail too (needs all builds). You can:
- Fix the failing workflow
- Temporarily remove from `release.yml` needs
- Use `enabled: false` for Windows if needed

### Q: Can we run multiple workflows in parallel?
**A:** Yes! GitHub Actions runs independent workflows in parallel. Release workflow coordinates them.

### Q: Do we need to update branch protection rules?
**A:** Recommended - add `ci.yml` as required check for PRs to main.

---

## Reference: Old vs New Mapping

| Old Workflow                    | New Workflow           | Change                          |
|---------------------------------|------------------------|---------------------------------|
| build-ubuntu-latest.yml         | ci.yml                 | Fast CI check only              |
| build-debian-stable.yml         | build-debian.yml       | Container matrix (3 variants)   |
| build-in-linux-container.yml    | build-debian.yml       | Consolidated                    |
| build-manylinux.yml             | build-manylinux.yml    | Static only, removed do_dynamic |
| build-macos.yml                 | build-macos.yml        | Updated runners, 4 variants     |
| build-windows.yml               | build-windows.yml      | Cleaned up, optional flag       |
| release.yml                     | release.yml            | Calls new build workflows       |
| doxygen-gh-pages.yml            | (unchanged)            | Keep as-is                      |

---

## Support

If you encounter issues during migration:
1. Check workflow run logs in GitHub Actions tab
2. Compare against `build-local-containers.sh` output
3. Verify artifact naming conventions match
4. Review `docs/WORKFLOW_REFACTORING.md` for detailed explanations
