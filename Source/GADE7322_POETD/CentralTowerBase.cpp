#include "CentralTowerBase.h"
#include "UObject/ConstructorHelpers.h"

ACentralTowerBase::ACentralTowerBase()
{
	PrimaryActorTick.bCanEverTick = false;

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMesh"));
	RootComponent = TowerMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		TowerMesh->SetStaticMesh(CubeMeshAsset.Object);
		TowerMesh->SetWorldScale3D(FVector(2.f, 2.f, 4.f));
	}

	TowerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TowerMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ACentralTowerBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDestroyed = false;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void ACentralTowerBase::ApplyDamage(float DamageAmount)
{
	if (bIsDestroyed || DamageAmount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.f)
	{
		bIsDestroyed = true;
		OnTowerDestroyed.Broadcast();
	}
}

bool ACentralTowerBase::IsDestroyed() const
{
	return bIsDestroyed;
}

float ACentralTowerBase::GetHealthPercent() const
{
	return MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;
}