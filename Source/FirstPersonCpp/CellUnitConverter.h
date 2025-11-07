// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class FIRSTPERSONCPP_API CellUnitConverter
{
public:
	CellUnitConverter(double metersPerCellUnit) : MetersPerCellUnit{ metersPerCellUnit } {}
	~CellUnitConverter();

	int MetersToCellRound(double meters);

	int MetersToCellFloor(double meters);

	double CellToMeters(int cellCoordinateComponent);

private:
	double MetersPerCellUnit;
};
