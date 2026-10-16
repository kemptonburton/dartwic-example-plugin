import React from "../sdk/react.ts";
import {useTaskConfigBridge} from "../sdk/tasks/index.ts";
import {Input, Label, Select, SelectContent, SelectItem, SelectTrigger, SelectValue} from "../sdk/ui/general.ts";
import {ChannelComboBox, convertChannelReferenceToChannelName, TaskBindingTable} from "../sdk/ui/dartwic.ts";

const EXAMPLE_READ_FIELDS = [
    "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
    "flow", "connected", "sensor_valid", "sample_age_s",
    "igniter_applied", "sample_counter", "fuel_position", "oxidizer_position",
    "fill_position", "vent_position", "fuel_open_applied", "fuel_close_applied",
    "oxidizer_open_applied", "oxidizer_close_applied", "fill_open_applied",
    "fill_close_applied", "vent_open_applied", "vent_close_applied"
];
const EXAMPLE_WRITE_FIELDS = [
    "fuel_open_coil", "fuel_close_coil", "oxidizer_open_coil", "oxidizer_close_coil",
    "fill_open_coil", "fill_close_coil", "vent_open_coil", "vent_close_coil", "igniter_command"
];

function exampleMappings(saved, fields, prefix) {
    return Array.isArray(saved) ? saved.map(row => ({device_field: row.device_field || "", channel: row.channel || ""}))
        : fields.map(device_field => ({device_field, channel: `${prefix}_${device_field}`}));
}

export function ExampleDeviceTaskConfig({task, operation, onSaved, onClose, taskEditor}) {
    const args = task.arguments || {};
    const isRead = task.task_type === "example_device_plugin.example_read";
    const fields = isRead ? EXAMPLE_READ_FIELDS : EXAMPLE_WRITE_FIELDS;
    const mappingKey = isRead ? "read_mappings" : "write_mappings";
    const initialPrefix = args.channel_prefix || "example_device";
    const initialMappings = exampleMappings(args[mappingKey], fields, initialPrefix);
    const [module, setModule] = React.useState(args.module_instance_name || "example_device_1");
    const [prefix, setPrefix] = React.useState(initialPrefix);
    const [mappings, setMappings] = React.useState(initialMappings);
    const [saving, setSaving] = React.useState(false);
    const [error, setError] = React.useState("");
    const validMappings = mappings.length > 0 && mappings.every(row => fields.includes(row.device_field) && row.channel.trim())
        && new Set(mappings.map(row => row.device_field)).size === mappings.length;
    const payload = {module_instance_name: module, channel_prefix: prefix.trim(), [mappingKey]: mappings};
    const original = {module_instance_name: args.module_instance_name || "example_device_1",
        channel_prefix: initialPrefix, [mappingKey]: initialMappings};
    const columns = [
        {key: "device_field", label: "DEVICE FIELD", width: "minmax(12rem,1fr)", render: (row, index, update) =>
            <Select value={row.device_field} onValueChange={device_field => update({...row, device_field})}>
                <SelectTrigger aria-label={`Device field ${index + 1}`} className="h-8 min-h-0 w-full rounded-none border-0 bg-transparent px-0 py-0 text-xs shadow-none"><SelectValue /></SelectTrigger>
                <SelectContent>{fields.map(field => <SelectItem key={field} value={field}>{field}</SelectItem>)}</SelectContent>
            </Select>},
        {key: "channel", label: isRead ? "STATE CHANNEL (OUTPUT)" : "STATE CHANNEL (INPUT)", width: "minmax(17rem,1.4fr)",
            render: (row, index, update) => <ChannelComboBox mode={isRead ? "write" : "read"} showFieldSelector={false}
                initialValue={row.channel} overrideValue={row.channel} placeholder="SELECT STATE CHANNEL"
                channelComboboxClassName="h-8 min-h-0 w-full rounded-none border-0 !bg-card px-0 py-0 text-xs normal-case shadow-none focus-visible:ring-0"
                onSelect={value => update({...row, channel: convertChannelReferenceToChannelName(value)})} />}
    ];
    useTaskConfigBridge(taskEditor, {
        isDirty: JSON.stringify(payload) !== JSON.stringify(original),
        isSaving: saving, canSave: Boolean(module && prefix.trim() && validMappings), errorMessage: error,
        moduleConnection: {pluginId: "example_device_plugin", moduleTypeIds: ["example_device"],
            value: module, onValueChange: setModule, showConnectionStatus: true},
        saveLabel: "SAVE", cancelLabel: "CANCEL", onCancel: onClose,
        onSave: async () => {
            if (!validMappings) return setError("MAP EACH DEVICE FIELD ONCE TO A STATE CHANNEL.");
            setSaving(true); setError("");
            try {
                const result = await operation("dartwic/create-task", {
                    portal_name: task.portal, task_name: task.name, task_type: task.task_type,
                    arguments: payload,
                }, 30000);
                if (result?.error) { setError(String(result.payload?.error || "FAILED TO SAVE EXAMPLE DEVICE TASK.").toUpperCase()); return; }
                await onSaved?.(); await onClose?.();
            } catch (failure) { setError(String(failure.message || failure)); }
            finally { setSaving(false); }
        },
    });
    return <div className="space-y-3">
        <div className="flex flex-wrap items-center gap-3">
            <Label htmlFor="example-device-prefix" className="shrink-0 text-xs">SIMULATION CONTROL PREFIX</Label>
            <Input id="example-device-prefix" className="h-8 min-w-40 max-w-xs flex-1" value={prefix} onChange={event => setPrefix(event.target.value)} />
            <span className="text-xs text-muted-foreground">Names the fault and ox-tank inputs.</span>
        </div>
        <TaskBindingTable title={isRead ? "READ BINDINGS (DEVICE → STATE CHANNEL)" : "WRITE BINDINGS (STATE CHANNEL → DEVICE)"}
            bindings={mappings} onBindingsChange={setMappings} bindingTypes={[]} columns={columns}
            channelMode={isRead ? "write" : "read"} minTableWidth="34rem"
            addDisabled={mappings.length >= fields.length}
            createBinding={() => ({device_field: fields.find(field => !mappings.some(row => row.device_field === field)) || fields[0], channel: ""})} />
    </div>;
}
