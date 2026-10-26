import {definePlugin} from "../sdk/index.ts";
import {moduleMap, stressModuleMap, readMap, writeMap, stressReadMap, stressWriteMap, settingsMap} from "./configMaps.mjs";
import {ExamplePluginSettings} from "./pluginSettings.jsx";
import {ExampleResource, ExampleSchematicNode} from "./resourcesAndSchematics.jsx";
import {ExampleDeviceTaskConfig} from "./taskConfigs.jsx";
import React from "../sdk/react.ts";

function ExampleDeviceIcon(props) {
    return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" {...props}>
            <rect x="3" y="6" width="18" height="13" rx="2"/>
            <path d="M8 3v3M16 3v3M8 11h8M8 15h4"/>
        </svg>
    );
}

function ModuleIcon(props) {
    return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" {...props}>
            <path d="m7.5 4.27 9 5.15"/>
            <path d="M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16Z"/>
            <path d="m3.3 7 8.7 5 8.7-5"/>
            <path d="M12 22V12"/>
        </svg>
    );
}

export default definePlugin({
    id: "example_device_plugin",
    name: "Example Device Plugin",
    register(registry) {
        for (const id of ["stress_read", "stress_write"]) registry.addTaskUi({id, name: id === "stress_read" ? "Stress Device Read" : "Stress Device Write", configMap: id === "stress_read" ? stressReadMap : stressWriteMap});
        registry.addModuleUi({id: "stress_device", name: "Stress Device", icon: ExampleDeviceIcon, configMap: stressModuleMap});
        for (const id of ["example_read", "example_write"]) registry.addTaskUi({
            id, name: id === "example_read" ? "Example Device Read" : "Example Device Write", editor: ExampleDeviceTaskConfig, configMap: id === "example_read" ? readMap : writeMap,
        });
        registry.addModuleUi({
            id: "example_device",
            name: "Example Device",
            icon: ModuleIcon,
            configMap: moduleMap,
            connection: ({instanceConfig}) => ({
                channel: `${instanceConfig?.parameters?.channel_prefix || "example_device"}_connected`,
                label: "Example Device", endpoint: "LOCAL SIMULATION", connectedValue: 1,
            }),
        });
        registry.addResource({
            id: "example_notes",
            name: "Example Telemetry",
            label: "Example Telemetry",
            type: "component",
            icon: ExampleDeviceIcon,
            component: ExampleResource,
            show_in_resource_tabs: true,
        });
        registry.addSchematicNode({
            id: "example_indicator",
            name: "Example Indicator",
            component: ExampleSchematicNode,
            dataDefaults: {label: "Example"},
            palette: {
                defaults: {
                    width: 160,
                    height: 72,
                },
            },
            resizable: true,
        });
        registry.addSettingsPanel({
            id: "general",
            name: "Example Settings",
            component: ExamplePluginSettings, configMap: settingsMap,
        });
    },
});
