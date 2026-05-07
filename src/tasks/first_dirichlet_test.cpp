#include "tasks/first_dirichlet_test.hpp"

#include "solver.hpp"
#include "task_utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

struct TestCoefficients {
    double kLeft;
    double kRight;
    double qLeft;
    double qRight;
    double fLeft;
    double fRight;
};

struct AnalyticSolution {
    double lambdaLeft;
    double lambdaRight;
    double shiftLeft;
    double shiftRight;
    double c1;
    double c2;
    double c3;
    double c4;
};

TestCoefficients makeTestCoefficients(const VariantData& variant) {
    const double xi = variant.xi;
    TestCoefficients tc{};
    tc.kLeft = 1.0;
    tc.kRight = std::exp(xi * xi);
    tc.qLeft = xi * xi;
    tc.qRight = 1.0 + xi * xi * xi * xi;
    tc.fLeft = xi * xi - 1.0;
    tc.fRight = 1.0;
    return tc;
}

double splitIntegral(double a, double b, double xi, double leftValue, double rightValue) {
    if (b <= a) {
        return 0.0;
    }
    if (b <= xi) {
        return (b - a) * leftValue;
    }
    if (a >= xi) {
        return (b - a) * rightValue;
    }
    return (xi - a) * leftValue + (b - xi) * rightValue;
}

std::array<double, 4> solve4x4(std::array<std::array<double, 5>, 4> a) {
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        double mx = std::abs(a[col][col]);
        for (int row = col + 1; row < 4; ++row) {
            const double cand = std::abs(a[row][col]);
            if (cand > mx) {
                mx = cand;
                pivot = row;
            }
        }

        if (mx < 1e-15) {
            throw std::runtime_error("Degenerate 4x4 system");
        }

        if (pivot != col) {
            std::swap(a[pivot], a[col]);
        }

        const double div = a[col][col];
        for (int j = col; j <= 4; ++j) {
            a[col][j] /= div;
        }

        for (int row = 0; row < 4; ++row) {
            if (row == col) {
                continue;
            }
            const double factor = a[row][col];
            for (int j = col; j <= 4; ++j) {
                a[row][j] -= factor * a[col][j];
            }
        }
    }

    return {a[0][4], a[1][4], a[2][4], a[3][4]};
}

AnalyticSolution buildAnalytic(const VariantData& variant, const TestCoefficients& tc) {
    AnalyticSolution s{};
    s.lambdaLeft = std::sqrt(tc.qLeft / tc.kLeft);
    s.lambdaRight = std::sqrt(tc.qRight / tc.kRight);
    s.shiftLeft = tc.fLeft / tc.qLeft;
    s.shiftRight = tc.fRight / tc.qRight;

    const double xi = variant.xi;
    const double eLx = std::exp(s.lambdaLeft * xi);
    const double eLmx = std::exp(-s.lambdaLeft * xi);
    const double eRx = std::exp(s.lambdaRight * xi);
    const double eRmx = std::exp(-s.lambdaRight * xi);
    const double eR1 = std::exp(s.lambdaRight);
    const double eRm1 = std::exp(-s.lambdaRight);

    std::array<std::array<double, 5>, 4> m{};
    m[0] = {1.0, 1.0, 0.0, 0.0, variant.mu1 - s.shiftLeft};
    m[1] = {0.0, 0.0, eR1, eRm1, variant.mu2 - s.shiftRight};
    m[2] = {eLx, eLmx, -eRx, -eRmx, s.shiftRight - s.shiftLeft};
    m[3] = {
        tc.kLeft * s.lambdaLeft * eLx,
        -tc.kLeft * s.lambdaLeft * eLmx,
        -tc.kRight * s.lambdaRight * eRx,
        tc.kRight * s.lambdaRight * eRmx,
        0.0
    };

    const auto c = solve4x4(m);
    s.c1 = c[0];
    s.c2 = c[1];
    s.c3 = c[2];
    s.c4 = c[3];
    return s;
}

double evalAnalytic(double x, const VariantData& variant, const AnalyticSolution& s) {
    if (x <= variant.xi) {
        return s.c1 * std::exp(s.lambdaLeft * x) + s.c2 * std::exp(-s.lambdaLeft * x) + s.shiftLeft;
    }
    return s.c3 * std::exp(s.lambdaRight * x) + s.c4 * std::exp(-s.lambdaRight * x) + s.shiftRight;
}

