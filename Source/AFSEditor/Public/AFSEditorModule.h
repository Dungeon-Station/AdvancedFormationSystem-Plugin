#pragma once

#include "CoreMinimal.h"
#include "IAssetTools.h"
#include "Modules/ModuleInterface.h"
#include "IAssetTypeActions.h"

class FAFSEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
};

