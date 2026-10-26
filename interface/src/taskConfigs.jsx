import React from "../sdk/react.ts";
import {ConfigMap} from "../sdk/config-maps/index.ts";

// One registered document; two tables interleaved with ordinary components.
export function ExampleDeviceTaskConfig() {
    return <div className="space-y-4">
        <p className="text-xs text-muted-foreground">Choose the simulated device and its control-channel prefix.</p>
        <ConfigMap groupIds={["configuration"]}/>
        <hr className="border-border"/>
        <p className="text-xs text-muted-foreground">Map device fields to RAPID. Write readbacks report simulated output state.</p>
        <ConfigMap groupIds={["bindings"]}/>
    </div>;
}
