import React from "../sdk/react.ts";
import {useDartwic, useDartwicChannelValues} from "../sdk/hooks/index.ts";
import {displayDefaults, displaySettings, formatSample, openExampleSettings} from "./settings.mjs";
import {exampleTaskMappings} from "./deviceBindings.mjs";
import {
    Badge, Button, Card, CardContent, CardDescription, CardHeader, CardTitle,
    Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle, DialogTrigger,
} from "../sdk/ui/general.ts";

export function ExampleResource({setIsLoaded}) {
    const {operation, addChannelToTelemetry, removeChannelFromTelemetry} = useDartwic();
    const [settings, setSettings] = React.useState(displayDefaults);
    const [bindings, setBindings] = React.useState({});
    const [moduleName, setModuleName] = React.useState("");
    const [error, setError] = React.useState("");
    const channelsToWatch = React.useMemo(() => [...new Set(Object.values(bindings).filter(Boolean))], [bindings]);
    const channels = useDartwicChannelValues(channelsToWatch);
    React.useEffect(() => {
        setIsLoaded?.(true);
    }, [setIsLoaded]);
    React.useEffect(() => {
        let active = true;
        const refresh = () => {
            openExampleSettings(operation).then(({snapshot}) => {
                if (active) { setSettings(displaySettings(snapshot)); setError(""); }
            }).catch(caught => { if (active) setError(caught.message); });
        };
        refresh();
        window.addEventListener('dartwic:example-settings-changed', refresh);
        window.addEventListener('focus', refresh);
        return () => {
            active = false;
            window.removeEventListener('dartwic:example-settings-changed', refresh);
            window.removeEventListener('focus', refresh);
        };
    }, [operation]);
    React.useEffect(() => {
        let active = true;
        const refresh = async () => {
            try {
                const response = await operation('dartwic/get-registered-tasks', {scope: 'tasks'}, 15000);
                if (response?.error) throw new Error(response.payload?.error || 'Could not list example device tasks.');
                const tasks = response?.payload?.tasks || [];
                const read = tasks.find(task => task.task_type === 'example_device_plugin.example_read');
                const write = tasks.find(task => task.task_type === 'example_device_plugin.example_write'
                    && task.arguments?.module_instance_name === read?.arguments?.module_instance_name);
                const rows = [...exampleTaskMappings(read), ...exampleTaskMappings(write)];
                const writeStates = Object.fromEntries(exampleTaskMappings(write)
                    .map(row => [`${row.device_field}_state`, row.readback_channel]));
                if (active) {
                    setModuleName(read?.arguments?.module_instance_name || '');
                    setBindings({...Object.fromEntries(rows.map(row => [row.device_field, row.channel])),
                        ...writeStates,
                        run_fill: `${read?.arguments?.channel_prefix || 'example_device'}_run_fill`});
                }
            } catch (cause) { if (active) setError(cause.message); }
        };
        void refresh();
        window.addEventListener('dartwic:tasks-changed', refresh);
        window.addEventListener('focus', refresh);
        return () => {
            active = false;
            window.removeEventListener('dartwic:tasks-changed', refresh);
            window.removeEventListener('focus', refresh);
        };
    }, [operation]);
    React.useEffect(() => {
        for (const channel of channelsToWatch) addChannelToTelemetry(channel);
        return () => {for (const channel of channelsToWatch) removeChannelFromTelemetry(channel);};
    }, [addChannelToTelemetry, removeChannelFromTelemetry, channelsToWatch]);

    const rows = [
        ['Engine temperature', 'temperature_1', 'K'],
        ['Pressure sensor', 'pressure_current_ma', 'mA'],
        ['Run tank fill', 'run_fill', '%'],
        ['Fuel valve command', 'fuel_open_coil', ''],
        ['Fuel open coil state', 'fuel_open_coil_state', ''],
        ['Fuel close coil state', 'fuel_close_coil_state', ''],
        ['Igniter state', 'igniter_command_state', ''],
        ['Connected', 'connected', ''],
    ];

    return (
        <div className="p-4">
            <Card className="max-w-xl">
                <CardHeader className="flex flex-row items-start justify-between gap-3">
                    <div className="space-y-1"><CardTitle>{settings.resource_title}</CardTitle><CardDescription>Live values from {moduleName || 'the configured example device'}.</CardDescription></div>
                    <Badge>Device</Badge>
                </CardHeader>
                <CardContent className="space-y-4">
                    <dl className="grid max-w-md grid-cols-2 gap-x-6 gap-y-2 text-sm">
                        {rows.map(([label, field, units]) => {
                            const value = channels?.[bindings[field]]?.channel_data?.value;
                            const numeric = Number(value);
                            return <React.Fragment key={field}>
                                <dt className="text-muted-foreground">{label}</dt>
                                <dd className="text-right font-mono tabular-nums">{value == null || !Number.isFinite(numeric) ? '—' : formatSample(numeric, units, settings).trim()}</dd>
                            </React.Fragment>;
                        })}
                    </dl>
                    <Dialog>
                        <DialogTrigger asChild><Button variant="outline" size="sm">About this resource</Button></DialogTrigger>
                        <DialogContent>
                            <DialogHeader><DialogTitle>Example resource</DialogTitle><DialogDescription>Settings change this resource title and number formatting. Values follow the read and write task binding tables.</DialogDescription></DialogHeader>
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
