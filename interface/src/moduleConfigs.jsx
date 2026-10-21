import React from "../sdk/react.ts";
import {ModuleRuntimeOverview} from "../sdk/ui/dartwic.ts";
import {exampleTaskTypeIds, resolveExampleTaskChannels} from "./deviceBindings.mjs";

export function ExampleModuleConfig({instanceConfig}) {
    const instanceName = instanceConfig?.name || "";
    return (
        <ModuleRuntimeOverview instanceName={instanceName} taskTypeIds={exampleTaskTypeIds}
            resolveTaskChannels={resolveExampleTaskChannels} />
    );
}
