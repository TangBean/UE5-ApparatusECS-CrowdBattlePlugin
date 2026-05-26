#pragma once

#include "CoreMinimal.h"
#include "SubjectiveActorComponent.h"
#include "BattleFrameStructs.h"
#include "AgentConfigDataAsset.h"
#include "BattleFrameBattleControl.h"
#include "BFSubjectiveAgentComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class BATTLEFRAME_API UBFSubjectiveAgentComponent : public USubjectiveActorComponent
{
    GENERATED_BODY()

public:

    UBFSubjectiveAgentComponent();

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | SubjectiveAgent", meta = (DisplayName = "Initialize With Data Asset"))
    void InitializeSubjectTraits(bool bAutoActivation = true, AActor* OwnerActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | SubjectiveAgent", meta = (DisplayName = "Activate Agent"))
    void ActivateAgent(FSubjectHandle Agent);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | SubjectiveAgent", meta = (DisplayName = "Sync Transform Subject To Actor"))
    void SyncTransformSubjectToActor(AActor* OwnerActor);

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSoftObjectPtr<UAgentConfigDataAsset> AgentConfigAsset;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TeamIndex = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D LaunchVelocity = FVector2d(0, 0);

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FSpawnerMult Multipliers = FSpawnerMult();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bAutoInitWithDataAsset = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bSyncTransformSubjectToActor = true;

    UWorld* CurrentWorld = nullptr;
    AMechanism* Mechanism = nullptr;
    ABattleFrameBattleControl* BattleControl = nullptr;

    // Agent Sub-Status Flags
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


protected:

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
