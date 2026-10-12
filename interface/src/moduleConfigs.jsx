import React from "../sdk/react.ts";
import {Badge, Card, CardContent, CardHeader, CardTitle} from "../sdk/ui/general.ts";

export function ExampleModuleConfig() {
    return (
        <Card><CardHeader className="flex flex-row items-center justify-between"><CardTitle>Example module</CardTitle><Badge>Mock device</Badge></CardHeader>
            <CardContent className="text-sm text-muted-foreground">Example module configuration body.</CardContent></Card>
    );
}

export function RocketModuleConfig() {
    return <div className="space-y-2 text-sm">
        <p>This module simulates device registers and actuator feedback. It opens no hardware connection.</p>
        <p>Bind Mock Rocket Read and Mock Rocket Write to this instance. Use their shared channel prefix to choose the signal namespace. Fault inputs are exposed on the rocket schematic.</p>
    </div>;
}
