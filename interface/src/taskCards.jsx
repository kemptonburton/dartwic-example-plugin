import React from "../sdk/react.ts";
import {Badge, Card, CardContent, CardHeader, CardTitle} from "../sdk/ui/general.ts";

export function ExampleTaskCard() {
    return (
        <Card><CardHeader className="flex flex-row items-center justify-between"><CardTitle>Example task</CardTitle><Badge>Mock</Badge></CardHeader>
            <CardContent className="text-sm text-muted-foreground">Example task card body.</CardContent></Card>
    );
}
