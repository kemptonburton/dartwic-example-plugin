import React from "../sdk/react.ts";
import {useDartwic} from "../sdk/hooks/index.ts";
import {displayDefaults, displaySettings, openExampleSettings} from "./settings.mjs";
import {TelemetryPreview} from "./pluginSettings.jsx";

export function ExampleResource({setIsLoaded}) {
    const {operation} = useDartwic();
    const [settings, setSettings] = React.useState(displayDefaults);
    const [error, setError] = React.useState("");
    React.useEffect(() => {
        setIsLoaded?.(true);
    }, [setIsLoaded]);
    React.useEffect(() => {
        let active = true;
        openExampleSettings(operation).then(({snapshot}) => {
            if (active) { setSettings(displaySettings(snapshot)); setError(""); }
        }).catch(caught => { if (active) setError(caught.message); });
        return () => { active = false; };
    }, [operation]);

    return (
        <div className="p-4 text-sm text-muted-foreground">
            <h2 className="mb-3 text-sm font-medium text-foreground">Sample telemetry</h2>
            <TelemetryPreview settings={settings}/>
            {error ? <div role="alert" className="mt-2 text-red">{error}</div> : null}
        </div>
    );
}

export function ExampleSchematicNode({data}) {
    return (
        <div className="h-full w-full rounded-md border border-border bg-card p-2 text-xs text-foreground">
            {data?.label ?? "Example"}
        </div>
    );
}
