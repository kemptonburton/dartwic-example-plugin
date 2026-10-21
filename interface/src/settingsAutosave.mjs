// Pending field patches merge; writes never overlap or replace newer edits.
export function createSettingsAutosave(save, {delay = 500, onState = () => {}, schedule = setTimeout, cancel = clearTimeout} = {}) {
    let pending = null, timer = null, running = null, error = '';
    const publish = () => onState({pending: pending !== null, saving: running !== null, error});
    const clearTimer = () => {if (timer !== null) cancel(timer); timer = null;};
    const flush = () => {
        clearTimer();
        if (running) return running;
        if (pending === null) return Promise.resolve();
        running = Promise.resolve().then(async () => {
            while (pending !== null) {
                const next = pending;
                pending = null;
                publish();
                try {await save(next); error = '';}
                catch (cause) {
                    pending = {...next, ...pending};
                    clearTimer();
                    error = cause?.message || 'Unable to save settings.';
                    break;
                }
            }
        }).finally(() => {running = null; publish();});
        publish();
        return running;
    };
    return {
        edit(patch) {
            clearTimer();
            pending = {...pending, ...patch};
            error = '';
            publish();
            timer = schedule(flush, delay);
        },
        discardFields(keys) {
            if (pending) {
                for (const key of keys) delete pending[key];
                if (!Object.keys(pending).length) {pending = null; clearTimer();}
            }
            publish();
        },
        discardPending() {clearTimer(); pending = null; error = ''; publish();},
        flush,
    };
}
