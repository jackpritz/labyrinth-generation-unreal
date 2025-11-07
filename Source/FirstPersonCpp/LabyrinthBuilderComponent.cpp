// Fill out your copyright notice in the Description page of Project Settings.

#include "LabyrinthBuilderComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"

#include "Math/Vector2D.h"

// Sets default values for this component's properties
ULabyrinthBuilderComponent::ULabyrinthBuilderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void ULabyrinthBuilderComponent::BuildLabyrinth()
{
    Converter = CellUnitConverter(CellUnit);

    ZeroDistanceCoordinates.Empty();

    DistanceField = TArray<TArray<int>>();
    DistanceField.Empty();
    DistanceField.Init(TArray<int>(), LabyrinthDimensions.Y);
    for (int y = 0; y < LabyrinthDimensions.Y; y++)
    {
        DistanceField[y].Init(DISTANCE_FIELD_UNCALCULATED, LabyrinthDimensions.X);
    }

    SpawnedRooms.Empty();
	
    if (NumberOfRoomsToSpawn < 1)
    {
        UE_LOG(LogTemp, Log, TEXT("Tried to build labyrinth with %i rooms"), NumberOfRoomsToSpawn);
        return;
    }

    // Set up random number generator
    if (UseExplicitRandomSeed)
    {
        RandomStream.Initialize(ExplicitRandomSeed);
        UE_LOG(LogTemp, Log, TEXT("Using explicit random seed %i"), ExplicitRandomSeed);
    }
    else
    {
        RandomStream.GenerateNewSeed();
        UE_LOG(LogTemp, Log, TEXT("Using generated random seed %i"), RandomStream.GetCurrentSeed());
    }

    SpawnRooms();

    SpawnDoorwayPrefabs();

    SpawnHallwayWalls();

    DebugTempLogDistanceField();
}


// Called when the game starts
void ULabyrinthBuilderComponent::BeginPlay()
{
	Super::BeginPlay();

	
	if (BuildOnBeginPlay)
	{
		BuildLabyrinth();
	}
}



AActor* ULabyrinthBuilderComponent::SpawnUClass(TSubclassOf<AActor> actor, FIntVector2 cell, FRotator spawnRotation, AActor* parent)
{
    AActor* Owner = GetOwner();

    FActorSpawnParameters SpawnParameters{};
    SpawnParameters.Owner = Owner;

    FVector SpawnLocation = FVector(
        Converter.CellToMeters(cell.X),
        Converter.CellToMeters(cell.Y),
        0);

    if (actor)
    {
        return SpawnUClass(actor, SpawnLocation, spawnRotation, parent);
    }

    return nullptr;
}

AActor* ULabyrinthBuilderComponent::SpawnUClass(TSubclassOf<AActor> actor, FVector spawnLocation, FRotator spawnRotation, AActor* parent)
{
    AActor* Owner = GetOwner();

    FActorSpawnParameters SpawnParameters{};
    SpawnParameters.Owner = Owner;

    if (actor)
    {
        AActor* newActor = GetWorld()->SpawnActor<AActor>(actor, spawnLocation, spawnRotation, SpawnParameters);
        newActor->AttachToActor(parent, FAttachmentTransformRules::KeepRelativeTransform);
        return newActor;
    }

    return nullptr;
}

