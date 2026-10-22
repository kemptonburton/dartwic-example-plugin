# DARTWIC 2.0 Example Plugin

The [stress device](docs/stress-device.md) supplies fixed register/channel binding tables and deterministic local I/O for the internal operator stress workspace.

This is a standalone, public plugin starter and a complete combined Engine/Interface example. It does not require access to the private DARTWIC source repository.

Developers edit `plugin.json`, `engine/include`, `engine/src`, `interface/src`, and bundled `assets/`. The compatible Engine and Interface SDK snapshots are already bundled under `engine/include/sdk` and `interface/sdk`. They belong to this tagged example version and are not refreshed from a private checkout.

Engine registration happens in `ExampleDevicePlugin::onPluginLoaded()` through `dartwic->registerModuleType`, `registerPeer`, `registerTaskType`, `registerOperation`, `registerDCodeFunction`, and `registerLoop`. IDs are local and become `<plugin-id>.<local-id>`. `createModule` receives the local module ID. The `example_flight_link` peer supplies a `TEMPEST::Transport` and a peer configuration callback; the engine owns RAPID and ARGUS behavior. Its registration includes UI-editable default connection values. Its raw local ZeroMQ transport requires both `receive_endpoint` and `send_endpoint`. The matching Windows flight application in `examples/flight-peer/main.cpp` uses the reversed endpoints; select native or custom transport at startup.

`analyze_samples` demonstrates the shared DCode function schema: its ordinary and prepared handlers return a sum and maximum, then change the first sample through the call-scoped mutable argument. DCode aliases observe that change when the synchronous call returns.

The interface uses `definePlugin({register})`. `addModuleUi` demonstrates both a React icon and a module configuration panel. The example module uses the standard module package icon; its config falls back to `assets/icons/example-device.svg`, which is bundled inside the installed plugin.

Bundle plugin-owned content under `assets/`. Schematic renderers register their palette definitions through the Interface SDK; custom shared files use `workspace/assets/plugins/<plugin-id>/`. Installation never copies assets into a workspace. Do not put mutable settings, credentials, runtime output, or caches in the installed plugin directory.

See [Storage and settings](docs/storage-and-settings.md) for the five location scopes, layered plugin/module settings, and shared asset examples. This release requires DARTWIC Engine and Interface 2.0.0 or newer. The example device read and write tasks use retained fixed channel handles.

The settings panel demonstrates **Decimal places**, **Show units**, and
**Resource title**. It saves changed fields automatically after 500 ms
through `createStorageClient`, with no Save button or location selector.
`interface/src/settings.mjs` handles defaults, validation, and project-scoped
storage; `settingsAutosave.mjs` coalesces field patches and serializes writes;
`useDisplaySettings.jsx` connects that queue to React and notifications. The
Example Telemetry resource updates its title and formats live values from the
example device read/write binding tables. These settings change presentation,
not device acquisition or control.

Pending edits flush when the panel closes. Each session captures its project
and uses the latest opaque revision, so a delayed write cannot target a new
project or silently overwrite a teammate's changes. Failed writes stay visibly
unsaved; Reload settings discards the draft and rereads the host. `npm test`
covers debounce, concurrent edits, project capture, validation, and recovery.

## Setup

Install the JavaScript dependency from this repository:

```shell
npm ci
```

`npm run build` and `npm run verify` need only Node.js. Native Engine builds additionally require CMake, a supported C++ toolchain, and a vcpkg checkout containing the dependencies in `vcpkg.json`. Set `VCPKG_ROOT` to that checkout before running packaging or deployment commands.

## GitHub release contract

Create one public GitHub release per plugin version. Attach these files directly as release assets:

```text
v2.0.0
  plugin.json                 # required shared manifest
  plugin-win32-x64.zip         # Windows x64; upload if supported
  plugin-linux-x64.zip         # Linux x64; upload if supported
  release-manifest.json        # optional checksums produced by the tooling
```

A plugin containing a native Engine side must upload at least one of the platform ZIPs above. An **Interface-only plugin** instead uploads **`plugin.zip`**, with the same root `plugin.json` and optional checksum receipt. Its manifest must explicitly set `contains_engine_plugin: false` and `contains_interface_plugin: true`. No separate Windows/Linux build is needed for a portable JavaScript bundle.

