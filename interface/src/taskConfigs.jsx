import React from "../sdk/react.ts";
import {defineTaskConfig, useTaskConfigBridge} from "../sdk/tasks/index.ts";
import {Input, Label} from "../sdk/ui/general.ts";
import {ChannelComboBox, convertChannelReferenceToChannelName, ModuleInstanceSelect} from "../sdk/ui/dartwic.ts";

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