void ULabyrinthBuilderComponent::SpawnRooms()
{
    SpawnFirstRoom();
    RecalculateDistanceField();

    FIntVector2 center{ LabyrinthDimensions.X / 2, LabyrinthDimensions.Y / 2 };
    int numToSpawn{ NumberOfRoomsToSpawn - 1 };

    while (numToSpawn > 0)
    {
        // Pick a random direction
        FVector2D direction{
            FMath::FRandRange(-1.0, 1.0) ,
            FMath::FRandRange(-1.0, 1.0) };

        // Find an open space.
        // Start at center and move in the chosen direction looking for enough space for the new room.
        int roomsizeX = Converter.MetersToCellRound(Room.GetDefaultObject()->RoomComponent->DimensionX);
        int roomsizeY = Converter.MetersToCellRound(Room.GetDefaultObject()->RoomComponent->DimensionY);

        FVector2D potentialRoomPosition{
            Converter.CellToMeters(center.X),
            Converter.CellToMeters(center.Y)
        };

        FIntVector2 potentialRoomCoordinates{ center.X, center.Y };
        bool foundSpawn{ false };

        // Search for open space along the search path until:
        // 1. we find open space or
        // 2. we hit the edge.
        while (AreRoomExtentsWithinLabyrinth(potentialRoomCoordinates, roomsizeX, roomsizeY))
        {
            bool roomPositionValid = true;

            // if we find overlap, increment our distance and continue
            for (int y = 0; y < roomsizeY; y++)
            {
                if (!roomPositionValid) { break; }

                for (int x = 0; x < roomsizeX; x++)
                {
                    int cellValue = DistanceField[potentialRoomCoordinates.Y + y][potentialRoomCoordinates.X + x];
                    if (cellValue == DISTANCE_FIELD_ROOM || cellValue <= 0)
                    {
                        potentialRoomPosition = NextCoordinateAlongSearchPath(potentialRoomPosition, direction);
                        potentialRoomCoordinates = FIntVector2(
                            Converter.MetersToCellFloor(potentialRoomPosition.X),
                            Converter.MetersToCellFloor(potentialRoomPosition.Y));

                        // break out to while loop
                        roomPositionValid = false;
                        break;
                    }
                }
            }

            // otherwise, we have found our spawn position
            if (roomPositionValid)
            {
                foundSpawn = true;
                break;
            }
        }

        if (!foundSpawn)
        {
            UE_LOG(LogTemp, Log, TEXT("LabyrinthBuilder could not spawn a room along a search path! Trying a new path."));
            continue; // try again
        }
        else
        {
            numToSpawn--;
        }

        ARoom* newRoom = SpawnRoom(potentialRoomCoordinates);

        ConnectToExistingRooms(potentialRoomCoordinates, newRoom);

        AddRoomDoorsToDistanceField(potentialRoomCoordinates);

        // Update distance field
        RecalculateDistanceField();
    }
}

void ULabyrinthBuilderComponent::SpawnFirstRoom()
{
    FVector roomWorldSpaceDimensions{ Room.GetDefaultObject()->RoomComponent->DimensionX, Room.GetDefaultObject()->RoomComponent->DimensionY, 0 };
    FIntVector2 roomCellDimensions{ Converter.MetersToCellRound(roomWorldSpaceDimensions.X), Converter.MetersToCellRound(roomWorldSpaceDimensions.Y) };

    FIntVector2 roomSpawnCell
    {
        (LabyrinthDimensions.X / 2) - (roomCellDimensions.X / 2),
        (LabyrinthDimensions.Y / 2) - (roomCellDimensions.Y / 2)
    };

    SpawnRoom(roomSpawnCell);

    AddRoomDoorsToDistanceField(roomSpawnCell);
}

/// <summary>
/// Spawn a room at the given location, which is the x, y minimum extent of the room.
/// </summary>
/// <param name="cell"></param>
ARoom* ULabyrinthBuilderComponent::SpawnRoom(FIntVector2 cell)
{
    AActor* Owner = GetOwner();

    FActorSpawnParameters SpawnParameters{};
    SpawnParameters.Owner = Owner;

    ARoom* SpawnedRoom = nullptr;

    if (Room)
    {
        FVector SpawnLocation = FVector(
            Converter.CellToMeters(cell.X),
            Converter.CellToMeters(cell.Y),
            0);
        FRotator SpawnRotation = Owner->GetActorRotation();

        SpawnedRoom = GetWorld()->SpawnActor<ARoom>(Room, SpawnLocation, SpawnRotation, SpawnParameters);
        SpawnedRoom->AttachToActor(Owner, FAttachmentTransformRules::KeepRelativeTransform);
    
        SpawnedRooms.Add(SpawnedRoom);
    }

    AddRoomToDistanceField(cell);

    return SpawnedRoom;
}

void ULabyrinthBuilderComponent::AddRoomToDistanceField(FIntVector2 cell)
{
    FVector roomWorldSpaceDimensions{ Room.GetDefaultObject()->RoomComponent->DimensionX, Room.GetDefaultObject()->RoomComponent->DimensionY, 0};
    FIntVector2 roomCellDimensions{ Converter.MetersToCellRound(roomWorldSpaceDimensions.X), Converter.MetersToCellRound(roomWorldSpaceDimensions.Y) };

    // Make room tiles distance max int. Max int denotes not passable.
    for (int y = 0; y < roomCellDimensions.Y; y++)
    {
        for (int x = 0; x < roomCellDimensions.X; x++)
        {
            int currentRoomX = cell.X + x;
            int currentRoomY = cell.Y + y;
            DistanceField[currentRoomY][currentRoomX] = DISTANCE_FIELD_ROOM;
        }
    }
}

