---
summary: Choose where settings, private device configuration, runtime files, and shared assets belong.
updated: 2026-10-05T16:11
created: 2026-10-05T15:38
---

# Storage and Settings

The public [example plugin](https://github.com/kemptonburton/dartwic-example-plugin)
includes a working autosaved settings panel: **Decimal places** controls numeric
precision, **Show units** controls unit suffixes, and **Resource title** names
the Example Telemetry resource. The open resource updates when a setting is saved
and shows live values from the example device read/write task bindings. They do not
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
| Diagnostic event enablement, limits and warning rules | Workspace: `settings.json`; optional project `settings.json` | Shared policy, with experiment-specific overrides |
| Recording queue/options and history retention policy | Same settings layers: `recording.options`, `recording.retention` | Portable recording policy |
| Engine-origin notification mutes | Same settings layers: `notifications.mutes` | Shared operator choice, not an individual browser preference |
| Interface-origin notification mutes | Interface: browser/client storage | A local presentation choice |
| Client display profiles | Workspace: `client_info.json` | Shared display metadata; this does not grant admin access |
| Reusable schematic definitions, plugin icons | Workspace: `assets/schematic_nodes/`, `assets/plugin_icons/` | Shared authored resources |
| Reusable images, media packages and 3D models | Workspace: `assets/<namespace>/<filename>` | One asset can be referenced by several projects |
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
code defaults -> workspace/settings.json -> workspace/<project>/settings.json
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

## Use Config Map for settings

Every example module, task and plugin-settings editor uses the shared Config Map API. `interface/src/configMaps.mjs` owns declarations and application adapters. The custom task/settings components render groups from that one declaration with `<ConfigMap groupIds={["display"]}/>`; they do not create separate documents or save queues.

Module and task editors use the host Apply flow. Plugin settings save on blur through the host's serialized session. The display adapter captures the selected project, validates values, sends only changed fields, and supplies the previous opaque revision. A project change or failed write stays visible; retry reloads accepted state before applying again. This avoids copying inherited defaults into permanent overrides.

`settings.mjs` retains the defaults and storage helpers used by the adapter and the live resource. There is no plugin-specific debounce queue or React autosave controller. Config Map is configuration I/O; device commands continue through their normal channel/task path.
