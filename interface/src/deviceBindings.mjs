export const EXAMPLE_READ_FIELDS = [
    "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
    "flow", "connected", "sensor_valid", "sample_age_s",
    "sample_counter"
];
export const EXAMPLE_WRITE_FIELDS = [
    "fuel_open_coil", "fuel_close_coil", "oxidizer_open_coil", "oxidizer_close_coil",
    "fill_open_coil", "fill_close_coil", "vent_open_coil", "vent_close_coil", "igniter_command"
];
export const exampleTaskTypeIds = ["example_device_plugin.example_read", "example_device_plugin.example_write"];

export function exampleMappings(saved, fields, prefix, withReadback = false) {
    const mappings = Array.isArray(saved)
        ? saved.map(row => ({device_field: row.device_field || "", channel: row.channel || "",
            ...(withReadback && row.readback_channel !== undefined ? {readback_channel: row.readback_channel} : {})}))
        : fields.map(device_field => ({device_field, channel: `${prefix}_${device_field}`}));
    return withReadback ? mappings.map(row => ({...row,
        readback_channel: row.readback_channel ?? `${row.channel}_state`})) : mappings;
}

export function exampleTaskMappings(task) {
    if (!exampleTaskTypeIds.includes(task?.task_type)) return [];
    const isRead = task.task_type === exampleTaskTypeIds[0];
    const args = task.arguments || {};
    return exampleMappings(args[isRead ? "read_mappings" : "write_mappings"],
        isRead ? EXAMPLE_READ_FIELDS : EXAMPLE_WRITE_FIELDS, args.channel_prefix ?? "example_device", !isRead);
}

export function resolveExampleTaskChannels(task) {
    const mappings = exampleTaskMappings(task);
    if (task?.task_type === exampleTaskTypeIds[0])
        return mappings.map(row => ({name: row.channel, label: row.device_field,
            direction: "output", detail: row.device_field}));
    return mappings.flatMap(row => [
        {name: row.channel, label: row.device_field, direction: "input", detail: row.device_field},
        {name: row.readback_channel, label: `${row.device_field}_state`, direction: "output",
            detail: `${row.device_field} state`},
    ]);
}
