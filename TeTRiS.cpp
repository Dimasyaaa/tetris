#include <iostream>
#include <Windows.h>
#include <thread>
#include <vector>
#include <fstream>
#include <string>
#define XPADDING 34
#define YPADDING 5

using namespace std;

class Screen
{
public:
    Screen(int, int);

    const int screenWidth;  // Ширина экрана
    const int screenHeight; // Высота экрана

    wchar_t* screen; // Буфер экрана

    HANDLE hConsole; // Дескриптор консоли
    DWORD dwBytesWritten; // Количество записанных байтов
};

Screen::Screen(int screenWidth, int screenHeight) : screenWidth(screenWidth), screenHeight(screenHeight)
{
    screen = new wchar_t[screenWidth * screenHeight];
    for (int i = 0; i < screenWidth * screenHeight; i++) screen[i] = L' ';
    hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    dwBytesWritten = 0;
}

// Класс Тетромино
class Tetromino
{
public:
    Tetromino(wstring, int, int, int);

    int y; // Вертикальная позиция
    int x; // Горизонтальная позиция
    int rotation; // Угол поворота

    wstring layout; // Макет тетромино

    int rotate(int, int); // Метод поворота тетромино
};

Tetromino::Tetromino(wstring layout, int startingX, int startingY, int startingRotation) : layout(layout), y(startingY), x(startingX), rotation(startingRotation)
{}

int Tetromino::rotate(int x, int y)
{
    /*
    * Поворачивает макет тетромино
    * в зависимости от заданного угла
    * 'rotation'
    */
    switch (rotation % 4) {
    case 0: return y * 4 + x;          // 0 градусов
    case 1: return 12 + y - (x * 4);   // 90 градусов
    case 2: return 15 - (y * 4) - x;   // 180 градусов
    case 3: return 3 - y + (x * 4);    // 270 градусов
    }

    return 0;
}

// Класс Игровое Поле
class PlayingField
{
public:
    PlayingField(int, int);

    const int fieldWidth; // Ширина игрового поля
    const int fieldHeight; // Высота игрового поля

    unsigned char* pField; // Буфер игрового поля

    bool doesPieceFit(Tetromino*, int, int, int); // Проверка, помещается ли фигура на поле
};

PlayingField::PlayingField(int fieldWidth, int fieldHeight) : fieldWidth(fieldWidth), fieldHeight(fieldHeight), pField(nullptr)
{
    // Создание буфера игрового поля
    pField = new unsigned char[fieldHeight * fieldWidth];
    for (int x = 0; x < fieldWidth; x++)
        for (int y = 0; y < fieldHeight; y++)
            // 0 символы - это пробелы, 9 - границы
            pField[y * fieldWidth + x] = (x == 0 || x == fieldWidth - 1 || y == fieldHeight - 1) ? 9 : 0;
}

bool PlayingField::doesPieceFit(Tetromino* tetromino, int rotation, int x, int y)
{
    // Проверка, помещается ли тетромино на игровом поле
    for (int px = 0; px < 4; px++)
        for (int py = 0; py < 4; py++) {
            int pi = tetromino->rotate(px, py);
            int fi = (y + py) * fieldWidth + (x + px);
            if (x + px >= 0 && x + px < fieldWidth)
                if (y + py >= 0 && y + py < fieldHeight)
                    // Если значение ячейки != 0, она занята
                    if (tetromino->layout[pi] == L'X' && pField[fi] != 0)
                        return false;
        }
    return true;
}

// Класс Тетрис
//==============================================================

class Tetris
{
public:
    Tetris(Screen*, PlayingField*, int);

    bool gameOver; // Флаг окончания игры
    int score; // Очки игрока

    void draw(); // Метод для отрисовки игрового поля
    void checkLines(); // Метод для проверки линий
    void computeNextState(); // Метод для вычисления следующего состояния игры
    void lockPieceOnField(); // Метод для закрепления фигуры на поле
    void processInput(); // Метод для обработки ввода
    void synchronizeMovement(); // Метод для синхронизации движения

private:
    int lines; // Количество линий
    int speed; // Скорость игры
    int nextPiece; // Следующая фигура
    int pieceCount; // Общее количество фигур
    int currentPiece; // Текущая фигура
    int speedCounter; // Счетчик скорости

