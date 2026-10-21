import React from "../sdk/react.ts";
import {useTaskConfigBridge} from "../sdk/tasks/index.ts";
import {Input} from "../sdk/ui/general.ts";
import {ChannelComboBox, TaskBindingTable, ModuleRuntimeOverview, convertChannelReferenceToChannelName} from "../sdk/ui/dartwic.ts";

const taskTypes = ["example_device_plugin.stress_read", "example_device_plugin.stress_write"];
export function StressModuleConfig({instanceConfig}) {
    return <ModuleRuntimeOverview instanceName={instanceConfig?.name || ""} endpoint="LOCAL SIMULATION"
        taskTypeIds={taskTypes} resolveTaskChannels={task => {
            const read = task.task_type === taskTypes[0];
            return (task.arguments?.[read ? "read_mappings" : "write_mappings"] || []).map(row => ({
                name: row.channel, label: `Register ${row.register}`, direction: read ? "output" : "input",
            }));
        }} />;
}

export function StressTaskConfig({task, operation, onSaved, onClose, taskEditor}) {
    const read = task.task_type === taskTypes[0];
    const key = read ? "read_mappings" : "write_mappings";
    const original = {module_instance_name: task.arguments?.module_instance_name || "", [key]: task.arguments?.[key] || []};
    const [module, setModule] = React.useState(original.module_instance_name);
    const [rows, setRows] = React.useState(original[key].map(row => ({...row})));
    const [saving, setSaving] = React.useState(false);
    const [error, setError] = React.useState("");
    const mappings = rows.map(row => ({register: Number(row.register), channel: row.channel?.trim() || ""}));
    const valid = mappings.length === 5 && mappings.every(row => Number.isInteger(row.register) && row.register >= 0 && row.register < 5 && row.channel)
        && new Set(mappings.map(row => row.register)).size === 5 && new Set(mappings.map(row => row.channel)).size === 5;
    const payload = {module_instance_name: module, [key]: mappings};
    useTaskConfigBridge(taskEditor, {
        isDirty: JSON.stringify(payload) !== JSON.stringify(original), isSaving: saving, canSave: Boolean(module && valid), errorMessage: error,
        moduleConnection: {pluginId: "example_device_plugin", moduleTypeIds: ["stress_device"], value: module, onValueChange: setModule,
            description: "Bind all five local simulated registers to fixed RAPID channels."},
        saveLabel: "SAVE", cancelLabel: "CANCEL", onCancel: onClose,
        onSave: async () => {
            setSaving(true); setError("");
            try {
                const result = await operation("dartwic/create-task", {portal_name: task.portal, task_name: task.name,
                    task_type: task.task_type, arguments: {...task.arguments, ...payload}}, 30000);
                if (result?.error) throw new Error(result.payload?.error || "Failed to save stress task.");
                await onSaved?.(); await onClose?.();
            } catch (failure) { setError(failure.message); } finally { setSaving(false); }
        },
    });
    const columns = [
        {key: "register", label: "DEVICE REGISTER (0–4)", width: "minmax(10rem,1fr)", render: (row, index, update) =>
            <Input aria-label={`Device register ${index + 1}`} type="number" min="0" max="4" value={row.register}
                onChange={event => update({...row, register: event.target.value})} />},
        {key: "channel", label: read ? "RAPID OUTPUT" : "RAPID INPUT", width: "minmax(17rem,2fr)", render: (row, index, update) =>
            <ChannelComboBox mode={read ? "write" : "read"} showFieldSelector={false} initialValue={row.channel} overrideValue={row.channel}
                onSelect={value => update({...row, channel: convertChannelReferenceToChannelName(value)})} />},
    ];
    return <TaskBindingTable title={read ? "READ MAPPINGS (DEVICE → RAPID)" : "WRITE MAPPINGS (RAPID → DEVICE)"}
        bindings={rows} onBindingsChange={setRows} bindingTypes={[]} columns={columns} channelMode={read ? "write" : "read"}
        addDisabled={rows.length >= 5} createBinding={() => ({register: [0,1,2,3,4].find(i => !rows.some(row => Number(row.register) === i)) ?? 0, channel: ""})} />;
}
