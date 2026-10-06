# Example Plugin 2.0.1

- Windows packages include both Release (`plugin/engine/`) and Debug (`plugin/engine-debug/`) binaries, so development Debug Engines can install the example too.
- The installer chooses the matching variant; Debug and Release C++ runtimes are not interchangeable.
- Engine and Interface minimum versions remain 2.0.0.

## Example Plugin 2.0.0

- Stable release for DARTWIC 2.0.0, with Engine and Interface minimums of 2.0.0.
- Bundles the matching 2.0.0 SDK and Engine Protocol sources with verified snapshot hashes.
- Core Engine and Interface downloads will be published separately.

## Included Changes

- Refreshed the bundled Engine and Interface SDKs, including scoped settings and shared asset helpers.
- Replaced the placeholder settings panel with a working run-label setting. Workspace/project writes retain the revision; reset removes only its JSON pointer.
- Added five focused settings tests and a self-contained storage/settings guide covering plugin settings, module settings, private files, and Model3D assets.
- Published the Windows x64 Release plugin, matching workspace, and standalone flight-peer downloads.

The new settings panel requires DARTWIC Engine and Interface 2.0.0 or newer.
Rocket simulation tasks do not call the new native SDK virtual methods. Hardware
and non-Windows integration were not tested for this release.
