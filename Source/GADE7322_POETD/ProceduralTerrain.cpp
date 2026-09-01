// Fill out your copyright notice in the Description page of Project Settings.

// ProceduralTerrain.cpp

#include "ProceduralTerrain.h"
#include "DrawDebugHelpers.h"

AProceduralTerrain::AProceduralTerrain()
{
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;

	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ProceduralMesh->bUseAsyncCooking = true;
}

void AProceduralTerrain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateTerrain();
}

void AProceduralTerrain::BeginPlay()
{
	Super::BeginPlay();

	// Regenerate on play to guarantee this session's data is current.
	GenerateTerrain();

	if (bDrawDebug)
	{
		DrawDebugVisualization();
	}

	if (CentralTowerClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedCentralTower = GetWorld()->SpawnActor<ACentralTowerBase>(
			CentralTowerClass, CentralTowerLocation, FRotator::ZeroRotator, SpawnParams);
	}
}

#if WITH_EDITOR
void AProceduralTerrain::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// Live-preview in the editor whenever a parameter changes (grid size, seed, etc.)
	RerunConstructionScripts();
}
#endif

void AProceduralTerrain::GenerateTerrain()
{
	GridWidth = FMath::Max(GridWidth, 4);
	GridHeight = FMath::Max(GridHeight, 4);
	NumPathways = FMath::Max(NumPathways, 3);

	RandomStream = FRandomStream(Seed);

	PathCellSet.Empty();
	PathwayNodes.Empty();
	Pathways.Empty();
	BuildGridLocations.Empty();

	GenerateGridMesh();
	GeneratePathways();
	GenerateBuildLocations();

	CentralTowerLocation = GridToWorldLocation(GridWidth / 2, GridHeight / 2);
}

void AProceduralTerrain::RandomizeSeedAndRegenerate()
{
	Seed = FMath::Rand();
	GenerateTerrain();

	if (bDrawDebug && GetWorld() && GetWorld()->IsGameWorld())
	{
		DrawDebugVisualization();
	}
}

FVector AProceduralTerrain::GridToWorldLocation(int32 GridX, int32 GridY) const
{
	return GetActorLocation() + FVector(GridX * CellSize, GridY * CellSize, 0.f);
}

void AProceduralTerrain::GenerateGridMesh()
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> VertexColors;

	const int32 VertsX = GridWidth + 1;
	const int32 VertsY = GridHeight + 1;

	Vertices.Reserve(VertsX * VertsY);
	Normals.Reserve(VertsX * VertsY);
	UVs.Reserve(VertsX * VertsY);
	Tangents.Reserve(VertsX * VertsY);

	for (int32 Y = 0; Y < VertsY; ++Y)
	{
		for (int32 X = 0; X < VertsX; ++X)
		{
			Vertices.Add(FVector(X * CellSize, Y * CellSize, 0.f));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(static_cast<float>(X) / GridWidth, static_cast<float>(Y) / GridHeight));
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}
	}

	Triangles.Reserve(GridWidth * GridHeight * 6);
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const int32 TopLeft = Y * VertsX + X;
			const int32 TopRight = TopLeft + 1;
			const int32 BottomLeft = (Y + 1) * VertsX + X;
			const int32 BottomRight = BottomLeft + 1;

			Triangles.Add(TopLeft);
			Triangles.Add(BottomLeft);
			Triangles.Add(TopRight);

			Triangles.Add(TopRight);
			Triangles.Add(BottomLeft);
			Triangles.Add(BottomRight);
		}
	}

	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);

	if (TerrainMaterial)
	{
		ProceduralMesh->SetMaterial(0, TerrainMaterial);
	}
}

FIntPoint AProceduralTerrain::GetRandomEdgeCell(int32 EdgeIndex) const
{
	// 0 = North (Y=0), 1 = South (Y=Max), 2 = West (X=0), 3 = East (X=Max)
	switch (EdgeIndex % 4)
	{
	case 0:
		return FIntPoint(RandomStream.RandRange(0, GridWidth - 1), 0);
	case 1:
		return FIntPoint(RandomStream.RandRange(0, GridWidth - 1), GridHeight - 1);
	case 2:
		return FIntPoint(0, RandomStream.RandRange(0, GridHeight - 1));
	default:
		return FIntPoint(GridWidth - 1, RandomStream.RandRange(0, GridHeight - 1));
	}
}

