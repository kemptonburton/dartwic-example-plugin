import React from "../sdk/react.ts";
import {useDartwic} from "../sdk/hooks/index.ts";
import {displayDefaults, displaySettings, openExampleSettings} from "./settings.mjs";
import {TelemetryPreview} from "./pluginSettings.jsx";
import {
    Badge, Button, Card, CardContent, CardDescription, CardHeader, CardTitle,
    Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle, DialogTrigger,
} from "../sdk/ui/general.ts";

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
        <div className="p-4">
            <Card className="max-w-xl">
                <CardHeader className="flex flex-row items-start justify-between gap-3">
                    <div className="space-y-1"><CardTitle>Sample telemetry</CardTitle><CardDescription>Mock device readings using host UI controls.</CardDescription></div>
                    <Badge>Example</Badge>
                </CardHeader>
                <CardContent className="space-y-4">
                    <TelemetryPreview settings={settings}/>
                    <Dialog>
                        <DialogTrigger asChild><Button variant="outline" size="sm">About this resource</Button></DialogTrigger>
                        <DialogContent>
                            <DialogHeader><DialogTitle>Example resource</DialogTitle><DialogDescription>This view uses the host's card, badge, button, and dialog components through the Interface SDK.</DialogDescription></DialogHeader>
                        </DialogContent>
                    </Dialog>
                    {error ? <div role="alert" className="text-sm text-red">{error}</div> : null}
                </CardContent>
            </Card>
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
