#pragma once

#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "SolidSubjectHandle.h"
#include "BattleFrameEnums.h"
#include "BattleFrameStructs.generated.h" 


USTRUCT(BlueprintType)
struct BATTLEFRAME_API FObjectTypes
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTraceResult
{
    GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "碰撞发生时，被索敌对象的SubjectHandle"))
    FSubjectHandle Subject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "碰撞发生时，被索敌对象的位置"))
    FVector SubjectLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "碰撞发生时，索敌图形的位置"))
	FVector ShapeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "碰撞发生时，索敌图形与被索敌对象的碰撞点"))
	FVector HitLocation = FVector::ZeroVector;

    float CachedDistSq = -1.0f;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDmgResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle DamagedSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle InstigatorSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle CauserSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsCritical = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsKill = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DmgDealt = 0;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSphereSweepParamsSpecific
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "启用后，使用独立的索敌参数，否则使用Trace里的通用参数"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野半径"))
	float TraceRadius = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UIMin = 0, UIMax = 360, Tooltip = "索敌视野角度"))
	float TraceAngle = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野高度"))
	float TraceHeight = 300.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "检测目标是不是在障碍物后面"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "朝向偏移"))
	float YawOffset = 0;

	FSphereSweepParamsSpecific() {};

	FSphereSweepParamsSpecific(const FSphereSweepParamsSpecific& Params)
	{
		bEnable = Params.bEnable;

	}

	FSphereSweepParamsSpecific& operator=(const FSphereSweepParamsSpecific& Params)
	{
		bEnable = Params.bEnable;

		return *this;
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSphereSweepParams
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野半径"))
	float TraceRadius = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (UIMin = 0, UIMax = 360, Tooltip = "索敌视野角度"))
	float TraceAngle = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "索敌视野高度"))
	float TraceHeight = 300.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "检测目标是不是在障碍物后面"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "朝向偏移"))
	float YawOffset = 0;

	FSphereSweepParams() {};

	FSphereSweepParams(const FSphereSweepParams& Params)
	{

	}

	FSphereSweepParams& operator=(const FSphereSweepParams& Params)
	{

		return *this;
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSubjectArray
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FSubjectHandle> Subjects = TArray<FSubjectHandle>();
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSpawnerMult
{
	GENERATED_BODY()

public:

	// 成员变量默认值
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "血量乘数"))
	float HealthMult = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "速度乘数"))
	float MoveSpeedMult = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "伤害乘数"))
	float DamageMult = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "尺寸乘数"))
	float ScaleMult = 1.f;

	// 默认构造函数（必须）
	FSpawnerMult() = default;
};

USTRUCT()
struct BATTLEFRAME_API FValidSubjects
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

	TArray<FSolidSubjectHandle, TInlineAllocator<128>> Subjects;

	FValidSubjects() {};

	FValidSubjects(const FValidSubjects& ValidSubjects)
	{
		LockFlag.store(ValidSubjects.LockFlag.load());
		Subjects = ValidSubjects.Subjects;
	}

	FValidSubjects& operator=(const FValidSubjects& ValidSubjects)
	{
		LockFlag.store(ValidSubjects.LockFlag.load());
		Subjects = ValidSubjects.Subjects;
		return *this;
	}
};


//------------------Event Callback Data--------------------

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAppearData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle SelfSubject = FSubjectHandle();

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTraceData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle SelfSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETraceEventState State = ETraceEventState::Begin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle TraceResult = FSubjectHandle();

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FMoveData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle SelfSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EMoveEventState State = EMoveEventState::Approach_Arrived;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAttackData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle SelfSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EAttackEventState State = EAttackEventState::Aiming;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle AttackTarget = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FDmgResult> DmgResults;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FHitData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle SelfSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle InstigatorSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle CauserSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsCritical = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsKill = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DmgDealt = 0;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDeathData
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSubjectHandle SelfSubject = FSubjectHandle();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EDeathEventState State = EDeathEventState::OutOfHealth;

};


//------------------Debug Draw Configs--------------------

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugPointConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置"))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float Size = 1.f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugLineConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "起点"))
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "终点"))
	FVector EndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 0.f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugArrowConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "起点"))
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "终点"))
	FVector EndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 0.f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugSphereConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "半径"))
	float Radius = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置"))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 0.f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugCapsuleConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置"))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "朝向"))
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "半径"))
	float Radius = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "高度"))
	float Height = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 0.f;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugSectorConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "半径"))
	float Radius = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "高度"))
	float Height = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "张开角度"))
	float Angle = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "张开角度"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置"))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "朝向"))
	FVector Direction = FVector::OneVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "层级"))
	int32 DepthPriority = 0;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebugCircleConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "半径"))
	float Radius = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "位置"))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 0.f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTraceDrawDebugConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "启用"))
	bool bDrawDebugShape = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "颜色"))
	FColor Color = FColor::Red;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "持续时间"))
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "线宽"))
	float LineThickness = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "碰撞点大小"))
	float HitPointSize = 1.f;

	FTraceDrawDebugConfig() = default;

};

