# Решение краевых задач для ОДУ

Лабораторная работа №3: **Решение краевых задач для ОДУ**.

Проект рассчитан на работу 4 человек в одном репозитории: каждый участник реализует свою задачу в отдельном C++ файле и делает Pull Request. UI уже разделен на 4 вкладки и ожидает результаты от общего C++ backend.

## Постановка задачи

Рассматривается стационарное уравнение теплопроводности:

```text
d/dx ( k(x) du/dx ) - q(x) u(x) = -f(x),  x in (0, 1)
```

Коэффициенты `k(x)`, `q(x)`, `f(x)` имеют разрыв в точке `xi in (0, 1)`:

```text
k(x) = k1(x),  x in [0, xi]
k(x) = k2(x),  x in (xi, 1]

q(x) = q1(x),  x in [0, xi]
q(x) = q2(x),  x in (xi, 1]

f(x) = f1(x),  x in [0, xi]
f(x) = f2(x),  x in (xi, 1]
```

Решение `u(x)` должно быть непрерывным, и тепловой поток

```text
w(x) = -k(x) du/dx
```

также должен быть непрерывным в точке разрыва.

Для численного решения используется метод баланса и метод прогонки для решения трехдиагональной СЛАУ.

## Вариант 8

Во всех четырех задачах используется один вариант:

```text
xi = 1 / sqrt(3)
mu1 = 2
mu2 = 1

k1(x) = 1
k2(x) = exp(x^2)

q1(x) = x^2
q2(x) = 1 + x^4

f1(x) = x^2 - 1
f2(x) = 1
```

В C++ эти данные хранятся в `VariantData` из [include/models.hpp](include/models.hpp).

## Распределение задач

Каждый участник работает в своем файле:

| № | Задача | C++ файл | UI вкладка |
|---|---|---|---|
| 1 | Первая краевая тестовая задача | [src/tasks/first_dirichlet_test.cpp](src/tasks/first_dirichlet_test.cpp) | `1. Тестовая` |
| 2 | Первая краевая основная задача | [src/tasks/first_dirichlet_main.cpp](src/tasks/first_dirichlet_main.cpp) | `2. Основная` |
| 3 | Смешанная краевая тестовая задача, классическая аппроксимация ГУ | [src/tasks/mixed_test_classic.cpp](src/tasks/mixed_test_classic.cpp) | `3. Смеш. тест.` |
| 4 | Смешанная краевая основная задача, улучшенная аппроксимация ГУ | [src/tasks/mixed_main_improved.cpp](src/tasks/mixed_main_improved.cpp) | `4. Смеш. осн. улучш.` |

Общие файлы:

- [src/solver.cpp](src/solver.cpp) - диспетчер задач, чтение input, запись `result.json` и CSV;
- [src/task_utils.cpp](src/task_utils.cpp) - общие заглушки и стандартные колонки таблиц;
- [include/models.hpp](include/models.hpp) - структуры данных, которые видит UI;
- [include/solver.hpp](include/solver.hpp) - общий метод прогонки `solveTridiagonal`.

## Как реализовать свою задачу

Откройте только свой файл из `src/tasks/`. Например, для задачи 4:

```cpp
TaskResult runMixedMainImprovedTask(const InputData& input, const VariantData& variant) {
    // здесь будет реализация
}
```

Функция должна вернуть `TaskResult`. Минимально нужно заполнить:

- `id` - не менять, UI по нему находит вкладку;
- `title`, `shortTitle`, `boundaryKind`, `approximationKind` - текст для UI;
- `status` - например `"done"` после реализации;
- `note` - справка: сетка `n`, точность, где достигнут максимум ошибки;
- `columns` - колонки таблицы;
- `rows` - строки таблицы для UI, CSV и графиков.

Проще всего начать с текущей заглушки:

```cpp
TaskResult task = makeTaskStub(
    "mixed-main-improved",
    "Смешанная краевая основная задача, улучшенная аппроксимация ГУ",
    "4. Смешанная основная, улучш. ГУ",
    "Смешанные граничные условия",
    "Улучшенная аппроксимация граничных условий",
    "Смешанная краевая основная задача, улучш. аппрокс. ГУ",
    makeMainTaskColumns());

task.status = "done";
task.note = "n = ..., epsilon_2 = ..., максимум разности в точке x = ...";
task.rows.push_back(TableRow{/* index */ 0, /* x */ 0.0, /* u */ 0.0, /* v */ 0.0, /* v2 */ 0.0, /* difference */ 0.0});
return task;
```

## Контракт таблиц для UI

UI умеет читать поля структуры `TableRow`:

```cpp
struct TableRow {
    int index;
    double x;
    double u;
    double v;
    double v2;
    double difference;
};
```