std::vector<double> solveGrid(int n, const VariantData& variant, const TestCoefficients& tc) {
    const double h = 1.0 / static_cast<double>(n);
    const int N = n - 1;

    std::vector<double> lower(static_cast<size_t>(std::max(0, N - 1)), 0.0);
    std::vector<double> diag(static_cast<size_t>(N), 0.0);
    std::vector<double> upper(static_cast<size_t>(std::max(0, N - 1)), 0.0);
    std::vector<double> rhs(static_cast<size_t>(N), 0.0);

    for (int i = 1; i <= N; ++i) {
        const double xi = i * h;

        const double invA = splitIntegral(xi - h, xi, variant.xi, 1.0 / tc.kLeft, 1.0 / tc.kRight);
        const double invB = splitIntegral(xi, xi + h, variant.xi, 1.0 / tc.kLeft, 1.0 / tc.kRight);
        const double ai = h / invA;
        const double aip1 = h / invB;

        const double di = splitIntegral(xi - 0.5 * h, xi + 0.5 * h, variant.xi, tc.qLeft, tc.qRight) / h;
        const double phi = splitIntegral(xi - 0.5 * h, xi + 0.5 * h, variant.xi, tc.fLeft, tc.fRight) / h;

        const double A = ai / h;
        const double B = aip1 / h;
        const double C = A + B + di * h;
        const double F = phi * h;

        const int idx = i - 1;
        diag[static_cast<size_t>(idx)] = C;
        rhs[static_cast<size_t>(idx)] = F;

        if (idx > 0) {
            lower[static_cast<size_t>(idx - 1)] = -A;
        }
        if (idx < N - 1) {
            upper[static_cast<size_t>(idx)] = -B;
        }

        if (i == 1) {
            rhs[static_cast<size_t>(idx)] += A * variant.mu1;
        }
        if (i == N) {
            rhs[static_cast<size_t>(idx)] += B * variant.mu2;
        }
    }

    const std::vector<double> inner = solveTridiagonal(lower, diag, upper, rhs);
    std::vector<double> full(static_cast<size_t>(n + 1), 0.0);
    full[0] = variant.mu1;
    full[static_cast<size_t>(n)] = variant.mu2;

    for (int i = 0; i < N; ++i) {
        full[static_cast<size_t>(i + 1)] = inner[static_cast<size_t>(i)];
    }

    return full;
}

std::string toSci(double value, int precision = 6) {
    std::ostringstream out;
    out << std::scientific << std::setprecision(precision) << value;
    return out.str();
}

}  // namespace

TaskResult runFirstDirichletTestTask(const InputData& input, const VariantData& variant) {
    TaskResult task = makeTaskStub(
        "first-dirichlet-test",
        "Первая краевая тестовая задача",
        "1. Тестовая",
        "u(0)=mu1, u(1)=mu2",
        "Метод баланса, тестовая задача с аналитическим решением",
        "Исполнитель 1",
        makeTestTaskColumns());

    const TestCoefficients tc = makeTestCoefficients(variant);
    const AnalyticSolution exact = buildAnalytic(variant, tc);

    int n = std::max(2, input.segments);
    bool reached = false;
    std::vector<double> v;
    double eps1 = 0.0;
    double maxX = 0.0;

    while (n <= input.maxSegments) {
        v = solveGrid(n, variant, tc);
        eps1 = 0.0;
        maxX = 0.0;

        for (int i = 0; i <= n; ++i) {
            const double x = static_cast<double>(i) / static_cast<double>(n);
            const double u = evalAnalytic(x, variant, exact);
            const double diff = std::abs(u - v[static_cast<size_t>(i)]);
            if (diff > eps1) {
                eps1 = diff;
                maxX = x;
            }
        }

        if (eps1 <= input.tolerance) {
            reached = true;
            break;
        }
        if (n > input.maxSegments / 2) {
            break;
        }
        n *= 2;
    }

    task.status = reached ? "done" : "warning";
    std::ostringstream note;
    note << "Для решения задачи использована равномерная сетка с числом разбиений n = " << n << ";\n"
         << "задача должна быть решена с погрешностью не более epsilon = " << toSci(input.tolerance, 1) << ";\n"
         << "задача решена с погрешностью epsilon_1 = " << toSci(eps1, 6) << ";\n"
         << "максимальное отклонение аналитического и численного решений наблюдается в точке x = "
         << std::fixed << std::setprecision(6) << maxX << ".";
    if (!reached) {
        note << "\nЗаданная точность не достигнута в пределах maxSegments.";
    }
    task.note = note.str();

    const int stride = std::max(1, input.tableStride);
    for (int i = 0; i <= n; i += stride) {
        const double x = static_cast<double>(i) / static_cast<double>(n);
        const double u = evalAnalytic(x, variant, exact);
        const double vv = v[static_cast<size_t>(i)];
        task.rows.push_back(TableRow{i, x, u, vv, 0.0, u - vv});
    }

    if (task.rows.empty() || task.rows.back().index != n) {
        const double u = evalAnalytic(1.0, variant, exact);
        const double vv = v[static_cast<size_t>(n)];
        task.rows.push_back(TableRow{n, 1.0, u, vv, 0.0, u - vv});
    }

    return task;
}

