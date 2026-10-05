import React from "../sdk/react.ts";
import {defineTaskConfig, useTaskConfigBridge} from "../sdk/tasks/index.ts";
import {Input, Label, Select, SelectContent, SelectItem, SelectTrigger, SelectValue} from "../sdk/ui/general.ts";
import {ChannelComboBox, convertChannelReferenceToChannelName, ModuleInstanceSelect, TaskBindingTable} from "../sdk/ui/dartwic.ts";

export function ExampleTaskConfig() {
    return (
        <div className="rounded-lg border border-border bg-card p-4 text-sm text-muted-foreground">
            Example task editor body.
        </div>
    );
}

function channelValue(value, fallback) {
    return String(value || fallback).trim();
}

function ChannelBinding({label, mode, value, fallback, onChange}) {
    return (
        <div className="space-y-1.5">
            <Label>{label}</Label>
            <ChannelComboBox
                mode={mode}
                showFieldSelector={false}
                overrideValue={value}
                placeholder={fallback}
                onSelect={(next) => onChange(convertChannelReferenceToChannelName(next))}
            />
        </div>
    );
}

export function MockDriverTaskConfig({task, operation, onSaved, onClose, taskEditor}) {
    const isRead = task.task_type === "example_device_plugin.mock_read";
    const argumentsValue = task.arguments || {};
    const [moduleInstanceName, setModuleInstanceName] = React.useState(argumentsValue.module_instance_name || "");
    const [prefix, setPrefix] = React.useState(argumentsValue.channel_prefix || "mock_device");
    const [measurementChannel, setMeasurementChannel] = React.useState(channelValue(argumentsValue.measurement_channel, "mock_device_measurement"));
    const [appliedChannel, setAppliedChannel] = React.useState(channelValue(argumentsValue.applied_channel, "mock_device_applied"));
    const [connectedChannel, setConnectedChannel] = React.useState(channelValue(argumentsValue.connected_channel, "mock_device_connected"));
    const [commandChannel, setCommandChannel] = React.useState(channelValue(argumentsValue.command_channel, "mock_device_command"));
    const [errorMessage, setErrorMessage] = React.useState("");
    const [isSaving, setIsSaving] = React.useState(false);

    const payload = isRead
        ? {
            module_instance_name: moduleInstanceName,
            channel_prefix: prefix,
            measurement_channel: measurementChannel,
            applied_channel: appliedChannel,
            connected_channel: connectedChannel,
        }
        : {
            module_instance_name: moduleInstanceName,
            channel_prefix: prefix,
            command_channel: commandChannel,
        };
    const initialPayload = isRead
        ? {
            module_instance_name: argumentsValue.module_instance_name || "",
            channel_prefix: argumentsValue.channel_prefix || "mock_device",
            measurement_channel: channelValue(argumentsValue.measurement_channel, "mock_device_measurement"),
            applied_channel: channelValue(argumentsValue.applied_channel, "mock_device_applied"),
            connected_channel: channelValue(argumentsValue.connected_channel, "mock_device_connected"),
        }
        : {
            module_instance_name: argumentsValue.module_instance_name || "",
            channel_prefix: argumentsValue.channel_prefix || "mock_device",
            command_channel: channelValue(argumentsValue.command_channel, "mock_device_command"),
        };
    const isDirty = JSON.stringify(payload) !== JSON.stringify(initialPayload);
    const bindingsAreComplete = isRead
        ? Boolean(measurementChannel && appliedChannel && connectedChannel)
        : Boolean(commandChannel);

    async function saveTask() {
        if (!moduleInstanceName) return setErrorMessage("SELECT A MOCK DEVICE MODULE.");
        if (!bindingsAreComplete) return setErrorMessage("SELECT EVERY REQUIRED CHANNEL BINDING.");
        setIsSaving(true);
        setErrorMessage("");
        try {
            const result = await operation("dartwic/create-task", {
                portal_name: task.portal,
                task_name: task.name,
                task_type: task.task_type,
                arguments: payload,
            }, 30000);
            if (result?.error) return setErrorMessage((result?.payload?.error || "FAILED TO SAVE TASK.").toUpperCase());
            await onSaved?.();
            await onClose?.();
        } finally {
            setIsSaving(false);
        }
    }

    useTaskConfigBridge(taskEditor, {
        isDirty,
        isSaving,
        canSave: Boolean(moduleInstanceName && bindingsAreComplete),
        errorMessage,
        saveLabel: "SAVE",
        cancelLabel: "CANCEL",
        onSave: saveTask,
        onCancel: onClose,
    });

    return (
        <div className="space-y-5">
            <div className="space-y-2">
                <Label>MODULE CONNECTION</Label>
                <ModuleInstanceSelect
                    pluginId="example_device_plugin"
                    moduleTypeIds={["example_device"]}
                    value={moduleInstanceName}
                    onValueChange={setModuleInstanceName}
                    placeholder="SELECT ONE MOCK DEVICE"
                    showStatus
                />
                <p className="text-xs text-muted-foreground">Both task types select the same module instance; the module owns the shared connection and device state.</p>
            </div>
            <div className="space-y-1.5">
                <Label>CHANNEL PREFIX</Label>
                <Input value={prefix} onChange={(event) => setPrefix(event.target.value)} placeholder="mock_device" />
            </div>
            {isRead ? <div className="space-y-4">
                <div className="text-xs font-medium text-muted-foreground">READ BINDINGS (DEVICE → RAPID)</div>
                <ChannelBinding label="Measurement" mode="write" value={measurementChannel} fallback="mock_device_measurement" onChange={setMeasurementChannel} />
                <ChannelBinding label="Applied feedback" mode="write" value={appliedChannel} fallback="mock_device_applied" onChange={setAppliedChannel} />
                <ChannelBinding label="Connection state" mode="write" value={connectedChannel} fallback="mock_device_connected" onChange={setConnectedChannel} />
            </div> : <div className="space-y-4">
                <div className="text-xs font-medium text-muted-foreground">WRITE BINDING (RAPID → DEVICE)</div>
                <ChannelBinding label="Command" mode="read" value={commandChannel} fallback="mock_device_command" onChange={setCommandChannel} />
            </div>}
        </div>
    );
}