Для тестовых задач используйте колонки:

```cpp
makeTestTaskColumns()
```

Они соответствуют таблице:

```text
i, x_i, u(x_i), v(x_i), u(x_i)-v(x_i)
```

Заполняйте:

- `index` - номер узла;
- `x` - координата узла;
- `u` - аналитическое решение;
- `v` - численное решение;
- `difference` - `u - v`.

Для основных задач используйте колонки:

```cpp
makeMainTaskColumns()
```

Они соответствуют таблице:

```text
i, x_i, v(x_i), v2(x_2i), v(x_i)-v2(x_2i)
```

Заполняйте:

- `index` - номер узла на сетке `n`;
- `x` - координата общего узла;
- `v` - решение на сетке `n`;
- `v2` - решение на сетке `2n` в том же узле;
- `difference` - `v - v2`.

Если сетка большая, можно выводить не все узлы. Для этого используйте `input.tableStride`: например, добавлять в `rows` каждый `tableStride`-й узел, но максимум ошибки считать по всем нужным узлам.

## Метод прогонки

Общий метод прогонки уже есть:

```cpp
std::vector<double> solveTridiagonal(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs);
```

Где:

- `diagonal[i]` - главная диагональ;
- `lower[i]` - нижняя диагональ, элемент под `diagonal[i + 1]`;
- `upper[i]` - верхняя диагональ, элемент над `diagonal[i]`;
- `rhs[i]` - правая часть.

Размеры:

```text
diagonal.size() == rhs.size()
lower.size() == diagonal.size() - 1
upper.size() == diagonal.size() - 1
```

## Как UI получает данные

Backend пишет файл:

```text
output/result.json
```

UI читает из него массив `tasks`. Каждая задача должна иметь стабильный `id`:

```text
first-dirichlet-test
first-dirichlet-main
mixed-test-classic
mixed-main-improved
```

Если `id` изменить, UI не сможет привязать результат к нужной вкладке.

После запуска C++ backend UI автоматически:

- показывает `note` в блоке "Справка и состояние";
- строит таблицу из `columns` и `rows`;
- строит графики по `x`, `u`, `v`, `v2`, `difference`.

## Входные параметры

Пример input находится в [input_examples/default_input.json](input_examples/default_input.json):

```json
{
  "segments": 20,
  "tolerance": 5e-7,
  "refinementMultiplier": 2,
  "maxSegments": 1000000,
  "tableStride": 1
}
```

Поля доступны в C++ через `InputData`:

- `segments` - начальное число разбиений `n`;
- `tolerance` - требуемая точность, по заданию `0.5 * 10^-6`;
- `refinementMultiplier` - во сколько раз сгущать сетку, обычно `2`;
- `maxSegments` - верхняя граница поиска по `n`;
- `tableStride` - шаг вывода строк в таблицу.

## Запуск

Сборка backend:

```powershell
.\run.ps1 -Mode build
```

Запуск backend на `input_examples/default_input.json`:

```powershell
.\run.ps1 -Mode run
```

Запуск UI:

```powershell
.\run.ps1 -Mode ui
```

Быстрый запуск на маленьких параметрах:

```powershell
.\run.ps1 -Mode quick
```

## Чеклист перед Pull Request

Перед PR проверьте:

- вы меняли только свой файл из `src/tasks/` и, если нужно, свой заголовок из `include/tasks/`;
- `id` задачи не изменен;
- `task.status = "done"` или понятный промежуточный статус;
- `task.note` содержит `n`, достигнутую точность и важные выводы;
- `task.rows` не пустой;
- для тестовой задачи заполнены `u`, `v`, `difference`;
- для основной задачи заполнены `v`, `v2`, `difference`;
- проект собирается через `.\run.ps1 -Mode build`;
- после `.\run.ps1 -Mode run` в UI корректно загружается `output/result.json`.

## Структура проекта

```text
.
├── CMakeLists.txt
├── include/
│   ├── models.hpp
│   ├── solver.hpp
│   ├── task_utils.hpp
│   └── tasks/
│       ├── first_dirichlet_test.hpp
│       ├── first_dirichlet_main.hpp
│       ├── mixed_test_classic.hpp
│       └── mixed_main_improved.hpp
├── src/
│   ├── main.cpp
│   ├── solver.cpp
│   ├── task_utils.cpp
│   └── tasks/
│       ├── first_dirichlet_test.cpp
│       ├── first_dirichlet_main.cpp
│       ├── mixed_test_classic.cpp
│       └── mixed_main_improved.cpp
├── ui/
│   ├── app.py
│   └── requirements.txt
├── input_examples/
│   ├── default_input.json
│   └── quick_input.json
├── docs/
│   └── report_template.md
└── run.ps1
```
# lab2
