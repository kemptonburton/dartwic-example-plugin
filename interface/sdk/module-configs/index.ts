import React from "../react.ts";
import type {
    ModuleConfigBridgeState,
    ModuleConfigDefinition,
    ModuleConfigRegistration,
} from "./types.ts";

/** Defines a module instance configuration component. @dartwic-reference @category Module Configuration */
export function defineModuleConfig(definition: ModuleConfigDefinition): ModuleConfigDefinition {
    return definition;
}

/** Synchronizes save, dirty, and validation state with the host module header. @dartwic-reference @category Module Configuration */
export function useModuleConfigBridge(
    moduleEditor: ModuleConfigRegistration | null | undefined,
    config: ModuleConfigBridgeState = {},
) {
    const {
        isDirty = false,
        isSaving = false,
        canSave = true,
        errorMessage = "",
        saveLabel = "SAVE CONFIG",
        onSave = null,
    } = config;

    (React as any).useEffect(() => {
        moduleEditor?.register?.({
            isDirty,
            isSaving,
            canSave,
            errorMessage,
            saveLabel,
            onSave,
        });
    }, [moduleEditor, isDirty, isSaving, canSave, errorMessage, saveLabel, onSave]);
}

export type {
    ModuleConfigBridgeState,
    ModuleConfigDefinition,
    ModuleConfigProps,
    ModuleConfigRegistration,
} from "./types.ts";
