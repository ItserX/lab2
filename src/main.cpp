#include "solver.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct PlotSeries {
    QString name;
    QColor color;
    QVector<QPointF> points;
};

class PlotWidget : public QWidget {
public:
    explicit PlotWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(210);
    }

    void setData(const QString& title, const std::vector<PlotSeries>& series) {
        title_ = title;
        series_ = series;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor(255, 255, 255));
        p.setPen(QColor(30, 30, 30));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        const QRect plotRect(52, 28, width() - 74, height() - 62);
        p.drawText(10, 18, title_);

        if (plotRect.width() < 10 || plotRect.height() < 10 || series_.empty()) {
            return;
        }

        double minX = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();

        int count = 0;
        for (const PlotSeries& s : series_) {
            for (const QPointF& pt : s.points) {
                if (!std::isfinite(pt.x()) || !std::isfinite(pt.y())) {
                    continue;
                }
                minX = std::min(minX, pt.x());
                maxX = std::max(maxX, pt.x());
                minY = std::min(minY, pt.y());
                maxY = std::max(maxY, pt.y());
                ++count;
            }
        }

        if (count == 0) {
            p.setPen(QColor(120, 120, 120));
            p.drawText(plotRect.center(), QString::fromUtf8("Нет данных"));
            return;
        }

        if (std::abs(maxX - minX) < 1e-14) {
            maxX += 1.0;
            minX -= 1.0;
        }
        if (std::abs(maxY - minY) < 1e-14) {
            maxY += 1.0;
            minY -= 1.0;
        }

        auto mapPoint = [&](double x, double y) {
            const double tx = (x - minX) / (maxX - minX);
            const double ty = (y - minY) / (maxY - minY);
            return QPointF(plotRect.left() + tx * plotRect.width(), plotRect.bottom() - ty * plotRect.height());
        };

        p.setPen(QColor(220, 220, 220));
        for (int i = 0; i <= 5; ++i) {
            const double gx = plotRect.left() + i * (plotRect.width() / 5.0);
            const double gy = plotRect.top() + i * (plotRect.height() / 5.0);
            p.drawLine(QPointF(gx, plotRect.top()), QPointF(gx, plotRect.bottom()));
            p.drawLine(QPointF(plotRect.left(), gy), QPointF(plotRect.right(), gy));
        }

        p.setPen(QColor(0, 0, 0));
        p.drawRect(plotRect);

        for (const PlotSeries& s : series_) {
            p.setPen(QPen(s.color, 2));
            QPointF prev;
            bool hasPrev = false;
            for (const QPointF& pt : s.points) {
                if (!std::isfinite(pt.x()) || !std::isfinite(pt.y())) {
                    hasPrev = false;
                    continue;
                }
                const QPointF cur = mapPoint(pt.x(), pt.y());
                if (hasPrev) {
                    p.drawLine(prev, cur);
                }
                prev = cur;
                hasPrev = true;
            }
        }

        int ly = plotRect.top() + 10;
        const int lx = std::max(12, plotRect.right() - 220);
        for (const PlotSeries& s : series_) {
            p.setPen(QPen(s.color, 2));
            p.drawLine(lx, ly, lx + 20, ly);
            p.setPen(QColor(0, 0, 0));
            p.drawText(lx + 24, ly + 4, s.name);
            ly += 16;
        }
    }

private:
    QString title_;
    std::vector<PlotSeries> series_;
};

struct TaskWidgets {
    QLabel* title = nullptr;
    QLabel* status = nullptr;
    QTextEdit* note = nullptr;
    PlotWidget* plotMain = nullptr;
    PlotWidget* plotDiff = nullptr;
    QTableWidget* table = nullptr;
};

class MainWindow : public QMainWindow {
public:
    MainWindow() {
        setupUi();
    }

private:
    QSpinBox* segmentsSpin_ = nullptr;
    QDoubleSpinBox* toleranceSpin_ = nullptr;
    QSpinBox* refinementSpin_ = nullptr;
    QSpinBox* maxSegmentsSpin_ = nullptr;
    QSpinBox* strideSpin_ = nullptr;
    QLabel* variantLabel_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    std::vector<TaskWidgets> taskViews_;
    ProjectResult lastResult_{};

    void setupUi() {
        setWindowTitle(QString::fromUtf8("Boundary Value Problems for ODEs (C++/Qt, Linux)"));
        resize(1320, 860);

        QWidget* central = new QWidget(this);
        auto* root = new QVBoxLayout(central);

        variantLabel_ = new QLabel(QString::fromUtf8("Вариант: —"), central);
        variantLabel_->setWordWrap(true);
        root->addWidget(variantLabel_);

        root->addWidget(buildInputGroup(central));
        root->addLayout(buildButtonsRow(central));
        root->addWidget(buildTabs(central));

        setCentralWidget(central);
    }