void ULabyrinthBuilderComponent::AddRoomDoorsToDistanceField(FIntVector2 cell)
{
    // Mark the space outside each door as a potential door.
    for (const FTransform& door : Room.GetDefaultObject()->RoomComponent->Doors)
    {
        FIntVector2 doorCoordinate{ FindDoorCoordinate(cell, door) };

        // Make sure we are still in the array and not overriding a room
        if (!IsInDistanceField(doorCoordinate) || DistanceField[doorCoordinate.Y][doorCoordinate.X] == DISTANCE_FIELD_ROOM)
        {
            continue;
        }

        // Make that coordinate 0 in the distance field
        SetPotentialDoorCell(doorCoordinate);
    }
}

FIntVector2 ULabyrinthBuilderComponent::FindDoorCoordinate(FIntVector2 roomCell, FTransform door)
{
    // Move door position forward into an adjoining cell.
        // The provided position is the door prefab spawn position.
        // The forward direction of the provided transform indicates "out of the room"
        // Adding half of a unit also accounts for float precision.
    FVector doorPosition = door.GetLocation();
    FVector hallPosition = FVector{ doorPosition.X, doorPosition.Y, doorPosition.Z };
    FVector doorForward3{ door.GetRotation().GetForwardVector() };
    FVector2D doorForward{ doorForward3.X, doorForward3.Y };
    hallPosition = hallPosition + ((FVector{ doorForward.X, doorForward.Y, 0 } * CellUnit * 0.5f));

    // Translate to coordinate in grid
    int doorX{ roomCell.X + Converter.MetersToCellFloor(hallPosition.X) };

    int doorY{ roomCell.Y + Converter.MetersToCellFloor(hallPosition.Y) };

    return FIntVector2{ doorX, doorY };
}

bool ULabyrinthBuilderComponent::IsInDistanceField(FIntVector2 cell)
{
    return
        (cell.X >= 0) &&
        (cell.Y >= 0) &&
        (cell.X < LabyrinthDimensions.X) &&
        (cell.Y < LabyrinthDimensions.Y);
}

bool ULabyrinthBuilderComponent::AreRoomExtentsWithinLabyrinth(FIntVector2 position, int sizeX, int sizeY)
{
    return
        IsInDistanceField(position + FIntVector2{ 0, sizeY }) &&
        IsInDistanceField(position) &&
        IsInDistanceField(position + FIntVector2{ sizeX, 0 }) &&
        IsInDistanceField(position + FIntVector2{ sizeX, sizeY });
}

void ULabyrinthBuilderComponent::SetPotentialDoorCell(FIntVector2 cell)
{
    // If cell not found in zeroDistanceCoordinates cache
    if (!ZeroDistanceCoordinates.Contains(cell))
    {
        DistanceField[cell.Y][cell.X] = DISTANCE_FIELD_POTENTIAL_DOOR;

        ZeroDistanceCoordinates.Add(cell);
    }
}

void ULabyrinthBuilderComponent::SetHallwayCell(FIntVector2 cell)
{
    // hall overrides potential door. Always set this.
    DistanceField[cell.Y][cell.X] = DISTANCE_FIELD_HALL;

    FIntVector2 coordinate{ cell.X, cell.Y };
    if (!ZeroDistanceCoordinates.Contains(coordinate))
    {
        ZeroDistanceCoordinates.Add(coordinate);
    }
}

FVector2D ULabyrinthBuilderComponent::NextCoordinateAlongSearchPath(FVector2D currentposition, FVector2D searchDirection)
{
    double nextX, nextY;

    if (searchDirection.X > 0)
    {
        nextX = currentposition.X + CellUnit;
    }
    else
    {
        nextX = currentposition.X - CellUnit;
    }

    if (searchDirection.Y > 0)
    {
        nextY = currentposition.Y + CellUnit;
    }
    else
    {
        nextY = currentposition.Y - CellUnit;
    }

    // Use z = mx + b to fill in missing values.
    // m: slope
    // b: z such that x = 0
    double slope = searchDirection.Y / searchDirection.X;
    double yIntercept = currentposition.Y - (slope * currentposition.X); // b = z - (m * x)
    FVector2D targetInterceptX = FVector2D{
        nextX,
        (slope * nextX) + yIntercept // y = (m * x) + b
    };
    FVector2D targetInterceptZ = FVector2D{
        (nextY - yIntercept) / slope, // x = (y - b) / m
        nextY
    };

    // Choose closest candidate as new currentPosition
    if (FVector2D::DistSquared(currentposition, targetInterceptX) < FVector2D::DistSquared(currentposition, targetInterceptZ))
    {
        return targetInterceptX;
    }
    else
    {
        return targetInterceptZ;
    }
}

