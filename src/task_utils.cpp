#include "task_utils.hpp"

#include <utility>

TaskResult makeTaskStub(
    std::string id,
    std::string title,
    std::string shortTitle,
    std::string boundaryKind,
    std::string approximationKind,
    std::string ownerHint,
    std::vector<TableColumn> columns) {

    TaskResult task;
    task.id = std::move(id);
    task.title = std::move(title);
    task.shortTitle = std::move(shortTitle);
    task.boundaryKind = std::move(boundaryKind);
    task.approximationKind = std::move(approximationKind);
    task.ownerHint = std::move(ownerHint);
    task.status = "pending";
    task.note = "Задача подготовлена. Выполняется расчет...";
    task.columns = std::move(columns);
    return task;
}

std::vector<TableColumn> makeTestTaskColumns() {
    return {
        {"index", "i"},
        {"x", "x_i"},
        {"u", "u(x_i)"},
        {"v", "v(x_i)"},
        {"difference", "u(x_i)-v(x_i)"}
    };
}

std::vector<TableColumn> makeMainTaskColumns() {
    return {
        {"index", "i"},
        {"x", "x_i"},
        {"v", "v(x_i)"},
        {"v2", "v2(x_2i)"},
        {"difference", "v(x_i)-v2(x_2i)"}
    };
}
