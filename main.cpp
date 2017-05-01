// Curses Tetris
// Lewis Moronta @ 2017

#include <ncurses.h>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iostream>

using std::vector;
using std::string;
using std::random_device;
using std::uniform_int_distribution;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

inline void swap(char& i, char& j) { i ^= j; j ^= i; i ^= j; }

class Game {
private:

    // rotation direction
    enum class Direction { CW, CCW };

    // block shape
    const unsigned int block = ' ' | A_REVERSE;

    // all tetrominos
    const vector< vector<string> > shapes {
        { "0000", "1111", "0000", "0000" }, // I
        { "0000", "2220", "0020", "0000" }, // J
        { "0000", "0333", "0300", "0000" }, // L
        { "0000", "0440", "0440", "0000" }, // O
        { "0000", "0550", "5500", "0000" }, // S
        { "0000", "6660", "0600", "0000" }, // T
        { "0000", "7700", "0770", "0000" }  // Z
    };

    // counts
    vector<unsigned int> aggr;
    
    // repr of an empty row
    const vector<char> zeroes;

public:
    Game():
        aggr(shapes.size(), 0),
        zeroes(10, '0'),
        distribution(0, shapes.size() - 1),        
        dropSpeed(0),        
        dropTime(600),
        runTime(0),
        lineCount(0) {

        initGrid();
        nextShape = distribution(generator);
        
        w = initscr();
        
        noecho(); // don't echo input
        nodelay(w, TRUE); // make getch non-blocking
        keypad(stdscr, TRUE); // enables aux keys
        curs_set(0); // hide cursor
        getmaxyx(stdscr, maxHeight, maxWidth); // get screen dimensions
        start_color();

        init_pair(0, COLOR_WHITE, COLOR_BLACK);
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_BLUE, COLOR_BLACK);
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_GREEN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_RED, COLOR_BLACK);
        init_pair(8, COLOR_BLACK, COLOR_WHITE);

        getNextShape();

        loop();
    }

    ~Game() {
        endwin();
        std::cout << "By Lewis Moronta @ 2017\nThanks for playing!" << std::endl;
    }

