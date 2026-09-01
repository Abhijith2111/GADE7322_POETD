// Fill out your copyright notice in the Description page of Project Settings.

// CentralTowerBase.h
// Phase 1 - Isolated dummy Central Tower actor.
// No references to inventory, currency, HUD, player controllers, or enemy AI.
// Exposes BlueprintAssignable delegates so future systems (HUD, GameMode) can
// bind to health changes without this class ever knowing they exist.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CentralTowerBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCentralTowerHealthChanged, float, NewHealth, float, InMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCentralTowerDestroyed);

/**
 * Dummy central objective actor for Phase 1 testing.
 * Has health only - no combat, targeting, or defense logic yet.
 */
UCLASS()
class GADE7322_POETD_API ACentralTowerBase : public AActor
{
	GENERATED_BODY()

public:
	ACentralTowerBase();

protected:
	virtual void BeginPlay() override;

public:
	// ---------- Components ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TowerMesh;

	// ---------- Health ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 500.f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	// ---------- Decoupled Events ----------
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnCentralTowerHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnCentralTowerDestroyed OnTowerDestroyed;

	// ---------- Public API ----------
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDestroyed() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const;

private:
	bool bIsDestroyed = false;
};