    bool key[4]; // Массив для хранения нажатий клавиш
    bool forceDown; // Сила падения
    bool rotateHold; // Флаг удержания поворота

    Screen* screenBuffer; // Буфер экрана
    Tetromino* tetromino[7]; // Массив тетромино
    PlayingField* playingField; // Игровое поле

    vector<int> fullLines; // Вектор полных линий
};

Tetris::Tetris(Screen* screenBuffer, PlayingField* playingField, int speed) : speed(speed), screenBuffer(screenBuffer), playingField(playingField)
{
    // Инициализация состояния игры
    score = 0;
    lines = 0;
    pieceCount = 0;
    speedCounter = 0;
    gameOver = false;
    forceDown = false;
    nextPiece = rand() % 7;
    currentPiece = rand() % 7;

    // Генерация фигур
    int startingPieceX = playingField->fieldWidth / 2;
    tetromino[0] = new Tetromino(L"..X...X...X...X.", startingPieceX, 0, 0);
    tetromino[1] = new Tetromino(L"..X..XX...X.....", startingPieceX, 0, 0);
    tetromino[2] = new Tetromino(L".....XX..XX.....", startingPieceX, 0, 0);
    tetromino[3] = new Tetromino(L"..X..XX..X......", startingPieceX, 0, 0);
    tetromino[4] = new Tetromino(L".X...XX...X.....", startingPieceX, 0, 0);
    tetromino[5] = new Tetromino(L".X...X...XX.....", startingPieceX, 0, 0);
    tetromino[6] = new Tetromino(L"..X...X..XX.....", startingPieceX, 0, 0);

    rotateHold = true;
}

void Tetris::synchronizeMovement()
{
    // Синхронизация игровых тиков
    this_thread::sleep_for(50ms);
    speedCounter++;
    forceDown = (speed == speedCounter);
}

