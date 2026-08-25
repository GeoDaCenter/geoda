# Apple Developer Notarization Setup

This document explains how to set up Apple Developer notarization for the GeoDa macOS builds in GitHub Actions.

## Overview

The GitHub Actions workflow now includes automatic notarization of the DMG installer before uploading artifacts. This ensures that macOS users can install GeoDa without encountering Gatekeeper warnings.

## Required GitHub Secrets

To enable notarization, you need to configure the following secrets in your GitHub repository:

### 1. APPLE_ID_USERNAME

- **Description**: Your Apple ID email address
- **Example**: `developer@example.com`
- **Required**: Yes

### 2. APPLE_ID_APP_SPECIFIC_PASSWORD

- **Description**: App-specific password for your Apple ID
- **How to create**:
  1. Go to https://appleid.apple.com
  2. Sign in with your Apple ID
  3. Go to "Security" → "App-Specific Passwords"
  4. Click "Generate Password"
  5. Give it a name like "GeoDa Notarization"
  6. Copy the generated password
- **Required**: Yes

### 3. APPLE_TEAM_ID

- **Description**: Your Apple Developer Team ID
- **How to find**:
  1. Go to https://developer.apple.com/account
  2. Sign in with your Apple ID
  3. Look for "Team ID" in the top right corner
  4. It's a 10-character string like `ABC123DEF4`
- **Required**: Yes

## Setting up GitHub Secrets

1. Go to your GitHub repository
2. Click on "Settings" → "Secrets and variables" → "Actions"
3. Click "New repository secret" for each of the three secrets above
4. Enter the secret name and value
5. Click "Add secret"

## How it Works

The workflow will:

1. Build GeoDa and create the DMG installer
2. Code sign the DMG with the Developer ID certificate
3. Submit the DMG to Apple for notarization using `xcrun notarytool`
4. Wait for notarization to complete
5. Staple the notarization ticket to the DMG using `xcrun stapler staple`
6. Verify the notarization was successful
7. Upload the notarized DMG as an artifact

## Fallback Behavior

If the Apple ID credentials are not configured (secrets are missing), the workflow will:

- Skip the notarization step
- Continue with uploading the code-signed (but not notarized) DMG
- Log a message indicating that notarization was skipped

## Troubleshooting

### Common Issues

1. **"Apple ID credentials not available"**

   - Check that all three secrets are properly configured
   - Verify the Apple ID username and password are correct

2. **Notarization fails**

   - Check that your Apple Developer account has notarization permissions
   - Verify the app-specific password is valid and not expired
   - Check that the Team ID is correct

3. **Stapling fails**
   - This usually means the notarization didn't complete successfully
   - Check the notarization logs for specific error messages

### Manual Notarization

If you need to notarize manually, you can use the existing script:

```bash
cd BuildTools/macosx
./notarize-app.sh <version>
```

## Security Notes

- App-specific passwords are more secure than using your main Apple ID password
- The secrets are encrypted and only accessible during workflow runs
- Never commit Apple ID credentials to the repository

## References

- [Apple Developer Notarization Guide](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
- [GitHub Actions Secrets Documentation](https://docs.github.com/en/actions/security-guides/encrypted-secrets)
