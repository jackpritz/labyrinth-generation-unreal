// Fill out your copyright notice in the Description page of Project Settings.


#include "CellUnitConverter.h"

#include "Math/UnrealMathUtility.h"

CellUnitConverter::~CellUnitConverter()
{
}

int CellUnitConverter::MetersToCellRound(double meters)
{
    return static_cast<int>(FMath::RoundHalfToZero(meters / MetersPerCellUnit));
}

int CellUnitConverter::MetersToCellFloor(double meters)
{
    return static_cast<int>(FMath::Floor(meters / MetersPerCellUnit));
}

double CellUnitConverter::CellToMeters(int cellCoordinateComponent)
{
    return cellCoordinateComponent * MetersPerCellUnit;
}
