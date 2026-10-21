export const EXAMPLE_READ_FIELDS = [
    "pressure_current_ma", "temperature_1", "temperature_2", "temperature_3",
    "flow", "connected", "sensor_valid", "sample_age_s",
    "igniter_applied", "sample_counter", "fuel_position", "oxidizer_position",
    "fill_position", "vent_position", "fuel_open_applied", "fuel_close_applied",
    "oxidizer_open_applied", "oxidizer_close_applied", "fill_open_applied",
    "fill_close_applied", "vent_open_applied", "vent_close_applied"
];
export const EXAMPLE_WRITE_FIELDS = [
    "fuel_open_coil", "fuel_close_coil", "oxidizer_open_coil", "oxidizer_close_coil",
    "fill_open_coil", "fill_close_coil", "vent_open_coil", "vent_close_coil", "igniter_command"
];
export const exampleTaskTypeIds = ["example_device_plugin.example_read", "example_device_plugin.example_write"];

export function exampleMappings(saved, fields, prefix) {
    return Array.isArray(saved) ? saved.map(row => ({device_field: row.device_field || "", channel: row.channel || ""}))
        : fields.map(device_field => ({device_field, channel: `${prefix}_${device_field}`}));
}

export function exampleTaskMappings(task) {
    if (!exampleTaskTypeIds.includes(task?.task_type)) return [];
    const isRead = task.task_type === exampleTaskTypeIds[0];
    const args = task.arguments || {};
    return exampleMappings(args[isRead ? "read_mappings" : "write_mappings"],
        isRead ? EXAMPLE_READ_FIELDS : EXAMPLE_WRITE_FIELDS, args.channel_prefix ?? "example_device");
}

export function resolveExampleTaskChannels(task) {
    return exampleTaskMappings(task).map(row => ({name: row.channel, label: row.device_field,
        direction: task.task_type === exampleTaskTypeIds[0] ? "output" : "input"}));
}
