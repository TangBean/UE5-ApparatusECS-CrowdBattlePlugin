#pragma once
 
#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "Traits/Damage.h"
#include "TemporalDamage.generated.h"
 

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTemporalDamage
{
	GENERATED_BODY()
 
  public:

	  FSubjectHandle TemporalDamageInstigator = FSubjectHandle();
	  FSubjectHandle TemporalDamageTarget = FSubjectHandle();

	  float TemporalDamageTimeout = 0.5f;
	  float TemporalDmgInterval = 0.5f;  // 伤害间隔

	  float RemainingTemporalDamage = 0.f;
	  float TotalTemporalDamage = 0.f;

	  int32 CurrentSegment = 0;          // 当前伤害段数
	  int32 TemporalDmgSegment = 4;      // 总伤害段数

	  EDmgType DmgType = EDmgType::Normal;

	  bool bJustSpawned = true;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTemporalDamaging
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

	TSet<FSubjectHandle> TemporalDamages;

	FTemporalDamaging() {};

	FTemporalDamaging(const FTemporalDamaging& TemporalDamaging)
	{
		LockFlag.store(TemporalDamaging.LockFlag.load());
		TemporalDamages = TemporalDamaging.TemporalDamages;
	}

	FTemporalDamaging& operator=(const FTemporalDamaging& TemporalDamaging)
	{
		LockFlag.store(TemporalDamaging.LockFlag.load());
		TemporalDamages = TemporalDamaging.TemporalDamages;
		return *this;
	}
};
