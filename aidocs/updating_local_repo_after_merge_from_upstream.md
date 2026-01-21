# Updating Local Repo After Merging from Upstream

This guide explains how to properly update your local repository after syncing your fork with the upstream repository, particularly when dealing with git submodules.

## The Problem

When you:
1. Sync your fork on GitHub with the original (upstream) repository
2. Run `git pull` locally to get the upstream changes

The `git pull` updates the **reference** to the submodule commit in your parent repository, but it does **NOT** automatically update the actual submodule directory contents.

### How to Identify This Issue

In Android Studio (or other Git clients), you'll see the submodule directory (e.g., `submodules/rive-runtime`) marked as "modified" even though you haven't changed any files inside it.

In the diff view, you'll see something like:
```
afd7b06a -> 95752737d  (Your version -> 60ddaf6d)
```

You can verify this on the command line:
```bash
# Check submodule status - a '+' prefix indicates mismatch
git submodule status
# Output: +60ddaf6d... submodules/rive-runtime (the '+' means out of sync)

# See the exact difference
git diff submodules/rive-runtime
# Shows: expected commit vs current commit
```

## The Solution

### Update Submodule to Match Parent Repository

Run this command to update the submodule to the commit that the parent repository expects:

```bash
git submodule update --init
```

This command will:
1. Check out the correct commit in the submodule directory
2. Clear the "modified" status
3. Ensure your submodule matches what upstream expects

### Full Workflow After Syncing Fork

Here's the complete workflow when syncing your fork with upstream:

```bash
# 1. On GitHub: Sync your fork with upstream (via GitHub UI or gh CLI)

# 2. Pull the changes locally
git pull

# 3. Update submodules to match the new references
git submodule update --init

# 4. Verify everything is in sync
git submodule status
# Should show the commit hash WITHOUT a '+' prefix
```

## Alternative: Recursive Submodule Update

If your project has nested submodules (submodules within submodules), use:

```bash
git submodule update --init --recursive
```

## Troubleshooting

### Submodule directory is empty
If the submodule directory is empty, the `--init` flag will initialize it:
```bash
git submodule update --init
```

### Changes in submodule you want to discard
If you have local changes in the submodule that you want to discard:
```bash
cd submodules/rive-runtime
git checkout .
git clean -fd
cd ../..
git submodule update --init
```

### Submodule URL changed
If the submodule URL has changed in upstream:
```bash
git submodule sync
git submodule update --init