export const taskConfigs = [
    defineTaskConfig({taskType: "example_device_plugin.mock_read", component: MockDriverTaskConfig}),
    defineTaskConfig({taskType: "example_device_plugin.mock_write", component: MockDriverTaskConfig}),
];

const ROCKET_READ_FIELDS = [
    "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
    "run_fill", "supply_fill", "flow", "connected", "sensor_valid", "sample_age_s",
    "igniter_applied", "sample_counter", "fuel_position", "oxidizer_position",
    "fill_position", "vent_position", "fuel_open_applied", "fuel_close_applied",
    "oxidizer_open_applied", "oxidizer_close_applied", "fill_open_applied",
    "fill_close_applied", "vent_open_applied", "vent_close_applied"
];
const ROCKET_WRITE_FIELDS = [
    "fuel_open_coil", "fuel_close_coil", "oxidizer_open_coil", "oxidizer_close_coil",
    "fill_open_coil", "fill_close_coil", "vent_open_coil", "vent_close_coil", "igniter_command"
];

function rocketMappings(saved, fields, prefix) {
    return Array.isArray(saved) ? saved.map(row => ({device_field: row.device_field || "", channel: row.channel || ""}))
        : fields.map(device_field => ({device_field, channel: `${prefix}_${device_field}`}));
}