Filenames are exact and case-sensitive. Native Engine plugins cannot use `plugin.zip`; separate `plugin-debug.zip` assets are not supported. GitHub's automatically generated source ZIP/TAR is not an installable plugin package. Keep the same root `plugin.json` in the tagged source; browser hosts read it from that tag. The tag, package version, uploaded manifest, and manifests inside the ZIP must identify the same plugin version. For version `2.0.0`, use tag `v2.0.0`.

The combined example uses this complete manifest:

```json
{
  "id": "example_device_plugin",
  "name": "Example Device Plugin",
  "description": "A device driver and its operator tools.",
  "author": "Your team",
  "version": "2.0.0",
  "minEngineVersion": "2.0.0",
  "minInterfaceVersion": "2.0.0",
  "contains_engine_plugin": true,
  "contains_interface_plugin": true
}
```

Use a stable ID matching the runtime definition and every `<id>` directory. Both `contains_*` fields are booleans and at least one must be true. A native side requires `minEngineVersion`; an Interface side requires `minInterfaceVersion`. These are tested minimum host versions using SemVer ordering, independent of the plugin's version. A minimum of `2.0.0` rejects `2.0.0-beta.1`.

Every plugin ZIP, including an Interface-only `plugin.zip`, has exactly one top-level `plugin/` directory:

```text
plugin/
  engine/<id>/
    plugin.json
    bin/<id>.dll              # Windows Release; lib<id>.so on Linux
    bin/<runtime dependencies>
    ...                       # native configuration/default files
  engine-debug/<id>/
    plugin.json
    bin/<id>.dll              # Windows Debug; lib<id>.so on Linux
    bin/<id>.pdb              # Windows debug symbols
    bin/<debug runtime dependencies>
    ...
  interface/<id>/
    plugin.json
    ui/index.js
    ...                       # UI assets
  # Each installed plugin side includes its managed assets/.
```

Omit both native directories for Interface-only plugins; omit `interface/` for native-only plugins. All included `plugin.json` files must match the root release manifest. Native packages include **both Release and Debug** in the same platform ZIP. A Release engine installs `engine/`; a Debug engine installs `engine-debug/`. There is no separate debug asset to select. Bundle each configuration's required runtime libraries beside its native library; Windows Debug packages include the plugin PDB. Linux Debug libraries retain compiler debug information.

The Engine chooses its own OS and architecture from the platform download map, independently of the Interface machine. A Windows Interface connected to a Linux Engine downloads its local UI from the Windows ZIP and sends both download options to the Engine; the Engine chooses the Linux ZIP. Missing platforms produce an unavailable-download error, never a fallback to another OS. Keep the Interface bundle identical between platform ZIPs. An Interface-only plugin uses one portable `plugin.zip`. The installer offers it on every Interface host and the browser labels it **Any platform**. This exception is never applied to a plugin declaring an Engine side.

## Build and publish

```sh
npm ci
npm run verify
npm run package
```

For a plugin declaring an Engine side, run `package` on each native platform you support: Windows x64 creates `plugin-win32-x64.zip`; Linux x64 creates `plugin-linux-x64.zip`. It builds both native configurations and the Interface bundle, checks manifests, native OS/architecture, Debug output and Windows symbols, then writes the ZIP and SHA-256 receipt. A failure stops packaging; Debug is not best-effort. `package-debug` is an alias for the same complete package. For an Interface-only manifest, `package` builds the JavaScript bundle once and creates `plugin.zip`; it does not require a supported native build host. Deploy commands remain separate and never publish anything.

Native builds require Node.js, CMake, and vcpkg (`VCPKG_ROOT`), plus Visual Studio 2022 with ClangCL on Windows or Clang, Ninja, and `zip` on Linux. Linux builds need the development prerequisites for the dependencies listed in `vcpkg.json`. To reuse an existing vcpkg installation, optionally set `DARTWIC_VCPKG_INSTALLED_DIR` to its installed directory. Interface-only packages need Node.js and a ZIP utility, with no C++ toolchain or vcpkg.

