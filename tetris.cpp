#include <iostream>
#include <fstream>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

#include <random>
#include <chrono>

class RNG {
public:
    RNG() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::mt19937::result_type seed = static_cast<std::mt19937::result_type>(now);
        engine.seed(seed);
    }

    int rand(int min, int max) {
        std::uniform_int_distribution<> distr(min, max);
        return distr(engine);
    }

private:
    std::mt19937 engine; // Mersenne Twister engine
};

/* ========================================================================= */
/*                      Matrix manipulation functions                        */
/* ========================================================================= */

void create_matrix(int*** matrix, int h, int w) {
    *matrix = new int*[h];

    for (int i = 0; i < h; ++i) {
        (*matrix)[i] = new int[w]{};
    }
}

void free_matrix(int** matrix, int h) {
    for (int i = 0; i < h; ++i) {
        delete matrix[i];
    }

    delete matrix;
}

void print_matrix(int** matrix, int h, int w) {
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            std::cout << matrix[i][j] << " ";
        }

        std::cout << '\n';
    }
}


/* ========================================================================= */
/*                              Tetris Class                                 */
/* ========================================================================= */

class Tetris {
private:
    int** m_field;
    int m_height{ 20 };
    int m_width{ 10 };

    int** m_figure;
    int m_size{ 4 };
    int m_x{ 0 };
    int m_y{ 0 };

    RNG rng;

    enum States {
        INITIAL,
        MOVING,
        SHIFTING,
        ATTACHING,
        PAUSE,
        GAME_OVER
    } state{ INITIAL };

public:
    Tetris() {
        init_ncurses();

        init_game();
    }

    ~Tetris() {
        deinit_game();

        deinit_ncurses();
    }

    void play() {
        render_game();

        int count = 0;
        int max_count = 20;
        while (state != GAME_OVER) {
            usleep(25000);

            switch (state) {
                case INITIAL:
                    wait_for_start();
                    break;
                case MOVING:
                    process_user_input();
                    break;
                case SHIFTING:
                    shift_figure();
                    break;
                case ATTACHING:
                    attach_figure();
                    break;
                case PAUSE:
                    process_user_input();
                    break;
                case GAME_OVER:
                    break;
            }

            if (state == MOVING && ++count == max_count) {
                count = 0;
                state = SHIFTING;
            }

            render_game();
        }
    }

private:

/* ========================================================================= */
/*                               Game Library                                */
/* ========================================================================= */

    void attach_figure() {
        add_figure_to_field();

        clear_rows();

        if (is_game_over()) {
            state = GAME_OVER;
            render_game();
            wait_to_quit_or_restart();
        }

        spawn_figure();

        state = MOVING;
    }

    void clear_rows() {
        for (int i = m_height - 1; i > 0; --i) {
            if (is_row_full(i)) {
                for (int k = i; k > 0; --k) {
                    for (int j = 0; j < m_width; ++j) {
                        m_field[k][j] = m_field[k - 1][j];
                    }
                }

                ++i;
            }
        }
    }

    bool is_row_full(int i) {
        bool is_full = true;

        for (int j = 0; j < m_width; ++j) {
            if (!m_field[i][j]) {
                is_full = false;
            }
        }

        return is_full;
    }

    void init_game() {
        create_matrix(&m_field, m_height, m_width);

        m_size = create_figure(&m_figure);

        m_x = (m_width - m_size) / 2;
        m_y = 0;
    }

    void deinit_game() {
        free_matrix(m_field, m_height);
        free_matrix(m_figure, m_size);
    }

    void restart_game() {
        deinit_game();

        init_game();

        state = INITIAL;
    }

    bool is_game_over() {
        return m_y <= 0;
    }

    void process_user_input() {
        char key = getch();

        if (key == 'q') {
            state = GAME_OVER;
        } else if (key == 'r') {
            restart_game();
        } else if (key == 'p') {
            state = (state == PAUSE) ? MOVING : PAUSE;
        } else if (state == MOVING) {
            if (key == 's') {
                move_figure_down();
            } else if (key == 'a') {
                move_figure_left();
            } else if (key == 'd') {
                move_figure_right();
            } else if (key == 'w') {
                rotate_figure();
            }

            if (is_figure_attached()) {
                state = ATTACHING;
            }
        }
    }

