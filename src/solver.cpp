#include "solver.hpp"

#include "tasks/first_dirichlet_main.hpp"
#include "tasks/first_dirichlet_test.hpp"
#include "tasks/mixed_main_improved.hpp"
#include "tasks/mixed_test_classic.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

std::string readTextFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open input file: " + path);
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

double parseNumber(const std::string& text, const std::string& key, double fallback) {
    const std::string needle = "\"" + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) {
        return fallback;
    }

    pos = text.find(':', pos);
    if (pos == std::string::npos) {
        return fallback;
    }
    ++pos;

    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }

    size_t end = pos;
    while (end < text.size()) {
        const char ch = text[end];
        const bool partOfNumber =
            std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == '+' || ch == 'e' || ch == 'E';
        if (!partOfNumber) {
            break;
        }
        ++end;
    }

    if (end == pos) {
        return fallback;
    }
    return std::stod(text.substr(pos, end - pos));
}

int parseInt(const std::string& text, const std::string& key, int fallback) {
    return static_cast<int>(std::llround(parseNumber(text, key, static_cast<double>(fallback))));
}

std::string escapeJson(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += ch;
                break;
        }
    }
    return out;
}

void validateInput(InputData& input) {
    if (input.segments <= 0) {
        throw std::runtime_error("segments must be positive");
    }
    if (input.tolerance <= 0.0) {
        throw std::runtime_error("tolerance must be positive");
    }
    if (input.refinementMultiplier < 2) {
        throw std::runtime_error("refinementMultiplier must be at least 2");
    }
    if (input.maxSegments < input.segments) {
        throw std::runtime_error("maxSegments must be >= segments");
    }
    if (input.tableStride <= 0) {
        input.tableStride = 1;
    }
}

void writeColumnsJson(std::ostream& out, const std::vector<TableColumn>& columns) {
    out << "[";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "{\"key\":\"" << escapeJson(columns[i].key) << "\",\"title\":\"" << escapeJson(columns[i].title) << "\"}";
    }
    out << "]";
}

void writeRowsJson(std::ostream& out, const std::vector<TableRow>& rows) {
    out << "[";
    for (size_t i = 0; i < rows.size(); ++i) {
        const TableRow& row = rows[i];
        if (i > 0) {
            out << ",";
        }
        out << "{"
            << "\"index\":" << row.index << ","
            << "\"x\":" << std::setprecision(16) << row.x << ","
            << "\"u\":" << row.u << ","
            << "\"v\":" << row.v << ","
            << "\"v2\":" << row.v2 << ","
            << "\"difference\":" << row.difference
            << "}";
    }
    out << "]";
}

void writeTaskJson(std::ostream& out, const TaskResult& task) {
    out << "{\n";
    out << "\"id\":\"" << escapeJson(task.id) << "\",\n";
    out << "\"title\":\"" << escapeJson(task.title) << "\",\n";
    out << "\"shortTitle\":\"" << escapeJson(task.shortTitle) << "\",\n";
    out << "\"boundaryKind\":\"" << escapeJson(task.boundaryKind) << "\",\n";
    out << "\"approximationKind\":\"" << escapeJson(task.approximationKind) << "\",\n";
    out << "\"ownerHint\":\"" << escapeJson(task.ownerHint) << "\",\n";
    out << "\"status\":\"" << escapeJson(task.status) << "\",\n";
    out << "\"note\":\"" << escapeJson(task.note) << "\",\n";
    out << "\"columns\":";
    writeColumnsJson(out, task.columns);
    out << ",\n\"rows\":";
    writeRowsJson(out, task.rows);
    out << "\n}";
}

std::string csvNameForTask(const std::string& id) {
    std::string name = id;
    std::replace(name.begin(), name.end(), '-', '_');
    return name + ".csv";
}

double valueByColumnKey(const TableRow& row, const std::string& key) {
    if (key == "index") {
        return static_cast<double>(row.index);
    }
    if (key == "x") {
        return row.x;
    }
    if (key == "u") {
        return row.u;
    }
    if (key == "v") {
        return row.v;
    }
    if (key == "v2") {
        return row.v2;
    }
    if (key == "difference") {
        return row.difference;
    }
    return 0.0;
}

}  // namespace

InputData parseInputJson(const std::string& path) {
    const std::string text = readTextFile(path);

    InputData input;
    input.segments = parseInt(text, "segments", input.segments);
    input.tolerance = parseNumber(text, "tolerance", input.tolerance);
    input.refinementMultiplier = parseInt(text, "refinementMultiplier", input.refinementMultiplier);
    input.maxSegments = parseInt(text, "maxSegments", input.maxSegments);
    input.tableStride = parseInt(text, "tableStride", input.tableStride);

    validateInput(input);
    return input;
}

