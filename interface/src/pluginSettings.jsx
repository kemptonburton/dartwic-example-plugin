import React from '../sdk/react.ts';
import {Button, Input, Label, Switch} from '../sdk/ui/general.ts';
import {useDisplaySettings} from './useDisplaySettings.jsx';

export function ExamplePluginSettings({operation}) {
    const settings = useDisplaySettings(operation);
    const [precision, setPrecision] = React.useState('2');
    const [resourceTitle, setResourceTitle] = React.useState('Engine test telemetry');
    const [invalid, setInvalid] = React.useState(false);
    const [invalidTitle, setInvalidTitle] = React.useState(false);
    React.useEffect(() => {setPrecision(String(settings.values.decimal_places)); setInvalid(false);}, [settings.values.decimal_places, settings.loaded]);
    React.useEffect(() => {setResourceTitle(settings.values.resource_title); setInvalidTitle(false);}, [settings.values.resource_title, settings.loaded]);
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
        <div className="max-w-md space-y-1.5">
            <Label htmlFor="example-resource-title">Resource title</Label>
            <Input id="example-resource-title" maxLength={60} value={resourceTitle} aria-invalid={invalidTitle}
                disabled={!settings.loaded || !!settings.error}
                onChange={event => {
                    const value = event.target.value;
                    setResourceTitle(value);
                    setInvalidTitle(!value.trim());
                    if (value.trim()) settings.edit({resource_title: value});
                    else settings.discardField('resource_title');
                }} onBlur={settings.flush}/>
        </div>
        <div className="border-t border-border pt-4">
            <p className="text-xs text-muted-foreground">Open Example Telemetry to see these saved settings applied to live values from the example device tasks.</p>
        </div>
        <div role="status" aria-live="polite" className="text-xs text-muted-foreground">
            {settings.error ? 'Not saved' : !settings.loaded ? 'Loading...' : invalid || invalidTitle ? 'Invalid value' : settings.saving ? 'Saving...' : settings.pending ? 'Pending...' : 'Saved'}
        </div>
        {invalid ? <div role="alert" className="text-red">Enter a whole number from 0 to 6.</div> : null}
        {invalidTitle ? <div role="alert" className="text-red">Enter a resource title.</div> : null}
        {settings.error ? <div role="alert" className="space-y-2 text-red">
            <div>{settings.error}</div>
            <Button variant="outline" onClick={settings.reload} disabled={settings.saving}>Reload settings</Button>
        </div> : null}
    </div>;
}
