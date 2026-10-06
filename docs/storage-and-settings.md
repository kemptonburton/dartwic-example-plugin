---
summary: Choose where settings, private device configuration, runtime files, and shared assets belong.
updated: 2026-10-05T16:11
created: 2026-10-05T15:38
---

# Storage and Settings

The public [example plugin](https://github.com/kemptonburton/dartwic-example-plugin)
includes a working autosaved settings panel: **Decimal places** controls numeric
precision and **Show units** controls unit suffixes. Both affect the sample
telemetry preview and the Example Telemetry resource when it opens. They do not
change device sampling or commands. There is no Save button or location selector:
the plugin chooses project scope because this example configures a shared
project resource, not the operator's personal Interface preferences.

The matching [example workspace](https://github.com/kemptonburton/dartwic-example-workspace)
contains workspace formatting defaults and a Model3D schematic that references a shared
model asset. These examples require DARTWIC Engine and Interface 2.0.0 or newer.
Core 2.0.0 packages will be published separately from the plugin release.

Start with one question: **should another computer get this file when it clones the workspace?**

If it describes the experiment, equipment setup, or shared operator policy, usually yes. If it contains a password, the last recorded value, or a compiled library, usually no. A workspace being tracked in Git does not make every setting a workspace setting.

## Five location scopes

In these examples, the installation is `engine/`, the instance is `engine/instances/primary/`, the workspace is `workspace/`, and the project is `default`. Real installations can use other directory names.

| Location | What it belongs to | Examples | Git |
| --- | --- | --- | --- |
| Installation | The installed application shared by engine instances | Executable, installed plugins, bundled SDK, updater files | Distribute as an installation, not a project |
| Instance | One engine on one computer | Server ports, admin password, device credentials, local recordings | Keep private; do not include in workspace Git |
| Workspace | All projects in one workspace | Node identity, shared assets, default diagnostic and recording policy | Track |
| Project | One experiment or setup | Tasks, modules, scripts, schematics, project policy overrides | Track |
| Interface | One browser or client | Theme, panel layout, local toast mutes | Browser/client storage; not workspace Git |

Instance files have three useful categories. These are directories, not three additional location scopes:

| Directory | Meaning | Example |
| --- | --- | --- |
| `settings/<project>/` | Durable local configuration that cannot travel with the project | TEMPEST passwords; a device-specific private configuration file |
| `runtime/<project>/` | Durable operational data | Recordings, ARGUS logs, resume values, dataframe state |
| `cache/<project>/` | Files the engine can generate again | Native DCode build output, temporary export parts, resolved recording configuration |

Do not delete `runtime/` to clear a cache. It may contain the only copy of recorded data. Likewise, `settings/` is not a disposable cache. Ignoring a file in Git does not encrypt it or remove secrets already committed to repository history.

## What goes where

This is the storage inventory for the engine and workspace, grouped by the kind of data rather than by file extension.

| Data | Location and current path | Reason |
| --- | --- | --- |
| Engine executable, libraries, installed engine plugins | Installation: `engine/`, `engine/plugins/` | Shared installed code |
| Installed interface plugin bundles | Interface installation: `interface/plugins/` | Installed code, not an operator preference |
| Bundled SDK, development/export tooling | Installation: `engine/sdk/`, driver tooling | Versioned with the host |
| Update staging, status, recovery plans, installation use locks | Installation: `engine/.dartwic/` | Coordinates changes to the shared installation |
| Listen address/port, admin password, viewer policy, active workspace/project, startup benchmark switches | Instance: `config.json` | This process's deployment and startup behavior |
| License and activation state | Instance: `license.json`, `.dartwic-license-state.json` | Private machine/instance state |
| Project rename transaction journal | Instance: `pending_project_rename.json` | Recovery for an interrupted local operation |
| Workspace node identity | Workspace: `workspace.json` | Portable engine name; project membership is discovered from project directories |
| Project dependencies | Project: `project.json` | Required plugins and explicitly declared files |
| Diagnostic event enablement, limits and warning rules | Workspace: `global_data/settings.json`; optional project `settings.json` | Shared policy, with experiment-specific overrides |
| Recording queue/options and history retention policy | Same settings layers: `recording.options`, `recording.retention` | Portable recording policy |
| Engine-origin notification mutes | Same settings layers: `notifications.mutes` | Shared operator choice, not an individual browser preference |
| Interface-origin notification mutes | Interface: browser/client storage | A local presentation choice |
| Client display profiles | Workspace: `global_data/client_info.json` | Shared display metadata; this does not grant admin access |
| Reusable schematic definitions, plugin icons | Workspace: `global_data/schematic_nodes/`, `global_data/plugin_icons/` | Shared authored resources |
| Reusable images, media packages and 3D models | Workspace: `global_data/assets/<namespace>/<filename>` | One asset can be referenced by several projects |
| Project tasks, module instances, scripts and schematics | Project: `tasks/`, `modules/`, `scripts/`, `schematics/` | Authored experiment setup |
| Channel metadata and declared startup values | Project: `rapid/channel_snapshot.json` | Portable channel configuration, not last live values |
| Remembered peer endpoints | Project: `edge_node_connections.json` | Portable connection definitions |
| Remembered peer credentials | Instance: `settings/<project>/tempest_peers/connections.json` | Credentials belong to this engine |
| Saved peer command definitions | Project: `command_presets.json` | Portable pinned operations |
| Private pinned-operation arguments | Instance: `settings/<project>/command_presets.json` | Passwords and declared secret fields cannot travel with presets |
| Channel recordings and resume state | Instance: `runtime/<project>/rapid/` | Operational data owned by this engine |
| ARGUS event database and log files | Instance: `runtime/<project>/argus/` | Operational history |
| Dataframe registry and maintenance results | Instance: `runtime/<project>/dataframes.json` | Refers to this engine's recordings |
| Measured startup benchmark reports | Instance: `runtime/<project>/diagnostics/startup_benchmarks.json` | Evidence from that startup, not a substitute for current measurements |
| Project/channel action audit records | Instance: `runtime/<project>/actions/` | Records actions, not authored project configuration |
| Completed exports | Instance: `runtime/<project>/rapid/exports/` | Preserve results until deliberately removed |
| Export staging parts | Instance: `cache/<project>/exports/` | Temporary generation work |
| Native DCode source generation, build trees and libraries | Instance: `cache/<project>/dcode/` | Rebuild from project scripts |
| Resolved RAPID recording config | Instance: `cache/<project>/recording_config.json` | Materialized from effective settings; not another source of truth |
| Theme, editor preferences, panel geometry and local notification state | Interface: browser/client storage | Should not change a teammate's interface |
| Local engine registry and remembered launch credentials | Interface host: local application-data `DARTWIC/local-engines.json` | Private launch configuration, not workspace policy |
| Live subscriptions, jobs and connection counters | Memory | A session is not a portable setting |

`project.json` is not a list of everything in a project. `schema_version` identifies its document format. `required_files` is optional: use it when a resource is a real dependency that the portability check should verify. An empty list is unnecessary. Do not add settings or runtime data there just because it is JSON.

## Defaults and overrides

Portable settings use this order:

```text
code defaults -> workspace/global_data/settings.json -> workspace/<project>/settings.json
```

Only store intentional overrides. For example, the workspace can enable diagnostic warnings, while a simulation project disables them without changing the hardware project:

```json
{
  "schema_version": 1,
  "diagnostics": {"warnings_enabled": false},
  "plugins": {"my_driver": {"discovery_interval_ms": 2000}}
}
```

Objects merge recursively. Arrays replace the entire array. `false`, `0`, an empty string, and `null` are explicit values, not requests to inherit. Core sections validate their types; a plugin must validate its own settings before using them.

Diagnostic warning rules are mode-aware: an `outside_range` rule uses `lower` and `upper`; an `above` or `below` rule uses `limit`. Fields belonging to the other mode are excluded from the effective rule. Recording options apply when RAPID starts; changing them requires an engine restart. Diagnostic policy is refreshed by the sampler, and retention policy is refreshed before maintenance and when read.

Notification mutes accumulate across workspace and project: a mute in either layer suppresses that notification. Removing only a project mute does not unmute a notification still muted at workspace scope.

To inherit again, remove the property using a reset JSON pointer. Resetting `/plugins/my_driver/discovery_interval_ms` at project scope leaves the workspace value in place. Resetting it at workspace scope falls back to the code default, unless the project has its own override. Resetting a section removes that scope's entire section.

Settings reads return `effective`, `workspace`, `project`, `sources`, and `revision`. `sources` maps leaf JSON pointers to `default`, `workspace`, or `project`. Treat `revision` as an opaque token: include the last one when saving to reject a stale edit, then reload on conflict. Read again with your defaults after saving; write responses do not include the defaults supplied by an earlier read.

## Add a plugin setting

An interface plugin can use the operation function from `useDartwic()`. The same helper works with an operation function passed into a plugin component.

```js
import {createStorageClient} from '@dartwic/interface-sdk/storage';

const storage = createStorageClient(operation);
const project = 'default'; // Capture the selected project before asynchronous work.
const defaults = {plugins: {my_driver: {discovery_interval_ms: 1000}}};
const snapshot = await storage.readEffectiveSettings(defaults, project);
const interval = snapshot.effective.plugins.my_driver.discovery_interval_ms;

await storage.writeSettingsOverride('project', {
    plugins: {my_driver: {discovery_interval_ms: 2000}},
}, {project, revision: snapshot.revision});

const saved = await storage.readEffectiveSettings(defaults, project);
await storage.resetSettingsOverride('project', [
    '/plugins/my_driver/discovery_interval_ms',
], {project, revision: saved.revision});
```

Use workspace scope for a default shared by all projects. Use project scope for a deliberate exception. Send the field that changed, not a copy of every effective value, or you will accidentally turn inherited defaults into permanent overrides. If a plugin ID contains `/` or `~`, escape JSON pointer tokens as `~1` and `~0` when resetting them.

An engine plugin has matching helpers:

```cpp
using Json = nlohmann::json;
const Json defaults = {{"plugins", {{"my_driver", {{"discovery_interval_ms", 1000}}}}}};
const auto snapshot = api.readEffectiveSettings(defaults, "default");
const auto interval = snapshot.at("effective").at("plugins")
    .at("my_driver").at("discovery_interval_ms").get<int>();
api.writeSettingsOverride("workspace", {
    {"plugins", {{"my_driver", {{"discovery_interval_ms", 2000}}}}}
});
api.resetSettingsOverride("project", Json::array({
    "/plugins/my_driver/discovery_interval_ms"
}), "default");
```

The host overlays `plugins.<plugin_id>` on the configuration passed to the plugin factory when loading it. That does not rewrite the installed `plugin.json`. A running plugin must explicitly reread settings and safely apply changes, or require an engine restart. Do not read files or parse JSON in a high-frequency task loop.

## Build an autosaved settings panel

The example plugin uses three small layers, all in its public source:

| File | Responsibility |
| --- | --- |
| `interface/src/settings.mjs` | Defaults, field validation, project capture, storage helper calls |
| `interface/src/settingsAutosave.mjs` | Debounce and a serialized queue of changed fields |
| `interface/src/useDisplaySettings.jsx` | React lifecycle, revision tracking, save state, and SDK notifications |

The sample settings are stored below your plugin ID:

```json
{
  "plugins": {
    "example_device_plugin": {
      "display": {"decimal_places": 3, "show_units": true}
    }
  }
}
```

The code default is two decimal places. The example workspace intentionally
sets three in `global_data/settings.json`. Changing the panel to four writes
only `decimal_places: 4` in the selected project's `settings.json`. It does
not copy `show_units` or unrelated effective settings into that override.
There is no plugin-specific settings file, localStorage copy, or manually built
absolute path. Resetting the two project pointers with
`resetSettingsOverride('project', pointers, {project, revision})` reveals the
workspace values again.

Capture the active project before loading settings or scheduling writes. The
example reads `active_project_name` through `dartwic/engine/get-config`, then
passes that same name into every storage helper call. A delayed save remains
attached to that session even if the active project changes.

The important persistence pattern is:

```js
const storage = createStorageClient(operation);
const initial = await storage.readEffectiveSettings(defaults, project);
let revision = initial.revision;

// createSettingsAutosave is example-plugin code, not a public SDK export.
const autosave = createSettingsAutosave(async changedFields => {
    const saved = await storage.writeSettingsOverride('project', {
        plugins: {example_device_plugin: {display: changedFields}},
    }, {project, revision});
    revision = saved.revision;
});

// A control updates its draft immediately and queues only its changed field.
autosave.edit({decimal_places: 4});
autosave.edit({show_units: false});
// Flush on blur or panel cleanup; there is no Save button.
await autosave.flush();
```

The queue waits 500 ms after the last edit. Several edits merge into one patch;
changing the same field repeatedly keeps its newest value. A write that is
already running finishes before another starts. The next write uses the
revision returned by the previous one. Save responses must not replace the
current React draft: the user may have typed again while that request was in
flight. Write responses omit code defaults, so the example's display reader
fills missing known fields from its defaults while preserving `0` and `false`.

Validate before queuing. Decimal places must be an integer from zero to six;
show-units must be a boolean. An empty or out-of-range numeric draft stays in the
input with an error and removes only its queued precision field. A valid queued
toggle is still saved. Do not turn an empty input into zero with `Number('')`.

The panel shows Pending, Saving, Saved, or Not saved. A failed patch stays
pending, and the SDK notification client reports the failure even if the panel
has closed. A revision conflict is not silently retried with a fresh token:
Reload settings deliberately discards the unsaved draft and reads the host's
current values. A disconnect or permission failure is also an error, not a
successful save.

Use this pattern for low-frequency configuration: display formatting for a
shared resource, a plugin's discovery interval, or a default timeout. Module
sample rates and channel bindings usually belong in module resources instead.
Device commands must use their command operation, not a debounced settings
write. Personal themes and editor preferences stay in Interface storage.

## Module settings are usually project resources

A module's baud rate, channel mappings, sample rate, or operating limits normally belong in its project `modules/<instance>.json`, under `parameters`. Use the existing module editor and `BaseModule::getParameter<T>()`; creating another settings store for the same values would introduce two competing sources of truth.

Use plugin policy for defaults that apply to the plugin as a whole. Use instance settings only for private or machine-specific values. For example, a module can retain its portable endpoint in its module resource while loading a private token separately:

```cpp
const auto privateConfig = api.readStorageJson(
    "settings", "my_driver/device_credentials.json", Json::object(), "default");
const auto token = privateConfig.value("token", std::string{});
if (token.empty()) throw std::runtime_error("Configure this engine's device token.");
```

To save that private file, call `api.writeStorageJson("settings", "my_driver/device_credentials.json", value, "default")`. This is plain local JSON, not a credential vault. Keep it out of Git and restrict access at the operating-system level. The generic file helpers do not automatically detect secrets in arbitrary plugin files.

Remembered peer connections and pinned commands do split recognized credential fields, preserving null placeholders in their portable definitions. Declare `secret_fields` JSON pointers for secrets whose names are not recognizable. A clean clone must supply those values on the receiving instance before connecting or running the command.

## Save ordinary files

The helpers are optional. They select a root and check that a relative path stays inside it; they do not force a plugin into one registered folder or provide a native-code sandbox.

| C++ helper | Interface helper | Selected root |
| --- | --- | --- |
| `installationPath(path)` | `installationPath(path)` | Shared engine installation |
| `instancePath(path)` | `instancePath(path)` | Current engine instance |
| `workspacePath(path)` | `workspacePath(path)` | Workspace |
| `projectPath(path, project)` | `projectPath(path, project)` | Named project; omitted project means active project |
| `projectSettingsPath(path, project)` | `projectSettingsPath(path, project)` | Instance `settings/<project>/` |
| `projectRuntimePath(path, project)` | `projectRuntimePath(path, project)` | Instance `runtime/<project>/` |
| `projectCachePath(path, project)` | `projectCachePath(path, project)` | Instance `cache/<project>/` |

C++ path helpers return `std::filesystem::path`. Interface helpers return a `{rootDir, path}` reference, not an absolute path on the browser computer. Interface instance subdirectory helpers require an explicit project name. Always capture it before starting an import or background operation so a project switch cannot redirect a later save.

```js
import {createStorageClient, projectPath, projectCachePath} from '@dartwic/interface-sdk/storage';
const storage = createStorageClient(operation);
await storage.writeJson(projectPath('my_driver/calibration.json', 'default'), {
    version: 1, offset: 0.12,
});
const calibration = await storage.readJson(projectPath('my_driver/calibration.json', 'default'));
await storage.writeJson(projectCachePath('my_driver/preview.json', 'default'), {
    generated_from: 'my_driver/calibration.json',
});
```

For engine code, use `readStorageJson(scope, path, missing, project)` and `writeStorageJson(scope, path, value, project)`. Their scope strings are `installation`, `instance`, `workspace`, `project`, `settings`, `runtime`, and `cache`. Writes are atomic file replacements; this is not a cross-file transaction or an automatic schema migration.

The interface helpers use existing `dartwic/get-file` and `dartwic/save-file` operations. Admin/viewer permissions still apply. General storage and settings operations are admin-only unless explicitly allowed by the host; shared asset reads retain the existing viewer-safe rules. These text operations are not binary upload endpoints.

## Share an asset like the 3D model node

Use an asset when the file should be reusable across projects. A schematic stores a portable reference to the asset, not the machine's absolute directory or another copy of the model.

```js
import {createWorkspaceAssetClient} from '@dartwic/interface-sdk/assets';
const assets = createWorkspaceAssetClient(operation);
const path = await assets.saveJson('my_driver', 'calibration-v1.json', {
    version: 1, points: [0, 10, 20],
});
// Save path in your project node/resource data.
const nodeData = {asset_path: path};
const calibration = await assets.readJson(nodeData.asset_path);
```

That returns `assets/my_driver/calibration-v1.json` and writes `workspace/global_data/assets/my_driver/calibration-v1.json`. Use your plugin ID as a namespace. Use a new filename when content changes, or invalidate the helper's cache after an external overwrite. Do not assume that one project's reference means the asset is unused everywhere else.

The built-in 3D node follows the same sequence:

1. Import one GLB/glTF model and its required sidecar files.
2. Package their relative paths and base64 bytes in a versioned JSON document.
3. Validate the package, then calculate a SHA-256 content hash.
4. Save `assets/models/<hash>.model.json` with `saveJson('models', ...)`.
5. Save `asset_path` in the schematic node; load the package through that reference.

Its package has this shape, with real base64 file content in place of the placeholder:

```json
{
  "version": 1,
  "entry": "vehicle.glb",
  "files": [{"path": "vehicle.glb", "base64": "<encoded file bytes>"}],
  "hash": "<sha256 of the package before adding hash>"
}
```

The model importer accepts glTF 2.0, checks that dependencies stay inside the package, and limits decoded import data to 20 MiB. `packageModelFiles()` is a built-in GUI implementation detail, not a public SDK export. A plugin can define its own validated package format, or implement this format if it needs compatibility with the built-in node. Do not pass raw binary to `saveJson()`.

Engine plugins can save the same kind of text package with `api.saveWorkspaceAsset(node, namespace, filename, content)` and read it with `api.readWorkspaceAsset(...)`. The returned reference has the same `assets/...` form.

## Internal ownership

Engine code uses `DARTWIC::Storage` in `implementations/storage.h`: root helpers, `readJson`, `writeJson`, `settingsDocument`, `effectiveSettings`, `settingsSnapshot`, and `updateSettings`. SDK calls forward to those functions. Project rename includes project files and the instance's settings, runtime, and cache directories, keeping the readable project name aligned.

Use the settings functions for portable policy so core validation, merge order, resets and stale-edit checks stay consistent. Use ordinary file helpers for plugin-owned documents with their own schemas. Keep schema/version checks near the reader, and report corrupt or unavailable data instead of silently treating it as an empty configuration.
