import React from "../sdk/react.ts";
import {Button, Input, Label} from "../sdk/ui/general.ts";
import {exampleSettings, runLabel, runLabelPointer} from "./settings.mjs";

export function ExamplePluginSettings({operation}) {
    const [snapshot, setSnapshot] = React.useState(null);
    const [label, setLabel] = React.useState("");
    const [scope, setScope] = React.useState("workspace");
    const [busy, setBusy] = React.useState(false);
    const [error, setError] = React.useState("");
    const storage = React.useMemo(() => exampleSettings(operation), [operation]);
    React.useEffect(() => {
        let active = true;
        storage.read().then(value => {
            if (active) { setSnapshot(value); setLabel(runLabel(value)); setError(""); }
        }).catch(caught => { if (active) setError(caught.message); });
        return () => { active = false; };
    }, [storage]);
    async function update(reset) {
        setBusy(true);
        setError("");
        try {
            const value = reset ? await storage.reset(scope, snapshot.revision)
                : await storage.save(scope, label, snapshot.revision);
            setSnapshot(value);
            setLabel(runLabel(value));
        } catch (caught) { setError(caught.message); }
        finally { setBusy(false); }
    }
    return (
        <div className="space-y-4 border-y border-border py-4 text-sm text-foreground">
            <fieldset className="flex flex-wrap gap-4" disabled={busy}>
                <legend className="mb-2 text-xs text-muted-foreground">LOCATION</legend>
                {["workspace", "project"].map(value => <label key={value} className="flex items-center gap-2">
                    <input type="radio" name="example-settings-scope" value={value} checked={scope === value}
                        onChange={() => setScope(value)} />
                    {value === "workspace" ? "Workspace" : "Current project"}
                </label>)}
            </fieldset>
            <div className="max-w-md space-y-2">
                <Label htmlFor="example-run-label">RUN LABEL</Label>
                <Input id="example-run-label" value={label} maxLength={80} disabled={!snapshot || busy}
                    onChange={event => setLabel(event.target.value)} />
                {snapshot ? <div className="text-xs text-muted-foreground">
                    SOURCE: {snapshot.sources?.[runLabelPointer] || "defaults"}
                </div> : null}
            </div>
            <div className="flex flex-wrap gap-2">
                <Button disabled={!snapshot || busy || !label.trim()} onClick={() => update(false)}>Save</Button>
                <Button variant="outline" disabled={!snapshot || busy} onClick={() => update(true)}>Reset</Button>
            </div>
            {error ? <div role="alert" className="text-red">{error}</div> : null}
        </div>
    );
}
