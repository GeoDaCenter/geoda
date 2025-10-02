# GeoDa Release Workflow

This document explains how to use the automated release workflow for GeoDa.

## Overview

The release workflow automatically creates a GitHub release with all installer artifacts when a new version tag is pushed to the repository.

## How It Works

1. **Tag Creation**: When you push a new tag (e.g., `v1.22.0`), it triggers:

   - All three build workflows (Windows, macOS, Ubuntu)
   - The release creation workflow

2. **Build Process**: The build workflows create installer artifacts:

   - **Windows**: 32-bit and 64-bit installers (regular and Windows 7+ versions)
   - **macOS**: Intel (x86_64) and Apple Silicon (ARM64) DMG files
   - **Ubuntu**: DEB packages for Ubuntu 20.04, 22.04, and 24.04
   - **AppImage**: Portable Linux executable with embedded dependencies

3. **Release Creation**: The release workflow:
   - Waits for builds to complete
   - Downloads all artifacts
   - Creates a GitHub release with the tag
   - Uploads all installer files as release assets
   - Generates release notes with download links

## Creating a New Release

### Prerequisites

- All build workflows must be working correctly
- You must have push access to the repository
- The version number should be updated in the codebase

### Steps

1. **Update Version**: Make sure the version number is updated in the source code (typically in `version.h`)

2. **Create and Push Tag**:

   ```bash
   # Create a new tag
   git tag v1.22.0

   # Push the tag to trigger the release workflow
   git push origin v1.22.0
   ```

3. **Monitor Progress**:
   - Check the GitHub Actions tab to monitor build progress
   - The release workflow will wait for all builds to complete
   - Once complete, the release will be created automatically

### Expected Artifacts

The release will include the following installer files:

**Windows:**

- `GeoDa_1.22.0_x86_Setup.exe` (32-bit)
- `GeoDa_1.22.0_x64_Setup.exe` (64-bit)
- `GeoDa_1.22.0_win7+x86_Setup.exe` (Windows 7+ 32-bit)
- `GeoDa_1.22.0_win7+x64_Setup.exe` (Windows 7+ 64-bit)

**macOS:**

- `GeoDa1.22.0-x86_64-Installer.dmg` (Intel)
- `GeoDa1.22.0-arm64-Installer.dmg` (Apple Silicon)

**Ubuntu/Debian:**

- `geoda_1.22.0-1focal1_amd64.deb` (Ubuntu 20.04)
- `geoda_1.22.0-1jammy1_amd64.deb` (Ubuntu 22.04)
- `geoda_1.22.0-1noble1_amd64.deb` (Ubuntu 24.04)

## Workflow Files

- `.github/workflows/create_release.yml` - Main release workflow
- `.github/workflows/windows_build.yml` - Windows build workflow
- `.github/workflows/osx_build.yml` - macOS build workflow
- `.github/workflows/ubuntu_build.yml` - Ubuntu build workflow
- `.github/workflows/appimage-build.yml` - AppImage build workflow (portable Linux)

## Troubleshooting

### Build Failures

If any build workflow fails, the release workflow will still attempt to create a release with the available artifacts. Check the build logs to identify and fix issues.

### Missing Artifacts

If some artifacts are missing, the release will still be created with the available files. The workflow uses `continue-on-error: true` for each upload step.

### Timing Issues

The release workflow waits up to 2 hours for builds to complete, checking every minute. This should accommodate builds that take 30+ minutes. If builds take longer than 2 hours, you may need to:

1. Manually trigger the release workflow again
2. Increase the maximum wait time in the workflow file
3. Check if builds are stuck or failed

### Manual Release Creation

If the automated workflow fails, you can manually create a release:

1. Go to the GitHub repository
2. Click "Releases" → "Create a new release"
3. Select the tag
4. Download artifacts from the build workflows
5. Upload them manually as release assets

## Configuration

### Version Format

The workflow expects tags in the format `vX.Y.Z` (e.g., `v1.22.0`). The version number is extracted by removing the `v` prefix.

### Wait Time

The default maximum wait time is 2 hours (120 minutes). You can adjust this in the workflow file:

```yaml
local max_attempts=120 # 2 hours maximum (120 * 60 seconds)
```

### Release Notes

The release notes are automatically generated with:

- Download links for all platforms
- Installation instructions
- System requirements
- Link to changelog

## Security

The workflow uses the default `GITHUB_TOKEN` secret for authentication. This token has the necessary permissions to:

- Create releases
- Upload release assets
- Download workflow artifacts

No additional secrets are required for basic functionality.
