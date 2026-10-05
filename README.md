# DARTWIC 2.0 Example Plugin

This is a standalone, public plugin starter and a complete combined Engine/Interface example. It does not require access to the private DARTWIC source repository.

Developers edit `plugin.json`, `engine/include`, `engine/src`, `interface/src`, and optional `files/`. The compatible Engine and Interface SDK snapshots are already bundled under `engine/include/sdk` and `interface/sdk`. They belong to this tagged example version and are not refreshed from a private checkout.

Engine registration happens in `ExampleDevicePlugin::onPluginLoaded()` through `dartwic->registerModuleType`, `registerPeer`, `registerTaskType`, `registerOperation`, `registerDCodeFunction`, and `registerLoop`. IDs are local and become `<plugin-id>.<local-id>`. `createModule` receives the local module ID. The `example_flight_link` peer supplies a `TEMPEST::Transport` and a peer configuration callback; the engine owns RAPID and ARGUS behavior. Its registration includes UI-editable default connection values. Its raw local ZeroMQ transport requires both `receive_endpoint` and `send_endpoint`. The matching Windows flight application in `examples/flight-peer/main.cpp` uses the reversed endpoints; select native or custom transport at startup.

The interface uses `definePlugin({register})`. `addModuleUi` demonstrates both a React icon and a module configuration panel. The module config falls back to `workspace/global_data/plugin_icons/example-device.svg`, which is supplied through `files/`.

Place every plugin-authored engine-root file under `files/`. This example includes a schematic-node JSON file at `files/workspace/global_data/schematic_nodes/device_examples/example_indicator.json`; packaging includes that tree and installation or local deployment mirrors it below the engine configuration root.

## Setup

Install the JavaScript dependency from this repository:

```shell
npm ci
```

`npm run build` and `npm run verify` need only Node.js. Native Engine builds additionally require CMake, a supported C++ toolchain, and a vcpkg checkout containing the dependencies in `vcpkg.json`. Set `VCPKG_ROOT` to that checkout before running packaging or deployment commands.

`npm run package` creates `plugin.zip` with `engine/`, `interface/`, and optional `files/`. `npm run package-debug` creates `plugin-debug.zip` with `engine-debug/`, `interface/`, and optional `files/`. Windows releases can attach `plugin.zip`, the mock rocket workspace, and `rocket-flight-peer-windows-x64.zip`. These binary packages let operators run the examples without a C++ toolchain.

Commands:

- `npm run build` bundles the interface plugin.
- `npm run verify` validates the standalone source, manifests, and bundled SDK snapshots from a clean clone.
- `npm run verify:sdk` verifies only the bundled SDK snapshots.
- `npm run package` builds, verifies, and creates the release `plugin.zip`.
- `npm run package-debug` builds, verifies, and creates `plugin-debug.zip`.
- `npm run deploy` builds both sides and copies them to your local DARTWIC installation.
- `npm run deploy-debug` does the same with the debug Engine plugin.

Deployment is optional and never assumes a DARTWIC source checkout. Set `DARTWIC_ENGINE_DIR` and `DARTWIC_INTERFACE_DIR`, or copy `deployment-settings.example.json` to the ignored `deployment-settings.json` file and configure your installation paths there.

## Mock rocket driver

`rocket_sim` models one shared device. `rocket_read` publishes measured values and applied outputs; `rocket_write` reads coil/igniter commands. `rocket_device_discovery` demonstrates the Modbus-style module-discovery UI contract with a simulated endpoint and presence lease. The small `example_device` driver stays available as an introductory example.

The [example workspace](https://github.com/kemptonburton/dartwic-example-workspace) is the matching, independently cloneable demo and the `workspaces/mock-rocket-test` submodule in DARTWIC. It has eleven operational walkthroughs, Lua DCode, YAML templates, and the rocket model asset. `rocket_read` and `rocket_write` expose direct device-field/state-channel binding tables. Engine/Interface 2.0.0-beta.3 are the tested minimum; its seven tasks require a suitable license.

The three `vendor/` source snapshots are part of `sdk-lock.json`. CMake consumes those bundled dependencies, so building the plugin and flight peer does not need the private monorepo.
