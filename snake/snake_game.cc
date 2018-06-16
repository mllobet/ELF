//File: snake_game.cc


#include "snake_game.h"
#include <mutex>
#include <cmath>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

static long compute_seed(int v) {
  auto now = system_clock::now();
  auto now_ms = time_point_cast<milliseconds>(now);
  auto value = now_ms.time_since_epoch();
  long duration = value.count();
  long seed = (time(NULL) * 1000 + duration) % 100000000 + v;
  return seed;
}

void SnakeGameSummary::Feed(reward_t last_reward) {
  _accu_reward += last_reward;
}

void SnakeGameSummary::Feed(const SnakeGameSummary &other) {
  _accu_reward_all_game += other._accu_reward_all_game;
  _n_complete_game += other._n_complete_game;
}

void SnakeGameSummary::OnEnd() {
  _accu_reward_all_game += _accu_reward;
  _accu_reward = 0;
  _n_complete_game ++;
}

void SnakeGameSummary::Print() const {
    if (_n_complete_game > 0) {
       std::cout << (float)_accu_reward_all_game / _n_complete_game << "[" << _n_complete_game << "]";
    } else {
       std::cout << "0[0]";
    }
    std::cout << " current accumulated reward: " << _accu_reward << std::endl;
}

SnakeGame::SnakeGame(const GameOptions& opt)
  : _h(opt.hist_len), _reward_clip(opt.reward_clip), _eval_only(opt.eval_only) {
  // lock_guard<mutex> lg(ALE_GLOBAL_LOCK);
  _snake.reset(new SnakeGameEngine(10,10,1));
  long seed = compute_seed(opt.seed);
  // _snake->set_seed(int(seed));
  _snake->set_seed(int(seed));
  // TODO: _snake->setInt("frame_skip", opt.frame_skip);

  // TODO: get state, refactor board access
  auto& s = _snake->get_state();
  _width = s[0].size(), _height = s.size();
  _action_set = _snake->get_minimal_action_set();
  _distr_action.reset(new std::uniform_int_distribution<>(0, _action_set.size() - 1));
  _buf.resize(kBufSize, 0);
}

void SnakeGame::MainLoop(const std::atomic_bool& done) {
  assert(_game_idx >= 0);  // init_comm has been called

  // Add some random actions.
  long seed = compute_seed(_game_idx);
  std::default_random_engine g;
  g.seed(seed);
  std::uniform_int_distribution<int> distr_start_loc(0, 0);
  std::uniform_int_distribution<int> distr_frame_skip(1, 1);
  while (true) {
    _snake->reset();
    _last_reward = 0;
    int start_loc = distr_start_loc(g);

    for (int i = 0;;i ++) {
      if (done.load()) {
        return;
      }
      auto& gs = _ai_comm->Prepare();
      _fill_state(gs);
      // std::cout << "State Filled" << std::endl;

      int act;
      if (i < start_loc) {
          act = (*_distr_action)(g);
          gs.a = act;
          gs.V = 0;
      } else {
          _ai_comm->SendDataWaitReply();
          act = gs.a;
      }

      // Illegal action.
      // std::cout << "[" << _game_idx << "][" << gs.seq.game_counter << "][" << gs.seq.seq << "] act: "
      //          << act << "[a=" << gs.reply.action << "][V=" << gs.reply.value << "]" << std::endl;
      if (act < 0 || act >= (int)_action_set.size() || _snake->game_over()) break;
      if (_eval_only) {
          act = _prevent_stuck(g, act);
          gs.a = act;
      }
      int frame_skip = distr_frame_skip(g);
      _last_reward = 0;
      for (int j = 0; j < frame_skip; ++j) {
          _last_reward += _snake->move(vector<Action>(1,_action_set.at(act)))[0];
      }
      _summary.Feed(_last_reward);
    }
    _ai_comm->Restart();
    _reset_stuck_state();
    _summary.OnEnd();
    // std::cout << "Main Loop End" << std::endl;
  }
}

int SnakeGame::_prevent_stuck(std::default_random_engine &g, int act) {
  if (act == _last_act) {
    _last_act_count ++;
    if (_last_act_count >= kMaxRep) {
      // The player might get stuck. Save it.
      act = (*_distr_action)(g);
    }
  } else {
    // Reset counter.
    _last_act = act;
    _last_act_count = 0;
  }
  return act;
}

void SnakeGame::_reset_stuck_state() {
  _last_act_count = 0;
  _last_act = -1;
}

void SnakeGame::_copy_screen(GameState &state) {
    _snake->get_state_array(_buf);
    if (_h.full()) _h.Pop();

    const int size = kBufSize;
    // Then copy it to the current state.
    auto &item = _h.ItemPush();
    item.resize(size);

    _h.Push();

    // Then you put all the history state to game state.
    state.s.resize(_h.maxlen() * size);
    for (int i = 0; i < _h.size(); ++i) {
        const auto &v = _h.get_from_push(i);
        std::copy(v.begin(), v.end(), &state.s[i * size]);
    }
    if (_h.size() < _h.maxlen()) {
        const int n_missing = _h.maxlen() - _h.size();
        ::memset(&state.s[_h.size() * size], 0, sizeof(float) * sizeof(n_missing * size));
    }
}

void SnakeGame::_fill_state(GameState& state) {
    state.tick = _snake->get_episode_frame_number();
    state.last_r = _last_reward;
    if (_reward_clip > 0.0) {
        state.last_r = std::max(std::min(state.last_r, _reward_clip), -_reward_clip);
    }
    _copy_screen(state);
}
