// Fill out your copyright notice in the Description page of Project Settings.


#include "LabyrinthBuilder.h"

// Sets default values
ALabyrinthBuilder::ALabyrinthBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("MyBillboardComponent"));
	RootComponent = BillboardComponent;

	LabyrinthBuilderComponent = CreateDefaultSubobject<ULabyrinthBuilderComponent>(TEXT("Labyrinth Builder"));
}

// Called when the game starts or when spawned
void ALabyrinthBuilder::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ALabyrinthBuilder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