private:
    void initGrid() {
        grid.clear();
        for (auto j = 0; j < 20; j++)
            grid.push_back(zeroes);
    }
    void drawGrid() {        
        for (auto j = 0; j < 20; j++) {
            for (auto i = 0, xoffset = bg.x; i < 10; i++) {
                auto cell = grid[j][i] - '0';
                if (cell) {
                    attron(COLOR_PAIR(cell));
                    mvaddch(bg.y + j, xoffset + i, block);
                    mvaddch(bg.y + j, ++xoffset + i, block);
                    attroff(COLOR_PAIR(cell));
                } else {
                    attron(COLOR_PAIR(0));
                    mvaddch(bg.y + j, ++xoffset + i, '.');
                    attroff(COLOR_PAIR(0));
                }
            }
        }
    }
    void getNextShape() {
        // grid is 10 units wide, printed in double columns for aesthetics
        initShape(4, -3, nextShape);
        nextShape = distribution(generator);
    }
    void initShape(int x, int y, unsigned int currentShape) {
        aggr[currentShape]++;
        player.x = x; player.y = y;
        player.shape.clear();
        for(auto line: shapes[currentShape])
            player.shape.push_back(line);
    }
    void moveShape(milliseconds& dropClock) {
        dropSpeed = dropTime - milliseconds(lineCount * 2);
        if (dropClock > dropSpeed) {
            dropClock = milliseconds::zero();
            player.y++;
        }
    }    
    void drawShape(vector<string> shape, int x, int y) {
        for (auto row = 0; row < shape.size(); row++) {
            for (auto col = 0, xoffset = (x * 2) + bg.x; col < shape[row].size(); col++) {
                auto cell = shape[row][col] - '0';
                if (cell) {
                    attron(COLOR_PAIR(cell));
                    mvaddch(y + row, xoffset + col, block);
                    mvaddch(y + row, ++xoffset + col, block);
                    attroff(COLOR_PAIR(cell));
                } else xoffset++;
            }
        }
    }    
    bool collideShape(vector<string> shape) {
        for (auto row = 0; row < shape.size(); row++) {
            for (auto col = 0; col < shape[row].size(); col++) {
                auto c = col + player.x;
                auto r = row + player.y;
                if (shape[row][col] > '0' && (r > 19 || ( r > 0 && grid[r][c] > '0' ))) return true;
            }
        }
        return false;
    }
    bool collideGrid(vector<string> shape) {
        for (auto row = 0; row < shape.size(); row++) {
            for (auto col = 0; col < shape[row].size(); col++) {
                auto c = col + player.x;
                auto r = row + player.y;
                if (shape[row][col] > '0' && (r > 19 || c < 0 || c > 9)) return true;
            }
        }
        return false;
    }
    bool collideAll(vector<string> shape) {
        return collideShape(player.shape) || collideGrid(player.shape);
    }
    void onGameOver(bool& gameover) {
        clear();

        auto centerY = maxHeight / 2 - 1;
        for (auto i = 0; i < maxWidth; i++) mvaddch(centerY, i, block);

        attron(COLOR_PAIR(8));
        string gameOverStr = "Game Over!";
        auto centerX = (maxWidth - gameOverStr.size()) / 2;
        mvprintw(centerY, centerX, &gameOverStr[0]);
        attroff(COLOR_PAIR(8));

        string instr = "R: retry, Q: quit";
        centerX = (maxWidth - instr.size()) / 2;
        mvprintw(centerY + 1, centerX, &instr[0]);

        bool menu = true;
        while (menu) {
            switch(getch()) {
                case 'q': case 'Q':
                    menu = false;
                    gameover = true;
                    break;
                case 'r': case 'R':
                    menu = false;
                    fill_n(aggr.begin(), 8, 0);
                    initGrid();
                    getNextShape();
                    break;
            }
        }
    }
    void placeShape(vector<string> shape, bool& gameover) {
        for (auto row = 0; row < shape.size(); row++) {
            for (auto col = 0; col < shape[row].size(); col++) {
                if (shape[row][col] > '0') {                        
                    auto r = row + player.y;
                    auto c = col + player.x;
                    if (r < 0) return onGameOver(gameover);
                    else grid[r][c] = shape[row][col];
                }
            }
        }
        getNextShape();
    }
    unsigned int clearLines() {                

        vector<unsigned short> cleared;
        for (auto row = 0; row < grid.size(); row++) {
            if (all_of(grid[row].begin(), grid[row].end(), [](const char& c) { return c > '0'; }))
                cleared.push_back(row);
        }

        unsigned short clearedCount = cleared.size();
        if (clearedCount) {

            for (auto i = 0; i < maxWidth; i++) mvaddch(0, i, ' ' | A_REVERSE);
            auto centerX = (maxWidth - 10) / 2;
            attron(COLOR_PAIR(8));
            mvprintw(0, centerX, "CLEARED: %d", clearedCount);
            attroff(COLOR_PAIR(8));

            for (auto blink = 0; blink < 4; blink++) {
                if (blink % 2 == 0) {
                    for (auto j = 0; j < clearedCount; j++)
                        mvprintw(cleared[j] + bg.y, bg.x, " . . . . . . . . . .");
                } else drawGrid();
                refresh();
                std::this_thread::sleep_for(milliseconds(200));
            }

            // erase each full row
            while (true) {
                auto clearedRows = false;
                for (auto row = 0; row < grid.size(); row++) {
                    if (all_of(grid[row].begin(), grid[row].end(), [](const char& c) { return c > '0'; })) {
                        grid.erase(grid.begin() + row);
                        clearedRows = true;
                        break;
                    }
                }
                if (!clearedRows) break;
            }

            // replace marked rows with empties at the top
            for (auto i = 0; i < clearedCount; i++) grid.insert(grid.begin(), zeroes);
        }
        return clearedCount;
    }
    void showStats() {        
        mvprintw( 4, bg.statsOffset, "Shapes");
        mvprintw( 5, bg.statsOffset, "======");
        mvprintw( 6, bg.statsOffset, "I: %d, J: %d", aggr[0], aggr[1]);
        mvprintw( 7, bg.statsOffset, "L: %d, O: %d", aggr[2], aggr[3]);
        mvprintw( 8, bg.statsOffset, "S: %d, T: %d", aggr[4], aggr[5]);
        mvprintw( 9, bg.statsOffset, "Z: %d", aggr[6]);

        mvprintw(11, bg.statsOffset, "Stats");
        mvprintw(12, bg.statsOffset, "=====");
        mvprintw(13, bg.statsOffset, "run time: %ds", runTime.count() / 1000);
        mvprintw(14, bg.statsOffset, "line count: %d", lineCount);
        mvprintw(15, bg.statsOffset, "drop speed: %dms", dropSpeed.count());

        mvprintw(17, bg.statsOffset, "Controls");
        mvprintw(18, bg.statsOffset, "========");
        mvprintw(19, bg.statsOffset, "Move <- or ->, up for drop");
        mvprintw(20, bg.statsOffset, "Space for CW, R for CCW");
        
    }
    void loop() {

        bool gameover = false;
        high_resolution_clock::time_point lastTime = high_resolution_clock::now();
        milliseconds dropClock(0);

        while (!gameover) {

            auto current = high_resolution_clock::now();
            auto delta = current - lastTime;
            lastTime = current;

            dropClock += duration_cast<milliseconds>(delta);
            runTime += duration_cast<milliseconds>(delta);

            clear();

            switch ( getch() ) {
                case KEY_LEFT: {
                    player.x--;
                    if (collideAll(player.shape)) player.x++;
                    break;
                }
                case KEY_RIGHT: {
                    player.x++;
                    if (collideAll(player.shape)) player.x--;
                    break;
                }
                case KEY_UP: {
                    while (true) {
                        player.y++;
                        if (collideShape(player.shape)) {
                            player.y--;
                            break;
                        }
                    };
                    placeShape(player.shape, gameover);
                    continue;
                    break;
                }
                case KEY_DOWN: {
                    player.y++;
                    if (collideAll(player.shape)) player.y--;
                    break;
                }
                case 'r': case 'R': {
                    player.rotate(Direction::CCW);
                    if (collideAll(player.shape)) player.rotate(Direction::CW);
                    break;
                }
                case ' ': {
                    player.rotate(Direction::CW);
                    if (collideAll(player.shape)) player.rotate(Direction::CCW);
                    break;
                }
                case 'n': getNextShape(); break;                                            
                case 'q': case 'Q': gameover = true; break;
                default: break;
            }    

            bg.draw();
            drawGrid();

            moveShape(dropClock);

            if (collideShape(player.shape)) {
                player.y--;
                placeShape(player.shape, gameover);
            }

            lineCount += clearLines();         
                  
            drawShape(shapes[nextShape], 12, 0); // draw next shape
            drawShape(player.shape, player.x, player.y); // convert coords to world space            

            showStats();
            refresh();

            // 10 fps
            std::this_thread::sleep_for(current + milliseconds(100) - high_resolution_clock::now());
        }
    }

