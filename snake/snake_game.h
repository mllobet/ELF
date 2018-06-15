//File: snake_game.h

#pragma once

#include <vector>
// #include <ale/ale_interface.hpp>
#include <string>

#include "elf/pybind_helper.h"
#include "elf/comm_template.h"
#include "elf/ai_comm.h"
#include "snake_game_specific.h"
#include "snake_engine.h"

class SnakeGameSummary {
private:
    reward_t _accu_reward = 0;
    reward_t _accu_reward_all_game = 0;
    int _n_complete_game = 0;
public:
    void Feed(reward_t curr_reward);
    void Feed(const SnakeGameSummary &);
    void OnEnd();
    void Print() const;
};

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

class SnakeGame {
  private:
    int _game_idx = -1;
    AIComm *_ai_comm;
    std::unique_ptr<SnakeGameEngine> _snake;

    int _width, _height;
    std::vector<Direction> _action_set;

    // Used to dump the current frame.
    // h * w * (not 3 [RGB] may change to one hot in the futre?)
    // 8 * 8 * 1
    // We also save history here.
    std::vector<unsigned char> _buf;
    CircularQueue<std::vector<unsigned char>> _h;

    float _reward_clip;
    bool _eval_only;

    float _last_reward = 0;
    SnakeGameSummary _summary;

    static const int kMaxRep = 30;
    int _last_act_count = 0;
    int _last_act = -1;

    std::unique_ptr<std::uniform_int_distribution<>> _distr_action;

    int _prevent_stuck(std::default_random_engine &g, int act);
    void _reset_stuck_state();

    void _fill_state(GameState&);
    void _copy_screen(GameState &);

  public:
    SnakeGame(const GameOptions&);

    void initialize_comm(int game_idx, AIComm* ai_comm) {
      _ai_comm = ai_comm;
      _game_idx = game_idx;
    }

    void MainLoop(const std::atomic_bool& done);

    int num_actions() const { return _action_set.size(); }
    const std::vector<Direction>& action_set() const { return _action_set; }
    int width() const { return _width; }
    int height() const { return _height; }
    const SnakeGameSummary &summary() const { return _summary; }
};