    QGroupBox* buildInputGroup(QWidget* parent) {
        auto* box = new QGroupBox(QString::fromUtf8("Input параметры"), parent);
        auto* form = new QFormLayout(box);

        segmentsSpin_ = new QSpinBox(box);
        segmentsSpin_->setRange(2, 1000000);
        segmentsSpin_->setValue(20);

        toleranceSpin_ = new QDoubleSpinBox(box);
        toleranceSpin_->setDecimals(12);
        toleranceSpin_->setRange(1e-15, 1.0);
        toleranceSpin_->setValue(0.5e-6);

        refinementSpin_ = new QSpinBox(box);
        refinementSpin_->setRange(2, 10);
        refinementSpin_->setValue(2);

        maxSegmentsSpin_ = new QSpinBox(box);
        maxSegmentsSpin_->setRange(4, 100000000);
        maxSegmentsSpin_->setValue(1000000);

        strideSpin_ = new QSpinBox(box);
        strideSpin_->setRange(1, 10000);
        strideSpin_->setValue(1);

        form->addRow(QString::fromUtf8("segments"), segmentsSpin_);
        form->addRow(QString::fromUtf8("tolerance"), toleranceSpin_);
        form->addRow(QString::fromUtf8("refinementMultiplier"), refinementSpin_);
        form->addRow(QString::fromUtf8("maxSegments"), maxSegmentsSpin_);
        form->addRow(QString::fromUtf8("tableStride"), strideSpin_);

        return box;
    }

    QHBoxLayout* buildButtonsRow(QWidget* parent) {
        auto* row = new QHBoxLayout();
        auto* runBtn = new QPushButton(QString::fromUtf8("Запустить расчет"), parent);
        auto* saveBtn = new QPushButton(QString::fromUtf8("Сохранить JSON/CSV"), parent);

        row->addWidget(runBtn);
        row->addWidget(saveBtn);
        row->addStretch();

        connect(runBtn, &QPushButton::clicked, this, [this]() { runAnalysis(); });
        connect(saveBtn, &QPushButton::clicked, this, [this]() { saveOutputs(); });

        return row;
    }

    QTabWidget* buildTabs(QWidget* parent) {
        tabs_ = new QTabWidget(parent);
        const std::vector<QString> names = {
            QString::fromUtf8("1. Первая тестовая"),
            QString::fromUtf8("2. Первая основная"),
            QString::fromUtf8("3. Смешанная тестовая"),
            QString::fromUtf8("4. Смешанная основная")};

        for (const QString& name : names) {
            auto* page = new QWidget(tabs_);
            auto* layout = new QVBoxLayout(page);

            TaskWidgets w{};
            w.title = new QLabel(page);
            w.title->setStyleSheet("font-weight: bold;");
            w.status = new QLabel(QString::fromUtf8("Статус: ожидание"), page);
            w.note = new QTextEdit(page);
            w.note->setReadOnly(true);
            w.note->setMinimumHeight(110);
            w.plotMain = new PlotWidget(page);
            w.plotDiff = new PlotWidget(page);
            w.table = new QTableWidget(page);
            w.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            w.table->setSelectionMode(QAbstractItemView::NoSelection);
            w.table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

            layout->addWidget(w.title);
            layout->addWidget(w.status);
            layout->addWidget(w.note);
            layout->addWidget(w.plotMain);
            layout->addWidget(w.plotDiff);
            layout->addWidget(w.table, 1);

            taskViews_.push_back(w);
            tabs_->addTab(page, name);
        }

        return tabs_;
    }

    InputData collectInput() const {
        InputData input;
        input.segments = segmentsSpin_->value();
        input.tolerance = toleranceSpin_->value();
        input.refinementMultiplier = refinementSpin_->value();
        input.maxSegments = maxSegmentsSpin_->value();
        input.tableStride = strideSpin_->value();
        return input;
    }

    static std::vector<PlotSeries> buildTaskMainPlot(const TaskResult& task) {
        std::vector<PlotSeries> series;

        const bool isTest = task.id == "first-dirichlet-test" || task.id == "mixed-test-classic";
        const bool isMain = task.id == "first-dirichlet-main" || task.id == "mixed-main-improved";

        if (task.rows.empty()) {
            return series;
        }

        PlotSeries s1;
        PlotSeries s2;
        s1.color = QColor(0, 102, 204);
        s2.color = QColor(200, 70, 0);

        if (isTest) {
            s1.name = "u(x)";
            s2.name = "v(x)";
            for (const TableRow& row : task.rows) {
                s1.points.push_back(QPointF(row.x, row.u));
                s2.points.push_back(QPointF(row.x, row.v));
            }
        } else if (isMain) {
            s1.name = "v(x)";
            s2.name = "v2(x)";
            for (const TableRow& row : task.rows) {
                s1.points.push_back(QPointF(row.x, row.v));
                s2.points.push_back(QPointF(row.x, row.v2));
            }
        }

        if (!s1.points.isEmpty()) {
            series.push_back(s1);
        }
        if (!s2.points.isEmpty()) {
            series.push_back(s2);
        }

        return series;
    }

    static std::vector<PlotSeries> buildTaskDiffPlot(const TaskResult& task) {
        std::vector<PlotSeries> series;
        if (task.rows.empty()) {
            return series;
        }

        PlotSeries s;
        s.name = "difference";
        s.color = QColor(30, 140, 40);
        for (const TableRow& row : task.rows) {
            s.points.push_back(QPointF(row.x, row.difference));
        }
        series.push_back(s);
        return series;
    }

