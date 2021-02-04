#include <vector>
#include <complex>

#pragma once

enum GameColors
{
	GC_empty,
	GC_blue,
	GC_red,
	GC_light_blue,
	GC_orange,
	GC_yellow,
	GC_green,
	GC_purple
};

enum BlockType
{
	BT_i,
	BT_j,
	BT_l,
	BT_o,
	BT_s,
	BT_t,
	BT_z,
};

typedef struct TetrisBlock
{
	int x;
	int y;
	std::complex<int> rotation;
	int block_type;

	void move_left();
	void move_right();
	void rotate();
	std::vector<std::complex<int>> get_squares_pos();
} TetrisBlock;

void mainloop();
void freeze_block();
void move_block();
void render_block_to_buffer();
void check_and_create_block();
int rand();
void render_board(int board[BOARD_X][BOARD_Y]);
bool block_is_grounded();
void handle_input();
void init_key_cooldown();

inline float index_to_pos(float index, float size) { return index / size * 2 - 1; }

inline int block_type_to_color(int block_type)
{
	switch (block_type)
	{
		case BT_i:
			return GC_light_blue;
		case BT_j:
			return GC_blue;
		case BT_l:
			return GC_orange;
		case BT_o:
			return GC_yellow;
		case BT_s:
			return GC_green;
		case BT_t:
			return GC_purple;
		case BT_z:
			return GC_red;
	}

	std::cout << block_type;
	return GC_empty;
}