Commit your source and matching manifests before creating the tag. Upload only the generated platform packages you actually built, plus the root manifest. For example, after collecting both platform builds:

```sh
gh release create v2.0.0 plugin.json plugin-win32-x64.zip plugin-linux-x64.zip --target main --title "2.0.0" --notes-file RELEASE_NOTES.md
```

For an Interface-only release, use the same process with just one ZIP:

```sh
gh release create v2.0.0 plugin.json plugin.zip --target main --title "2.0.0" --notes-file RELEASE_NOTES.md
```

Attach `release-manifest.json` as well when desired. Each platform build's receipt covers that build's assets; combine checksums when collecting several builds. Verify on a compatible host before claiming device, driver, or real-time support. Source builds and package checks alone do not qualify physical hardware.

## Local development

`npm run build` bundles the Interface. `npm run verify` validates source metadata and bundled SDK locks; `verify:package` checks built output. `npm run deploy` builds and copies to your local installation; `deploy-debug` selects the Debug engine and `deploy:interface` copies only UI. Set `DARTWIC_ENGINE_DIR` and `DARTWIC_INTERFACE_DIR`, or copy `deployment-settings.example.json` to the ignored `deployment-settings.json`. Deployment never assumes a private DARTWIC checkout.

## Example device driver

`example_device` is the single simulated module type. `example_read` and `example_write` share one instance and expose device-field to state-channel binding tables. The read task publishes measurements, connection and validity state, applied output readback, and measured valve positions. The write task consumes mapped command channels. Both retain fixed channel handles and buffers after configuration; the simulated device uses a module-local lock with priority inheritance on Linux. `example_device_1_discovery` demonstrates the Modbus-style provisioning flow with a simulated endpoint and presence lease. It recommends one module and the two tasks, with editable previews of all 22 read and nine write bindings. The shared Configure step groups its five fixed simulation inputs with the Read task bindings. Review shows task names and counts. Module-owned resources and task-owned resources use the shared `DiscoveryResources` SDK descriptor, without a plugin-specific setup component. The rocket-themed workspace keeps its existing channel names as one application of this generic example device.

The matching workspace uses a periodic DCode task for ox-tank physics and a separate state machine for fill and vent control. The device read task consumes the ox-tank level as a fixed input for the engine sensor response and exclusively publishes the module's connection channel. It refreshes its measurements and applied output readback every cycle with a one-second stale timeout; the physics task does the same for tank measurements. Discovery only advertises an available device for provisioning; it does not write device readback channels. The module page shows both linked tasks and their live bindings. When a task omits its mapping table, the module page and Example Telemetry resolve the same prefix-based defaults as the task editor and native driver (22 read bindings and nine write bindings). An explicitly empty table stays empty; custom mappings replace the defaults.

The [example workspace](https://github.com/kemptonburton/dartwic-example-workspace) is the matching, independently cloneable demo and the `tests/workspaces/dartwic-example-workspace` submodule in DARTWIC. Add its `example-workspace/` folder in the Interface; the sibling `flight-computer/`, `tools/`, and `walkthroughs/` folders are source and operating aids. It has eleven operational walkthroughs, Lua DCode, YAML templates, and the rocket model asset. Its `example_device_read` and `example_device_write` task instances use the single example device driver. The matching workspace requires Engine and Interface 2.0.0 or newer; its seven tasks require a suitable license.

The three `vendor/` source snapshots are part of `sdk-lock.json`. CMake consumes those bundled dependencies, so building the plugin and flight peer does not need the private monorepo.

## Asset ownership

Managed files in this repository's `assets/` ship inside the installed plugin and
are replaced with it. Keep custom shared files in
`workspace/assets/plugins/<plugin-id>/` using the SDK helpers. Project settings
remain project-scoped; local secrets and runtime data belong to Engine instances.
Workspaces contain authored Git-trackable content; installed Engine and Interface
directories contain replaceable software and local state. Plugin installation
does not copy files into workspaces. See `assets/README.md` for the storage rules.