void AProceduralTerrain::GeneratePathways()
{
	const FIntPoint CenterCell(GridWidth / 2, GridHeight / 2);
	const int32 MaxSteps = (GridWidth + GridHeight) * 3;

	// Shuffle edge order so the first 4 pathways favor 4 distinct edges.
	TArray<int32> EdgeOrder = { 0, 1, 2, 3 };
	for (int32 i = EdgeOrder.Num() - 1; i > 0; --i)
	{
		const int32 j = RandomStream.RandRange(0, i);
		EdgeOrder.Swap(i, j);
	}

	static const FColor PathColors[] = { FColor::Red, FColor::Blue, FColor::Yellow, FColor::Cyan, FColor::Magenta, FColor::Orange };

	for (int32 PathIndex = 0; PathIndex < NumPathways; ++PathIndex)
	{
		FProceduralPathway NewPath;
		NewPath.DebugColor = PathColors[PathIndex % UE_ARRAY_COUNT(PathColors)];

		FIntPoint Current = GetRandomEdgeCell(EdgeOrder[PathIndex % EdgeOrder.Num()]);

		int32 Steps = 0;
		while (Current != CenterCell && Steps < MaxSteps)
		{
			PathCellSet.Add(Current);
			NewPath.Nodes.Add(GridToWorldLocation(Current.X, Current.Y));

			const int32 DeltaX = CenterCell.X - Current.X;
			const int32 DeltaY = CenterCell.Y - Current.Y;

			// 75% of steps move toward the center on the axis with greater distance.
			// 25% of steps wander onto the other axis for an organic, non-straight path.
			bool bMoveX;
			if (RandomStream.FRand() < 0.75f)
			{
				bMoveX = FMath::Abs(DeltaX) >= FMath::Abs(DeltaY);
			}
			else
			{
				bMoveX = RandomStream.FRand() < 0.5f;
			}

			if (bMoveX && DeltaX != 0)
			{
				Current.X += FMath::Sign(DeltaX);
			}
			else if (DeltaY != 0)
			{
				Current.Y += FMath::Sign(DeltaY);
			}
			else if (DeltaX != 0)
			{
				Current.X += FMath::Sign(DeltaX);
			}

			Current.X = FMath::Clamp(Current.X, 0, GridWidth - 1);
			Current.Y = FMath::Clamp(Current.Y, 0, GridHeight - 1);

			++Steps;
		}

		// Guarantee the path terminates exactly at the central tower cell.
		PathCellSet.Add(CenterCell);
		NewPath.Nodes.Add(GridToWorldLocation(CenterCell.X, CenterCell.Y));

		Pathways.Add(NewPath);
		PathwayNodes.Append(NewPath.Nodes);
	}
}

bool AProceduralTerrain::IsNearPathCell(const FIntPoint& Cell) const
{
	for (int32 OffsetX = -PathBufferCells; OffsetX <= PathBufferCells; ++OffsetX)
	{
		for (int32 OffsetY = -PathBufferCells; OffsetY <= PathBufferCells; ++OffsetY)
		{
			if (PathCellSet.Contains(FIntPoint(Cell.X + OffsetX, Cell.Y + OffsetY)))
			{
				return true;
			}
		}
	}
	return false;
}

void AProceduralTerrain::GenerateBuildLocations()
{
	const FIntPoint CenterCell(GridWidth / 2, GridHeight / 2);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FIntPoint Cell(X, Y);

			if (Cell == CenterCell)
			{
				continue; // reserved for the central tower
			}

			if (IsNearPathCell(Cell))
			{
				continue; // keep a clean buffer around pathways
			}

			BuildGridLocations.Add(GridToWorldLocation(X, Y));
		}
	}
}

void AProceduralTerrain::DrawDebugVisualization()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ZOffset = 10.f;

	// Pathways: connected lines + node spheres, colored per-path for clarity.
	for (const FProceduralPathway& Path : Pathways)
	{
		for (int32 i = 0; i < Path.Nodes.Num(); ++i)
		{
			const FVector NodeLocation = Path.Nodes[i] + FVector(0.f, 0.f, ZOffset);
			DrawDebugSphere(World, NodeLocation, DebugSphereRadius, 8, Path.DebugColor, true, -1.f, 0, 2.f);

			if (i > 0)
			{
				const FVector PrevLocation = Path.Nodes[i - 1] + FVector(0.f, 0.f, ZOffset);
				DrawDebugLine(World, PrevLocation, NodeLocation, Path.DebugColor, true, -1.f, 0, 4.f);
			}
		}
	}

	// Valid build locations.
	for (const FVector& BuildLocation : BuildGridLocations)
	{
		DrawDebugSphere(World, BuildLocation + FVector(0.f, 0.f, ZOffset), DebugSphereRadius * 0.5f,
			6, BuildLocationDebugColor, true, -1.f, 0, 1.f);
	}

	// Central tower marker.
	DrawDebugSphere(World, CentralTowerLocation + FVector(0.f, 0.f, ZOffset * 2.f),
		DebugSphereRadius * 2.f, 12, FColor::White, true, -1.f, 0, 3.f);
}