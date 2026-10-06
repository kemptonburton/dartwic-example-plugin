import React from '../sdk/react.ts';
import {Button, Input, Label, Switch} from '../sdk/ui/general.ts';
import {formatSample} from './settings.mjs';
import {useDisplaySettings} from './useDisplaySettings.jsx';

export function TelemetryPreview({settings}) {
    return <dl className="grid max-w-md grid-cols-2 gap-x-6 gap-y-2 text-sm">
        <dt className="text-muted-foreground">Altitude</dt>
        <dd className="text-right font-mono tabular-nums">{formatSample(1234.56789, 'm', settings)}</dd>
        <dt className="text-muted-foreground">Tank pressure</dt>
        <dd className="text-right font-mono tabular-nums">{formatSample(18.76543, 'bar', settings)}</dd>
    </dl>;
}

export function ExamplePluginSettings({operation}) {
    const settings = useDisplaySettings(operation);
    const [precision, setPrecision] = React.useState('2');
    const [invalid, setInvalid] = React.useState(false);
    React.useEffect(() => {setPrecision(String(settings.values.decimal_places)); setInvalid(false);}, [settings.values.decimal_places, settings.loaded]);
    const changePrecision = event => {
        const text = event.target.value;
        setPrecision(text);
        const value = Number(text);
        const valid = text.trim() !== '' && Number.isInteger(value) && value >= 0 && value <= 6;
        setInvalid(!valid);
        if (valid) settings.edit({decimal_places: value});
        else settings.discardField('decimal_places');
    };
    return <div className="space-y-4 border-y border-border py-4 text-sm text-foreground">
        <div className="flex max-w-md items-center justify-between gap-4">
            <Label htmlFor="example-decimal-places">Decimal places</Label>
            <Input id="example-decimal-places" className="w-20" type="number" min={0} max={6} step={1}
                value={precision} aria-invalid={invalid} disabled={!settings.loaded || !!settings.error}
                onChange={changePrecision} onBlur={settings.flush}/>
        </div>
        <div className="flex max-w-md items-center justify-between gap-4">
            <Label htmlFor="example-show-units">Show units</Label>
            <Switch id="example-show-units" checked={settings.values.show_units} disabled={!settings.loaded || !!settings.error}
                onCheckedChange={value => settings.edit({show_units: value})}/>
        </div>
        <div className="border-t border-border pt-4">
            <h3 className="mb-3 text-xs text-muted-foreground">Sample telemetry</h3>
            <TelemetryPreview settings={settings.values}/>
        </div>
        <div role="status" aria-live="polite" className="text-xs text-muted-foreground">
            {settings.error ? 'Not saved' : !settings.loaded ? 'Loading...' : invalid ? 'Invalid value' : settings.saving ? 'Saving...' : settings.pending ? 'Pending...' : 'Saved'}
        </div>
        {invalid ? <div role="alert" className="text-red">Enter a whole number from 0 to 6.</div> : null}
        {settings.error ? <div role="alert" className="space-y-2 text-red">
            <div>{settings.error}</div>
            <Button variant="outline" onClick={settings.reload} disabled={settings.saving}>Reload settings</Button>
        </div> : null}
    </div>;
}
