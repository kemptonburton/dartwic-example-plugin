import crypto from "node:crypto";
import fs from "node:fs/promises";
import path from "node:path";
import {fileURLToPath} from "node:url";

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const pluginRoot = path.resolve(scriptDir, "..");
const lockPath = path.resolve(pluginRoot, "sdk-lock.json");
const ignoredDirectoryNames = new Set([".git", "build", "node_modules"]);

async function listFiles(root, current = root) {
  const files = [];
  for (const entry of await fs.readdir(current, {withFileTypes: true})) {
    if (entry.isDirectory() && ignoredDirectoryNames.has(entry.name)) continue;
    const entryPath = path.join(current, entry.name);
    if (entry.isDirectory()) files.push(...await listFiles(root, entryPath));
    else if (entry.isFile()) files.push(path.relative(root, entryPath).replaceAll("\\", "/"));
  }
  return files.sort((left, right) => left.localeCompare(right));
}

async function hashDirectory(root) {
  const hash = crypto.createHash("sha256");
  for (const relativePath of await listFiles(root)) {
    hash.update(relativePath);
    hash.update("\0");
    const contents = await fs.readFile(path.join(root, relativePath), "utf8");
    hash.update(contents.replaceAll("\r\n", "\n").replaceAll("\r", "\n"));
    hash.update("\0");
  }
  return hash.digest("hex");
}

export async function buildSdkLock() {
  return {
    peerDependenciesSha256: await hashDirectory(path.resolve(pluginRoot, "vendor")),
    engineSdkSha256: await hashDirectory(path.resolve(pluginRoot, "engine", "include", "sdk")),
    interfaceSdkSha256: await hashDirectory(path.resolve(pluginRoot, "interface", "sdk")),
  };
}

export async function writeSdkLock() {
  const lock = await buildSdkLock();
  await fs.writeFile(lockPath, `${JSON.stringify(lock, null, 2)}\n`, "utf8");
  return lock;
}

export async function verifySdkLock() {
  const expected = JSON.parse(await fs.readFile(lockPath, "utf8"));
  const actual = await buildSdkLock();
  if (expected.engineSdkSha256 !== actual.engineSdkSha256 || expected.interfaceSdkSha256 !== actual.interfaceSdkSha256 || expected.peerDependenciesSha256 !== actual.peerDependenciesSha256) {
    throw new Error("Bundled SDK files differ from sdk-lock.json. Restore the SDK snapshot from this repository tag.");
  }
  return actual;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const write = process.argv.includes("--write");
  const result = write ? await writeSdkLock() : await verifySdkLock();
  console.log(`${write ? "Wrote" : "Verified"} SDK lock: ${lockPath}`);
  console.log(result);
}
