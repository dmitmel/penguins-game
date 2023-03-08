#include "gui/bot_thread.hh"
#include "gui/controllers.hh"
#include "gui/game_state.hh"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <wx/debug.h>
#include <wx/thread.h>

void BotThreadShared::wait_for_exit() {
  wxMutexLocker lock(this->mutex);
  while (!this->exited) {
    wxCondError code WX_ATTRIBUTE_UNUSED = this->condvar.Wait();
    wxASSERT(code == wxCOND_NO_ERROR);
  }
}

void BotThreadShared::notify_exit() {
  wxMutexLocker lock(this->mutex);
  wxASSERT(!this->exited);
  this->exited = true;
  this->condvar.Broadcast();
}

BotThread::BotThread(BotTurnController* controller)
: wxThread(wxTHREAD_DETACHED), controller(controller), bot_params(controller->state.bot_params) {
  this->game.reset(game_clone(controller->state.game.get()));
  this->bot_state.reset(bot_state_new(this->bot_params.get(), this->game.get()));
  this->cancelled_ptr = &this->bot_state->cancelled;
}

BotThread::~BotThread() {
  this->cancelled_ptr = nullptr;
}

void BotThread::cancel() {
  *this->cancelled_ptr = true;
}

void BotThread::OnExit() {
  // The thread is unregistered in the OnExit method instead of the destructor
  // so as to not accidentally use a half-destroyed object. And also because
  // joinable threads don't destruct themselves automatically.
  this->controller->unregister_bot_thread(this);
  this->shared->notify_exit();
}

wxThread::ExitCode BotPlacementThread::Entry() {
  this->SetName("bot-placement");
  Coords target;
  bool ok = bot_make_placement(this->bot_state.get(), &target);
  bool cancelled = this->bot_state->cancelled;
  auto controller = this->controller;
  auto shared = this->shared;
  controller->CallAfter([=]() -> void {
    shared->wait_for_exit();
    if (!cancelled && ok) place_penguin(controller->game, target);
    controller->on_bot_thread_done_work(cancelled);
  });
  return 0;
}

wxThread::ExitCode BotMovementThread::Entry() {
  this->SetName("bot-movement");
  Coords penguin, target;
  bool ok = bot_make_move(this->bot_state.get(), &penguin, &target);
  bool cancelled = this->bot_state->cancelled;
  auto controller = this->controller;
  auto shared = this->shared;
  controller->CallAfter([=]() -> void {
    shared->wait_for_exit();
    if (!cancelled && ok) move_penguin(controller->game, penguin, target);
    controller->on_bot_thread_done_work(cancelled);
  });
  return 0;
}
