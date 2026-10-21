# DARTWIC 2.0 Example Plugin

The [stress device](docs/stress-device.md) supplies fixed register/channel binding tables and deterministic local I/O for the internal operator stress workspace.

This is a standalone, public plugin starter and a complete combined Engine/Interface example. It does not require access to the private DARTWIC source repository.

Developers edit `plugin.json`, `engine/include`, `engine/src`, `interface/src`, and optional `files/`. The compatible Engine and Interface SDK snapshots are already bundled under `engine/include/sdk` and `interface/sdk`. They belong to this tagged example version and are not refreshed from a private checkout.

Engine registration happens in `ExampleDevicePlugin::onPluginLoaded()` through `dartwic->registerModuleType`, `registerPeer`, `registerTaskType`, `registerOperation`, `registerDCodeFunction`, and `registerLoop`. IDs are local and become `<plugin-id>.<local-id>`. `createModule` receives the local module ID. The `example_flight_link` peer supplies a `TEMPEST::Transport` and a peer configuration callback; the engine owns RAPID and ARGUS behavior. Its registration includes UI-editable default connection values. Its raw local ZeroMQ transport requires both `receive_endpoint` and `send_endpoint`. The matching Windows flight application in `examples/flight-peer/main.cpp` uses the reversed endpoints; select native or custom transport at startup.

`analyze_samples` demonstrates the shared DCode function schema: its ordinary and prepared handlers return a sum and maximum, then change the first sample through the call-scoped mutable argument. DCode aliases observe that change when the synchronous call returns.

The interface uses `definePlugin({register})`. `addModuleUi` demonstrates both a React icon and a module configuration panel. The example module uses the standard module package icon; its config falls back to `workspace/global_data/plugin_icons/example-device.svg`, which is supplied through `files/`.

Place portable plugin-supplied workspace content under `files/workspace/`. This example includes a schematic-node JSON file at `files/workspace/global_data/schematic_nodes/device_examples/example_indicator.json`; installation copies that content into the selected workspace. Do not put mutable settings, credentials, runtime output, or caches in the installed plugin directory.

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

`npm run package` creates `plugin.zip` with both `engine/` and `engine-debug/`, `interface/`, and optional `files/`. `npm run package-debug` creates `plugin-debug.zip` with `engine-debug/`, `interface/`, and optional `files/`. Windows releases can attach `plugin.zip`, the mock rocket workspace, and `rocket-flight-peer-windows-x64.zip`. These binary packages let operators run the examples without a C++ toolchain.

Commands:

- `npm run build` bundles the interface plugin.
- `npm run verify` validates the standalone source, manifests, and bundled SDK snapshots from a clean clone.
- `npm run verify:sdk` verifies only the bundled SDK snapshots.
- `npm run package` builds, verifies, and creates the release `plugin.zip`.
- `npm run package-debug` builds, verifies, and creates `plugin-debug.zip`.
- `npm run deploy` builds both sides and copies them to your local DARTWIC installation.
- `npm run deploy-debug` does the same with the debug Engine plugin.

Deployment is optional and never assumes a DARTWIC source checkout. Set `DARTWIC_ENGINE_DIR` and `DARTWIC_INTERFACE_DIR`, or copy `deployment-settings.example.json` to the ignored `deployment-settings.json` file and configure your installation paths there.

## Example device driver

`example_device` is the single simulated module type. `example_read` and `example_write` share one instance and expose device-field to state-channel binding tables. The read task publishes measurements, connection and validity state, applied output readback, and measured valve positions. The write task consumes mapped command channels. Both retain fixed channel handles and buffers after configuration; the simulated device uses a module-local lock with priority inheritance on Linux. `example_device_1_discovery` demonstrates the Modbus-style provisioning flow with a simulated endpoint and presence lease. It recommends one module and the two tasks, with editable previews of all 22 read and nine write bindings. The shared Configure step groups its five fixed simulation inputs with the Read task bindings. Review shows task names and counts. Module-owned resources and task-owned resources use the shared `DiscoveryResources` SDK descriptor, without a plugin-specific setup component. The rocket-themed workspace keeps its existing channel names as one application of this generic example device.

The matching workspace uses a periodic DCode task for ox-tank physics and a separate state machine for fill and vent control. The device read task consumes the ox-tank level as a fixed input for the engine sensor response and exclusively publishes the module's connection channel. It refreshes its measurements and applied output readback every cycle with a one-second stale timeout; the physics task does the same for tank measurements. Discovery only advertises an available device for provisioning; it does not write device readback channels. The module page shows both linked tasks and their live bindings. When a task omits its mapping table, the module page and Example Telemetry resolve the same prefix-based defaults as the task editor and native driver (22 read bindings and nine write bindings). An explicitly empty table stays empty; custom mappings replace the defaults.

The [example workspace](https://github.com/kemptonburton/dartwic-example-workspace) is the matching, independently cloneable demo and the `workspaces/dartwic-example-workspace` submodule in DARTWIC. Add its `example-workspace/` folder in the Interface; the sibling `flight-computer/`, `tools/`, and `walkthroughs/` folders are source and operating aids. It has eleven operational walkthroughs, Lua DCode, YAML templates, and the rocket model asset. Its `example_device_read` and `example_device_write` task instances use the single example device driver. The matching workspace requires Engine and Interface 2.0.0 or newer; its seven tasks require a suitable license.

The three `vendor/` source snapshots are part of `sdk-lock.json`. CMake consumes those bundled dependencies, so building the plugin and flight peer does not need the private monorepo.
