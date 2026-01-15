#ifndef BINDINGS_COMMANDQUEUE_INTERNAL_HPP
#define BINDINGS_COMMANDQUEUE_INTERNAL_HPP

#include <jni.h>
#include <vector>
#include "command_server.hpp"
#include "jni_helpers.hpp"
#include "rive_log.hpp"

using namespace rive_android;
using namespace rive_mp;

// Cached method IDs for callbacks
extern jmethodID g_onFileLoadedMethodID;
extern jmethodID g_onFileErrorMethodID;
extern jmethodID g_onFileDeletedMethodID;
extern jmethodID g_onArtboardNamesListedMethodID;
extern jmethodID g_onStateMachineNamesListedMethodID;
extern jmethodID g_onViewModelNamesListedMethodID;
extern jmethodID g_onQueryErrorMethodID;
extern jmethodID g_onArtboardCreatedMethodID;
extern jmethodID g_onArtboardErrorMethodID;
extern jmethodID g_onArtboardDeletedMethodID;
extern jmethodID g_onStateMachineCreatedMethodID;
extern jmethodID g_onStateMachineErrorMethodID;
extern jmethodID g_onStateMachineDeletedMethodID;
extern jmethodID g_onStateMachineSettledMethodID;
// Input operation callbacks
extern jmethodID g_onInputCountResultMethodID;
extern jmethodID g_onInputNamesListedMethodID;
extern jmethodID g_onInputInfoResultMethodID;
extern jmethodID g_onNumberInputValueMethodID;
extern jmethodID g_onBooleanInputValueMethodID;
extern jmethodID g_onInputOperationSuccessMethodID;
extern jmethodID g_onInputOperationErrorMethodID;
// ViewModelInstance callbacks
extern jmethodID g_onVMICreatedMethodID;
extern jmethodID g_onVMIErrorMethodID;
extern jmethodID g_onVMIDeletedMethodID;
// Property operation callbacks (Phase D.2)
extern jmethodID g_onNumberPropertyValueMethodID;
extern jmethodID g_onStringPropertyValueMethodID;
extern jmethodID g_onBooleanPropertyValueMethodID;
extern jmethodID g_onPropertyErrorMethodID;
extern jmethodID g_onPropertySetSuccessMethodID;
// Additional property callbacks (Phase D.3)
extern jmethodID g_onEnumPropertyValueMethodID;
extern jmethodID g_onColorPropertyValueMethodID;
extern jmethodID g_onTriggerFiredMethodID;
// Property subscription update callbacks (Phase D.4)
extern jmethodID g_onNumberPropertyUpdatedMethodID;
extern jmethodID g_onStringPropertyUpdatedMethodID;
extern jmethodID g_onBooleanPropertyUpdatedMethodID;
extern jmethodID g_onEnumPropertyUpdatedMethodID;
extern jmethodID g_onColorPropertyUpdatedMethodID;
extern jmethodID g_onTriggerPropertyFiredMethodID;
// List operation callbacks (Phase D.5)
extern jmethodID g_onListSizeResultMethodID;
extern jmethodID g_onListItemResultMethodID;
extern jmethodID g_onListOperationSuccessMethodID;
extern jmethodID g_onListOperationErrorMethodID;
// Nested VMI operation callbacks (Phase D.5)
extern jmethodID g_onInstancePropertyResultMethodID;
extern jmethodID g_onInstancePropertySetSuccessMethodID;
extern jmethodID g_onInstancePropertyErrorMethodID;
// Asset property operation callbacks (Phase D.5)
extern jmethodID g_onAssetPropertySetSuccessMethodID;
extern jmethodID g_onAssetPropertyErrorMethodID;
// VMI binding operation callbacks (Phase D.6)
extern jmethodID g_onVMIBindingSuccessMethodID;
extern jmethodID g_onVMIBindingErrorMethodID;
extern jmethodID g_onDefaultVMIResultMethodID;
extern jmethodID g_onDefaultVMIErrorMethodID;
// Render target operation callbacks (Phase C.2.3)
extern jmethodID g_onRenderTargetCreatedMethodID;
extern jmethodID g_onRenderTargetErrorMethodID;
extern jmethodID g_onRenderTargetDeletedMethodID;
// Asset operation callbacks (Phase E.1)
extern jmethodID g_onImageDecodedMethodID;
extern jmethodID g_onImageErrorMethodID;
extern jmethodID g_onAudioDecodedMethodID;
extern jmethodID g_onAudioErrorMethodID;
extern jmethodID g_onFontDecodedMethodID;
extern jmethodID g_onFontErrorMethodID;

/**
 * Initialize cached method IDs for JNI callbacks.
 * This is called once during first use to avoid repeated JNI lookups.
 */
void initCallbackMethodIDs(JNIEnv* env, jobject commandQueue);

#endif // BINDINGS_COMMANDQUEUE_INTERNAL_HPP