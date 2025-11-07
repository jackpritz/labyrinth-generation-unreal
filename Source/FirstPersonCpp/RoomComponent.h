// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Containers/Array.h"

#include "RoomComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIRSTPERSONCPP_API URoomComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URoomComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	float DimensionX { 2.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	float DimensionY{ 2.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	TArray<FTransform> Doors;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
