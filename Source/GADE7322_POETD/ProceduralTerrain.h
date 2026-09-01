// Fill out your copyright notice in the Description page of Project Settings.

// ProceduralTerrain.h
// Phase 1 - Isolated procedural grid terrain + pathway generator.
// No references to player controllers, inventory, currency, HUD, or enemy AI.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "CentralTowerBase.h"
#include "ProceduralTerrain.generated.h"

/** A single generated pathway: its grid nodes (world space) and a debug color for visualization. */
USTRUCT(BlueprintType)
struct FProceduralPathway
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Pathway")
	TArray<FVector> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Pathway")
	FColor DebugColor = FColor::Red;
};

/**
 * Builds a dynamic grid mesh at runtime, generates 3+ pathways from map edges
 * to a central point, and separates the grid into pathway cells vs. valid
 * build cells. Fully testable standalone via debug draw - no gameplay
 * systems required.
 */
UCLASS()
class GADE7322_POETD_API AProceduralTerrain : public AActor
{
	GENERATED_BODY()

public:
	AProceduralTerrain();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// ---------- Components ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* ProceduralMesh;

	// ---------- Grid Configuration ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid", meta = (ClampMin = "4"))
	int32 GridWidth = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid", meta = (ClampMin = "4"))
	int32 GridHeight = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid", meta = (ClampMin = "10.0"))
	float CellSize = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid")
	int32 Seed = 12345;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid")
	UMaterialInterface* TerrainMaterial;

	// ---------- Pathway Configuration ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Pathways", meta = (ClampMin = "3"))
	int32 NumPathways = 3;

	/** How many cells of buffer to keep clear around each pathway when generating build locations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Pathways", meta = (ClampMin = "0"))
	int32 PathBufferCells = 1;

	// ---------- Central Tower ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Tower")
	TSubclassOf<ACentralTowerBase> CentralTowerClass;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain|Tower")
	FVector CentralTowerLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain|Tower")
	ACentralTowerBase* SpawnedCentralTower;

	// ---------- Debug Visualization ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Debug", meta = (ClampMin = "0.0"))
	float DebugSphereRadius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Debug")
	FColor BuildLocationDebugColor = FColor::Green;

	// ---------- Output Data (required by spec) ----------
	UPROPERTY(BlueprintReadOnly, Category = "Terrain|Output")
	TArray<FVector> PathwayNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain|Output")
	TArray<FVector> BuildGridLocations;

	/** Same data as PathwayNodes, but organized per-path with its own debug color. Useful for Phase 2 enemy routing. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain|Output")
	TArray<FProceduralPathway> Pathways;

	// ---------- Public Functions ----------

	/** Regenerates mesh, pathways, and build locations from current parameters. Callable from the Details panel. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain")
	void GenerateTerrain();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain")
	void RandomizeSeedAndRegenerate();

	UFUNCTION(BlueprintPure, Category = "Terrain")
	FVector GridToWorldLocation(int32 GridX, int32 GridY) const;

private:
	FRandomStream RandomStream;
	TSet<FIntPoint> PathCellSet;

	void GenerateGridMesh();
	void GeneratePathways();
	void GenerateBuildLocations();
	void DrawDebugVisualization();

	bool IsNearPathCell(const FIntPoint& Cell) const;
	FIntPoint GetRandomEdgeCell(int32 EdgeIndex) const;
};