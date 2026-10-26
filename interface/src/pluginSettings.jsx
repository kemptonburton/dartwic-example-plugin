import React from '../sdk/react.ts';
import {ConfigMap} from '../sdk/config-maps/index.ts';

export function ExamplePluginSettings() {
    return <div className="space-y-4">
        <ConfigMap groupIds={["display"]}/>
        <hr className="border-border"/>
        <p className="text-xs text-muted-foreground">Open Example Telemetry to see these project display settings applied to live device values.</p>
    </div>;
}
