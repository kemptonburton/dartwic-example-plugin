import React from "../sdk/react.ts";
import {ModuleRuntimeOverview} from "../sdk/ui/dartwic.ts";
import {resolveExampleTaskChannels} from "./deviceBindings.mjs";

export function ExampleModuleConfig({instanceConfig}) {
    const instanceName = instanceConfig?.name || "";
    return <ModuleRuntimeOverview instanceName={instanceName}
        resolveTaskChannels={resolveExampleTaskChannels}
        emptyMessage="NO TASKS LINKED TO THIS MODULE" className="pt-2" />;
}
