import React from "../sdk/react.ts";
import {ModuleRuntimeOverview} from "../sdk/ui/dartwic.ts";

const taskTypeIds = ["example_device_plugin.example_read", "example_device_plugin.example_write"];
const resolveTaskChannels = task => {
    const isRead = task.task_type === taskTypeIds[0];
    const rows = task.arguments?.[isRead ? "read_mappings" : "write_mappings"] || [];
    return rows.map(row => ({name: row.channel, label: row.device_field,
        direction: isRead ? "output" : "input"}));
};

export function ExampleModuleConfig({instanceConfig}) {
    const instanceName = instanceConfig?.name || "";
    const prefix = instanceConfig?.parameters?.channel_prefix || "example_device";
    return (
        <ModuleRuntimeOverview instanceName={instanceName} connectionChannel={`${prefix}_connected`}
            connectionLabel="SIMULATED DEVICE" endpoint="LOCAL SIMULATION"
            taskTypeIds={taskTypeIds} resolveTaskChannels={resolveTaskChannels} />
    );
}
