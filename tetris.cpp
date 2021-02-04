#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <complex>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

#include <sys/time.h>

#define BOARD_X 10
#define BOARD_Y 24
#include "tetris.h"

#define KEY_COOLDOWN 7
#define BLOCK_MOVE_DELTA 30

GLFWwindow* window;
int game_board[BOARD_X][BOARD_Y];
int block_buffer[BOARD_X][BOARD_Y];
std::optional<TetrisBlock> current_block;
std::map<int, int> key_cooldown;
int block_move_counter = 0;
bool fast_fall = false;

// clang-format off
static std::complex<int> blocks[][4] = {
    {{-1, 0}, {0, 0}, {1, 0}, {2, 0}},
    {{0, -1}, {1, -1}, {1, 0}, {1, 1}},
    {{0, 1}, {0, 0}, {0, -1}, {1, -1}},
    {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
    {{0, -1}, {1, 0}, {-1, 0}, {0, 0}},
    {{-1, 0}, {0, 0}, {0, -1}, {1, 0}}
};
// clang-format on

static int keys_with_cooldown[] = {GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN};

void TetrisBlock::move_left()
{
	auto positions = this->get_squares_pos();

	for (auto pos : positions)
	{
		if (pos.real() <= 0)
			return;

		if (game_board[pos.real() - 1][pos.imag()] != GC_empty)
			return;
	}

	this->x--;
}

void TetrisBlock::move_right()
{
	auto positions = this->get_squares_pos();

	for (auto pos : positions)
	{
		if (pos.real() >= BOARD_X - 1)
			return;

		if (game_board[pos.real() + 1][pos.imag()] != GC_empty)
			return;
	}

	this->x++;
}

void TetrisBlock::rotate()
{
	this->rotation *= (std::complex<int>){0, 1};

	auto positions = this->get_squares_pos();

	for (auto pos : positions)
	{
		if (pos.real() < 0)
			this->x++;
		if (pos.real() >= BOARD_X)
			this->x--;
	}
}

std::vector<std::complex<int>> TetrisBlock::get_squares_pos()
{
	std::vector<std::complex<int>> ret = {};

	for (int i = 0; i < 4; i++)
	{
		ret.push_back(blocks[this->block_type][i] * this->rotation +
			      (std::complex<int>){this->x, this->y});
	}

	return ret;
}

void init_key_cooldown()
{
	key_cooldown[GLFW_KEY_LEFT] = 0;
	key_cooldown[GLFW_KEY_RIGHT] = 0;
	key_cooldown[GLFW_KEY_UP] = 0;
	key_cooldown[GLFW_KEY_DOWN] = 0;
}

void handle_input()
{

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (current_block)
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS &&
		    key_cooldown[GLFW_KEY_LEFT] == 0)
		{
			key_cooldown[GLFW_KEY_LEFT] = KEY_COOLDOWN;
			current_block->move_left();
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS &&
		    key_cooldown[GLFW_KEY_RIGHT] == 0)
		{
			key_cooldown[GLFW_KEY_RIGHT] = KEY_COOLDOWN;
			current_block->move_right();
		}

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && key_cooldown[GLFW_KEY_UP] == 0)
		{
			key_cooldown[GLFW_KEY_UP] = KEY_COOLDOWN;
			// current_block->rotation *= std::complex<int>{0, 1};
			current_block->rotate();
		}

		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS &&
		    key_cooldown[GLFW_KEY_DOWN] == 0)
		{
			key_cooldown[GLFW_KEY_DOWN] = KEY_COOLDOWN;

			if (!block_is_grounded())
			{
				current_block->y--;
				fast_fall = false;
			}
		}
	}

	for (auto key : keys_with_cooldown)
	{
		if (key_cooldown[key] > 0)
			key_cooldown[key]--;
	}
}

void draw_square(double x, double y, double dx, double dy, std::tuple<double, double, double> rgb)
{
	auto [r, g, b] = rgb;
	glColor3d(r, g, b);

	glBegin(GL_POLYGON);

	glVertex2d(x, y);
	glVertex2d(x + dx, y);
	glVertex2d(x + dx, y + dy);
	glVertex2d(x, y + dy);

	glEnd();
	glFlush();
}

