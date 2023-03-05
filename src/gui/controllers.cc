#include "gui/controllers.hh"
#include "board.h"
#include "gui/bot_thread.hh"
#include "gui/canvas.hh"
#include "gui/game.hh"
#include "gui/game_state.hh"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <memory>
#include <wx/colour.h>
#include <wx/debug.h>
#include <wx/gdicmn.h>
#include <wx/geometry.h>
#include <wx/pen.h>
#include <wx/types.h>

GameController::GameController(GameFrame* game_frame)
: game_frame(game_frame)
, canvas(game_frame->canvas)
, state(game_frame->state)
, game(state.game.get()) {}

void GameController::update_game_state_and_indirectly_delete_this() {
  this->game_frame->update_game_state();
}

void GameController::on_activated() {
  this->game_frame->stop_bot_progress();
}

void BotTurnController::on_activated() {
  this->game_frame->start_bot_progress();
}

void BotPlacementController::on_activated() {
  this->BotTurnController::on_activated();
  wxASSERT(!this->executing_bot_turn);
  this->executing_bot_turn = true;
  this->run_bot_thread(new BotPlacementThread(this));
}

void BotMovementController::on_activated() {
  this->BotTurnController::on_activated();
  wxASSERT(!this->executing_bot_turn);
  this->executing_bot_turn = true;
  this->run_bot_thread(new BotMovementThread(this));
}

BotTurnController::~BotTurnController() {
  this->stop_bot_thread();
  this->executing_bot_turn = false;
}

void BotTurnController::on_bot_thread_done_work(bool cancelled) {
  this->executing_bot_turn = false;
  if (!cancelled) {
    this->update_game_state_and_indirectly_delete_this();
  } else {
    this->game_frame->stop_bot_progress();
  }
}

void BotTurnController::run_bot_thread(BotThread* thread) {
  wxASSERT(wxThread::IsMain());
  wxCriticalSectionLocker enter(this->bot_thread_cs);
  wxASSERT(this->bot_thread == nullptr);
  this->bot_thread = thread;
  wxThreadError code WX_ATTRIBUTE_UNUSED = thread->Run();
  wxASSERT(code == wxTHREAD_NO_ERROR);
}

void BotTurnController::stop_bot_thread() {
  wxASSERT(wxThread::IsMain());
  // Well, this ain't the best way of doing thread synchronization, but it sure
  // does work.
  std::shared_ptr<BotThreadShared> shared = nullptr;
  {
    wxCriticalSectionLocker enter(this->bot_thread_cs);
    if (this->bot_thread) {
      this->bot_thread->cancel();
      shared = this->bot_thread->shared;
      this->bot_thread = nullptr;
    }
  }
  if (shared) {
    shared->wait_for_exit();
  }
}

void BotTurnController::unregister_bot_thread(BotThread* thread) {
  wxASSERT(wxThread::GetCurrentId() == thread->GetId());
  wxCriticalSectionLocker enter(this->bot_thread_cs);
  // Another bot thread might have just been spun up after cancelling this one,
  // so check that we are unregistering the correct one to be sure.
  if (this->bot_thread == thread) {
    this->bot_thread = nullptr;
  }
}

void GameController::update_tile_attributes() {
  this->canvas->set_all_tiles_attr(TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
}

void PlayerPlacementController::update_tile_attributes() {
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  bool is_a_tile_selected =
    this->canvas->mouse_within_window && is_tile_in_bounds(game, curr_coords);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      bool blocked = !validate_placement_simple(game, coords);
      this->canvas->set_tile_attr(coords, TILE_BLOCKED_FOR_CURSOR, blocked);
      this->canvas->set_tile_attr(coords, TILE_BLOCKED, blocked && is_a_tile_selected);
    }
  }
}

void PlayerMovementController::update_tile_attributes() {
  Coords selected_penguin = this->canvas->get_selected_penguin_coords();
  if (!is_tile_in_bounds(game, selected_penguin)) {
    int current_player_id = game_get_current_player(game)->id;
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        int tile = get_tile(game, coords);
        bool blocked = get_tile_player_id(tile) != current_player_id;
        this->canvas->set_tile_attr(coords, TILE_BLOCKED_FOR_CURSOR, blocked);
        this->canvas->set_tile_attr(coords, TILE_BLOCKED, false);
      }
    }
    return;
  }
  // A penguin is selected
  this->canvas->set_all_tiles_attr(TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, true);
  PossibleSteps moves = calculate_penguin_possible_moves(game, selected_penguin);
  bool any_steps = false;
  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords coords = selected_penguin;
    Coords d = DIRECTION_TO_COORDS[dir];
    any_steps = any_steps || moves.steps[dir] != 0;
    for (int steps = moves.steps[dir]; steps > 0; steps--) {
      coords.x += d.x, coords.y += d.y;
      this->canvas->set_tile_attr(coords, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
    }
  }
  this->canvas->set_tile_attr(selected_penguin, TILE_BLOCKED, false);
  if (any_steps && !this->canvas->mouse_is_down) {
    this->canvas->set_tile_attr(selected_penguin, TILE_BLOCKED_FOR_CURSOR, false);
  }
}

void BotTurnController::update_tile_attributes() {
  this->canvas->set_all_tiles_attr(TILE_BLOCKED, false);
  this->canvas->set_all_tiles_attr(TILE_BLOCKED_FOR_CURSOR, true);
}

