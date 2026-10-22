import assert from "node:assert/strict";
import test from "node:test";
import fs from "node:fs/promises";
import path from "node:path";
import os from "node:os";
import {spawnSync} from "node:child_process";
import {getPluginPlatform, packagePluginRelease, verifyPluginOutput} from "./plugin-release.mjs";

async function fixture(t, engine = false) {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), "plugin-release-test-"));
  t.after(() => fs.rm(root, {recursive: true, force: true}));
  const manifest = {id: "fixture", name: "Fixture", version: "1.0.0", minEngineVersion: "2.0.0", minInterfaceVersion: "2.0.0", contains_engine_plugin: engine, contains_interface_plugin: !engine};
  await fs.writeFile(path.join(root, "package.json"), JSON.stringify({version: "1.0.0"}));
  await fs.writeFile(path.join(root, "plugin.json"), JSON.stringify(manifest));
  const output = path.join(root, "plugin", engine ? "engine" : "interface", "fixture");
  await fs.mkdir(path.join(output, engine ? "bin" : "ui"), {recursive: true});
  await fs.writeFile(path.join(output, "plugin.json"), JSON.stringify(manifest));
  if (!engine) await fs.writeFile(path.join(output, "ui/index.js"), "console.log('fixture')");
  return root;
}

test("interface-only release creates one portable ZIP without native tools", async t => {
  const root = await fixture(t);
  await fs.mkdir(path.join(root, "plugin/engine/stale"), {recursive: true});
  await fs.writeFile(path.join(root, "plugin/engine/stale/secret.txt"), "stale");
    for (const name of ["plugin-win32-x64.zip", "plugin-linux-x64.zip", "plugin-debug.zip"]) await fs.writeFile(path.join(root, name), "obsolete");
  const receipt = await packagePluginRelease(root);
  assert.deepEqual(receipt.platforms, ["any"]);
  assert.deepEqual(Object.keys(receipt.files), ["plugin.json", "plugin.zip"]);
  const zip = path.join(root, "plugin.zip");
  const listing = spawnSync(process.platform === "win32" ? "tar" : "unzip", process.platform === "win32" ? ["-tf", zip] : ["-Z", "-1", zip], {encoding: "utf8"});
  assert.equal(listing.status, 0);
  assert.match(listing.stdout, /plugin[/\\]interface[/\\]fixture[/\\]ui[/\\]index.js/);
  assert.doesNotMatch(listing.stdout, /stale|secret|engine/);
  for (const name of ["plugin-win32-x64.zip", "plugin-linux-x64.zip", "plugin-debug.zip"]) await assert.rejects(fs.access(path.join(root, name)));
  await assert.doesNotReject(verifyPluginOutput(root, "darwin-arm64"));
  const manifest = JSON.parse(await fs.readFile(path.join(root, "plugin/interface/fixture/plugin.json")));
  manifest.version = "0.0.1";
  await fs.writeFile(path.join(root, "plugin/interface/fixture/plugin.json"), JSON.stringify(manifest));
  await assert.rejects(verifyPluginOutput(root), /differs from the root manifest/);
});

test("native release cannot be relabeled for another host", async t => {
  const root = await fixture(t, true);
  const other = getPluginPlatform() === "win32-x64" ? "linux-x64" : "win32-x64";
  await assert.rejects(packagePluginRelease(root, [other]), /never relabel/);
  assert.throws(() => getPluginPlatform("darwin", "arm64"), /Unsupported/);
});
