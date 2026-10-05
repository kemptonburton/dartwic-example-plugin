import assert from 'node:assert/strict';
import test from 'node:test';
import {exampleSettings, pluginId, runLabelPointer, settingsDefaults} from '../interface/src/settings.mjs';

function fixture() {
    const calls = [];
    const client = exampleSettings(async (name, payload) => {
        calls.push({name, payload});
        return {payload: {effective: settingsDefaults, revision: 'opaque-revision'}};
    });
    return {client, calls};
}
test('settings read supplies plugin defaults', async () => {
    const {client, calls} = fixture();
    await client.read();
    assert.deepEqual(calls[0], {name: 'dartwic/settings/get', payload: {defaults: settingsDefaults, project: ''}});
});
for (const scope of ['workspace', 'project']) test(`save targets ${scope} and keeps the revision`, async () => {
    const {client, calls} = fixture();
    await client.save(scope, '  Run 12  ', 'revision-12');
    assert.deepEqual(calls[0].payload, {scope, patch: {plugins: {[pluginId]: {operator: {run_label: 'Run 12'}}}},
        reset: [], project: '', revision: 'revision-12'});
    assert.equal(calls[1].name, 'dartwic/settings/get');
});
test('reset removes only this setting, never the settings file', async () => {
    const {client, calls} = fixture();
    await client.reset('project', 'revision-12');
    assert.deepEqual(calls[0].payload.reset, [runLabelPointer]);
    assert.deepEqual(calls[0].payload.patch, {});
    assert.equal(calls[0].payload.revision, 'revision-12');
});
test('invalid labels do not write and host errors propagate', async () => {
    const {client, calls} = fixture();
    await assert.rejects(client.save('workspace', ' ', 'rev'), /run label/);
    await assert.rejects(client.save('project', 'a'.repeat(81), 'rev'), /run label/);
    assert.equal(calls.length, 0);
    const failing = exampleSettings(async () => ({error: true, payload: {error: 'Settings changed; reload.'}}));
    await assert.rejects(failing.save('workspace', 'Run 1', 'rev'), /reload/);
});
