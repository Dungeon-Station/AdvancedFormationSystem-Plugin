#pragma once

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_Formation : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override { return FText::FromString("FormationAsset"); }
    virtual UClass* GetSupportedClass() const override;
    virtual FColor GetTypeColor() const override { return FColor::White; }
    virtual void OpenAssetEditor(
        const TArray<UObject*>& InObjects,
        TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
    virtual uint32 GetCategories() override
    {
        return EAssetTypeCategories::Misc;
    }
};