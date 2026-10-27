import {defineConfigMap, createModuleConfigAdapter, createTaskConfigAdapter} from '../sdk/config-maps/index.ts';
import {EXAMPLE_READ_FIELDS, EXAMPLE_WRITE_FIELDS, exampleTaskMappings} from './deviceBindings.mjs';
import {displaySettings, openExampleSettings, exampleSettings, pluginId} from './settings.mjs';

export const moduleMap = defineConfigMap({...createModuleConfigAdapter(), title: 'Example device', groups: [
    {id: 'simulation', label: 'Simulation', fields: [{path: 'parameters.channel_prefix', label: 'Simulation channel prefix', type: 'text', required: true, defaultValue: 'example_device'}]},
]});
export const stressModuleMap = defineConfigMap({...createModuleConfigAdapter(), title: 'Stress device', groups: [
    {id: 'waveform', label: 'Waveform', fields: [
        {path: 'parameters.maximum', label: 'Maximum', type: 'number', required: true, min: 3, integer: true, defaultValue: 1000},
        {path: 'parameters.threshold', label: 'Threshold', type: 'number', required: true, min: 2, integer: true, defaultValue: 800},
    ]},
], validate: value => (value.parameters?.threshold ?? 800) < (value.parameters?.maximum ?? 1000) ? '' : 'Threshold must be below maximum.'});

export function taskMap(read, stress = false) {
    const adapter = createTaskConfigAdapter();
    const key = read ? 'read_mappings' : 'write_mappings';
    const fields = read ? EXAMPLE_READ_FIELDS : EXAMPLE_WRITE_FIELDS;
    return defineConfigMap({...adapter, title: `${stress ? 'Stress' : 'Example'} ${read ? 'read' : 'write'}`,
        async load(context) {
            const task = await adapter.load(context);
            // Defaults are materialized only when the legacy task omitted its array.
            // Existing rows keep every optional/unknown field, including readbacks.
            if (!stress && !Array.isArray(task.arguments[key])) {
                task.arguments[key] = exampleTaskMappings(task);
                task._configMapDefaultedBindings = key;
            }
            if (read && !stress && task.arguments[key]?.some(row => row.device_field === 'connected')) {
                task._configMapOriginalBindings = task.arguments[key];
                task.arguments[key] = task.arguments[key].filter(row => row.device_field !== 'connected');
            }
            return task;
        },
        async apply(next, previous, context) {
            const baseline = {...previous, arguments: {...previous.arguments}};
            if (previous._configMapDefaultedBindings === key) delete baseline.arguments[key];
            if (previous._configMapOriginalBindings) baseline.arguments[key] = previous._configMapOriginalBindings;
            return adapter.apply(next, baseline, context);
        },
        groups: [{id: 'configuration', label: 'Task configuration', fields: [
            {path: 'arguments.module_instance_name', label: 'Module connection', type: 'module', required: true,
                pluginId, moduleTypeIds: [stress ? 'stress_device' : 'example_device']},
            ...(stress ? [{path: 'arguments.record_mode', label: 'Recording', type: 'select', defaultValue: 'on_value_change',
                options: [{value: 'on_value_change', label: 'On value change'}, {value: 'every_value', label: 'Every sample'}]}] : []),
            ...(!stress ? [{path: 'arguments.channel_prefix', label: 'Default channel prefix', type: 'text', required: true, defaultValue: 'example_device'}] : []),
        ]}, {id: 'bindings', label: 'Channel bindings', tables: [{path: `arguments.${key}`, label: read ? 'Read bindings' : 'Write bindings', columns: [
            stress ? {path: 'register', label: 'Device register (0–4)', type: 'number', required: true, min: 0, max: 4, integer: true}
                : {path: 'device_field', label: 'Device field', type: 'select', required: true, options: fields.map(value => ({value, label: value}))},
            {path: 'channel', label: read ? 'Publish channel' : 'Command channel', type: 'channel', channelMode: read ? 'write' : 'read', required: true},
            ...(!read && !stress ? [{path: 'readback_channel', label: 'State readback', type: 'channel', channelMode: 'write',
                defaultValue: (_document, row) => row?.channel ? `${row.channel}_state` : undefined}] : []),
        ], createRow: () => ({})}]}],
        validate(value) {
            const rows = value.arguments?.[key] || [];
            const identity = stress ? 'register' : 'device_field';
            if (!rows.length || stress && rows.length !== 5) return stress ? 'Bind all five registers.' : 'Add at least one binding.';
            if (new Set(rows.map(row => row[identity])).size !== rows.length || new Set(rows.map(row => row.channel)).size !== rows.length)
                return 'Use unique device fields/registers and channels.';
            const outputs = rows.flatMap(row => [row.channel, ...(!read && !stress && row.readback_channel ? [row.readback_channel] : [])]);
            return new Set(outputs).size === outputs.length ? '' : 'Command and readback channels must be unique.';
        },
    });
}
export const readMap = taskMap(true), writeMap = taskMap(false);
export const stressReadMap = taskMap(true, true), stressWriteMap = taskMap(false, true);

// Pin the existing project boundary for this editing session, including retries.
const settingsProjects = new WeakMap();
export const settingsMap = defineConfigMap({title: 'Example display', groups: [{id: 'display', label: 'Display', fields: [
    {path: 'display.decimal_places', label: 'Decimal places', type: 'number', integer: true, min: 0, max: 6, required: true},
    {path: 'display.show_units', label: 'Show units', type: 'toggle'},
    {path: 'display.resource_title', label: 'Resource title', type: 'text', required: true,
        validate: value => value.length <= 60 ? '' : 'Use at most 60 characters.'},
]}], async load(context) {
    const {project, snapshot} = await openExampleSettings(context.operation);
    const pinned = settingsProjects.get(context);
    if (pinned && pinned !== project) throw new Error('The active project changed. Reconnect before editing settings for the new project.');
    settingsProjects.set(context, project);
    return {project, revision: snapshot.revision, display: displaySettings(snapshot)};
}, async apply(next, previous, context) {
    const current = await openExampleSettings(context.operation);
    if (current.project !== previous.project) throw new Error('The active project changed. Reconnect before editing settings for the new project.');
    const patch = Object.fromEntries(Object.entries(next.display).filter(([key, value]) => value !== previous.display[key]));
    const storage = exampleSettings(context.operation, previous.project);
    await storage.save(patch, previous.revision);
    if (typeof window !== 'undefined') window.dispatchEvent(new Event('dartwic:example-settings-changed'));
    const snapshot = await storage.read();
    return {project: previous.project, revision: snapshot.revision, display: displaySettings(snapshot)};
}});
