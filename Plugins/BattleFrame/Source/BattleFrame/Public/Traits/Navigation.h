#pragma once

#include "CoreMinimal.h"
#include "FlowField.h"
#include "BattleFrameEnums.h"
#include "Navigation.generated.h"

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FNavigation
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "流场"))
	TSoftObjectPtr<AFlowField> FlowFieldToUse;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "更换流场后要设该值为true来通知更新数据"))
	bool bReloadFlowField = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "使用A星寻路。禁用后会直线移动到目标点。"))
	bool bUseAStar = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "A星寻路重新计算时间间隔"))
	float AStarCoolDown = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "绘制Debug图形"))
	bool bDrawDebugShape = false;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FNavigating
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "路径"))
	TArray<FVector> PathPoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "路径刷新剩余时间，设置为0可立刻触发刷新"))
	float TimeLeft = 0;

	AFlowField* FlowField = nullptr;

	ENavMode PreviousNavMode = ENavMode::None;

};