void ULabyrinthBuilderComponent::ConnectToExistingRooms(FIntVector2 roomSpawnCoordinate, ARoom* room)
{
    TArray<FTransform> doors = room->RoomComponent->Doors;
    if (doors.IsEmpty()) { return; }

    FIntVector2 minimumDistanceDoor{};
    int currentMinimumDistance{ std::numeric_limits<int>::max() };

    // pick a door to connect based on minimum distance in distance field
    for (const FTransform& door : doors)
    {
        FIntVector2 doorCoordinates = FindDoorCoordinate(roomSpawnCoordinate, door);
        int currentDistance{ DistanceField[doorCoordinates.Y][doorCoordinates.X] };

        if (IsInDistanceField(doorCoordinates) &&
            currentDistance < currentMinimumDistance)
        {
            currentMinimumDistance = currentDistance;
            minimumDistanceDoor = doorCoordinates;
        }
    }

    FIntVector2 currentPathLocation = minimumDistanceDoor;

    TArray<FIntVector2> path{};
    path.Add(currentPathLocation);

    while (DistanceField[currentPathLocation.Y][currentPathLocation.X] > 0)
    {
        // look in all directions for minimum distance. set that as new current location.
        FIntVector2 minimumDistanceCell{};
        int currentMinimumCellDistance{ std::numeric_limits<int>::max() };

        for (FIntVector2 direction : TraversalDirections)
        {
            FIntVector2 cellCoord{ currentPathLocation + direction };
            int cellValue{ DistanceField[cellCoord.Y][cellCoord.X] };

            if (cellValue < currentMinimumCellDistance)
            {
                currentMinimumCellDistance = cellValue;
                minimumDistanceCell = cellCoord;
            }
        }

        currentPathLocation = minimumDistanceCell;
        path.Add(currentPathLocation);
    }

    AActor* Owner = GetOwner();
    for (FIntVector2 cell : path)
    {
        if (DistanceField[cell.Y][cell.X] != DISTANCE_FIELD_HALL)
        {
            // spawn hall floor
            SpawnUClass(HallFloorCeilingBlueprint, cell, Owner->GetActorRotation(), Owner);

            SetHallwayCell(cell);
        }
    }
}

void ULabyrinthBuilderComponent::RecalculateDistanceField()
{
    // Check all zero distance coordinates for recalculation of neighbors
    TQueue<FIntVector2> toCheck{};
    for (FIntVector2 coordinate : ZeroDistanceCoordinates)
    {
        toCheck.Enqueue(coordinate);
    }

    while (!toCheck.IsEmpty())
    {
        FIntVector2 currentCoordinate{ *toCheck.Peek()};
        toCheck.Pop();

        int currentCoordinateDistance = DistanceField[currentCoordinate.Y][currentCoordinate.X];

        // max(currentCoordinateDistance, 0) to handle sentinel values.
        if (currentCoordinateDistance < 0) { currentCoordinateDistance = 0; }

        // Check each direction. If a neighbor cell needs a value, update and queue it. Otherwise move to the next cell.
        for (FIntVector2 direction : TraversalDirections)
        {
            FIntVector2 cellToCheck = currentCoordinate + direction;

            if (!IsInDistanceField(cellToCheck))
            {
                continue;
            }

            if (DistanceField[cellToCheck.Y][cellToCheck.X] != DISTANCE_FIELD_ROOM
                && DistanceField[cellToCheck.Y][cellToCheck.X] > currentCoordinateDistance + 1)
            {
                DistanceField[cellToCheck.Y][cellToCheck.X] = currentCoordinateDistance + 1;
                toCheck.Enqueue(cellToCheck);
            }
        }
    }
}

