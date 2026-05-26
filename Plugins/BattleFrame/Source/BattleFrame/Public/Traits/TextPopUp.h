#pragma once

#include "CoreMinimal.h"
#include "TextPopUp.generated.h"

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTextPopUp
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用文本弹出效果"))
	bool Enable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "白色文本显示的百分比阈值（低于此值时显示白色文本）"))
	float WhiteTextBelowPercent = 0.333f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "橙色文本显示的百分比阈值（低于此值时显示黄色文本）"))
	float OrangeTextAbovePercent = 0.667f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "文本大小"))
	float TextScale = 300;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FPoppingText
{
    GENERATED_BODY()

private:

    mutable std::atomic<bool> LockFlag{ false };

public:

    void Lock() const
    {
        while (LockFlag.exchange(true, std::memory_order_acquire));
    }

    void Unlock() const
    {
        LockFlag.store(false, std::memory_order_release);
    }

public:

    TArray<FVector> TextLocationArray;
    TArray<FVector4> Text_Value_Style_Scale_Offset_Array; // 4 in one

    FPoppingText() {};

    FPoppingText(const FPoppingText& PoppingText)
    {
        LockFlag.store(PoppingText.LockFlag.load());

        TextLocationArray = PoppingText.TextLocationArray;
        Text_Value_Style_Scale_Offset_Array = PoppingText.Text_Value_Style_Scale_Offset_Array;
    }

    FPoppingText& operator=(const FPoppingText& PoppingText)
    {
        LockFlag.store(PoppingText.LockFlag.load());

        TextLocationArray = PoppingText.TextLocationArray;
        Text_Value_Style_Scale_Offset_Array = PoppingText.Text_Value_Style_Scale_Offset_Array;

        return *this;
    }

};

