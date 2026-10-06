import {definePlugin} from "../sdk/index.ts";
import {ExampleModuleConfig, RocketModuleConfig} from "./moduleConfigs.jsx";
import {ExamplePluginSettings} from "./pluginSettings.jsx";
import {ExampleResource, ExampleSchematicNode} from "./resourcesAndSchematics.jsx";
import {ExampleTaskCard} from "./taskCards.jsx";
import {ExampleTaskConfig, taskConfigs, RocketDriverTaskConfig} from "./taskConfigs.jsx";
import React from "../sdk/react.ts";

function ExampleDeviceIcon(props) {
    return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" {...props}>
            <rect x="3" y="6" width="18" height="13" rx="2"/>
            <path d="M8 3v3M16 3v3M8 11h8M8 15h4"/>
        </svg>
    );
}

function ExampleNotesIcon(props) {
    return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" {...props}>
            <path d="M6 3h9l3 3v15H6z"/>
            <path d="M15 3v4h4M9 12h6M9 16h6"/>
        </svg>
    );
}

export default definePlugin({
    id: "example_device_plugin",
    name: "Example Device Plugin",
    register(registry) {
        for (const id of ["rocket_read", "rocket_write"]) registry.addTaskUi({
            id, name: id === "rocket_read" ? "Mock Rocket Read" : "Mock Rocket Write", editor: RocketDriverTaskConfig,
        });
        registry.addModuleUi({id: "rocket_sim", name: "Mock Rocket Device", icon: ExampleDeviceIcon, panel: RocketModuleConfig});
        registry.addTaskUi({
            id: "example_task",
            name: "Example Task UI",
            card: ExampleTaskCard,
            editor: ExampleTaskConfig,
        });
        registry.addTaskUi({
            id: "mock_read",
            name: "Mock Device Read",
            editor: taskConfigs[0].component,
        });
        registry.addTaskUi({
            id: "mock_write",
            name: "Mock Device Write",
            editor: taskConfigs[1].component,
        });
        registry.addModuleUi({
            id: "example_device",
            name: "Example Device",
            icon: ExampleDeviceIcon,
            panel: ExampleModuleConfig,
        });
        registry.addResource({
            id: "example_notes",
            name: "Example Telemetry",
            label: "Example Telemetry",
            type: "component",
            icon: ExampleNotesIcon,
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
            component: ExamplePluginSettings,
        });
    },
});