export function RocketDriverTaskConfig({task, operation, onSaved, onClose, taskEditor}) {
    const args = task.arguments || {};
    const isRead = task.task_type === "example_device_plugin.rocket_read";
    const fields = isRead ? ROCKET_READ_FIELDS : ROCKET_WRITE_FIELDS;
    const mappingKey = isRead ? "read_mappings" : "write_mappings";
    const initialPrefix = args.channel_prefix || "rocket_sim";
    const initialMappings = rocketMappings(args[mappingKey], fields, initialPrefix);
    const [module, setModule] = React.useState(args.module_instance_name || "rocket_device");
    const [prefix, setPrefix] = React.useState(initialPrefix);
    const [mappings, setMappings] = React.useState(initialMappings);
    const [saving, setSaving] = React.useState(false);
    const [error, setError] = React.useState("");
    const validMappings = mappings.length > 0 && mappings.every(row => fields.includes(row.device_field) && row.channel.trim())
        && new Set(mappings.map(row => row.device_field)).size === mappings.length;
    const payload = {module_instance_name: module, channel_prefix: prefix.trim(), [mappingKey]: mappings};
    const original = {module_instance_name: args.module_instance_name || "rocket_device",
        channel_prefix: initialPrefix, [mappingKey]: initialMappings};
    const columns = [
        {key: "device_field", label: "DEVICE FIELD", width: "minmax(12rem,1fr)", render: (row, index, update) =>
            <Select value={row.device_field} onValueChange={device_field => update({...row, device_field})}>
                <SelectTrigger aria-label={`Device field ${index + 1}`} className="h-8 min-h-0 w-full rounded-none border-0 bg-transparent text-xs"><SelectValue /></SelectTrigger>
                <SelectContent>{fields.map(field => <SelectItem key={field} value={field}>{field}</SelectItem>)}</SelectContent>
            </Select>},
        {key: "channel", label: isRead ? "STATE CHANNEL (OUTPUT)" : "STATE CHANNEL (INPUT)", width: "minmax(17rem,1.4fr)",
            render: (row, index, update) => <ChannelComboBox mode={isRead ? "write" : "read"} showFieldSelector={false}
                initialValue={row.channel} overrideValue={row.channel} placeholder="SELECT STATE CHANNEL"
                onSelect={value => update({...row, channel: convertChannelReferenceToChannelName(value)})} />}
    ];
    useTaskConfigBridge(taskEditor, {
        isDirty: JSON.stringify(payload) !== JSON.stringify(original),
        isSaving: saving, canSave: Boolean(module && prefix.trim() && validMappings), errorMessage: error,
        saveLabel: "SAVE", cancelLabel: "CANCEL", onCancel: onClose,
        onSave: async () => {
            if (!validMappings) return setError("MAP EACH DEVICE FIELD ONCE TO A STATE CHANNEL.");
            setSaving(true); setError("");
            try {
                const result = await operation("dartwic/create-task", {
                    portal_name: task.portal, task_name: task.name, task_type: task.task_type,
                    arguments: payload,
                }, 30000);
                if (result?.error) { setError(String(result.payload?.error || "FAILED TO SAVE ROCKET TASK.").toUpperCase()); return; }
                await onSaved?.(); await onClose?.();
            } catch (failure) { setError(String(failure.message || failure)); }
            finally { setSaving(false); }
        },
    });
    return <div className="space-y-4">
        <Label>SIMULATED DEVICE MODULE</Label>
        <ModuleInstanceSelect pluginId="example_device_plugin" moduleTypeIds={["rocket_sim"]}
            value={module} onValueChange={setModule} showStatus />
        <Label>SIMULATION CONTROL PREFIX</Label>
        <Input value={prefix} onChange={event => setPrefix(event.target.value)} />
        <p className="text-xs text-muted-foreground">The prefix names the fault and phase controls. Each device measurement or command has its own state-channel binding below.</p>
        <TaskBindingTable title={isRead ? "READ BINDINGS (DEVICE → STATE CHANNEL)" : "WRITE BINDINGS (STATE CHANNEL → DEVICE)"}
            bindings={mappings} onBindingsChange={setMappings} bindingTypes={[]} columns={columns}
            channelMode={isRead ? "write" : "read"} minTableWidth="34rem"
            addDisabled={mappings.length >= fields.length}
            createBinding={() => ({device_field: fields.find(field => !mappings.some(row => row.device_field === field)) || fields[0], channel: ""})} />
    </div>;
}
