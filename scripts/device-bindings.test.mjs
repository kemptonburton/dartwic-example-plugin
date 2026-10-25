import test from "node:test";
import assert from "node:assert/strict";
import {readFile} from "node:fs/promises";
import {EXAMPLE_READ_FIELDS, EXAMPLE_WRITE_FIELDS, exampleTaskMappings,
    resolveExampleTaskChannels} from "../interface/src/deviceBindings.mjs";

test("default read and write task channels match the native driver's fields", async () => {
    const header = await readFile(new URL("../engine/include/example_device_module.h", import.meta.url), "utf8");
    for (const [kind, fields, direction] of [["read", EXAMPLE_READ_FIELDS, "output"], ["write", EXAMPLE_WRITE_FIELDS, "input"]]) {
        const declaration = header.match(new RegExp(`${kind}Fields\\{([\\s\\S]*?)\\};`))[1];
        const driverFields = [...declaration.matchAll(/"([^"]+)"/g)].map(match => match[1]);
        assert.deepEqual(fields, driverFields);
        const task = {task_type: `example_device_plugin.example_${kind}`,
            arguments: {module_instance_name: "test", channel_prefix: "example_device"}};
        assert.deepEqual(resolveExampleTaskChannels(task), driverFields.map(field => ({
            name: `example_device_${field}`, label: field, direction
        })));
    }
});

test("every device command has exactly one matching state field", () => {
    const expected = EXAMPLE_WRITE_FIELDS.map(field => `${field}_state`);
    assert.deepEqual(EXAMPLE_READ_FIELDS.filter(field => field.endsWith("_state")),
        [expected.at(-1), ...expected.slice(0, -1)]);
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
    assert.deepEqual(resolveExampleTaskChannels(task), [{name: "custom_igniter", label: "igniter_command", direction: "input"}]);
    assert.deepEqual(exampleTaskMappings({...task, arguments: {write_mappings: []}}), []);
    assert.deepEqual(exampleTaskMappings(undefined), []);
});
