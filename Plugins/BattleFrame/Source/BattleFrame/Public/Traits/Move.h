#pragma once

#include "CoreMinimal.h"
#include "BattleFrameEnums.h"
#include "Move.generated.h"


USTRUCT(BlueprintType)
struct BATTLEFRAME_API FYawMovement
{
	GENERATED_BODY()

public:

	//---------------Yaw Movement-----------------//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用转向"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "转向角速度"))
	float TurnSpeed = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "转向角加速度"))
	float TurnAcceleration = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "转向模式"))
	EOrientMode TurnMode = EOrientMode::ToMovementForwardAndBackward;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FXYMovement
{
	GENERATED_BODY()

public:

	//---------------XY Movement-----------------//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否禁用主动移动"))
	bool bStopActiveMovement = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "移动速度"))
	float MoveSpeed = 600.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "移动加速度"))
	float MoveAcceleration = 3000.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "刹车减速度"))
	float MoveDeceleration = 3000.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "速度映射 (X: 与移动方向的夹角, Y: 对应的速度乘数, Z:与移动方向的夹角, W:对应的速度乘数)"))
	FVector4 MoveSpeedRangeMapByAngle = FVector4(0, 1, 180, 1);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "速度映射 (X: 与目标点的距离, Y: 对应的速度乘数, Z:与目标点的距离, W:对应的速度乘数)"))
	FVector4 MoveSpeedRangeMapByDist = FVector4(100, 1, 1000, 1);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", Tooltip = "每次地面弹跳移动速度衰减"))
	FVector2D MoveBounceVelocityDecay = FVector2D(0.5f, 0.f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "与移动目标点距离低于该值时停止移动"))
	float AcceptanceRadius = 100.f;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FFall
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用坠落"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否可以飞行"))
	bool bCanFly = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "随机飞行高度，出生后固定 (X: 最小高度, Y: 最大高度)", EditCondition = "bCanFly == true", EditConditionHides))
	FVector2D FlyHeight = FVector2D(200.f, 400.f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否可以播放坠落动画"))
	bool bFallAnim = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "重力"))
	float Gravity = -2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Tooltip = "强制死亡高度 (低于此高度时强制移除)"))
	float KillZ = -10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Tooltip = "地面检测模式"))
	EGroundTraceMode GroundTraceMode = EGroundTraceMode::FlowFieldAndSphereTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Tooltip = "坡度大于这个值，使用球扫检测地面"))
	float SphereTraceAngleThreshold = 45.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "地面碰撞类型"))
	TArray<TEnumAsByte<EObjectTypeQuery>> GroundObjectType;

	// 添加构造函数
	FFall()
	{
		// 默认添加WorldStatic到GroundObjectType
		GroundObjectType.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FMove
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用移动属性"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "绘制Debug图形"))
	bool bDrawDebugShape = false;

	//---------------Yaw Movement-----------------//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "转向", DisplayName = "Yaw Movement"))
	FYawMovement Yaw;

	//---------------XY Movement-----------------//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "平面运动", DisplayName = "XY Movement"))
	FXYMovement XY;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FMoving
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "当前角速度"))
	float CurrentAngularVelocity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "当前线速度"))
	FVector CurrentVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "想要达成的线速度"))
	FVector DesiredVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "想要抵达的位置"))
	FVector Goal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (Tooltip = "有移动目的地？"))
	bool bHasGoal = true;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (Tooltip = "正在下落？"))
	bool bFalling = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (Tooltip = "被击飞中？"))
	bool bLaunching = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (Tooltip = "在被推着走？"))
	bool bPushedBack = false;

	EMoveState MoveState = EMoveState::Dirty;
	FVector LaunchVelSum = FVector::ZeroVector;
	float PushBackSpeedOverride = 0;
	float FlyingHeight = 0;
	float MoveSpeedMult = 1;
	float TurnSpeedMult = 1;

	// 速度历史记录
	TArray<FVector, TInlineAllocator<5>> HistoryVelocity;
	FVector AverageVelocity = FVector::ZeroVector;
	FVector CachedSumVelocity = FVector::ZeroVector;
	int32 CurrentIndex = 0;
	float TimeLeft = 0.f;
	bool bShouldInit = true;
	bool bBufferFilled = false;

	FMoving() {};

	// 更新速度记录
	FORCEINLINE void Initialize()
	{
		HistoryVelocity.Init(FVector::ZeroVector, 5);
		bShouldInit = false;
	}

	FORCEINLINE void UpdateVelocityHistory(const FVector& NewVelocity)
	{
		// 减去即将被覆盖的值
		CachedSumVelocity -= HistoryVelocity[CurrentIndex];

		// 记录新值并更新缓存
		HistoryVelocity[CurrentIndex] = NewVelocity;
		CachedSumVelocity += NewVelocity;

		// 更新环形索引
		CurrentIndex = (CurrentIndex + 1) % 5;

		// 标记缓冲区是否已填满
		if (!bBufferFilled && CurrentIndex == 0)
		{
			bBufferFilled = true;
		}

		// 计算当前平均速度
		const int32 ValidFrames = bBufferFilled ? 5 : CurrentIndex;
		AverageVelocity = (ValidFrames > 0) ? (CachedSumVelocity / ValidFrames) : FVector::ZeroVector;

		TimeLeft = 0.1f;
	}

	FMoving(const FMoving& Moving)
	{
		LockFlag.store(Moving.LockFlag.load());

		DesiredVelocity = Moving.DesiredVelocity;
		CurrentVelocity = Moving.CurrentVelocity;
		bFalling = Moving.bFalling;
		bLaunching = Moving.bLaunching;
		bPushedBack = Moving.bPushedBack;
		LaunchVelSum = Moving.LaunchVelSum;
		PushBackSpeedOverride = Moving.PushBackSpeedOverride;
		FlyingHeight = Moving.FlyingHeight;
		MoveSpeedMult = Moving.MoveSpeedMult;
		TurnSpeedMult = Moving.TurnSpeedMult;
		CurrentAngularVelocity = Moving.CurrentAngularVelocity;
		bHasGoal = Moving.bHasGoal;
		Goal = Moving.Goal;
		MoveState = Moving.MoveState;

		HistoryVelocity = Moving.HistoryVelocity;
		AverageVelocity = Moving.AverageVelocity;
		CachedSumVelocity = Moving.CachedSumVelocity;
		CurrentIndex = Moving.CurrentIndex;
		TimeLeft = Moving.TimeLeft;
		bShouldInit = Moving.bShouldInit;
		bBufferFilled = Moving.bBufferFilled;
	}

	FMoving& operator=(const FMoving& Moving)
	{
		LockFlag.store(Moving.LockFlag.load());

		DesiredVelocity = Moving.DesiredVelocity;
		CurrentVelocity = Moving.CurrentVelocity;
		bFalling = Moving.bFalling;
		bLaunching = Moving.bLaunching;
		bPushedBack = Moving.bPushedBack;
		LaunchVelSum = Moving.LaunchVelSum;
		PushBackSpeedOverride = Moving.PushBackSpeedOverride;
		FlyingHeight = Moving.FlyingHeight;
		MoveSpeedMult = Moving.MoveSpeedMult;
		TurnSpeedMult = Moving.TurnSpeedMult;
		CurrentAngularVelocity = Moving.CurrentAngularVelocity;
		bHasGoal = Moving.bHasGoal;
		Goal = Moving.Goal;
		MoveState = Moving.MoveState;

		HistoryVelocity = Moving.HistoryVelocity;
		AverageVelocity = Moving.AverageVelocity;
		CachedSumVelocity = Moving.CachedSumVelocity;
		CurrentIndex = Moving.CurrentIndex;
		TimeLeft = Moving.TimeLeft;
		bShouldInit = Moving.bShouldInit;
		bBufferFilled = Moving.bBufferFilled;

		return *this;
	}
};

