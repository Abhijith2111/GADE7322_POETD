#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DefenderBase.h"
#include "ProceduralTerrain.h"
#include "TDPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefenderPlacementSucceeded, ADefenderBase*, PlacedDefender);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefenderPlacementFailed, FString, Reason);

UCLASS()
class GADE7322_POETD_API ATDPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	TSubclassOf<ADefenderBase> DefenderClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0"))
	int32 DefenderCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 LocalTestingGold = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0"))
	float PathExclusionDistance = 150.f;

	UPROPERTY(BlueprintAssignable, Category = "Placement")
	FOnDefenderPlacementSucceeded OnDefenderPlacementSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Placement")
	FOnDefenderPlacementFailed OnDefenderPlacementFailed;

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void TryPlaceDefender();

private:
	UPROPERTY()
	AProceduralTerrain* TerrainRef;

	TSet<int32> OccupiedGridIndices;

	bool FindNearestBuildLocation(const FVector& ClickLocation, FVector& OutLocation, int32& OutIndex) const;
	bool IsFarEnoughFromPathways(const FVector& Location) const;
};