std::vector<double> solveTridiagonal(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs) {

    const size_t n = diagonal.size();
    if (rhs.size() != n || lower.size() + 1 < n || upper.size() + 1 < n) {
        throw std::runtime_error("Invalid tridiagonal system size");
    }
    if (n == 0) {
        return {};
    }

    std::vector<double> cPrime(n, 0.0);
    std::vector<double> dPrime(n, 0.0);

    if (std::abs(diagonal[0]) < 1e-15) {
        throw std::runtime_error("Degenerate tridiagonal matrix");
    }

    cPrime[0] = n > 1 ? upper[0] / diagonal[0] : 0.0;
    dPrime[0] = rhs[0] / diagonal[0];

    for (size_t i = 1; i < n; ++i) {
        const double denom = diagonal[i] - lower[i - 1] * cPrime[i - 1];
        if (std::abs(denom) < 1e-15) {
            throw std::runtime_error("Degenerate tridiagonal matrix");
        }
        cPrime[i] = (i + 1 < n) ? upper[i] / denom : 0.0;
        dPrime[i] = (rhs[i] - lower[i - 1] * dPrime[i - 1]) / denom;
    }

    std::vector<double> x(n, 0.0);
    x[n - 1] = dPrime[n - 1];
    for (size_t i = n - 1; i-- > 0;) {
        x[i] = dPrime[i] - cPrime[i] * x[i + 1];
    }
    return x;
}

ProjectResult runProjectAnalysis(const InputData& inputData) {
    InputData input = inputData;
    validateInput(input);

    ProjectResult result;
    result.input = input;
    result.variant.number = 1;
    result.variant.xi = 0.4;
    result.variant.mu1 = 0.0;
    result.variant.mu2 = 1.0;
    result.variant.k1 = "x + 1";
    result.variant.k2 = "x";
    result.variant.q1 = "x";
    result.variant.q2 = "x^2";
    result.variant.f1 = "x";
    result.variant.f2 = "exp(-x)";

    result.tasks.push_back(runFirstDirichletTestTask(input, result.variant));
    result.tasks.push_back(runFirstDirichletMainTask(input, result.variant));
    result.tasks.push_back(runMixedTestClassicTask(input, result.variant));
    result.tasks.push_back(runMixedMainImprovedTask(input, result.variant));

    return result;
}

void writeResultJson(const std::string& outputPath, const ProjectResult& result) {
    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot write output JSON: " + outputPath);
    }

    out << "{\n";
    out << "\"variant\":{"
        << "\"number\":" << result.variant.number << ","
        << "\"xi\":" << std::setprecision(16) << result.variant.xi << ","
        << "\"mu1\":" << result.variant.mu1 << ","
        << "\"mu2\":" << result.variant.mu2 << ","
        << "\"k1\":\"" << escapeJson(result.variant.k1) << "\","
        << "\"k2\":\"" << escapeJson(result.variant.k2) << "\","
        << "\"q1\":\"" << escapeJson(result.variant.q1) << "\","
        << "\"q2\":\"" << escapeJson(result.variant.q2) << "\","
        << "\"f1\":\"" << escapeJson(result.variant.f1) << "\","
        << "\"f2\":\"" << escapeJson(result.variant.f2) << "\""
        << "},\n";

    out << "\"input\":{"
        << "\"segments\":" << result.input.segments << ","
        << "\"tolerance\":" << std::setprecision(16) << result.input.tolerance << ","
        << "\"refinementMultiplier\":" << result.input.refinementMultiplier << ","
        << "\"maxSegments\":" << result.input.maxSegments << ","
        << "\"tableStride\":" << result.input.tableStride
        << "},\n";

    out << "\"tasks\":[\n";
    for (size_t i = 0; i < result.tasks.size(); ++i) {
        writeTaskJson(out, result.tasks[i]);
        if (i + 1 < result.tasks.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "]\n";
    out << "}\n";
}

void writeCsvTables(const std::string& outDir, const ProjectResult& result) {
    const std::filesystem::path base(outDir);
    std::filesystem::create_directories(base);

    for (const TaskResult& task : result.tasks) {
        std::ofstream out(base / csvNameForTask(task.id));
        if (!out) {
            throw std::runtime_error("Cannot write task CSV");
        }

        for (size_t i = 0; i < task.columns.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << task.columns[i].title;
        }
        out << "\n";

        out << std::setprecision(12);
        for (const TableRow& row : task.rows) {
            for (size_t i = 0; i < task.columns.size(); ++i) {
                if (i > 0) {
                    out << ",";
                }
                if (task.columns[i].key == "index") {
                    out << row.index;
                } else {
                    out << valueByColumnKey(row, task.columns[i].key);
                }
            }
            out << "\n";
        }

    }
}
