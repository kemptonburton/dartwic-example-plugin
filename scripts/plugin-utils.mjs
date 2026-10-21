import { spawn } from "node:child_process";
import fs from "node:fs/promises";
import path from "node:path";

export const repoRoot = process.cwd();
export const packageJsonPath = path.resolve(repoRoot, "package.json");
export const pluginManifestPath = path.resolve(repoRoot, "plugin.json");
export const deploymentSettingsPath = path.resolve(repoRoot, "deployment-settings.json");
export const pluginOutputRoot = path.resolve(repoRoot, "plugin");

export function validateStandalonePackageDependencies(packageJson) {
  for (const field of ["dependencies", "devDependencies", "optionalDependencies", "peerDependencies"]) {
    for (const [name, spec] of Object.entries(packageJson[field] ?? {})) {
      if (typeof spec !== "string") continue;
      if (!/^(?:file:|link:|\.{1,2}[\\/]|[\\/]|[A-Za-z]:[\\/])/.test(spec)) continue;
      const target = path.resolve(repoRoot, spec.replace(/^(?:file:|link:)/, ""));
      const relative = path.relative(repoRoot, target);
      if (relative === ".." || relative.startsWith(`..${path.sep}`) || path.isAbsolute(relative)) {
        throw new Error(`${field} dependency '${name}' points outside this standalone repository: ${spec}`);
      }
    }
  }
}

export async function readJson(filePath, fallback = null) {
  try {
    const text = await fs.readFile(filePath, "utf8");
    return JSON.parse(text);
  } catch (error) {
    if (error && typeof error === "object" && error.code === "ENOENT") {
      return fallback;
    }

    throw error;
  }
}

export async function writeJson(filePath, value) {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
  await fs.writeFile(filePath, `${JSON.stringify(value, null, 2)}\n`, "utf8");
}

export async function copyDirectory(sourceDir, targetDir) {
  await fs.mkdir(path.dirname(targetDir), { recursive: true });
  await fs.cp(sourceDir, targetDir, { recursive: true, force: true });
}

export function assertNonEmptyString(value, fieldName) {
  const normalized = String(value ?? "").trim();
  if (!normalized) {
    throw new Error(`${fieldName} is required.`);
  }

  return normalized;
}

export function validatePluginManifest(manifest, { requireVersion = true } = {}) {
  if (!manifest || typeof manifest !== "object") {
    throw new Error("plugin.json must contain an object.");
  }

  const pluginId = assertNonEmptyString(manifest.id, "plugin.json id");
  const pluginName = assertNonEmptyString(manifest.name, "plugin.json name");

  if (requireVersion) {
    assertNonEmptyString(manifest.version, "plugin.json version");
  }

  if (typeof manifest.contains_engine_plugin !== "boolean") {
    throw new Error("plugin.json must declare contains_engine_plugin as a boolean.");
  }

  if (typeof manifest.contains_interface_plugin !== "boolean") {
    throw new Error("plugin.json must declare contains_interface_plugin as a boolean.");
  }

  if (!manifest.contains_engine_plugin && !manifest.contains_interface_plugin) {
    throw new Error("plugin.json must contain an engine plugin, an interface plugin, or both.");
  }

  const minEngineVersion = manifest.contains_engine_plugin
    ? assertNonEmptyString(manifest.minEngineVersion, "plugin.json minEngineVersion")
    : String(manifest.minEngineVersion ?? "").trim();
  const minInterfaceVersion = manifest.contains_interface_plugin
    ? assertNonEmptyString(manifest.minInterfaceVersion, "plugin.json minInterfaceVersion")
    : String(manifest.minInterfaceVersion ?? "").trim();

  return {
    pluginId,
    pluginName,
    minEngineVersion,
    minInterfaceVersion,
  };
}

function normalizeComparableVersion(value) {
  const normalizedValue = String(value ?? "").trim();
  if (!normalizedValue) {
    return [];
  }

  const [versionCore, preRelease = ""] = normalizedValue.replace(/_/g, "-").split("-");
  const coreParts = versionCore
    .split(".")
    .map((part) => Number.parseInt(part, 10))
    .map((part) => (Number.isFinite(part) ? part : 0));

  while (coreParts.length < 3) {
    coreParts.push(0);
  }

  return [...coreParts.slice(0, 3), preRelease ? -1 : 0, preRelease];
}

export function compareVersions(left, right) {
  const leftParts = normalizeComparableVersion(left);
  const rightParts = normalizeComparableVersion(right);
  const maxLength = Math.max(leftParts.length, rightParts.length);

  for (let index = 0; index < maxLength; index += 1) {
    const leftPart = leftParts[index] ?? 0;
    const rightPart = rightParts[index] ?? 0;

    if (typeof leftPart === "number" && typeof rightPart === "number") {
      if (leftPart !== rightPart) {
        return leftPart > rightPart ? 1 : -1;
      }
      continue;
    }

    const comparison = String(leftPart).localeCompare(String(rightPart), undefined, { numeric: true });
    if (comparison !== 0) {
      return comparison > 0 ? 1 : -1;
    }
  }

  return 0;
}


export function getNpmCommand() {
  return process.platform === "win32" ? "npm.cmd" : "npm";
}

function quoteWindowsArgument(value) {
  const normalized = String(value ?? "");
  if (!normalized) {
    return "\"\"";
  }

  if (!/[\s"]/u.test(normalized)) {
    return normalized;
  }

  return `"${normalized.replace(/"/g, '\\"')}"`;
}

export async function runCommand(command, args, options = {}) {
  await new Promise((resolve, reject) => {
    const normalizedArgs = Array.isArray(args) ? args : [];
    const normalizedCommand = String(command ?? "");
    const cwd = options.cwd ?? repoRoot;
    const env = options.env ?? process.env;
    const isWindowsCommandScript = process.platform === "win32"
      && /\.(cmd|bat)$/iu.test(normalizedCommand);

    const child = isWindowsCommandScript
      ? spawn(normalizedCommand, normalizedArgs, {
        cwd,
        env,
        stdio: "inherit",
        shell: true,
      })
      : spawn(normalizedCommand, normalizedArgs, {
        cwd,
        env,
        stdio: "inherit",
        shell: false,
      });

    child.on("error", reject);
    child.on("close", (code) => {
      if (code === 0) {
        resolve();
        return;
      }

      reject(new Error(`${command} ${args.join(" ")} failed with exit code ${code ?? 1}.`));
    });
  });
}

export async function removePath(targetPath) {
  await fs.rm(targetPath, { recursive: true, force: true });
}

export async function pathExists(targetPath) {
  try {
    await fs.access(targetPath);
    return true;
  } catch {
    return false;
  }
}

export function getPlatformBinaryName(pluginId) {
  if (process.platform === "win32") {
    return `${pluginId}.dll`;
  }

  if (process.platform === "darwin") {
    return `lib${pluginId}.dylib`;
  }

  return `lib${pluginId}.so`;
}

export function getPluginSidePaths(pluginId) {
  return {
    engineReleaseDir: path.resolve(pluginOutputRoot, "engine", pluginId),
    engineDebugDir: path.resolve(pluginOutputRoot, "engine-debug", pluginId),
    interfaceDir: path.resolve(pluginOutputRoot, "interface", pluginId),
  };
}
