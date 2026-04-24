# Git Hooks Configuration

This document describes the Git hooks configured for the project to enforce commit message standards and code quality.

**Note:** This file contains TODO comments for documentation purposes only.

## 📋 Available Hooks

### 1. **pre-commit** - Pre-commit checks
**Purpose:** Run basic checks before allowing a commit
**Checks:**
- No large files (>10MB)
- No .pyc files
- No Jupyter checkpoint directories
- Python syntax validation
- requirements.txt format check
- TODO/FIXME comments warning

### 2. **prepare-commit-msg** - Commit message template
**Purpose:** Automatically add commit message template
**Behavior:**
- Adds template to empty commit messages
- Shows format guidelines
- Provides examples

### 3. **commit-msg** - Commit message validation
**Purpose:** Enforce commit message format
**Validation Rules:**
- Format: `<type>(<scope>): <subject>`
1. Type validation: feat, fix, docs, style, refactor, perf, test, chore, ci, build
2. Scope validation: lesson, notebook, docs, assets, build, ci, deps (or empty)
3. Subject length: ≤ 50 characters
4. Subject starts with lowercase
5. Subject doesn't end with period
6. Body lines: ≤ 72 characters (if provided)

## 🎯 Commit Message Template (.gitmessage)

The project uses a commit message template that is:
1. Automatically added by `prepare-commit-msg` hook
2. Configured via `git config commit.template .gitmessage`
3. Validated by `commit-msg` hook

## 🔧 Installation

Hooks are automatically installed in `.git/hooks/` directory. To verify:

```bash
# Check hooks are executable
ls -la .git/hooks/

# Check commit template configuration
git config --get commit.template
```

## 🚀 Testing the Hooks

### Test valid commit messages:
```bash
# Valid examples
echo "feat(lesson): Add lesson 2 - Video color space" | git commit --allow-empty -F -
echo "docs: Update course development guide" | git commit --allow-empty -F -
echo "chore(assets): Add test video for lesson 2" | git commit --allow-empty -F -
```

### Test invalid commit messages (should fail):
```bash
# Invalid type
echo "feature: Add new feature" | git commit --allow-empty -F -

# Invalid scope
echo "feat(wrongscope): Add something" | git commit --allow-empty -F -

# Subject too long
echo "feat(lesson): This subject is way too long and exceeds the fifty character limit" | git commit --allow-empty -F -

# Subject starts with uppercase
echo "feat(lesson): Add new lesson" | git commit --allow-empty -F -

# Subject ends with period
echo "feat(lesson): Add new lesson." | git commit --allow-empty -F -
```

## 📝 Examples for This Project

### New Course Lesson
```
feat(lesson): Add lesson 2 - Video color space

- Implement RGB to YUV420P conversion
- Add YUV component separation experiment
- Include 3 challenge tasks

Closes #2
```

### Documentation Update
```
docs: Update course development guide

- Add memory management best practices
- Update color space conversion notes
- Fix typos in lesson templates
```

### Bug Fix
```
fix(notebook): Fix color display issue in lesson 1

- Correct BGR to RGB conversion
- Update matplotlib display settings
- Add color space conversion notes

Fixes #15
```

### Test Assets
```
chore(assets): Add test video for lesson 2

- Download 5-second 360p test video
- Verify video format and size
- Update asset download script
```

## 🔄 Bypassing Hooks (if needed)

In rare cases, you might need to bypass hooks:

```bash
# Skip all hooks
git commit --no-verify -m "Emergency fix"

# Skip specific hook
git commit --no-verify -m "docs: Update README"
```

**Warning:** Only bypass hooks when absolutely necessary, and document the reason.

## 🛠️ Troubleshooting

### Hook not running
```bash
# Check if hooks are executable
chmod +x .git/hooks/*

# Check git config
git config --get commit.template
```

### Hook failing unexpectedly
```bash
# Run hook manually to see error
.git/hooks/pre-commit
.git/hooks/commit-msg .git/COMMIT_EDITMSG
```

### Disable hooks temporarily
```bash
# Rename hooks to disable
mv .git/hooks/pre-commit .git/hooks/pre-commit.disabled
mv .git/hooks/commit-msg .git/hooks/commit-msg.disabled

# Re-enable hooks
mv .git/hooks/pre-commit.disabled .git/hooks/pre-commit
mv .git/hooks/commit-msg.disabled .git/hooks/commit-msg
```

## 📚 Related Documentation

- [CONTRIBUTING.md](./CONTRIBUTING.md): Detailed commit message guidelines
- [.gitmessage](./.gitmessage): Commit message template
- [COURSE_BLUEPRINT.md](./COURSE_BLUEPRINT.md): Course development standards

---

**Last Updated:** 2026-04-24  
**Maintainer:** opencode
