import React from '../sdk/react.ts';
import {createNotificationClient} from '../sdk/notifications/index.ts';
import {displayDefaults, displaySettings, openExampleSettings, pluginId, validateDisplayPatch} from './settings.mjs';
import {createSettingsAutosave} from './settingsAutosave.mjs';

export function useDisplaySettings(operation) {
    const [values, setValues] = React.useState(displayDefaults);
    const [loaded, setLoaded] = React.useState(false);
    const [state, setState] = React.useState({pending: false, saving: false, error: ''});
    const [generation, setGeneration] = React.useState(0);
    const controller = React.useRef(null);
    React.useEffect(() => {
        let active = true;
        let autosave;
        const notifications = createNotificationClient(pluginId);
        const report = next => {
            if (active) setState(next);
            if (next.error) notifications.reportIssue({id: 'display-settings', severity: 'error',
                title: 'Example display settings not saved', description: next.error,
                location: {tab: 'plugins', pluginName: 'Example Device Plugin'}});
            else notifications.clearIssue('display-settings');
        };
        setLoaded(false);
        setState({pending: false, saving: false, error: ''});
        openExampleSettings(operation).then(({storage, snapshot}) => {
            if (!active) return;
            let revision = snapshot.revision;
            autosave = createSettingsAutosave(async patch => {
                const saved = await storage.save(patch, revision);
                revision = saved.revision;
            }, {onState: report});
            controller.current = autosave;
            setValues(displaySettings(snapshot));
            setLoaded(true);
            notifications.clearIssue('display-settings');
        }).catch(cause => {if (active) report({pending: false, saving: false, error: cause.message});});
        return () => {
            active = false;
            controller.current = null;
            // The old session retains its operation, project, and revision.
            void autosave?.flush();
        };
    }, [operation, generation]);
    const edit = patch => {
        validateDisplayPatch(patch);
        if (!controller.current) return;
        setValues(previous => ({...previous, ...patch}));
        controller.current.edit(patch);
    };
    return {values, loaded, ...state, edit, flush: () => controller.current?.flush(),
        discardField: key => controller.current?.discardFields([key]),
        reload: () => {controller.current?.discardPending(); setGeneration(value => value + 1);}};
}