void render_board(int board[BOARD_X][BOARD_Y])
{
	std::optional<std::tuple<double, double, double>> colors = {};

	for (int x = 0; x < BOARD_X; x++)
	{
		for (int y = 0; y < BOARD_Y; y++)
		{
			switch (board[x][y])
			{
				case GC_blue:
					colors = std::tuple(0, 0, 1);
					break;
				case GC_red:
					colors = std::tuple(1, 0, 0);
					break;
				case GC_light_blue:
					colors = std::tuple(1, 0.3, 0.3);
					break;
				case GC_orange:
					colors = std::tuple(1, 0.6, 0);
					break;
				case GC_yellow:
					colors = std::tuple(1, 1, 0);
					break;
				case GC_green:
					colors = std::tuple(0, 1, 0);
					break;
				case GC_purple:
					colors = std::tuple(0.4, 0.1, 0.4);
					break;
				default:
					colors.reset();
					break;
			}

			if (colors)
			{
				draw_square(index_to_pos(x, BOARD_X), index_to_pos(y, BOARD_Y),
					    2.0 / BOARD_X, 2.0 / BOARD_Y, colors.value());
			}
		}
	}
}

int rand()
{
	FILE* fp = std::fopen("/dev/urandom", "r");

	char buffer[4];
	std::fgets(buffer, 4, fp);

	std::fclose(fp);

	return *((int*) buffer);
}

void check_and_create_block()
{
	if (!current_block)
	{
		current_block = (TetrisBlock){
		    .x = 5,
		    .y = 23,
		    .rotation = {1, 0},
		    .block_type = rand() % 7,
		};
	}
}

void render_block_to_buffer()
{
	std::memset((int*) block_buffer, GC_empty, sizeof(int) * BOARD_X * BOARD_Y);

	if (current_block)
	{
		int x = current_block->x;
		int y = current_block->y;

		std::complex<int>* block_cubes = blocks[current_block->block_type];
		for (int i = 0; i < 4; i++)
		{
			std::complex<int> block_pos = block_cubes[i] * current_block->rotation;

			if (y + block_pos.imag() < BOARD_Y)
				block_buffer[x + block_pos.real()][y + block_pos.imag()] =
				    block_type_to_color(current_block->block_type);
		}
	}
}

void move_block()
{
	if (block_move_counter < BLOCK_MOVE_DELTA)
	{
		block_move_counter++;
		return;
	}
	else
	{
		block_move_counter = 0;
	}

	if (block_is_grounded())
		freeze_block();

	if (current_block && !fast_fall)
	{
		current_block->y--;
	}
}

void freeze_block()
{
	if (current_block)
	{
		auto positions = current_block->get_squares_pos();

		for (auto pos : positions)
		{
			game_board[pos.real()][pos.imag()] =
			    block_type_to_color(current_block->block_type);
		}

		current_block.reset();
	}
}

bool block_is_grounded()
{
	if (current_block)
	{
		auto positions = current_block->get_squares_pos();

		for (auto pos : positions)
		{
			if (pos.imag() == 0 || game_board[pos.real()][pos.imag() - 1] != GC_empty)
			{
				// freeze_block();
				return true;
			}
		}
	}

	return false;
}

bool check_row(int row)
{
	for (int x = 0; x < BOARD_X; x++)
		if (game_board[x][row] == GC_empty)
			return false;
	return true;
}

void clear_and_move(int row)
{
	for (int y = row + 1; y < BOARD_Y; y++)
	{
		for (int x = 0; x < BOARD_X; x++)
		{
			game_board[x][y - 1] = game_board[x][y];
		}
	}
}

void check_rows()
{
	for (int row = 0; row < BOARD_Y; row++)
	{
		if (check_row(row))
			clear_and_move(row);
	}
}

void draw_cutoff_line()
{
	glColor3d(1, 0, 0);
	glBegin(GL_LINES);

	glVertex2d(-1, 0.66);
	glVertex2d(1, 0.66);

	glEnd();
	glFlush();
}

void mainloop()
{
	handle_input();
	move_block();
	check_rows();

	check_and_create_block();
	render_block_to_buffer();

	glClear(GL_COLOR_BUFFER_BIT);
	render_board(game_board);
	render_board(block_buffer);
	draw_cutoff_line();

	glfwSwapBuffers(window);
	glfwPollEvents();
}

int main()
{
	glfwInit();
	window = glfwCreateWindow(300, 720, "Tetris", NULL, NULL);

	glfwMakeContextCurrent(window);
	glewInit();

	init_key_cooldown();

	struct timeval start, end;
	gettimeofday(&start, NULL);
	int delta = 0;

	while (!glfwWindowShouldClose(window))
	{
		gettimeofday(&end, NULL);
		delta += (end.tv_sec - start.tv_sec) * 1'000'000 + (end.tv_usec - start.tv_usec);
		start = end;

		// 1,000,000 / 60 = 16,666 -> Time delta for 60 FPS
		if (delta > 16666)
		{
			mainloop();
			delta -= 16666;
		}
	}

	glfwDestroyWindow(window);
}