void Tetris::processInput()
{
    // Обработка ввода с клавиатуры
    for (int k = 0; k < 4; k++)
        key[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;

    // Обработка ввода
    Tetromino* currentTetromino = tetromino[currentPiece];
    currentTetromino->x += (key[0] && playingField->doesPieceFit(currentTetromino, currentTetromino->rotation, currentTetromino->x + 1, currentTetromino->y)) ? 1 : 0;
    currentTetromino->x -= (key[1] && playingField->doesPieceFit(currentTetromino, currentTetromino->rotation, currentTetromino->x - 1, currentTetromino->y)) ? 1 : 0;
    currentTetromino->y += (key[2] && playingField->doesPieceFit(currentTetromino, currentTetromino->rotation, currentTetromino->x, currentTetromino->y + 1)) ? 1 : 0;

    if (key[3]) {
        currentTetromino->rotation += (rotateHold && playingField->doesPieceFit(currentTetromino, currentTetromino->rotation + 1, currentTetromino->x, currentTetromino->y)) ? 1 : 0;
        rotateHold = false;
    }
    else {
        rotateHold = true;
    }
}

void Tetris::computeNextState()
{
    // Вычисление следующего состояния игры
    if (forceDown) {
        Tetromino* currentTetromino = tetromino[currentPiece];
        if (playingField->doesPieceFit(currentTetromino, currentTetromino->rotation, currentTetromino->x, currentTetromino->y + 1)) {
            currentTetromino->y++;
        }
        else {
            lockPieceOnField();

            // Подготовка новой фигуры
            currentPiece = nextPiece;
            nextPiece = rand() % 7;
            tetromino[currentPiece]->rotation = 0;
            tetromino[currentPiece]->y = 0;
            tetromino[currentPiece]->x = playingField->fieldWidth / 2;

            // Увеличение скорости игры каждые 10 тиков
            pieceCount++;
            if (pieceCount % 10 == 0)
                if (speed >= 10) speed--;

            checkLines();

            score += 25;
            if (!fullLines.empty()) score += (1 << fullLines.size()) * 100;

            // Игра окончена, если фигура не помещается
            gameOver = !playingField->doesPieceFit(tetromino[currentPiece], tetromino[currentPiece]->rotation, tetromino[currentPiece]->x, tetromino[currentPiece]->y);
        }
        speedCounter = 0;
    }
}

void Tetris::lockPieceOnField()
{
    // Закрепление фигуры на поле
    Tetromino* currentTetromino = tetromino[currentPiece];
    for (int px = 0; px < 4; px++)
        for (int py = 0; py < 4; py++)
            if (currentTetromino->layout[currentTetromino->rotate(px, py)] == L'X')
                // nCurrentPiece + 1, потому что 0 означает пустые места на игровом поле
                playingField->pField[(currentTetromino->y + py) * playingField->fieldWidth + (currentTetromino->x + px)] = currentPiece + 1;
}

void Tetris::checkLines()
{
    // Проверка заполненных линий
    Tetromino* currentTetromino = tetromino[currentPiece];
    for (int py = 0; py < 4; py++) {
        if (currentTetromino->y + py < playingField->fieldHeight - 1) {
            bool bLine = true;
            for (int px = 1; px < playingField->fieldWidth; px++)
                // Если любая ячейка пустая, линия не завершена
                bLine &= (playingField->pField[(currentTetromino->y + py) * playingField->fieldWidth + px]) != 0;
            if (bLine) {
                // Рисуем символы '='
                for (int px = 1; px < playingField->fieldWidth - 1; px++)
                    playingField->pField[(currentTetromino->y + py) * playingField->fieldWidth + px] = 8;
                fullLines.push_back(currentTetromino->y + py);
                lines++;
            }
        }
    }
}

void Tetris::draw()
{
    // Отрисовка игрового поля
    for (int x = 0; x < playingField->fieldWidth; x++)
        for (int y = 0; y < playingField->fieldHeight; y++)
            // Привязка игрового поля (' ', 1,..., 9) к символам экрана (' ', A,...,#)
            screenBuffer->screen[(y + YPADDING) * screenBuffer->screenWidth + (x + XPADDING)] = L" ABCDEFG=#"[playingField->pField[y * playingField->fieldWidth + x]];

    // Отрисовка фигур
    for (int px = 0; px < 4; px++)
        for (int py = 0; py < 4; py++) {
            if (tetromino[currentPiece]->layout[tetromino[currentPiece]->rotate(px, py)] == L'X')
                // Отрисовка текущей фигуры (n + ASCII код символа 'A') 0 -> A, 1 -> B, ...
                screenBuffer->screen[(tetromino[currentPiece]->y + py + YPADDING) * screenBuffer->screenWidth + (tetromino[currentPiece]->x + px + XPADDING)] = currentPiece + 65;
            if (tetromino[nextPiece]->layout[tetromino[nextPiece]->rotate(px, py)] == L'X')
                // Отрисовка следующей фигуры (n + ASCII код символа 'A') 0 -> A, 1 -> B, ...
                screenBuffer->screen[(YPADDING + 3 + py) * screenBuffer->screenWidth + (XPADDING / 2 + px + 3)] = nextPiece + 65;
            else
                screenBuffer->screen[(YPADDING + 3 + py) * screenBuffer->screenWidth + (XPADDING / 2 + px + 3)] = ' ';
        }

    // Отображение счета и линий
    swprintf_s(&screenBuffer->screen[YPADDING * screenBuffer->screenWidth + XPADDING / 4], 16, L"SCORE: %8d", score);
    swprintf_s(&screenBuffer->screen[(YPADDING + 1) * screenBuffer->screenWidth + XPADDING / 4], 16, L"LINES: %8d", lines);
    swprintf_s(&screenBuffer->screen[(YPADDING + 4) * screenBuffer->screenWidth + XPADDING / 4], 13, L"NEXT PIECE: ");

    // Проверка и удаление заполненных линий
    if (!fullLines.empty()) {
        WriteConsoleOutputCharacter(screenBuffer->hConsole, screenBuffer->screen, screenBuffer->screenWidth * screenBuffer->screenHeight, { 0,0 }, &screenBuffer->dwBytesWritten);
        this_thread::sleep_for(400ms);
        for (auto& v : fullLines)
            for (int px = 1; px < playingField->fieldWidth - 1; px++) {
                for (int py = v; py > 0; py--)
                    // Очистка линии, перемещение линий выше на одну единицу вниз
                    playingField->pField[py * playingField->fieldWidth + px] = playingField->pField[(py - 1) * playingField->fieldWidth + px];
                playingField->pField[px] = 0;
            }
        fullLines.clear();
    }

    // Отображение экрана
    WriteConsoleOutputCharacter(screenBuffer->hConsole, screenBuffer->screen, screenBuffer->screenWidth * screenBuffer->screenHeight, { 0,0 }, &screenBuffer->dwBytesWritten);
}

// Функция для отображения меню
void showMenu() {
    cout << "1. Начать игру" << endl;
    cout << "2. Изменить цвет консоли" << endl;
    cout << "3. Таблица лидеров" << endl;
    cout << "4. Управление" << endl;
    cout << "5. Выход" << endl;
    cout << "Выберите опцию: ";
}

// Функция для изменения цвета консоли
void changeConsoleColor() {
    int color;
    cout << "Выберите цвет (0 - 15):" << endl;
    cout << "0 - Черный, 1 - Синий, 2 - Зеленый, 3 - Голубой, 4 - Красный, 5 - Пурпурный, 6 - Желтый, 7 - Белый," << endl;
    cout << "8 - Серый, 9 - Светло-синий, 10 - Светло-зеленый, 11 - Светло-голубой, 12 - Светло-красный, 13 - Светло-пурпурный, 14 - Светло-желтый, 15 - Ярко-белый." << endl;
    cin >> color;

    // Установка цвета текста
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Функция для записи в таблицу лидеров
void writeToLeaderboard(int score) {
    ofstream leaderboardFile("leaders.txt", ios::app);
    if (leaderboardFile.is_open()) {
        string name;
        cout << "Введите ваше имя для таблицы лидеров: ";
        cin >> name;
        leaderboardFile << name << ": " << score << endl;
        leaderboardFile.close();
        cout << "Результат записан в таблицу лидеров." << endl;
    }
    else {
        cout << "Не удалось открыть файл таблицы лидеров." << endl;
    }
}

// Функция для отображения таблицы лидеров
void showLeaderboard() {
    ifstream leaderboardFile("leaders.txt");
    if (leaderboardFile.is_open()) {
        string line;
        cout << "Таблица лидеров:" << endl;
        while (getline(leaderboardFile, line)) {
            cout << line << endl;
        }
        leaderboardFile.close();
    }
    else {
        cout << "Файл таблицы лидеров не найден." << endl;
    }
}

int main(void) {

    setlocale(LC_ALL,"Rus");

    int choice;

    do {
        showMenu();
        cin >> choice;

        switch (choice) {
        case 1: {
            // Начать игру
            Screen* screenBuffer = new Screen(80, 30);
            PlayingField* playingField = new PlayingField(12, 18);
            Tetris* tetrisGame = new Tetris(screenBuffer, playingField, 20);

            // Основной игровой цикл
            while (!tetrisGame->gameOver) {
                // Время
                tetrisGame->synchronizeMovement();
                // Ввод
                tetrisGame->processInput();
                // Логика
                tetrisGame->computeNextState();
                // Отрисовка
                tetrisGame->draw();
            }

            CloseHandle(screenBuffer->hConsole);
            cout << "Игра окончена! Счет:" << tetrisGame->score << endl;
            writeToLeaderboard(tetrisGame->score); // Запись в таблицу лидеров
            system("pause");
            delete tetrisGame;
            delete playingField;
            delete screenBuffer;
            break;
        }
        case 2:
            changeConsoleColor(); // Смена цвета консоли
            break;
        case 3:
            showLeaderboard(); // Показываем таблицу лидеров
            break;
        case 4:
            cout << "УПРАВЛЕНИЕ:" << endl;
            cout << "Z- переворот" << endl;
            cout << "Остальное управление стрелками" << endl;
        case 5:
            cout << "Выход из игры." << endl;
            break;
        default:
            cout << "Некорректный выбор. Попробуйте снова." << endl;
            break;
        }
    } while (choice != 5);

    return 0;
}
