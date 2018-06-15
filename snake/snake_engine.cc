/**
* Multiplayer Snake Game
*/

#include "snake_engine.h"
#include "stdlib.h"

#include <numeric>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <cassert>

char EMPTY_TILE = '-';
char FRUIT_TILE = '#';

int FRUIT_POINTS = 1;
int FRUIT_EXTRA_LENGTH = 3;

int dx[] = {0,0,0,-1,1};
int dy[] = {0,1,-1,0,0};

int myrandom (int i) { return rand()%i;}

SnakeGameEngine::SnakeGameEngine(int height, int width, int n_snakes) {
  _height = height;
  _width = width;
  _n_snakes = n_snakes;
  _episode_frame_number = 0;

  srand(time(0));

  setup_game();
}

SnakeGameEngine::SnakeGameEngine(int height, int width, int n_snakes, int seed) {
  _height = height;
  _width = width;
  _n_snakes = n_snakes;
  _episode_frame_number = 0;

  srand(seed);

  setup_game();
}

void SnakeGameEngine::setup_game() {
  _board = std::vector<std::vector<char>>(_height, std::vector<char>(_width, EMPTY_TILE));
  _snakes = std::vector<Snake>(_n_snakes);
  _scores = std::vector<int>(_n_snakes, 0);

  // select random snake positions and store on board
  std::vector<unsigned int> indices(_height*_width);
  std::iota(indices.begin(), indices.end(), 0);
  std::random_shuffle(indices.begin(), indices.end(), myrandom);

  assert(_n_snakes <= _height*_width);
  for (int i = 0; i < _n_snakes; ++i){
    int x = indices[i]%_width;
    int y = int(indices[i]/_width);

    _snakes[i] = Snake(x,y);
    _board[y][x] = char('0' + 2*i);
  }

  // add random fruit
  add_random_fruit();
}

void SnakeGameEngine::add_random_fruit(){
  // find size of empty_positions
  int size = 0;
  for (Snake &s : _snakes) {
    size += s.body.size();
  }
  std::vector<int> empty_positions(_height*_width - size);

  // store empty positions
  int pos = 0;
  for (int i = 0; i < _height; ++i) {
    for (int j = 0; j < _width; ++j) {
      if (_board[i][j] == EMPTY_TILE){
        empty_positions[pos] = i*_width + j;
        ++pos;
      }
    }
  }

  // select random fruit position and add to board
  int fruit_pos = empty_positions[rand() % empty_positions.size() - 1];
  _fruit_x = fruit_pos%_width;
  _fruit_y = int(fruit_pos/_width);
  _board[_fruit_y][_fruit_x] = FRUIT_TILE;
}

void SnakeGameEngine::set_fruit(int x, int y) {
  _board[_fruit_y][_fruit_x] = EMPTY_TILE;
  _fruit_y = y;
  _fruit_x = x;
  _board[_fruit_y][_fruit_x] = FRUIT_TILE;
}

void SnakeGameEngine::set_direction(int snake_idx, Direction dir) {
  assert(snake_idx < _n_snakes);
  if (_snakes[snake_idx].alive != true) return;
  _snakes[snake_idx].dir = dir;
}

void SnakeGameEngine::reset(){
  setup_game();
}

std::vector<std::vector<char>> SnakeGameEngine::get_board() {
  return _board;
}

std::vector<Direction> SnakeGameEngine::get_minimal_action_set(){
  std::vector<Direction> dirs = {Up, Down, Right, Left};
  return dirs;
}

const std::vector<std::vector<char>>& SnakeGameEngine::get_state(){
  return _board;
}

// Returns board in flattened format
void SnakeGameEngine::get_state_array(std::vector<unsigned char> &dest){
  assert(dest.size() == _height*_width);
  int i = 0;
  for (auto &row : _board) {
    for (auto col : row) {
      dest[i++] = col;
    }
  }
}

// todo fix
std::vector<float> SnakeGameEngine::move(std::vector<Direction> moves){
  assert(moves.size() == _snakes.size());
  for (int i = 0; i < int(_snakes.size()); ++i) {
    set_direction(i, moves[i]);
  }
  step();
  std::vector<float> rewards = {1.0};
  return rewards;
}

void SnakeGameEngine::step(){
  // move each snake 1 step per direction
  for (int snake_idx = 0; snake_idx < _snakes.size(); ++snake_idx) {
    Snake &s = _snakes[snake_idx];
    if (not s.alive) continue;
    if (s.dir == Stopped) continue;

    // get new front coordinates
    int head_x = s.body.front().first + dx[s.dir];
    int head_y = s.body.front().second + dy[s.dir];

    // check out of bounds
    if ((head_x < 0 or head_x >= _width) or (head_y < 0 or head_y >= _height)) {
      clear_snake(s);
      continue;
    }

    // check collision
    if ((_board[head_y][head_x] != EMPTY_TILE) and (_board[head_y][head_x] != FRUIT_TILE)) {
      clear_snake(s);
      continue;
    }

    // ingest fruit
    bool fruit_eaten = false;
    if (_board[head_y][head_x] == FRUIT_TILE) {
      _scores[snake_idx] += FRUIT_POINTS;
      s.extra_length += FRUIT_EXTRA_LENGTH;
      fruit_eaten = true;
    }

    // add head to front
    s.body.push_front(std::make_pair(head_x, head_y));

    // pop back
    if (s.extra_length != 0) {
      --s.extra_length;
    } else {
      // unpaint last
      std::pair<int,int> tail = s.body.back();
      _board[tail.second][tail.first] = EMPTY_TILE;
      s.body.pop_back();
    }

    // paint new head
    std::pair<int,int> head = s.body.front();
    _board[head.second][head.first] = '0' + snake_idx*2;

    // turn previous head into body
    const auto &it = std::next(s.body.begin());
    if (it != s.body.end()) {
      _board[it->second][it->first] = '0' + snake_idx*2 + 1;
    }

    // add new fruit if eaten
    if (fruit_eaten) {
      add_random_fruit();
    }
  }
  ++_episode_frame_number;
}

void SnakeGameEngine::clear_snake(Snake &s){
  // clear body from _board
  for (auto &part : s.body) {
    _board[part.second][part.first] = EMPTY_TILE;
  }
  s.body.clear();
  s.alive = false;
}

void SnakeGameEngine::print(){
  for (auto &row : _board) {
    for (auto &col : row) {
      std::cout << col;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

bool SnakeGameEngine::game_over(){
  return _snakes[0].alive;
}

std::string SnakeGameEngine::to_string(){
  std::string s = "";
  for (auto &row : _board) {
    for (auto &col : row) {
      s += col;
    }
    s += "\n";
  }
  s += "\n";
  return s;
}

void SnakeGameEngine::set_seed(int seed) {
  srand(seed);
}

int SnakeGameEngine::get_episode_frame_number(){
  return _episode_frame_number;
}
