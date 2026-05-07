#pragma once

#include "models.hpp"

#include <string>
#include <vector>

TaskResult makeTaskStub(
    std::string id,
    std::string title,
    std::string shortTitle,
    std::string boundaryKind,
    std::string approximationKind,
    std::string ownerHint,
    std::vector<TableColumn> columns);

std::vector<TableColumn> makeTestTaskColumns();
std::vector<TableColumn> makeMainTaskColumns();