void PlayerTurnController::paint_overlay(wxDC& dc) {
  Coords current_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, current_coords) && this->canvas->mouse_within_window) {
    wxByte tile_attrs = *this->canvas->tile_attrs_ptr(current_coords);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen((tile_attrs & TILE_BLOCKED_FOR_CURSOR) != 0 ? *wxRED : *wxGREEN, 5));
    dc.DrawRectangle(this->canvas->get_tile_rect(current_coords));
  }
}

enum ArrowHeadType {
  ARROW_HEAD_NORMAL = 1,
  ARROW_HEAD_CROSS = 2,
};

static void draw_arrow_head(
  wxDC& dc,
  wxPoint start,
  wxPoint end,
  wxSize head_size,
  ArrowHeadType head_type = ARROW_HEAD_NORMAL
) {
  if (start == end) return;
  wxPoint2DDouble norm(end - start);
  norm.Normalize();
  wxPoint2DDouble perp(-norm.m_y, norm.m_x);
  wxPoint2DDouble head1 = -norm * head_size.x + perp * head_size.y;
  wxPoint2DDouble head2 = -norm * head_size.x - perp * head_size.y;
  wxPoint head1i(head1.m_x, head1.m_y), head2i(head2.m_x, head2.m_y);
  if (head_type == ARROW_HEAD_NORMAL) {
    dc.DrawLine(end, end + head1i);
    dc.DrawLine(end, end + head2i);
  } else if (head_type == ARROW_HEAD_CROSS) {
    dc.DrawLine(end - head1i, end + head1i);
    dc.DrawLine(end - head2i, end + head2i);
  }
}

void PlayerMovementController::paint_overlay(wxDC& dc) {
  this->PlayerTurnController::paint_overlay(dc);

  if (!this->canvas->mouse_is_down) return;
  Coords penguin = this->canvas->get_selected_penguin_coords();
  Coords target = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (!(is_tile_in_bounds(game, penguin) && is_tile_in_bounds(game, target))) return;
  if (coords_same(penguin, target)) return;

  Coords move_fail = penguin;
  MovementError result = validate_movement(game, penguin, target, &move_fail);
  wxPoint arrow_start = this->canvas->get_tile_centre(penguin);
  wxPoint arrow_fail = this->canvas->get_tile_centre(move_fail);
  wxPoint arrow_end = this->canvas->get_tile_centre(target);

  wxSize head_size(8, 8);
  wxPen bg_pen(*wxBLACK, 6);
  wxPen green_pen((*wxGREEN).ChangeLightness(75), 4);
  wxPen red_pen((*wxRED).ChangeLightness(75), 4);

  if (result != VALID_INPUT && !coords_same(move_fail, penguin)) {
    dc.SetPen(bg_pen);
    dc.DrawLine(arrow_start, arrow_fail);
    dc.SetPen(green_pen);
    dc.DrawLine(arrow_start, arrow_fail);
    dc.SetPen(bg_pen);
    draw_arrow_head(dc, arrow_start, arrow_fail, head_size, ARROW_HEAD_CROSS);
    dc.DrawLine(arrow_fail, arrow_end);
    draw_arrow_head(dc, arrow_fail, arrow_end, head_size);
    dc.SetPen(red_pen);
    draw_arrow_head(dc, arrow_start, arrow_fail, head_size, ARROW_HEAD_CROSS);
    dc.DrawLine(arrow_fail, arrow_end);
    draw_arrow_head(dc, arrow_fail, arrow_end, head_size);
  } else {
    dc.SetPen(bg_pen);
    dc.DrawLine(arrow_start, arrow_end);
    draw_arrow_head(dc, arrow_start, arrow_end, head_size);
    dc.SetPen(result == VALID_INPUT ? green_pen : red_pen);
    dc.DrawLine(arrow_start, arrow_end);
    draw_arrow_head(dc, arrow_start, arrow_end, head_size);
  }
}

void PlayerMovementController::on_mouse_down(wxMouseEvent& WXUNUSED(event)) {
  this->canvas->Refresh();
}

void PlayerPlacementController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->prev_mouse_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (coords_same(curr_coords, prev_coords)) return;
  this->canvas->Refresh();
}

void PlayerMovementController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->prev_mouse_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (coords_same(curr_coords, prev_coords)) return;
  Coords selected_penguin = this->canvas->get_selected_penguin_coords();
  if (is_tile_in_bounds(game, selected_penguin)) {
    this->canvas->set_tile_attr(selected_penguin, TILE_DIRTY, true);
  }
  this->canvas->Refresh();
}

void PlayerPlacementController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, curr_coords) && coords_same(prev_coords, curr_coords)) {
    if (validate_placement(game, curr_coords) == PLACEMENT_VALID) {
      this->game_frame->place_penguin(curr_coords);
      this->update_game_state_and_indirectly_delete_this();
      return;
    }
  }
}

void PlayerMovementController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, curr_coords) && is_tile_in_bounds(game, prev_coords)) {
    if (coords_same(prev_coords, curr_coords) && validate_movement_start(game, curr_coords)) {
      // This is a hacky way of doing what I want: when the user has simply
      // clicked on a penguin, let them then move it without dragging the mouse
      // all the way.
      this->canvas->mouse_is_down = true;
    } else if (validate_movement(game, prev_coords, curr_coords, nullptr) == VALID_INPUT) {
      this->game_frame->move_penguin(prev_coords, curr_coords);
      this->update_game_state_and_indirectly_delete_this();
      return;
    }
  }
  this->canvas->Refresh();
}

void BotTurnController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  if (!this->executing_bot_turn) {
    this->on_activated();
  }
}
