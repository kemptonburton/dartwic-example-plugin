import assert from 'node:assert/strict';
import test from 'node:test';
import {displayDefaults, displayPointers, displaySettings, exampleSettings, formatSample,
    openExampleSettings, pluginId, settingsDefaults} from '../interface/src/settings.mjs';
import {createSettingsAutosave} from '../interface/src/settingsAutosave.mjs';

function fixture() {
    const calls = [];
    let revision = 0;
    const client = exampleSettings(async (name, payload) => {
        calls.push({name, payload});
        return {payload: {effective: settingsDefaults, revision: `revision-${++revision}`}};
    }, 'rocket-test');
    return {client, calls};
}

function clock() {
    let id = 0;
    const callbacks = new Map();
    return {schedule: callback => {callbacks.set(++id, callback); return id;}, cancel: key => callbacks.delete(key),
        tick: async () => {const pending = [...callbacks.values()]; callbacks.clear(); await Promise.all(pending.map(callback => callback()));},
        count: () => callbacks.size};
}

test('settings read supplies defaults and a captured project', async () => {
    const {client, calls} = fixture();
    await client.read();
    assert.deepEqual(calls[0], {name: 'dartwic/settings/get', payload: {defaults: settingsDefaults, project: 'rocket-test'}});
});

test('save writes only the changed field to project scope with revision protection', async () => {
    const {client, calls} = fixture();
    await client.save({show_units: false}, 'revision-12');
    assert.deepEqual(calls[0].payload, {scope: 'project', patch: {plugins: {[pluginId]: {display: {show_units: false}}}},
        reset: [], project: 'rocket-test', revision: 'revision-12'});
    assert.equal(calls.length, 1);
});

test('reset removes only the demo fields, never other plugin settings', async () => {
    const {client, calls} = fixture();
    await client.reset('revision-12');
    assert.deepEqual(calls[0].payload.reset, displayPointers);
    assert.deepEqual(calls[0].payload.patch, {});
    assert.equal(calls[0].payload.project, 'rocket-test');
    assert.equal(calls[0].payload.revision, 'revision-12');
});

test('invalid settings do not write and host errors propagate', async () => {
    const {client, calls} = fixture();
    for (const value of [-1, 7, 1.5, '2', null]) assert.throws(() => client.save({decimal_places: value}, 'rev'), /Decimal places/);
    assert.throws(() => client.save({show_units: 'false'}, 'rev'), /Show units/);
    assert.throws(() => client.save({run_label: 'old'}, 'rev'), /Unknown/);
    assert.equal(calls.length, 0);
    const failing = exampleSettings(async () => ({error: true, payload: {error: 'Settings changed; reload.'}}), 'rocket-test');
    await assert.rejects(failing.save({decimal_places: 3}, 'rev'), /reload/);
});

test('defaults fill missing write-response fields without losing zero or false', () => {
    assert.deepEqual(displaySettings({effective: {}}), displayDefaults);
    const settings = displaySettings({effective: {plugins: {[pluginId]: {display: {decimal_places: 0, show_units: false}}}}});
    assert.equal(formatSample(1234.56789, 'm', settings), '1235');
    assert.equal(formatSample(18.76543, 'bar', displayDefaults), '18.77 bar');
});

test('opening captures the active project before a delayed save', async () => {
    const calls = [];
    let active = 'first';
    const session = await openExampleSettings(async (name, payload) => {
        calls.push({name, payload});
        return {payload: name === 'dartwic/engine/get-config' ? {active_project_name: active}
            : {effective: settingsDefaults, revision: 'first-revision'}};
    });
    active = 'second';
    await session.storage.save({decimal_places: 4}, session.snapshot.revision);
    assert.equal(session.project, 'first');
    assert.equal(calls[1].payload.project, 'first');
    assert.equal(calls[2].payload.project, 'first');
});

