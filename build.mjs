import path from "node:path";
import {fileURLToPath} from "node:url";
import {cp, mkdir, readFile} from "node:fs/promises";
import {build} from "esbuild";
import postcss from "postcss";
import tailwindcss from "tailwindcss";
import dartwicTailwindPreset from "./interface/sdk/tailwind-preset.ts";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const sourceManifestPath = path.resolve(__dirname, "plugin.json");
const sourceManifest = JSON.parse(await readFile(sourceManifestPath, "utf8"));
const pluginId = String(sourceManifest.id ?? "").trim();
const minEngineVersion = String(sourceManifest.minEngineVersion ?? "").trim();
const minInterfaceVersion = String(sourceManifest.minInterfaceVersion ?? "").trim();
const releasePluginDir = path.resolve(__dirname, "plugin", "interface", pluginId);
const pluginManifestPath = path.resolve(releasePluginDir, "plugin.json");
const outputPath = path.resolve(releasePluginDir, "ui", "index.js");

if (!pluginId || !minEngineVersion || !minInterfaceVersion || sourceManifest.contains_interface_plugin !== true) {
    throw new Error("plugin.json must include id, minEngineVersion, minInterfaceVersion, and contains_interface_plugin=true.");
}

await mkdir(path.dirname(outputPath), {recursive: true});

// Compile only the plugin's own utility classes. Host components already carry
// their styling; semantic CSS variables keep plugin layout theme-aware.
const {css: pluginCss} = await postcss([tailwindcss({
    presets: [dartwicTailwindPreset],
    content: [path.resolve(__dirname, "interface/src/**/*.{js,jsx,ts,tsx}")],
    corePlugins: {preflight: false},
})]).process("@tailwind utilities;", {from: undefined});

await build({
    entryPoints: [path.resolve(__dirname, "interface", "src", "runtime-entry.jsx")],
    outfile: outputPath,
    bundle: true,
    format: "iife",
    platform: "browser",
    target: ["es2022"],
    jsx: "transform",
    minify: true,
    legalComments: "none",
    plugins: [{
        name: "dartwic-plugin-css",
        setup(pluginBuild) {
            pluginBuild.onResolve({filter: /^dartwic-plugin-css$/}, () => ({path: "dartwic-plugin-css", namespace: "dartwic-plugin"}));
            pluginBuild.onLoad({filter: /.*/, namespace: "dartwic-plugin"}, () => ({contents: pluginCss, loader: "text"}));
        },
    }],
});

await cp(sourceManifestPath, pluginManifestPath, {force: true});

console.log(`Built EtherCAT interface plugin from interface/src for ${pluginId}.`);
console.log(`- ${releasePluginDir}`);
