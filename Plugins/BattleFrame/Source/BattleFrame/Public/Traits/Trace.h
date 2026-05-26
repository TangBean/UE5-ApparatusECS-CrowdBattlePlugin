#pragma once

#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "BattleFrameStructs.h"
#include "Trace.generated.h"

class UNeighborGridComponent;


USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSectorTraceParams
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "检测目标是不是在障碍物后面"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野半径", DisplayName = "Radius"))
	float TraceRadius = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UIMin = 0, UIMax = 360, Tooltip = "索敌视野角度", DisplayName = "Angle"))
	float TraceAngle = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野高度", DisplayName = "Height"))
	float TraceHeight = 300.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "朝向偏移"))
	float YawOffset = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "每几秒索敌一次", DisplayName = "Frequency"))
	float CoolDown = 2;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSectorTraceParamsSpecific
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "启用"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "检测目标是不是在障碍物后面"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野半径", DisplayName = "Radius"))
	float TraceRadius = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UIMin = 0, UIMax = 360, Tooltip = "索敌视野角度", DisplayName = "Angle"))
	float TraceAngle = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野高度", DisplayName = "Height"))
	float TraceHeight = 300.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "朝向偏移"))
	float YawOffset = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "每几秒索敌一次", DisplayName = "Frequency"))
	float CoolDown = 2;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSectorTraceShape
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "通用的扇形索敌参数", DisplayName = "Common"))
	FSectorTraceParams Common = FSectorTraceParams{ true };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Sleep专用扇形索敌范围，可禁用，禁用后使用通用参数", DisplayName = "Sleep"))
	FSectorTraceParamsSpecific Sleep = FSectorTraceParamsSpecific{ false };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Patrol专用扇形索敌范围，可禁用，禁用后使用通用参数", DisplayName = "Patrol"))
	FSectorTraceParamsSpecific Patrol = FSectorTraceParamsSpecific{ false };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Chase专用扇形索敌范围，可禁用，禁用后使用通用参数", DisplayName = "Chase"))
	FSectorTraceParamsSpecific Chase = FSectorTraceParamsSpecific{ false };

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FBFFilter
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "包含具有这些特征的目标"))
	TArray<UScriptStruct*> IncludeTraits;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "排除具有这些特征的目标"))
	TArray<UScriptStruct*> ExcludeTraits;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "障碍物碰撞类型"))
	TArray<TEnumAsByte<EObjectTypeQuery>> ObstacleObjectType;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTrace
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用索敌功能"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "绘制Debug图形"))
	bool bDrawDebugShape = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "扇形索敌参数"))
	FSectorTraceShape SectorTrace = FSectorTraceShape();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌模式 (TargetIsPlayer_0: 目标为玩家, SphereTraceByTraits: 根据特征索敌)"))
	ETraceMode Mode = ETraceMode::TargetIsPlayer_0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "索敌条件过滤器"))
	FBFFilter Filter = FBFFilter();

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTracing
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "索敌结果"))
	FSubjectHandle TraceResult = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "位于的邻居网格"))
	UNeighborGridComponent* NeighborGrid = nullptr;

	float TimeLeft = 0.f;


	FTracing() {};

	FTracing(const FTracing& Tracing)
	{
		LockFlag.store(Tracing.LockFlag.load());

		NeighborGrid = Tracing.NeighborGrid;
		TraceResult = Tracing.TraceResult;
		TimeLeft = Tracing.TimeLeft;
	}

	FTracing& operator=(const FTracing& Tracing)
	{
		LockFlag.store(Tracing.LockFlag.load());

		NeighborGrid = Tracing.NeighborGrid;
		TraceResult = Tracing.TraceResult;
		TimeLeft = Tracing.TimeLeft;

		return *this;
	}
};




