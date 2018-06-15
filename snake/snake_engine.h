/**
* Multiplayer Snake Game
*/

#ifndef SNAKE_H
#define SNAKE_H

#include <list>
#include <vector>
#include <string>
#include <iostream>

extern char EMPTY_TILE;
extern char FRUIT_TILE;

extern int FRUIT_POINTS;
extern int FRUIT_EXTRA_LENGTH;

extern int dx[];
extern int dy[];

enum Direction {Stopped, Down, Up, Left, Right};

struct Snake {
  std::list<std::pair<int,int>> body;
  Direction dir;
  int extra_length;
  bool alive;

  Snake() {
    extra_length = 0;
    alive = true;
  }

  Snake(int head_x, int head_y) {
    body.push_front(std::make_pair(head_x,head_y));
    dir = Stopped;
    extra_length = 0;
    alive = true;
  }

  Snake(int head_x, int head_y, Direction d) {
    body.push_front(std::make_pair(head_x,head_y));
    dir = Stopped;
    extra_length = 0;
    alive = true;
    dir = d;
  }
};

class SnakeGameEngine {
public:
  SnakeGameEngine(int height, int width, int n_snakes);
  SnakeGameEngine(int height, int width, int n_snakes, int seed);

  void reset();
  void step();
  bool game_over();

  const std::vector<std::vector<char>>& get_state();
  void get_state_array(std::vector<unsigned char> &dest);
  std::vector<float> move(std::vector<Direction> moves);


  void print();
  void set_fruit(int x, int y);
  void set_direction(int snake_idx, Direction dir);
  void set_seed(int seed);

  std::vector<std::vector<char>> get_board();
  std::vector<Direction> get_minimal_action_set();
  int get_episode_frame_number();

  std::string to_string();
private:
  std::vector<std::vector<char>> _board;
  std::vector<Snake> _snakes;
  std::vector<int> _scores;

  std::vector<std::pair<int,int>> corners;

  int _n_snakes;
  int _height, _width;
  int _fruit_x, _fruit_y;

  int _episode_frame_number;

  void add_random_fruit();
  void setup_game();
  void clear_snake(Snake &s);
};

#endif
