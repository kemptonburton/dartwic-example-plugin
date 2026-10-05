import React from "../sdk/react.ts";
import {useDartwic} from "../sdk/hooks/index.ts";
import {exampleSettings, runLabel} from "./settings.mjs";

export function ExampleResource({setIsLoaded}) {
    const {operation} = useDartwic();
    const [label, setLabel] = React.useState("Mock Rocket Test");
    const [error, setError] = React.useState("");
    React.useEffect(() => {
        setIsLoaded?.(true);
    }, [setIsLoaded]);
    React.useEffect(() => {
        let active = true;
        exampleSettings(operation).read().then(snapshot => {
            if (active) { setLabel(runLabel(snapshot)); setError(""); }
        }).catch(caught => { if (active) setError(caught.message); });
        return () => { active = false; };
    }, [operation]);

    return (
        <div className="p-4 text-sm text-muted-foreground">
            <h2 className="text-sm font-medium text-foreground">{label}</h2>
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
