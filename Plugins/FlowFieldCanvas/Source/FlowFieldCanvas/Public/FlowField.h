// LeroyWorks 2024 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "TextureResource.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Engine/World.h"
#include "Async/ParallelFor.h"
#include "Templates/Atomic.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"


#include "FlowField.generated.h"

//--------------------------ENUM-----------------------------

UENUM(BlueprintType)
enum class EInitMode : uint8
{
	Construction UMETA(DisplayName = "Construction"),
	BeginPlay UMETA(DisplayName = "BeginPlay"),
	Runtime UMETA(DisplayName = "Runtime")
};

UENUM(BlueprintType)
enum class EStyle : uint8
{
	AdjacentFirst UMETA(DisplayName = "AdjacentFirst"),
	DiagonalFirst UMETA(DisplayName = "DiagonalFirst")
};

UENUM(BlueprintType)
enum class ECellType : uint8
{
	Ground UMETA(DisplayName = "Ground"),
	Obstacle UMETA(DisplayName = "Obstacle"),
	Empty UMETA(DisplayName = "Empty")
};

UENUM(BlueprintType)
enum class EDigitType : uint8
{
	Cost UMETA(DisplayName = "Cost"),
	Dist UMETA(DisplayName = "Dist")
};

//--------------------------Struct-----------------------------

USTRUCT(BlueprintType) struct FCellStruct
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "CostField", ToolTip = "The cost of this cell"))
	int32 cost = 255;// using a byte here can improve performance but increases code complexity

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "IntegrationField", ToolTip = "Generated integration field. Same as water, it always flow towards the the ground with the lowest value nearby."))
	int32 dist = 65535;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "FlowField", ToolTip = "The direction the pawn need to go next when standing on this cell"))
	FVector dir = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "CellType", ToolTip = "The cell's type"))
	ECellType type = ECellType::Ground;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "XYCoordinate", ToolTip = "The coordinate of this cell on grid"))
	FVector2D gridCoord = FVector2D(0, 0);

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "GroundNormal", ToolTip = "Which coord it will end up in"))
	FVector2D goalCoord = FVector2D(-1, -1);

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "WorldLocation", ToolTip = "The world location of this cell's center point"))
	FVector worldLoc = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (DisplayName = "GroundNormal", ToolTip = "The ground's up vector, decided by line trace if ground trace enabled"))
	FVector normal = FVector(0, 0, 1);

};


//--------------------------FlowFieldClass-----------------------------

UCLASS()
class FLOWFIELDCANVAS_API AFlowField : public AActor
{
	GENERATED_BODY()

	//--------------------------------------------------------Functions-----------------------------------------------------------------

public:

	AFlowField();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "FFCanvas")
	void DrawDebug();

	UFUNCTION(BlueprintCallable, Category = "FFCanvas", meta = (ToolTip = "Recalculate flow field"))
	void UpdateFlowField();

	UFUNCTION(BlueprintCallable, Category = "FFCanvas", meta = (ToolTip = "Recalculate flow field periodically by timer"))
	void TickFlowField();

	UFUNCTION(BlueprintCallable, Category = "FFCanvas", meta = (ToolTip = "Get the grid coordinate at the given world location"))
	bool WorldToGridBP(UPARAM(ref) const FVector& Location, FVector2D& gridCoord);

	UFUNCTION(BlueprintCallable, Category = "FFCanvas", meta = (ToolTip = "Get the array index at the given world location"))
	bool WorldToIndexBP(UPARAM(ref) const FVector& Location, int32& Index);

	UFUNCTION(BlueprintCallable, Category = "FFCanvas", meta = (ToolTip = "Get the cell at the given world location"))
	FCellStruct& GetCellAtLocationBP(UPARAM(ref) const FVector& Location, bool& bOutIsValid);

	UFUNCTION(BlueprintCallable, Category = "FFCanvas", meta = (ToolTip = "Get the average flow field direction at the given world location within radius"))
	FVector GetAverageDirectionBP(UPARAM(ref) const FVector& Location, const float Radius, bool& bOutIsValid);

	FORCEINLINE bool WorldToGrid(const FVector& Location, FVector2D& gridCoord)
	{
		FVector relativeLocation = (Location - actorLoc).RotateAngleAxis(-actorRot.Yaw, FVector(0, 0, 1)) + offsetLoc;
		float cellRadius = cellSize / 2;

		int32 gridCoordX = FMath::RoundToInt((relativeLocation.X - cellRadius) / cellSize);
		int32 gridCoordY = FMath::RoundToInt((relativeLocation.Y - cellRadius) / cellSize);

		// inside grid?
		bool bIsValidCoord = (gridCoordX >= 0 && gridCoordX < xNum) && (gridCoordY >= 0 && gridCoordY < yNum);

		// clamp to nearest
		gridCoord = FVector2D(FMath::Clamp(gridCoordX, 0, xNum - 1), FMath::Clamp(gridCoordY, 0, yNum - 1));

		return bIsValidCoord;
	};

	FORCEINLINE bool WorldToIndex(const FVector& Location, int32& Index)
	{
		if (!bIsBeginPlay)
		{
			FVector2D NearestCoord;
			const bool bIsValidCoord = WorldToGrid(Location, NearestCoord);

			const int32 NearestIndex = CoordToIndex(NearestCoord);
			const int32 CellCount = CurrentCellsArray.Num();
			const bool bIsValidIndex = NearestIndex < CellCount;

			// 计算最终索引（确保不越界）
			Index = FMath::Clamp(NearestIndex, 0, CellCount - 1);

			return bIsValidCoord && bIsValidIndex;
		}
		else
		{
			Index = 0;
			return false;
		}
	}

	FORCEINLINE int32 CoordToIndex(const FVector2D& GridCoord)
	{
		return GridCoord.X * yNum + GridCoord.Y;
	};

	FORCEINLINE FCellStruct& GetCellAtLocation(const FVector& Location, bool& bOutIsValid)
	{
		if (!bIsBeginPlay)
		{
			FCellStruct* ResultCell = &CurrentCellsArray[0];
			bOutIsValid = false;

			FVector2D NearestCoord;
			const bool bIsValidCoord = WorldToGrid(Location, NearestCoord);

			const int32 Index = CoordToIndex(NearestCoord);
			const int32 CellCount = CurrentCellsArray.Num();
			const bool bIsValidIndex = Index < CellCount;

			// 计算最终索引（确保不越界）
			const int32 NearestIndex = FMath::Clamp(Index, 0, CellCount - 1);
			ResultCell = &CurrentCellsArray[NearestIndex];

			// 设置有效性标志
			bOutIsValid = bIsValidCoord && bIsValidIndex;

			return *ResultCell;
		}
		else
		{
			bOutIsValid = false;
			return DefaultCell;
		}
	}

	FORCEINLINE FCellStruct& GetCellAtCoord(const FVector2D& Coord, bool& bOutIsValid)
	{
		// 默认返回第一个单元格（防止返回无效引用）
		FCellStruct* ResultCell = &CurrentCellsArray[0];
		bOutIsValid = false;

		const bool bIsValidCoord = (Coord.X >= 0 && Coord.X < xNum) && (Coord.Y >= 0 && Coord.Y < yNum);

		const int32 Index = CoordToIndex(Coord);
		const int32 CellCount = CurrentCellsArray.Num();
		const bool bIsValidIndex = Index < CellCount;

		// 计算最终索引（确保不越界）
		const int32 NearestIndex = FMath::Clamp(Index, 0, CellCount - 1);
		ResultCell = &CurrentCellsArray[NearestIndex];

		// 设置有效性标志
		bOutIsValid = bIsValidCoord && bIsValidIndex;

		return *ResultCell;
	}

	FORCEINLINE FVector GetAverageDirection(const FVector& Location, const float Radius, bool& bOutIsValid)
	{
		bOutIsValid = false;

		if (bIsBeginPlay)
		{
			return FVector::ZeroVector;
		}

		FVector TotalDir = FVector::ZeroVector;
		int32 ValidCellCount = 0;

		// 计算圆形参数
		const float RadiusSq = Radius * Radius;
		const float HalfCellSize = cellSize * 0.5f;
		const float CellBoundOffset = HalfCellSize + Radius; // 相交检测优化

		// 计算圆心在相对坐标系中的位置
		FVector RelativeCenter = (Location - actorLoc).RotateAngleAxis(-actorRot.Yaw, FVector(0, 0, 1)) + offsetLoc;

		// 计算网格坐标范围（扩大范围确保覆盖所有可能相交的格子）
		const int32 MinGridX = FMath::Clamp(FMath::FloorToInt((RelativeCenter.X - CellBoundOffset) / cellSize), 0, xNum - 1);
		const int32 MaxGridX = FMath::Clamp(FMath::CeilToInt((RelativeCenter.X + CellBoundOffset) / cellSize), 0, xNum - 1);
		const int32 MinGridY = FMath::Clamp(FMath::FloorToInt((RelativeCenter.Y - CellBoundOffset) / cellSize), 0, yNum - 1);
		const int32 MaxGridY = FMath::Clamp(FMath::CeilToInt((RelativeCenter.Y + CellBoundOffset) / cellSize), 0, yNum - 1);

		// 遍历所有可能相交的网格
		for (int32 x = MinGridX; x <= MaxGridX; ++x)
		{
			for (int32 y = MinGridY; y <= MaxGridY; ++y)
			{
				const FVector2D GridCoord(x, y);
				const int32 Index = CoordToIndex(GridCoord);

				if (Index >= 0 && Index < CurrentCellsArray.Num())
				{
					const FCellStruct& Cell = CurrentCellsArray[Index];

					// 检查格子类型
					//if (Cell.cost == 255) continue;

					// 计算格子边界
					const float CellMinX = Cell.worldLoc.X - HalfCellSize;
					const float CellMaxX = Cell.worldLoc.X + HalfCellSize;
					const float CellMinY = Cell.worldLoc.Y - HalfCellSize;
					const float CellMaxY = Cell.worldLoc.Y + HalfCellSize;

					// 计算最近点并检查相交
					const float ClosestX = FMath::Clamp(Location.X, CellMinX, CellMaxX);
					const float ClosestY = FMath::Clamp(Location.Y, CellMinY, CellMaxY);

					const float Dx = Location.X - ClosestX;
					const float Dy = Location.Y - ClosestY;
					const float DistSq = Dx * Dx + Dy * Dy;

					if (DistSq <= RadiusSq)
					{
						TotalDir += Cell.dir;
						++ValidCellCount;
					}
				}
			}
		}

		// 计算平均方向
		if (ValidCellCount > 0)
		{
			TotalDir /= ValidCellCount;
			TotalDir.Z = 0; // 确保Z轴为0

			if (!TotalDir.IsNearlyZero(1e-4f))
			{
				TotalDir.Normalize();
			}

			bOutIsValid = true;
			return TotalDir;
		}

		return FVector::ZeroVector;
	}


	void InitFlowField(EInitMode InitMode);
	void GetGoalLocation();
	void CreateGrid();
	void CalculateFlowField(TArray<FCellStruct>& InCurrentCellsArray);
	void DrawCells(EInitMode InitMode);
	void DrawArrows(EInitMode InitMode);
	void UpdateTimer();
	FCellStruct EnvQuery(const FVector2D gridCoord);


	//--------------------------------------------------------Exposed To Instance Settings-----------------------------------------------------------------

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|Visualize", meta = (ToolTip = "Live update the flowfield in editor"))
	bool bEditorLiveUpdate = true;

	UPROPERTY(BlueprintReadWrite, EditAnyWhere, Category = "FFCanvas|Visualize", meta = (ToolTip = "If True: Shows the debug grid in-editor"))
	bool drawCellsInEditor = true;

	UPROPERTY(BlueprintReadWrite, EditAnyWhere, Category = "FFCanvas|Visualize", meta = (ToolTip = "If True: Shows the flow field arrows in-editor"))
	bool drawArrowsInEditor = true;

	UPROPERTY(BlueprintReadWrite, EditAnyWhere, Category = "FFCanvas|Visualize", meta = (ToolTip = "If True: Shows the debug grid in-game"))
	bool drawCellsInGame = true;

	UPROPERTY(BlueprintReadWrite, EditAnyWhere, Category = "FFCanvas|Visualize", meta = (ToolTip = "If True: Shows the flow field arrows in-game"))
	bool drawArrowsInGame = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas", meta = (ToolTip = "Goal Actor. If none, will use variable goalLocation instead"))
	TArray<TSoftObjectPtr<AActor>> GoalActors;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas", meta = (ToolTip = "The scale of the flow field pathfinding boundary in units"))
	FVector flowFieldSize = FVector(1000, 1000, 300);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas", meta = (ToolTip = "The scale of the cells in units"))
	float cellSize = 150.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas", meta = (ToolTip = "Initial cost of all cells, ranging from 0 to 255"))
	int32 initialCost = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas", meta = (ToolTip = "Which direction flowfield prefer to go ?"))
	EStyle Style = EStyle::AdjacentFirst;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|EnvQuery", meta = (ToolTip = "If True: Trace all cells for any given Ground Object Type and aligns it to the ground"))
	bool traceGround = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|EnvQuery", meta = (ToolTip = "Object types that the flow field system will align to"))
	TArray<TEnumAsByte<EObjectTypeQuery>> groundObjectType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|EnvQuery", meta = (ToolTip = "If True: When creating the grid it will sphere-trace for any mesh that isnt of the given Ground Object Type"))
	bool traceObstacles = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|EnvQuery", meta = (ToolTip = "Type of object the flow field deem as an obstacle"))
	TArray<TEnumAsByte<EObjectTypeQuery>> obstacleObjectType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|EnvQuery", meta = (ToolTip = "Cell inclination over this limit will be recognized as obstacle."))
	float maxWalkableAngle = 45.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|Performance", meta = (ToolTip = "How long until next update in seconds during runtime"))
	float RefreshInterval = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFCanvas|Performance", meta = (ToolTip = "Skip cells inside obstacles during calculation"))
	bool bIgnoreInternalObstacleCells = false;


	//--------------------------------------------------------Not Exposed Settings-----------------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FFCanvas|Material")
	TObjectPtr<UMaterialInterface> ArrowBaseMat;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FFCanvas|Material")
	TObjectPtr<UMaterialInterface> CellBaseMat;

	//UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FFCanvas|Material")
	//TObjectPtr<UMaterialInterface> DigitBaseMat;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FFCanvas", meta = (ToolTip = "Set this value manually if you don't want to fill in a goal actor"))
	TArray<FVector> GoalLocations;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FFCanvas", meta = (ToolTip = "The cost value here will be added to the cell's cost"))
	TMap<int32,int32> ExtraCosts;


	//--------------------------------------------------------Components-----------------------------------------------------------------

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "FFCanvas|Components")
	TObjectPtr<UBoxComponent> Volume;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "FFCanvas|Components")
	TObjectPtr<UInstancedStaticMeshComponent> ISM_Arrows;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "FFCanvas|Components")
	TObjectPtr<UDecalComponent> Decal_Cells;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "FFCanvas|Components")
	TObjectPtr<UBillboardComponent> Billboard;

	//--------------------------------------------------------ReadOnly-----------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (ToolTip = "Store ground info"))
	TArray<FCellStruct> InitialCellsArray;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "FFCanvas", meta = (ToolTip = "Store generated flow field data"))
	TArray<FCellStruct> CurrentCellsArray;

	//--------------------------------------------------------Cached-----------------------------------------------------------------

	float nextTickTimeLeft = RefreshInterval;

	bool bIsGridDirty = true;
	bool bIsBeginPlay = true;

	FVector actorLoc = GetActorLocation();
	FRotator actorRot = GetActorRotation();

	FVector offsetLoc = FVector(0, 0, 0);
	FVector relativeLoc = FVector(0, 0, 0);

	TObjectPtr<UMaterialInstanceDynamic> ArrowDMI;
	TObjectPtr<UMaterialInstanceDynamic> CellDMI;
	TObjectPtr<UMaterialInstanceDynamic> DigitDMI;

	TSet<FVector2D> GridModifiedSet;
	TSet<FVector2D> DirModifiedSet;

	TArray<FVector> GoalActorLocations;

	TArray<bool> bIsValidGoalCoords;
	TArray<FVector2D> GoalGridCoords;

	int32 xNum = FMath::RoundToInt(flowFieldSize.X / cellSize);
	int32 yNum = FMath::RoundToInt(flowFieldSize.Y / cellSize);

	TObjectPtr<UTexture2D> TransientTexture;

	TAtomic<int32> TraceRemaining{ 0 };

	FCellStruct DefaultCell = FCellStruct();

};