private:
    WINDOW* w;
    unsigned int maxWidth, maxHeight;
    unsigned short nextShape;
    random_device generator;
    uniform_int_distribution<int> distribution;
    milliseconds dropSpeed;
    milliseconds dropTime;    
    milliseconds runTime;
    unsigned int lineCount;
    vector< vector<char> > grid;
    struct Player { // player pimpl
        int x, y;
        vector<string> shape;        
        void rotate(Direction dir) {
            switch ( dir ) {
                case Direction::CW:
                    for (auto row = 0; row < shape.size(); row++) {
                        for (auto col = row; col < shape[row].size(); col++)
                            if (row != col) swap(shape[col][row], shape[row][col]);
                        reverse(shape[row].begin(), shape[row].end());
                    }                
                    break;
                case Direction::CCW:
                    for (auto row = 0; row < shape.size(); row++)
                        reverse(shape[row].begin(), shape[row].end());
                    for (auto row = 0; row < shape.size(); row++) {
                        for (auto col = row; col < shape[row].size(); col++)
                            if (row != col) swap(shape[col][row], shape[row][col]);                        
                    }   
                    break;
                default: break;
            }
        }
    } player;
    struct Background { // stage pimpl
        const int x = 30, y = 1;
        const int statsOffset = 53;
        const vector<string> data {
            "|| . . . . . . . . . .||",
            " \\+------------------+/",
            "  +------------------+"
        };
        void draw() {
            string buffer = "";
            string margin(x > 1 ? x - 2 : x, ' '); 
            for(auto i = 0; i < 20; i++)
                buffer += margin + data[0] + '\n';
            buffer += margin + data[1] + '\n';
            buffer += margin + data[2] + '\n';
            mvprintw(y, 0, buffer.c_str());
        }
    } bg;
};

int main() { Game(); }
