/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h" // Include the Niagara System header
#include "Machine.h"
#include "HAL/PlatformMisc.h"
#include "NiagaraFXRenderer.generated.h"

UENUM(BlueprintType)
enum class EFxMode : uint8
{
	InPlace UMETA(DisplayName = "InPlace"),
	Attached UMETA(DisplayName = "Attached")
};

class UWorld;
class AMechanism;
class ABattleFrameBattleControl;

UCLASS()
class BATTLEFRAME_API ANiagaraFXRenderer : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANiagaraFXRenderer();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void IdleCheck();

	FSubjectHandle AddRenderBatch();

	void RemoveRenderBatch(FSubjectHandle RenderBatch);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	UNiagaraSystem* NiagaraSystemAsset;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	//EFxMode Mode = EFxMode::InPlace;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Filter")
	UScriptStruct* TraitType = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Filter")
	UScriptStruct* SubType = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Performance")
	float PoollingCoolDown = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Performance")
	int32 RenderBatchSize = 500;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Performance")
	int32 MaxThreadsAllowed = FMath::Clamp(FPlatformMisc::NumberOfWorkerThreadsToSpawn(), 1, FPlatformMisc::NumberOfCoresIncludingHyperthreads());

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Performance")
	int32 MinBatchSizeAllowed = 100;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CachedVars")
	TArray<FSubjectHandle> SpawnedRenderBatches;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CachedVars")
	int32 ProjectileCount = 0;

	int32 ThreadsCount = 1;
	int32 BatchSize = 1;
	bool Initialized = false;
	UWorld* CurrentWorld = nullptr;
	AMechanism* Mechanism = nullptr;
	ABattleFrameBattleControl* BattleControl = nullptr;

};
