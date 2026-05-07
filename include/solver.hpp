#pragma once

#include "models.hpp"

#include <string>
#include <vector>

InputData parseInputJson(const std::string& path);
ProjectResult runProjectAnalysis(const InputData& input);
void writeResultJson(const std::string& outputPath, const ProjectResult& result);
void writeCsvTables(const std::string& outDir, const ProjectResult& result);

std::vector<double> solveTridiagonal(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs);