test('opening reports missing projects, failed operations, and corrupt plugin values', async () => {
    assert.throws(() => exampleSettings(() => {}, ''), /project/);
    await assert.rejects(openExampleSettings(async () => ({error: true, payload: {error: 'Disconnected'}})), /Disconnected/);
    assert.throws(() => displaySettings({effective: {plugins: {[pluginId]: {display: {decimal_places: 9}}}}}), /Decimal places/);
});

test('rapid edits debounce into one patch containing both changed fields', async () => {
    const timer = clock(), writes = [], states = [];
    const autosave = createSettingsAutosave(async patch => writes.push(patch), {...timer, onState: state => states.push(state)});
    autosave.edit({decimal_places: 1});
    autosave.edit({decimal_places: 3});
    autosave.edit({show_units: false});
    assert.deepEqual(writes, []);
    assert.equal(timer.count(), 1);
    await timer.tick();
    assert.deepEqual(writes, [{decimal_places: 3, show_units: false}]);
    assert.deepEqual(states.at(-1), {pending: false, saving: false, error: ''});
});

test('closing or blurring flushes pending edits exactly once', async () => {
    const timer = clock(), writes = [];
    const autosave = createSettingsAutosave(async patch => writes.push(patch), timer);
    autosave.edit({decimal_places: 6});
    await autosave.flush();
    await timer.tick();
    await autosave.flush();
    assert.deepEqual(writes, [{decimal_places: 6}]);
    assert.equal(timer.count(), 0);
});

test('in-flight writes serialize and later writes use the returned revision', async () => {
    const timer = clock(), writes = [];
    let release, revision = 'initial';
    const autosave = createSettingsAutosave(async patch => {
        writes.push({patch, revision});
        if (writes.length === 1) await new Promise(resolve => {release = resolve;});
        revision = `saved-${writes.length}`;
    }, timer);
    autosave.edit({decimal_places: 1});
    const first = autosave.flush();
    await Promise.resolve();
    autosave.edit({decimal_places: 4});
    autosave.edit({decimal_places: 5});
    autosave.edit({show_units: false});
    const closing = autosave.flush();
    assert.equal(writes.length, 1);
    release();
    await first;
    await closing;
    await timer.tick();
    assert.deepEqual(writes, [{patch: {decimal_places: 1}, revision: 'initial'},
        {patch: {decimal_places: 5, show_units: false}, revision: 'saved-1'}]);
});

test('failed edits remain pending and merge with newer edits on retry', async () => {
    const timer = clock(), writes = [], states = [];
    let fail = true;
    const autosave = createSettingsAutosave(async patch => {
        writes.push(patch);
        if (fail) throw new Error('Engine disconnected');
    }, {...timer, onState: state => states.push(state)});
    autosave.edit({show_units: false});
    await autosave.flush();
    assert.deepEqual(states.at(-1), {pending: true, saving: false, error: 'Engine disconnected'});
    fail = false;
    autosave.edit({decimal_places: 3});
    await timer.tick();
    assert.deepEqual(writes.at(-1), {show_units: false, decimal_places: 3});
    assert.equal(states.at(-1).error, '');
});

test('invalid numeric drafts cancel only that field, not a queued toggle', async () => {
    const timer = clock(), writes = [];
    const autosave = createSettingsAutosave(async patch => writes.push(patch), timer);
    autosave.edit({decimal_places: 4, show_units: false});
    autosave.discardFields(['decimal_places']);
    await timer.tick();
    assert.deepEqual(writes, [{show_units: false}]);
    autosave.edit({decimal_places: 3});
    autosave.discardFields(['decimal_places']);
    await autosave.flush();
    assert.equal(writes.length, 1);
});

test('explicit reload discards failed drafts without writing them during cleanup', async () => {
    const timer = clock(), writes = [];
    const autosave = createSettingsAutosave(async patch => {writes.push(patch); throw new Error('Conflict');}, timer);
    autosave.edit({decimal_places: 3});
    await autosave.flush();
    autosave.discardPending();
    await autosave.flush();
    await timer.tick();
    assert.equal(writes.length, 1);
});
