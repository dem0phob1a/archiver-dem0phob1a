# Huffman Archiver

Консольный архиватор на основе алгоритма Хаффмана, написанный на C99.  
Поддерживает сжатие и распаковку произвольных бинарных файлов в формате `.huff`.

## Возможности

- Сжатие и распаковка любых файлов (текст, бинарники, и т.д.)
- Собственный формат `.huff` с заголовком и таблицей частот
- Побитовая запись/чтение с LSB-first упаковкой
- Приоритетная очередь (min-heap) для построения дерева Хаффмана
- Полный набор юнит- и интеграционных тестов
- Сборка через **Make** или **CMake**

## Структура проекта

```
archiver/
├── include/
│   ├── pqueue.h       # Min-heap приоритетная очередь
│   ├── huffman.h      # Дерево Хаффмана и кодовая таблица
│   ├── bitstream.h    # Побитовый ввод/вывод
│   └── compress.h     # Высокоуровневый API сжатия/распаковки
├── src/
│   ├── pqueue.c
│   ├── huffman.c
│   ├── bitstream.c
│   └── compress.c
├── tests/
│   ├── test_pqueue.c
│   ├── test_huffman.c
│   ├── test_bitstream.c
│   └── test_compress.c
├── main.c             # CLI точка входа
├── Makefile
├── CMakeLists.txt
└── LICENSE
```

## Сборка

### Make

```bash
# Собрать бинарник
make

# Собрать и запустить все тесты
make test

# Сборка с AddressSanitizer + UBSan
make debug

# Очистить артефакты сборки
make clean
```

### CMake

```bash
mkdir build && cd build

# Обычная сборка
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Запустить тесты
ctest --output-on-failure

# Сборка с санитайзерами
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build .
ctest --output-on-failure
```

## Использование

```bash
# Сжать файл
./archiver compress <input> <output.huff>

# Распаковать файл
./archiver decompress <input.huff> <output>

# Просмотреть метаданные .huff файла
./archiver info <input.huff>
```

### Примеры

```bash
# Сжатие текстового файла
./archiver compress README.md README.huff
# Compressed  : README.md → README.huff
# Size        : 1200 → 820 bytes (68.3% of original)

# Распаковка
./archiver decompress README.huff README_out.md
diff README.md README_out.md && echo "Файлы идентичны"

# Информация о сжатом файле
./archiver info README.huff
# File      : README.huff
# Version   : 1
# Orig size : 1200 bytes
# Comp size : 820 bytes
# Symbols   : 67 distinct
# Padding   : 3 bit(s)
# Ratio     : 68.33% of original
```

## Формат файла `.huff`

| Поле          | Размер   | Описание                                      |
|---------------|----------|-----------------------------------------------|
| Magic         | 4 байта  | `H U F F` (0x48 0x55 0x46 0x46)              |
| Version       | 1 байт   | Версия формата (текущая: `0x01`)              |
| Orig size     | 8 байт   | Размер оригинального файла (uint64, LE)       |
| Distinct      | 2 байта  | Количество уникальных символов (uint16, LE)   |
| Freq table    | N × 9 байт | Таблица частот: `[symbol: u8][freq: i64 LE]`|
| Padding bits  | 1 байт   | Количество нулевых бит-паддингов (0–7)        |
| Bitstream     | M байт   | Сжатые данные (LSB-first)                     |

## Алгоритм

1. **Подсчёт частот** — однопроходное сканирование входного файла
2. **Построение дерева** — min-heap из листьев, итеративное объединение двух минимальных узлов
3. **Генерация кодов** — DFS по дереву, бит `0` — левый ребёнок, `1` — правый
4. **Кодирование** — повторное чтение файла, запись битовых кодов через `BitWriter`
5. **Декодирование** — побитовый обход дерева от корня до листа, повтор `orig_size` раз

Коды хранятся LSB-first: первый бит пути записывается в младший бит байта.

## Особенности реализации

- **Файл из одного символа** — дерево из одного листа, каждый символ кодируется 1 битом (`0`), декодер использует специальный путь
- **Пустой файл** — записывается минимальный заголовок, данных нет
- **Большие файлы** — буферизованный I/O блоками по 64 КиБ, два прохода по источнику

## Зависимости

- Компилятор C99 (GCC, Clang, MSVC)
- CMake ≥ 3.10 (опционально) или GNU Make

## Лицензия

MIT — см. файл [LICENSE](LICENSE).