void ULabyrinthBuilderComponent::SpawnDoorwayPrefabs()
{
    for (ARoom * room : SpawnedRooms)
    {

        for (FTransform door : room->RoomComponent->Doors)
        {
            FVector doorLocation = door.GetLocation();

            FVector roomRelativeLocation{ room->GetRootComponent()->GetRelativeLocation() };
            FIntVector2 roomCell{
                Converter.MetersToCellFloor(roomRelativeLocation.X),
                Converter.MetersToCellFloor(roomRelativeLocation.Y)
            };

            FIntVector2 doorCoordinate{ FindDoorCoordinate(roomCell, door) };

            FRotator doorForward{ door.GetRotation() };

            if (DistanceField[doorCoordinate.Y][doorCoordinate.X] == DISTANCE_FIELD_HALL)
            {
                SpawnUClass(DoorOpenBlueprint, doorLocation, doorForward, room);
            }
            else
            {
                SpawnUClass(DoorClosedBlueprint, doorLocation, doorForward, room);
            }
        }
    }
}

void ULabyrinthBuilderComponent::SpawnHallwayWalls()
{
    for (FIntVector2 hallCell : ZeroDistanceCoordinates)
    {
        if (DistanceField[hallCell.Y][hallCell.X] != DISTANCE_FIELD_HALL)
        {
            continue;
        }

        for (FIntVector2 direction : TraversalDirections)
        {
            FIntVector2 adjacentCellCoordinates = hallCell + direction;
            int adjacentCellValue = DistanceField[adjacentCellCoordinates.Y][adjacentCellCoordinates.X];

            if (adjacentCellValue != DISTANCE_FIELD_HALL &&
                adjacentCellValue != DISTANCE_FIELD_ROOM)
            {
                SpawnHallwayWall(hallCell, direction);
            }
        }
    }
}

void ULabyrinthBuilderComponent::SpawnHallwayWall(FIntVector2 hallwayCell, FIntVector2 wallDirection)
{
    FRotator hallwayRotation{ UKismetMathLibrary::FindLookAtRotation(FVector{}, FVector(wallDirection.X, wallDirection.Y, 0)) };
    AActor* newWall = SpawnUClass(
        HallWallBlueprint, 
        hallwayCell, 
        UKismetMathLibrary::FindLookAtRotation(FVector{}, FVector(wallDirection.X, wallDirection.Y, 0)),
        GetOwner());

    if (wallDirection.X == 0 && wallDirection.Y == 1)
    {
        newWall->AddActorLocalOffset(FVector{0, Converter.CellToMeters(-1), 0});
    }
    else if (wallDirection.X == 0 && wallDirection.Y == -1)
    {
        newWall->AddActorLocalOffset(FVector{ Converter.CellToMeters(-1), 0, 0 });
    }
    else if (wallDirection.X == -1 && wallDirection.Y == 0)
    {
        newWall->AddActorLocalOffset(FVector{ Converter.CellToMeters(-1), Converter.CellToMeters(-1), 0 });
    }
    else if (wallDirection.X == 1 && wallDirection.Y == 0) {} // no op
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Error! Unexpected direction found in ULabyrinthBuilderComponent::SpawnHallwayWall: %i, %i"), wallDirection.X, wallDirection.Y);
    }
}

void ULabyrinthBuilderComponent::DebugTempLogDistanceField()
{
    FString LogString{"Distance field:\n"};

    for (int y = 0; y < LabyrinthDimensions.Y; y++)
    {
        // print row number
        LogString += FString::Printf(TEXT("%02d"), y);

        for (int x = 0; x < LabyrinthDimensions.X; x++)
        {
            // give ourselvs a visual indication of every 10 columns
            if (x % 5 == 0)
            {
                LogString += TEXT(" | ");
            }

            int currentCellValue = DistanceField[y][x];

            if (currentCellValue == DISTANCE_FIELD_ROOM)
            {
                LogString += TEXT("room  ");
            }
            else if (currentCellValue == DISTANCE_FIELD_UNCALCULATED)
            {
                LogString += TEXT("....  ");
            }
            else if (currentCellValue == DISTANCE_FIELD_HALL)
            {
                LogString += TEXT("hall  ");
            }
            else
            {
                LogString += FString::Printf(TEXT(" %02d   "), currentCellValue);
            }
        }

        // on to the next row
        LogString += TEXT("\n");
    }

    UE_LOG(LogTemp, Log, TEXT("%s"), *LogString);
}


// Called every frame
void ULabyrinthBuilderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

