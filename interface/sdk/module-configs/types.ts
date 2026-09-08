/** Bridge exposed by the host module editor to a plugin configuration component. @dartwic-reference @category Module Configuration */
export interface ModuleConfigRegistration {
    register(nextState?: ModuleConfigBridgeState): void;
    setDirty?(isDirty: boolean): void;
    setSaving?(isSaving: boolean): void;
    setCanSave?(canSave: boolean): void;
    setErrorMessage?(errorMessage: string): void;
    reset?(): void;
}

/** State published to the host by `useModuleConfigBridge`. @dartwic-reference @category Module Configuration */
export interface ModuleConfigBridgeState {
    isDirty?: boolean;
    isSaving?: boolean;
    canSave?: boolean;
    errorMessage?: string;
    saveLabel?: string;
    onSave?: (() => Promise<void> | void) | null;
}

/** Props passed to a module configuration component. @dartwic-reference @category Module Configuration */
export interface ModuleConfigProps {
    instanceConfig: any;
    setInstanceConfig: (nextValue: any) => void;
    save: () => Promise<void> | void;
    moduleConfig?: any;
    moduleEditor?: ModuleConfigRegistration | null;
}

/** Component registered as a module configuration editor. @dartwic-reference @category Module Configuration */
export interface ModuleConfigDefinition {
    component: (props: ModuleConfigProps) => any;
}
