#pragma once

#include "CoreMinimal.h"
#include "SubType.h"
#include "ActorSpawnConfig.h"
#include "SoundConfig.h"
#include "FxConfig.h"
#include "ProjectileConfig.h"
#include "SubjectRecord.h"
#include "ProjectileConfigDataAsset.h"
#include "Attack.generated.h"


UENUM(BlueprintType)
enum class EAttackMode : uint8
{
	None UMETA(DisplayName = "None", Tooltip = "无"),
	ApplyDMG UMETA(DisplayName = "Apply Damage", Tooltip = "造成伤害"),
	SuicideATK UMETA(DisplayName = "Apply Damage And Despawn", Tooltip = "造成伤害后自毁"),
	Despawn UMETA(DisplayName = "Despawn", Tooltip = "自毁")
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAttack
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用攻击功能"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "绘制Debug图形"))
	bool bDrawDebugShape = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "每轮攻击的持续时长"))
	float DurationPerRound = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "最短瞄准时间"))
	float MinAimTime = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "施加伤害的时刻"))
	float TimeOfHit = 0.35f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "攻击冷却时长"))
	float CoolDown = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "距离小于该值可以发起攻击", DisplayName = "Attack Range"))
	float Range = 200.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "夹角小于该值可以发起攻击", DisplayName = "Attack Angle"))
	float AngleToleranceATK = 15.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "距离小于该值可以击中", DisplayName = "Hit Range"))
	float RangeToleranceHit = 400.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "夹角小于该值可以击中", DisplayName = "Hit Angle"))
	float AngleToleranceHit = 180.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "击中时刻的行为"))
	EAttackMode TimeOfHitAction = EAttackMode::ApplyDMG;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否播放攻击动画"))
	bool bCanPlayAnim = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	TArray<FProjectileConfig> SpawnProjectile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	TArray<FActorSpawnConfig_Attack> SpawnActor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	TArray<FFxConfig_Attack> SpawnFx;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
	TArray<FSoundConfig_Attack> PlaySound;

	//add a random stream variable

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAttacking
{
	GENERATED_BODY()

public:

	float AimTime = 0.0f;
	float ATKTime = 0.0f;
	float CoolTime = 0.0f;
	EAttackState State = EAttackState::Aim;

	FORCEINLINE void Reset()
	{
		AimTime = 0.0f;
		ATKTime = 0.0f;
		CoolTime = 0.0f;
		State = EAttackState::Aim;
	}
};
