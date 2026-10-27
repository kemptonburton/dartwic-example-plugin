import test from "node:test";
import assert from "node:assert/strict";
import {readFile} from "node:fs/promises";
import {EXAMPLE_READ_FIELDS, EXAMPLE_WRITE_FIELDS, exampleTaskMappings} from "../interface/src/deviceBindings.mjs";

test("default read and write task channels match the native driver's fields", async () => {
    const header = await readFile(new URL("../engine/include/example_device_module.h", import.meta.url), "utf8");
    for (const [kind, fields] of [["read", EXAMPLE_READ_FIELDS], ["write", EXAMPLE_WRITE_FIELDS]]) {
        const declaration = header.match(new RegExp(`${kind}Fields\\{([\\s\\S]*?)\\};`))[1];
        const driverFields = [...declaration.matchAll(/"([^"]+)"/g)].map(match => match[1]);
        assert.deepEqual(fields, driverFields);
        const task = {task_type: `example_device_plugin.example_${kind}`,
            arguments: {module_instance_name: "test", channel_prefix: "example_device"}};
        const expected = driverFields.map(device_field => ({device_field, channel: `example_device_${device_field}`,
            ...(kind === "write" ? {readback_channel: `example_device_${device_field}_state`} : {})}));
        assert.deepEqual(exampleTaskMappings(task), expected);
    }
});

test("state readbacks belong to write mappings and sensor reads have no command states", () => {
    assert.equal(EXAMPLE_READ_FIELDS.filter(field => field.endsWith("_state")).length, 0);
    const task = {task_type: "example_device_plugin.example_write", arguments: {channel_prefix: "example_device"}};
    assert.deepEqual(exampleTaskMappings(task).map(row => row.readback_channel),
        EXAMPLE_WRITE_FIELDS.map(field => `example_device_${field}_state`));
    assert.ok(EXAMPLE_READ_FIELDS.every(field => !field.endsWith("_applied") && !field.endsWith("_position")));
});

test("default mappings use the configured prefix and the driver's fallback prefix", () => {
    const task = {task_type: "example_device_plugin.example_read", arguments: {channel_prefix: "other"}};
    assert.equal(exampleTaskMappings(task)[0].channel, "other_pressure_current_ma");
    assert.equal(exampleTaskMappings({...task, arguments: {}})[0].channel, "example_device_pressure_current_ma");
});

test("explicit mappings replace defaults, including an explicitly empty table", () => {
    const task = {task_type: "example_device_plugin.example_write", arguments: {
        write_mappings: [{device_field: "igniter_command", channel: "custom_igniter"}]
    }};
    assert.deepEqual(exampleTaskMappings(task), [
        {device_field: "igniter_command", channel: "custom_igniter", readback_channel: "custom_igniter_state"},
    ]);
    assert.equal(exampleTaskMappings({...task, arguments: {write_mappings: [
        {device_field: "igniter_command", channel: "custom_igniter", readback_channel: "custom_feedback"},
    ]}})[0].readback_channel, "custom_feedback");
    assert.deepEqual(exampleTaskMappings({...task, arguments: {write_mappings: []}}), []);
    assert.deepEqual(exampleTaskMappings(undefined), []);
});
