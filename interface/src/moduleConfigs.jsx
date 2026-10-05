import React from "../sdk/react.ts";

export function ExampleModuleConfig() {
    return (
        <div className="rounded-lg border border-border bg-card p-4 text-sm text-muted-foreground">
            Example module configuration body.
        </div>
    );
}

export function RocketModuleConfig() {
    return <div className="space-y-2 text-sm">
        <p>This module simulates device registers and actuator feedback. It opens no hardware connection.</p>
        <p>Bind Mock Rocket Read and Mock Rocket Write to this instance. Use their shared channel prefix to choose the signal namespace. Fault inputs are exposed on the rocket schematic.</p>
    </div>;
}
