#pragma once

#include "bot.h"
#include "game.h"
#include <memory>
#include <wx/thread.h>
#include <wx/version.h>

class GameFrame;

class BotThread : public wxThread {
public:
  BotThread(GameFrame* frame);
  ~BotThread();
  void cancel();

  std::shared_ptr<wxSemaphore> exit_semaphore{ new wxSemaphore() };

protected:
  virtual void OnExit() override;

#if !wxCHECK_VERSION(3, 1, 6)
  // Stub for when this method is not available.
  bool SetName(const wxString& WXUNUSED(name)) {
    return false;
  }
#endif

  GameFrame* frame;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::shared_ptr<BotParameters> bot_params{ nullptr };
  std::unique_ptr<BotState, decltype(&bot_state_free)> bot_state{ nullptr, bot_state_free };
  volatile bool* cancelled_ptr = nullptr;
};

class BotPlacementThread : public BotThread {
public:
  BotPlacementThread(GameFrame* frame) : BotThread(frame) {}

protected:
  virtual ExitCode Entry() override;
};

class BotMovementThread : public BotThread {
public:
  BotMovementThread(GameFrame* frame) : BotThread(frame) {}

protected:
  virtual ExitCode Entry() override;
};
