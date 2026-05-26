#pragma once

#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "Curves/CurveFloat.h"
#include "BattleFrameStructs.h"
#include "Traits/Trace.h"
#include "ProjectileConfig.generated.h" 

class UNeighborGridComponent;
class UProjectileConfigDataAsset;

UENUM(BlueprintType)
enum class EProjectileSolveMode : uint8
{
	FromPitch UMETA(DisplayName = "FromPitch", Tooltip = ""),
	FromSpeed UMETA(DisplayName = "FromSpeed", Tooltip = ""),
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FIsAttachedFx
{
	GENERATED_BODY()

public:

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FIsBurstFx
{
	GENERATED_BODY()

public:

};


//------------------Common Params----------------------

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileParams
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "碰撞检测半径"))
	float Radius = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "剩余寿命"))
	float LifeSpan = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "血量，每发生一次碰撞，血量-1，归零后不再造成伤害"))
	int32 Health = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "碰撞条件"))
	FBFFilter Filter = FBFFilter();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "绘制调试图形"))
	bool bDrawDebugShape = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "朝向运动方向"))
	bool bRotationFollowVelocity = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "仅在抵达目标点后进行一次碰撞检测，可以节约性能"))
	bool bTraceOnlyOnArrival = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "只对每个目标最多造成1次伤害。如果禁用，子弹与目标停止重叠后再重新发生碰撞就又可以造成伤害"))
	bool bHurtTargetOnlyOnce = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "能否与环境碰撞"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "寿命归零后销毁"))
	bool bRemoveOnNoLifeSpan = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "血量归零后销毁"))
	bool bRemoveOnNoHealth = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "抵达目标后销毁"))
	bool bRemoveOnArrival = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "与环境碰撞后销毁"))
	bool bRemoveOnHitObstacle = true;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileParamsRT
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害归属者"))
	FSubjectHandle Instigator = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "忽略列表"))
	FSubjectArray IgnoreSubjects = FSubjectArray();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "在哪个邻居网格中检索目标, 不填会尝试自动获取关卡中第一个"))
	UNeighborGridComponent* NeighborGridComponent = nullptr;

};


//--------------------Movement----------------------

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMove_Static
{
	GENERATED_BODY()

public:

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMove_Interped
{
	GENERATED_BODY()

public:

	FProjectileMove_Interped()
	{
		InitializeCurve(XOffset, { {0.0f, 0.0f}, {1.0f, 0.0f} });
		InitializeCurve(YOffset, { {0.0f, 0.0f}, {1.0f, 0.0f} });
		InitializeCurve(ZOffset, { {0.0f, 0.0f}, {1.0f, 0.0f} });
	}

private:

	void InitializeCurve(FRuntimeFloatCurve& Curve, const TArray<TPair<float, float>>& Keyframes)
	{
		Curve.GetRichCurve()->Reset();

		for (const auto& Keyframe : Keyframes)
		{
			FKeyHandle KeyHandle = Curve.GetRichCurve()->AddKey(Keyframe.Key, Keyframe.Value);
			Curve.GetRichCurve()->SetKeyInterpMode(KeyHandle, RCIM_Cubic);
			Curve.GetRichCurve()->SetKeyTangentMode(KeyHandle, RCTM_Auto);
		}
	}

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "速度"))
	float Speed = 2000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Curves, meta = (Tooltip = "前后偏移曲线"))
	FRuntimeFloatCurve XOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Curves, meta = (Tooltip = "前后偏移-距离映射乘数"))
	FVector4 XOffsetRangeMap = FVector4(0, 1, 10000, 1);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Curves, meta = (Tooltip = "左右偏移曲线"))
	FRuntimeFloatCurve YOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Curves, meta = (Tooltip = "左右偏移-距离映射乘数"))
	FVector4 YOffsetRangeMap = FVector4(0, 1, 10000, 1);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Curves, meta = (Tooltip = "上下偏移曲线"))
	FRuntimeFloatCurve ZOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Curves, meta = (Tooltip = "上下偏移-距离映射乘数"))
	FVector4 ZOffsetRangeMap = FVector4(0, 1, 10000, 1);

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMoving_Interped
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "起点"))
	FVector FromPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "终点"))
	FVector ToPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "目标"))
	FSubjectHandle Target = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "生成时间"))
	float BirthTime = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	float XOffsetMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	float YOffsetMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	float ZOffsetMult = 1;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMove_Ballistic
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "最大速度"))
	float MaxSpeed = 10000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "重力"))
	float Gravity = -980;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "预判精度"))
	int32 Iterations = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "弹道计算模式"))
	EProjectileSolveMode SolveMode = EProjectileSolveMode::FromPitch;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "抛射仰角角度", EditCondition = "SolveMode == EProjectileSolveMode::FromPitch", EditConditionHides))
	float Pitch = 35;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "初速度", EditCondition = "SolveMode == EProjectileSolveMode::FromSpeed", EditConditionHides))
	float Speed = 3000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "选择高抛弹道还是平抛弹道", EditCondition = "SolveMode == EProjectileSolveMode::FromSpeed", EditConditionHides))
	bool bFavorHighArc = false;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMoving_Ballistic
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "起点"))
	FVector FromPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "终点"))
	FVector ToPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "初速度"))
	FVector InitialVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "生成时间"))
	float BirthTime = 0;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMove_Tracking
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "最大速度"))
	float MaxSpeed = 5000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "初速度"))
	float Speed = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "加速度"))
	float Acceleration = 1000;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMoving_Tracking
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "起点"))
	FVector FromPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "终点"))
	FVector ToPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "目标"))
	FSubjectHandle Target = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "目标速度"))
	FVector TargetVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "自身速度"))
	FVector CurrentVelocity = FVector::ZeroVector;

};


