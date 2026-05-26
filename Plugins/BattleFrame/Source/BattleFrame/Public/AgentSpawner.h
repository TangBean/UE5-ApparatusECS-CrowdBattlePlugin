/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Machine.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"
#include "AgentConfigDataAsset.h"
#include "BattleFrameStructs.h"
#include "Traits/PrimaryType.h"
#include "Traits/Transform.h"
#include "AgentSpawner.generated.h"

class ABattleFrameBattleControl;

UCLASS()
class BATTLEFRAME_API AAgentSpawner : public AActor
{
	GENERATED_BODY()

protected:

	void BeginPlay() override;

public:

	AAgentSpawner();

	UWorld* CurrentWorld = nullptr;
	AMechanism* Mechanism = nullptr;
	ABattleFrameBattleControl* BattleControl = nullptr;

	EFlagmarkBit AppearDissolveFlag = EFlagmarkBit::A;
	EFlagmarkBit DeathDissolveFlag = EFlagmarkBit::B;

	EFlagmarkBit HitGlowFlag = EFlagmarkBit::C;
	EFlagmarkBit HitJiggleFlag = EFlagmarkBit::D;
	EFlagmarkBit HitPoppingTextFlag = EFlagmarkBit::E;
	EFlagmarkBit HitDecideHealthFlag = EFlagmarkBit::F;
	EFlagmarkBit DeathDisableCollisionFlag = EFlagmarkBit::G;
	EFlagmarkBit RegisterMultipleFlag = EFlagmarkBit::H;

	EFlagmarkBit AppearAnimFlag = EFlagmarkBit::I;
	EFlagmarkBit AttackAnimFlag = EFlagmarkBit::J;
	EFlagmarkBit HitAnimFlag = EFlagmarkBit::K;
	EFlagmarkBit DeathAnimFlag = EFlagmarkBit::L;
	EFlagmarkBit FallAnimFlag = EFlagmarkBit::M;

	TArray<FSubjectHandle> SpawnAgentsByConfigRectangular
	(
		const bool bAutoActivate = true,
		const TSoftObjectPtr<UAgentConfigDataAsset> DataAsset = nullptr,
		const int32 Quantity = 1,
		const int32 Team = 0,
		const FVector& Origin = FVector::ZeroVector,
		const FVector2D& Region = FVector2D::ZeroVector,
		const FVector2D& LaunchVelocity = FVector2D::ZeroVector,
		const EInitialDirection InitialDirection = EInitialDirection::FacePlayer,
		const FVector2D& CustomDirection = FVector2D(1, 0),
		const FSpawnerMult& Multipliers = FSpawnerMult()
	);

	UFUNCTION(BlueprintCallable, Category = "BattleFrame | AgentSpawner", meta = (DisplayName = "Activate Agent"))
	void ActivateAgent(FSubjectHandle Agent);

	UFUNCTION(BlueprintCallable, Category = "BattleFrame | AgentSpawner", meta = (DisplayName = "Kill All Agents"))
	void KillAllAgents();

	UFUNCTION(BlueprintCallable, Category = "BattleFrame | AgentSpawner", meta = (DisplayName = "Kill Agents By Subtype"))
	void KillAgentsBySubtype(int32 Index);

};