    void rotate_figure() {
        int** rotated_figure;
        create_matrix(&rotated_figure, m_size, m_size);

        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                rotated_figure[j][m_size - i - 1] = m_figure[i][j];
            }
        }

        int** temp_figure = m_figure;
        m_figure = rotated_figure;

        if (!is_figure_in_field() && !is_figure_attached()) {
            m_figure = temp_figure;

            free_matrix(rotated_figure, m_size);
        } else {
            free_matrix(temp_figure, m_size);
        }
    }

    void move_figure_down() {
        ++m_y;
    }

    bool does_figure_fit() {
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                int x = m_x + j;
                int y = m_y + i;

                if (m_figure[i][j] && m_field[y][x]) {
                    return false;
                }
            }
        }

        return true;
    }

    void move_figure_left() {
        --m_x;

        if (!is_figure_in_field() || !does_figure_fit()) {
            ++m_x;
        }
    }

    void move_figure_right() {
        ++m_x;

        if (!is_figure_in_field() || !does_figure_fit()) {
            --m_x;
        }
    }

    void wait_for_start() {
        char key;
        while ((key = getch()) != '\n') {
            if (key == 'q') {
                return;
            }
        }

        state = MOVING;
    }

    void wait_to_quit_or_restart() {
        char key;
        while ((key = getch()) != 'q') {
            if (key == 'r' || key == '\n') {
                restart_game();
                return;
            }
        }

        state = GAME_OVER;
    }

    void spawn_figure() {
        free_matrix(m_figure, m_size);

        create_figure(&m_figure);

        m_x = (m_width - m_size) / 2 + rng.rand(-3, 3);
        m_y = 0;
    }

    int create_figure(int*** figure) {
        std::ifstream fin("figures.txt");

        int figure_count;
        fin >> figure_count;

        int figure_number = rng.rand(1, figure_count);

        int count = 0;

        int size = 0;
        while (count != figure_number) {
            fin >> size;

            if (size > 1) {
                ++count;
            }
        }

        create_matrix(&m_figure, size, size);

        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                fin >> (*figure)[i][j];
            }
        }

        fin.close();

        m_size = size;

        return size;
    }

    void shift_figure() {
        if (!is_figure_attached()) {
            m_y += 1;
            state = MOVING;
        }

        if (is_figure_attached()) {
            state = ATTACHING;
        }
    }

    bool is_figure_attached() {
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                int x = m_x + j;
                int y = m_y + i + 1;

                if (m_figure[i][j] && (y >= m_height || m_field[y][x])) {
                    return true;
                }
            }
        }

        return false;
    }

    void add_figure_to_field() {
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                int x = m_x + j;
                int y = m_y + i;

                if (m_figure[i][j] && is_block_in_field(x, y)) {
                    m_field[y][x] = 1;
                }
            }
        }
    }

    void remove_figure_from_field() {
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                int x = m_x + j;
                int y = m_y + i;

                if (m_figure[i][j] && is_block_in_field(x, y)) {
                    m_field[y][x] = 0;
                }
            }
        }
    }

    bool is_figure_in_field() {
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                int x = m_x + j;
                int y = m_y + i;

                if (m_figure[i][j] && !is_block_in_field(x, y)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool is_block_in_field(int x, int y) {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    void init_ncurses() {
        initscr();
        nodelay(stdscr, TRUE);
        curs_set(0);
        cbreak();
        keypad(stdscr, TRUE);
        noecho();
    }

    void deinit_ncurses() {
        endwin();
    }

    void render_game() {
        if (state != INITIAL && state != GAME_OVER && state != PAUSE) { add_figure_to_field(); }

        WINDOW *game_window = newwin(m_height + 2, 3 * m_width + 2, 0, 0);

        refresh();
        box(game_window, 0, 0);

        if (state == INITIAL) {
            mvwprintw(game_window, 1, 2, "[ENTER]   to start");
            mvwprintw(game_window, 2, 2, "[A][S][D] to move");
            mvwprintw(game_window, 3, 2, "[W]       to rotate");
            mvwprintw(game_window, 4, 2, "[R]       to restart");
            mvwprintw(game_window, 5, 2, "[Q]       to quit");
            mvwprintw(game_window, 10, 13, "[START]");
        } else if (state == GAME_OVER) {
            mvwprintw(game_window, 10, 11, "[GAME_OVER]");
        } else if (state == PAUSE) {
            mvwprintw(game_window, 10, 13, "[PAUSE]");
        } else {
            for (int i = 0; i < m_height; ++i) {
                for (int j = 0; j < m_width; ++j) {
                    if (m_field[i][j] == 1) {
                        mvwprintw(game_window, i + 1, 3 * j + 1, "[+]");
                    } else {
                        mvwprintw(game_window, i + 1, 3 * j + 1, "   ");
                    }
                }
            }
        }

        wrefresh(game_window);

        delwin(game_window);

        if (state != INITIAL && state != GAME_OVER && state != PAUSE) { remove_figure_from_field(); }
    }

};

int main() {
    Tetris tetris;

    tetris.play();

    return 0;
}