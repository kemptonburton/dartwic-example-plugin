# Example Plugin 2.0.0-beta.2

- Refreshed the bundled Engine and Interface SDKs, including scoped settings and shared asset helpers.
- Replaced the placeholder settings panel with a working run-label setting. Workspace/project writes retain the revision; reset removes only its JSON pointer.
- Added five focused settings tests and a self-contained storage/settings guide covering plugin settings, module settings, private files, and Model3D assets.
- Published the Windows x64 Release plugin, matching workspace, and standalone flight-peer downloads.

The new settings panel requires the updated storage-enabled development host. The
original public Engine/Interface beta.3 downloads do not supply the settings API.
Rocket simulation tasks do not call the new native SDK virtual methods. Hardware
and non-Windows integration were not tested for this release.