    void renderTask(int index, const TaskResult& task) {
        if (index < 0 || index >= static_cast<int>(taskViews_.size())) {
            return;
        }
        TaskWidgets& w = taskViews_[static_cast<size_t>(index)];

        w.title->setText(QString::fromUtf8((task.shortTitle + " | " + task.title).c_str()));
        w.status->setText(QString::fromUtf8(("Статус: " + task.status).c_str()));
        w.note->setPlainText(QString::fromUtf8(task.note.c_str()));

        w.table->clear();
        w.table->setRowCount(static_cast<int>(task.rows.size()));
        w.table->setColumnCount(static_cast<int>(task.columns.size()));

        QStringList headers;
        for (const TableColumn& c : task.columns) {
            headers << QString::fromUtf8(c.title.c_str());
        }
        w.table->setHorizontalHeaderLabels(headers);

        for (int r = 0; r < static_cast<int>(task.rows.size()); ++r) {
            const TableRow& row = task.rows[static_cast<size_t>(r)];
            for (int c = 0; c < static_cast<int>(task.columns.size()); ++c) {
                const std::string& key = task.columns[static_cast<size_t>(c)].key;
                QString value;
                if (key == "index") {
                    value = QString::number(row.index);
                } else if (key == "x") {
                    value = QString::number(row.x, 'g', 12);
                } else if (key == "u") {
                    value = QString::number(row.u, 'g', 12);
                } else if (key == "v") {
                    value = QString::number(row.v, 'g', 12);
                } else if (key == "v2") {
                    value = QString::number(row.v2, 'g', 12);
                } else if (key == "difference") {
                    value = QString::number(row.difference, 'g', 12);
                }
                auto* item = new QTableWidgetItem(value);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                w.table->setItem(r, c, item);
            }
        }

        const auto mainSeries = buildTaskMainPlot(task);
        const auto diffSeries = buildTaskDiffPlot(task);

        QString mainTitle = QString::fromUtf8("График 1");
        QString diffTitle = QString::fromUtf8("График 2: difference");
        if (task.id == "first-dirichlet-test" || task.id == "mixed-test-classic") {
            mainTitle = QString::fromUtf8("u(x) и v(x)");
            diffTitle = QString::fromUtf8("u(x)-v(x)");
        } else if (task.id == "first-dirichlet-main" || task.id == "mixed-main-improved") {
            mainTitle = QString::fromUtf8("v(x) и v2(x)");
            diffTitle = QString::fromUtf8("v(x)-v2(x)");
        }

        w.plotMain->setData(mainTitle, mainSeries);
        w.plotDiff->setData(diffTitle, diffSeries);
        w.plotMain->setVisible(!mainSeries.empty());
        w.plotDiff->setVisible(!diffSeries.empty());
    }

    void runAnalysis() {
        try {
            const InputData input = collectInput();
            lastResult_ = runProjectAnalysis(input);

            std::ostringstream vinfo;
            vinfo << "Вариант " << lastResult_.variant.number
                  << ": xi=" << lastResult_.variant.xi
                  << ", mu1=" << lastResult_.variant.mu1
                  << ", mu2=" << lastResult_.variant.mu2
                  << "; k1=" << lastResult_.variant.k1
                  << ", k2=" << lastResult_.variant.k2
                  << ", q1=" << lastResult_.variant.q1
                  << ", q2=" << lastResult_.variant.q2
                  << ", f1=" << lastResult_.variant.f1
                  << ", f2=" << lastResult_.variant.f2;
            variantLabel_->setText(QString::fromUtf8(vinfo.str().c_str()));

            for (size_t i = 0; i < taskViews_.size(); ++i) {
                if (i < lastResult_.tasks.size()) {
                    renderTask(static_cast<int>(i), lastResult_.tasks[i]);
                }
            }
            if (!lastResult_.tasks.empty()) {
                tabs_->setCurrentIndex(0);
            }
        } catch (const std::exception& ex) {
            QMessageBox::critical(this, QString::fromUtf8("Ошибка"), QString::fromUtf8(ex.what()));
        }
    }

    void saveOutputs() {
        if (lastResult_.tasks.empty()) {
            QMessageBox::information(this, QString::fromUtf8("Нет данных"), QString::fromUtf8("Сначала выполните расчет."));
            return;
        }

        const QString outDir = QFileDialog::getExistingDirectory(this, QString::fromUtf8("Выберите папку для сохранения"));
        if (outDir.isEmpty()) {
            return;
        }

        try {
            writeCsvTables(outDir.toStdString(), lastResult_);
            writeResultJson((outDir + "/result.json").toStdString(), lastResult_);
            QMessageBox::information(this, QString::fromUtf8("Готово"), QString::fromUtf8("Файлы result.json и CSV сохранены."));
        } catch (const std::exception& ex) {
            QMessageBox::critical(this, QString::fromUtf8("Ошибка сохранения"), QString::fromUtf8(ex.what()));
        }
    }
};

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

