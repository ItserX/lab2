#pragma once

#include <string>
#include <vector>

struct InputData {
    int segments = 20;
    double tolerance = 0.5e-6;
    int refinementMultiplier = 2;
    int maxSegments = 1000000;
    int tableStride = 1;
};

struct VariantData {
    int number = 8;
    double xi = 0.0;
    double mu1 = 2.0;
    double mu2 = 1.0;
    std::string k1 = "1";
    std::string k2 = "exp(x^2)";
    std::string q1 = "x^2";
    std::string q2 = "1 + x^4";
    std::string f1 = "x^2 - 1";
    std::string f2 = "1";
};

struct TableColumn {
    std::string key;
    std::string title;
};

struct TableRow {
    int index = 0;
    double x = 0.0;
    double u = 0.0;
    double v = 0.0;
    double v2 = 0.0;
    double difference = 0.0;
};

struct TaskResult {
    std::string id;
    std::string title;
    std::string shortTitle;
    std::string boundaryKind;
    std::string approximationKind;
    std::string ownerHint;
    std::string status;
    std::string note;
    std::vector<TableColumn> columns;
    std::vector<TableRow> rows;
};

struct ProjectResult {
    InputData input;
    VariantData variant;
    std::vector<TaskResult> tasks;
};