//-------------------Multipliers----------------------

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMultipliers_Static
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "尺寸乘数"))
	float ScaleMult = 1;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMultipliers_Interped
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "尺寸乘数"))
	float ScaleMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "速度乘数"))
	float SpeedMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "前后偏移乘数"))
	float XOffsetMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "左右偏移乘数"))
	float YOffsetMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "上下偏移乘数"))
	float ZOffsetMult = 1;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMultipliers_Ballistic
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "尺寸乘数"))
	float ScaleMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "重力乘数"))
	float GravityMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "抛射仰角角度乘数"))
	float PitchMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "抛射初速度乘数"))
	float SpeedMult = 1;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMultipliers_Tracking
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "尺寸乘数"))
	float ScaleMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "初速度乘数"))
	float SpeedMult = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "加速度乘数"))
	float AccelerationMult = 1;

};


//-------------------Spawn Config-------------------

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileMultipliers
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = ""))
	FProjectileMultipliers_Static Static;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = ""))
	FProjectileMultipliers_Interped Interped;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = ""))
	FProjectileMultipliers_Ballistic Ballistic;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = ""))
	FProjectileMultipliers_Tracking Tracking;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "启用"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "子弹数据资产"))
	TSoftObjectPtr<UProjectileConfigDataAsset> ProjectileConfigDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "子弹参数乘数"))
	FProjectileMultipliers Multipliers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Tooltip = "原点"))
	ESpawnOrigin SpawnOrigin = ESpawnOrigin::AtSelf;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "偏移量"))
	FTransform Transform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "数量"))
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "延时生成"))
	float Delay = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "附着"))
	bool bAttached = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "附着点失效后自毁"))
	bool bDespawnWhenNoParent = true;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectileConfig_Final
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "启用"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "子弹数据资产"))
	TSoftObjectPtr<UProjectileConfigDataAsset> ProjectileConfigDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "子弹参数乘数"))
	FProjectileMultipliers Multipliers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Tooltip = "原点"))
	ESpawnOrigin SpawnOrigin = ESpawnOrigin::AtSelf;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "偏移量"))
	FTransform Transform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "数量"))
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "延时生成"))
	float Delay = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "附着"))
	bool bAttached = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "附着点失效后自毁"))
	bool bDespawnWhenNoParent = true;

	//-----------------------------------------------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "生成者"))
	FSubjectHandle OwnerSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "附着到"))
	FSubjectHandle AttachToSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "目标"))
	FSubjectHandle TargetSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	FTransform InitialRelativeTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	TArray<FSubjectHandle> SpawnedProjectiles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	FVector FromPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	FVector ToPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	FVector TargetVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	UNeighborGridComponent* NeighborGrid = nullptr;

	bool bInitialized = false;
	bool bSpawned = false;

	//-----------------------------------------------

	FProjectileConfig_Final(){};

	FProjectileConfig_Final(const FProjectileConfig& Config)
	{
		bEnable = Config.bEnable;
		ProjectileConfigDataAsset = Config.ProjectileConfigDataAsset;
		Multipliers = Config.Multipliers;
		SpawnOrigin = Config.SpawnOrigin;
		Transform = Config.Transform;
		Quantity = Config.Quantity;
		Delay = Config.Delay;
		bAttached = Config.bAttached;
		bDespawnWhenNoParent = Config.bDespawnWhenNoParent;
	}
};