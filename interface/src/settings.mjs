import {createStorageClient} from '../sdk/storage/client.js';

export const pluginId = 'example_device_plugin';
export const displayDefaults = {decimal_places: 2, show_units: true};
export const settingsDefaults = {plugins: {[pluginId]: {display: displayDefaults}}};
export const displayPointers = Object.keys(displayDefaults).map(key => `/plugins/${pluginId}/display/${key}`);

export function validateDisplayPatch(patch) {
    if (!patch || typeof patch !== 'object' || Array.isArray(patch)) throw new Error('Expected display settings.');
    for (const [key, value] of Object.entries(patch)) {
        if (key === 'decimal_places') {
            if (!Number.isInteger(value) || value < 0 || value > 6) throw new Error('Decimal places must be an integer from 0 to 6.');
        } else if (key === 'show_units') {
            if (typeof value !== 'boolean') throw new Error('Show units must be true or false.');
        } else throw new Error(`Unknown display setting: ${key}`);
    }
    return patch;
}

export function displaySettings(snapshot) {
    const value = {...displayDefaults, ...snapshot.effective.plugins?.[pluginId]?.display};
    return validateDisplayPatch(value);
}

export function formatSample(value, units, settings) {
    return `${value.toFixed(settings.decimal_places)}${settings.show_units ? ` ${units}` : ''}`;
}

export function exampleSettings(operation, project) {
    if (!project) throw new Error('Select a project before editing settings.');
    const storage = createStorageClient(operation);
    return {
        read: () => storage.readEffectiveSettings(settingsDefaults, project),
        save(patch, revision) {
            validateDisplayPatch(patch);
            return storage.writeSettingsOverride('project', {plugins: {[pluginId]: {display: patch}}}, {project, revision});
        },
        reset: revision => storage.resetSettingsOverride('project', displayPointers, {project, revision}),
    };
}

export async function openExampleSettings(operation) {
    const response = await operation('dartwic/engine/get-config', {});
    if (!response || response.error) throw new Error(response?.payload?.error || 'Could not read the active project.');
    // Capture once so a delayed save cannot follow a project switch.
    const project = response.payload.active_project_name;
    const storage = exampleSettings(operation, project);
    const snapshot = await storage.read();
    displaySettings(snapshot);
    return {project, storage, snapshot};
}
