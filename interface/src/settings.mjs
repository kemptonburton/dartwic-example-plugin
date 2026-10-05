import {createStorageClient} from '../sdk/storage/client.js';

export const pluginId = 'example_device_plugin';
export const settingsDefaults = {plugins: {[pluginId]: {operator: {run_label: 'Mock Rocket Test'}}}};
export const runLabelPointer = `/plugins/${pluginId}/operator/run_label`;

export function runLabel(snapshot) {
    return snapshot.effective.plugins[pluginId].operator.run_label;
}

export function exampleSettings(operation) {
    const storage = createStorageClient(operation);
    return {
        read: () => storage.readEffectiveSettings(settingsDefaults),
        async save(scope, label, revision) {
            const value = String(label).trim();
            if (!value || value.length > 80) throw new Error('Enter a run label from 1 to 80 characters.');
            await storage.writeSettingsOverride(scope, {plugins: {[pluginId]: {operator: {run_label: value}}}}, {revision});
            return storage.readEffectiveSettings(settingsDefaults);
        },
        async reset(scope, revision) {
            await storage.resetSettingsOverride(scope, [runLabelPointer], {revision});
            return storage.readEffectiveSettings(settingsDefaults);
        },
    };
}
