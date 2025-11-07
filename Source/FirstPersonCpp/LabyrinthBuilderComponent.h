// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <limits>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "CellUnitConverter.h"
#include "Room.h"

#include "LabyrinthBuilderComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIRSTPERSONCPP_API ULabyrinthBuilderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULabyrinthBuilderComponent();

	void BuildLabyrinth();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	bool BuildOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Seed")
	bool UseExplicitRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random Seed")
	int ExplicitRandomSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	FIntVector2 LabyrinthDimensions = FIntVector2(40, 40);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	int NumberOfRoomsToSpawn = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labyrinth Builder")
	double CellUnit = 2.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Assets")
	TSubclassOf<ARoom> Room;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Assets")
	TSubclassOf<AActor> DoorOpenBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Assets")
	TSubclassOf<AActor> DoorClosedBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Assets")
	TSubclassOf<AActor> HallFloorCeilingBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Assets")
	TSubclassOf<AActor> HallWallBlueprint;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Sentinel values for the distance field
	static const int DISTANCE_FIELD_ROOM{ std::numeric_limits<int>::max() };
	static const int DISTANCE_FIELD_UNCALCULATED{ std::numeric_limits<int>::max() - 1 };
	static const int DISTANCE_FIELD_POTENTIAL_DOOR{ 0 };
	static const int DISTANCE_FIELD_HALL{ std::numeric_limits<int>::min() };

	AActor* SpawnUClass(TSubclassOf<AActor> actor, FIntVector2 cell, FRotator spawnRotation, AActor* parent);
	AActor* SpawnUClass(TSubclassOf<AActor> actor, FVector spawnLocation, FRotator spawnRotation, AActor* parent);

	CellUnitConverter Converter = CellUnitConverter(CellUnit);

	TArray<ARoom*> SpawnedRooms = TArray<ARoom*>();

	TArray<FIntVector2> ZeroDistanceCoordinates = TArray<FIntVector2>();

	TArray<TArray<int>> DistanceField = TArray<TArray<int>>();

	TArray<FIntVector2> TraversalDirections{ {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

	FRandomStream RandomStream;

private:
	void SpawnRooms();
	void SpawnFirstRoom();
	ARoom* SpawnRoom(FIntVector2 cell);

	void AddRoomToDistanceField(FIntVector2 cell);
	void AddRoomDoorsToDistanceField(FIntVector2 cell);

	FIntVector2 FindDoorCoordinate(FIntVector2 roomCell, FTransform door);
	bool        IsInDistanceField(FIntVector2 cell);
	bool        AreRoomExtentsWithinLabyrinth(FIntVector2 position, int sizeX, int sizeY);

	void SetPotentialDoorCell(FIntVector2 cell);
	void SetHallwayCell(FIntVector2 cell);

	FVector2D NextCoordinateAlongSearchPath(FVector2D currentposition, FVector2D searchDirection);

	void ConnectToExistingRooms(FIntVector2 roomSpawnCoordinate, ARoom* room);

	void RecalculateDistanceField();

	void SpawnDoorwayPrefabs();

	void SpawnHallwayWalls();
	void SpawnHallwayWall(FIntVector2 hallwayCell, FIntVector2 wallDirection);

	void DebugTempLogDistanceField();
};
