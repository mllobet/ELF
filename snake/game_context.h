/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

//File: game_context.h

#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include "snake_game.h"
#include "elf/pybind_interface.h"

class GameContext {
  public:
    using GC = Context;

  private:
    std::unique_ptr<GC> _context;
    std::vector<SnakeGame> games;

    int _hist_len;
    int _width, _height, _num_action;

  public:
    GameContext(const ContextOptions& context_options, const GameOptions& options) {
      _context.reset(new GC{context_options, options});
      _hist_len = options.hist_len;

      for (int i = 0; i < context_options.num_games; ++i) {
        games.emplace_back(options);
        if (i == 0) {
          auto& game = games.back();
          _width = game.width(), _height = game.height(), _num_action = game.num_actions();
          std::cout << "Action set: ";
          for (const auto &a : game.action_set()) {
              std::cout << a << " ";
          }
          std::cout << std::endl;
          // print more logs for the first game instance
          // ale::Logger::setMode(ale::Logger::mode::Error);
        }
      }
    }

    void Start() {
        auto f = [this](int game_idx, const ContextOptions &context_options, const GameOptions&,
                const elf::Signal& signal, GC::Comm* comm) {
            GC::AIComm ai_comm(game_idx, comm);
            auto &state = ai_comm.info().data;
            state.InitHist(context_options.T);
            for (auto &s : state.v()) {
                s.Init(game_idx, _num_action);
            }
            auto& game = games[game_idx];
            game.initialize_comm(game_idx, &ai_comm);
            game.MainLoop(signal.done());
        };
        _context->Start(f);
    }

    std::map<std::string, int> GetParams() const {
        return std::map<std::string, int>{
          { "width", _width },
          { "height", _height },
          { "num_action", _num_action },
          { "feature_width", kWidth },
          { "feature_height", kHeight },
        };
    }

    EntryInfo EntryFunc(const std::string &key) {
        auto *mm = GameState::get_mm(key);
        if (mm == nullptr) return EntryInfo();

        std::string type_name = mm->type();

        if (key == "s") return EntryInfo(key, type_name, {_hist_len, kHeight, kWidth});
        else if (key == "last_r" || key == "last_terminal" || key == "id" || key == "seq" || key == "game_counter") return EntryInfo(key, type_name);
        else if (key == "pi") return EntryInfo(key, type_name, {_num_action});
        else if (key == "a" || key == "rv" || key == "V") return EntryInfo(key, type_name);

        return EntryInfo();
    }

    CONTEXT_CALLS(GC, _context);

    void Stop() {
      _context.reset(nullptr); // first stop the threads, then destroy the games
      SnakeGameSummary summary;
      for (const auto& game : games) {
          summary.Feed(game.summary());
      }
      std::cout << "Overall reward per game: ";
      summary.Print();
    }
};
