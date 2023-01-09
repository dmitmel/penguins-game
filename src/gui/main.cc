#include "gui/main.hh"
#include "gui/new_game_dialog.hh"
#include <cmath>
#include <wx/bitmap.h>
#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/geometry.h>
#include <wx/graphics.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/pen.h>
#include <wx/peninfobase.h>
#include <wx/sizer.h>
#include <wx/types.h>
#include <wx/window.h>

extern "C" {
#include "board.h"
#include "gamestate.h"
#include "movement.h"
#include "random.h"
}

wxIMPLEMENT_APP(PenguinsApp);

PenguinsApp::PenguinsApp() {}

bool PenguinsApp::OnInit() {
  random_init();

  this->game_frame = new GameFrame(nullptr, wxID_ANY, this->game_state);
  this->game_frame->Centre();
  this->game_frame->Show();

  this->Yield(true);
  this->game_frame->start_new_game();

  return true;
}

enum {
  ID_NEW_GAME = wxID_HIGHEST + 1,
};

// clang-format off
wxBEGIN_EVENT_TABLE(GameFrame, wxFrame)
  EVT_MENU(ID_NEW_GAME, GameFrame::on_new_game)
  EVT_MENU(wxID_ABOUT, GameFrame::on_about)
  EVT_MENU(wxID_EXIT, GameFrame::on_exit)
wxEND_EVENT_TABLE();
// clang-format on

GameFrame::GameFrame(wxWindow* parent, wxWindowID id, GuiGameState& state)
: wxFrame(parent, id, "Penguins game"), state(state) {
  auto menu_file = new wxMenu();
  menu_file->Append(ID_NEW_GAME, "&New Game...\tCtrl-N", "Start a new game");
  menu_file->AppendSeparator();
  menu_file->Append(wxID_EXIT);

  auto menu_help = new wxMenu();
  menu_help->Append(wxID_ABOUT);

  auto menu_bar = new wxMenuBar();
  menu_bar->Append(menu_file, "&File");
  menu_bar->Append(menu_help, "&Help");

  this->SetMenuBar(menu_bar);

  auto vbox = new wxBoxSizer(wxVERTICAL);
  auto hbox = new wxBoxSizer(wxHORIZONTAL);

  this->canvas_panel = new CanvasPanel(this, wxID_ANY, state);
  vbox->Add(this->canvas_panel, wxSizerFlags(1).Centre().Border());
  hbox->Add(vbox, wxSizerFlags(1).Centre().Border());

  this->SetSizerAndFit(hbox);

  this->CreateStatusBar();
  this->SetStatusText("Welcome to wxWidgets!");
}

void GameFrame::on_new_game(wxCommandEvent& event) {
  this->start_new_game();
}

void GameFrame::on_exit(wxCommandEvent& event) {
  this->Close(true);
}

void GameFrame::on_about(wxCommandEvent& event) {
  wxMessageBox(
    "This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION
  );
}

void GameFrame::start_new_game() {
  std::unique_ptr<NewGameDialog> dialog(new NewGameDialog(this, wxID_ANY));
  int result = dialog->ShowModal();
  if (result != wxID_OK) {
    return;
  }

  state.penguins_per_player = dialog->get_penguins_per_player();
  state.players_count = dialog->get_number_of_players();
  state.player_names.reset(new wxString[state.players_count]);
  for (int i = 0; i < state.players_count; i++) {
    state.player_names[i] = dialog->get_player_name(i);
  }
  state.board.reset(new Board(init_board(dialog->get_board_width(), dialog->get_board_height())));
  state.blocked_cells.reset(new bool[state.board->width * state.board->height]);

  state.game_phase = PHASE_PLACEMENT;
  state.current_player = 0;
  state.penguins_left_to_place = state.players_count * state.penguins_per_player;

  Board* board = state.board.get();
  generate_random_board(board);
  this->canvas_panel->update_blocked_cells();

  this->canvas_panel->mark_board_dirty();
  this->canvas_panel->InvalidateBestSize();
  this->GetSizer()->SetSizeHints(this);
  this->Refresh();
  this->Centre();
}

// clang-format off
wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
  EVT_PAINT(CanvasPanel::on_paint)
  EVT_MOUSE_EVENTS(CanvasPanel::on_any_mouse_event)
wxEND_EVENT_TABLE();
// clang-format on

CanvasPanel::CanvasPanel(wxWindow* parent, wxWindowID id, GuiGameState& state)
: wxPanel(parent, id), state(state) {}

wxSize CanvasPanel::get_board_size() const {
  Board* board = state.board.get();
  return board ? wxSize(board->width, board->height) : wxSize(0, 0);
}

bool CanvasPanel::is_cell_in_bounds(wxPoint cell) const {
  wxSize size = this->get_board_size();
  return 0 <= cell.x && cell.x < size.x && 0 <= cell.y && cell.y < size.y;
}

wxSize CanvasPanel::get_canvas_size() const {
  return CELL_SIZE * this->get_board_size();
}

int* CanvasPanel::cell_ptr(wxPoint cell) const {
  wxASSERT(this->is_cell_in_bounds(cell));
  return &state.board->grid[cell.y][cell.x];
}

wxPoint CanvasPanel::get_cell_by_coords(wxPoint point) const {
  return wxPoint(point.x / CELL_SIZE, point.y / CELL_SIZE);
}

wxRect CanvasPanel::get_cell_rect(wxPoint cell) const {
  return wxRect(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
}

wxPoint CanvasPanel::get_cell_centre(wxPoint cell) const {
  wxRect rect = this->get_cell_rect(cell);
  return rect.GetPosition() + rect.GetSize() / 2;
}

bool CanvasPanel::is_cell_blocked(wxPoint cell) const {
  wxASSERT(this->is_cell_in_bounds(cell));
  return state.blocked_cells[cell.x + cell.y * state.board->width];
}

void CanvasPanel::update_blocked_cells() {
  Board* board = state.board.get();
  if (!board) return;
  if (state.game_phase == PHASE_PLACEMENT) {
    for (int y = 0; y < board->height; y++) {
      for (int x = 0; x < board->width; x++) {
        state.blocked_cells[x + y * board->width] =
          !is_spot_valid_for_placement(board, Coords{ x, y });
      }
    }
  } else if (state.game_phase == PHASE_MOVEMENT) {
    wxPoint curr_cell =
      this->get_cell_by_coords(this->mouse_is_down ? this->mouse_drag_pos : this->mouse_pos);
    if (!this->is_cell_in_bounds(curr_cell)) return;
    if (*this->cell_ptr(curr_cell) < 0) {
      // A penguin is selected
      for (int y = 0; y < board->height; y++) {
        for (int x = 0; x < board->width; x++) {
          MovementError result = this->validate_movement(curr_cell, wxPoint(x, y));
          if (!this->mouse_is_down && result == CURRENT_LOCATION) {
            result = VALID_INPUT;
          }
          state.blocked_cells[x + y * board->width] = result != VALID_INPUT;
        }
      }
    } else {
      for (int y = 0; y < board->height; y++) {
        for (int x = 0; x < board->width; x++) {
          state.blocked_cells[x + y * board->width] =
            board->grid[y][x] != -(state.current_player + 1);
        }
      }
    }
  }
  this->mark_board_dirty();
}

MovementError CanvasPanel::validate_movement(wxPoint start, wxPoint target, wxPoint* fail) {
  return ::validate_movement(
    state.board.get(),
    *reinterpret_cast<Coords*>(&start),
    *reinterpret_cast<Coords*>(&target),
    state.current_player + 1,
    reinterpret_cast<Coords*>(fail)
  );
}

wxSize CanvasPanel::DoGetBestClientSize() const {
  wxSize size = this->get_canvas_size();
  if (!(size.x > 0 && size.y > 0)) {
    return CELL_SIZE * wxSize(20, 20);
  }
  return size;
}

void CanvasPanel::on_paint(wxPaintEvent& event) {
  wxPaintDC win_dc(this);
  wxSize size = this->get_canvas_size();
  if (!(size.x > 0 && size.y > 0)) {
    this->board_dirty = false;
    this->board_bitmap.UnRef();
    return;
  }

  if (!this->board_bitmap.IsOk() || this->board_bitmap.GetSize() != size) {
    this->board_dirty = true;
    this->board_bitmap.Create(size);
  }
  wxMemoryDC mem_dc(this->board_bitmap);
  if (this->board_dirty) {
    this->board_dirty = false;
    mem_dc.SetBackground(*wxWHITE_BRUSH);
    mem_dc.Clear();
    this->paint_board(mem_dc);
  }
  win_dc.Blit(0, 0, size.x, size.y, &mem_dc, 0, 0);

  this->paint_overlay(win_dc);
}

void CanvasPanel::paint_board(wxDC& dc) {
  Board* board = state.board.get();
  if (!board) return;

  dc.SetTextForeground(*wxBLACK);
  const wxFont default_font = dc.GetFont();
  wxFont cell_font(default_font);
  cell_font.MakeBold();
  cell_font.SetPointSize(CELL_FONT_SIZE);
  dc.SetFont(cell_font);

  for (int y = 0; y < board->height; y++) {
    for (int x = 0; x < board->width; x++) {
      wxPoint cell(x, y);
      int cell_value = *this->cell_ptr(cell);
      wxRect cell_rect = this->get_cell_rect(cell);
      wxPoint cell_centre = this->get_cell_centre(cell);

      int lightness = 100;
      if (this->mouse_within_window && this->is_cell_blocked(cell)) {
        lightness += BLOCKED_CELL_LIGHTNESS;
      }
      auto blend_colour = [lightness](const wxColour& colour) {
        return colour.ChangeLightness(lightness);
      };

      dc.SetPen(*wxBLACK);

      if (cell_value > 0) {
        // an ice floe with fish on it
        int fish_count = cell_value;
        dc.SetBrush(blend_colour(*wxWHITE));
        dc.DrawRectangle(cell_rect);

        dc.SetPen(wxNullPen);
        dc.SetBrush(blend_colour((*wxGREY_BRUSH).GetColour()));
        if (fish_count == 1) {
          dc.DrawCircle(cell_centre, FISH_CIRCLE_RADIUS);
        } else {
          for (int i = 0; i < fish_count; i++) {
            float angle = (float)i / fish_count * 2 * M_PI;
            float offset = (float)CELL_SIZE / 5;
            wxPoint centre(
              (float)cell_centre.x + cos(angle) * offset,
              (float)cell_centre.y + sin(angle) * offset
            );
            dc.DrawCircle(centre, FISH_CIRCLE_RADIUS);
          }
        }

      } else if (cell_value < 0) {
        // a penguin
        int player = -cell_value;
        dc.SetBrush(*wxYELLOW);
        dc.DrawRectangle(cell_rect);

        wxString str;
        str << player;
        dc.DrawLabel(str, cell_rect, wxALIGN_CENTRE);

      } else {
        // a water tile
        dc.SetBrush(blend_colour(*wxCYAN));
        dc.DrawRectangle(cell_rect);
      }
    }
  }
}

enum ArrowHeadType {
  ArrowHead_Normal = 1,
  ArrowHead_Cross = 2,
};

static void draw_arrow_head(
  wxDC& dc,
  wxPoint start,
  wxPoint end,
  wxSize head_size,
  ArrowHeadType head_type = ArrowHead_Normal
) {
  if (start == end) return;
  wxPoint2DDouble norm(end - start);
  norm.Normalize();
  wxPoint2DDouble perp(-norm.m_y, norm.m_x);
  wxPoint2DDouble head1 = -norm * head_size.x + perp * head_size.y;
  wxPoint2DDouble head2 = -norm * head_size.x - perp * head_size.y;
  wxPoint head1i(head1.m_x, head1.m_y), head2i(head2.m_x, head2.m_y);
  if (head_type == ArrowHead_Normal) {
    dc.DrawLine(end, end + head1i);
    dc.DrawLine(end, end + head2i);
  } else if (head_type == ArrowHead_Cross) {
    dc.DrawLine(end - head1i, end + head1i);
    dc.DrawLine(end - head2i, end + head2i);
  }
}

void CanvasPanel::paint_overlay(wxDC& dc) {
  wxPoint current_cell = this->get_cell_by_coords(this->mouse_pos);
  if (this->mouse_within_window && this->is_cell_in_bounds(current_cell)) {
    dc.SetBrush(wxNullBrush);
    dc.SetPen(wxPen(this->is_cell_blocked(current_cell) ? *wxRED : *wxGREEN, 5));
    dc.DrawRectangle(this->get_cell_rect(current_cell));
  }

  if (this->mouse_is_down && state.game_phase == PHASE_MOVEMENT) {
    wxPoint start_cell = this->get_cell_by_coords(this->mouse_drag_pos);
    wxPoint dest_cell = this->get_cell_by_coords(this->mouse_pos);
    if (this->is_cell_in_bounds(start_cell) && this->is_cell_in_bounds(dest_cell)) {
      if (start_cell != dest_cell && *this->cell_ptr(start_cell) == -(state.current_player + 1)) {
        wxPoint move_fail_cell(start_cell);
        MovementError result = this->validate_movement(start_cell, dest_cell, &move_fail_cell);
        wxPoint arrow_start = this->get_cell_centre(start_cell);
        wxPoint arrow_fail = this->get_cell_centre(move_fail_cell);
        wxPoint arrow_end = this->get_cell_centre(dest_cell);

        wxSize head_size(8, 8);
        wxPen bg_pen(*wxBLACK, 6);
        wxPen green_pen((*wxGREEN).ChangeLightness(75), 4);
        wxPen red_pen((*wxRED).ChangeLightness(75), 4);
        dc.SetBrush(wxNullBrush);

        if (result != VALID_INPUT && move_fail_cell != start_cell) {
          dc.SetPen(bg_pen);
          dc.DrawLine(arrow_start, arrow_fail);
          dc.SetPen(green_pen);
          dc.DrawLine(arrow_start, arrow_fail);
          dc.SetPen(bg_pen);
          draw_arrow_head(dc, arrow_start, arrow_fail, head_size, ArrowHead_Cross);
          dc.DrawLine(arrow_fail, arrow_end);
          draw_arrow_head(dc, arrow_fail, arrow_end, head_size);
          dc.SetPen(red_pen);
          draw_arrow_head(dc, arrow_start, arrow_fail, head_size, ArrowHead_Cross);
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
    }
  }
}

void CanvasPanel::on_any_mouse_event(wxMouseEvent& event) {
  this->prev_mouse_pos = this->mouse_pos;
  this->mouse_pos = event.GetPosition();

  if (!this->mouse_is_down) {
    this->mouse_drag_pos = this->mouse_pos;
  }
  if (event.ButtonDown()) {
    this->mouse_is_down = true;
    this->mouse_drag_pos = this->mouse_pos;
  } else if (event.ButtonUp()) {
    this->mouse_is_down = false;
  }

  if (event.Entering()) {
    this->mouse_within_window = true;
  } else if (event.Leaving()) {
    this->mouse_within_window = false;
  }

  if (!state.board) {
    event.Skip();
    return;
  }

  // NOTE: a `switch` is unusable here because the event types are defined as
  // `extern` variables. `switch` in C++ can only work with statically-known
  // constants.
  auto event_type = event.GetEventType();
  if (event_type == wxEVT_LEFT_DOWN) {
    this->on_mouse_down(event);
  } else if (event_type == wxEVT_MOTION) {
    this->on_mouse_move(event);
  } else if (event_type == wxEVT_LEFT_UP) {
    this->on_mouse_up(event);
  } else if (event_type == wxEVT_ENTER_WINDOW) {
    this->on_mouse_enter_leave(event);
  } else if (event_type == wxEVT_LEAVE_WINDOW) {
    this->on_mouse_enter_leave(event);
  } else {
    event.Skip();
  }
}

void CanvasPanel::on_mouse_down(wxMouseEvent& event) {
  if (state.game_phase == PHASE_MOVEMENT) {
    this->update_blocked_cells();
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_move(wxMouseEvent& event) {
  wxPoint prev_cell = this->get_cell_by_coords(this->prev_mouse_pos);
  wxPoint curr_cell = this->get_cell_by_coords(this->mouse_pos);
  if (curr_cell != prev_cell) {
    if (this->is_cell_in_bounds(curr_cell)) {
      this->update_blocked_cells();
    }
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_up(wxMouseEvent& event) {
  wxPoint prev_cell = this->get_cell_by_coords(this->mouse_drag_pos);
  wxPoint curr_cell = this->get_cell_by_coords(this->mouse_pos);
  if (state.game_phase == PHASE_PLACEMENT) {
    if (this->is_cell_in_bounds(curr_cell) && prev_cell == curr_cell) {
      if (!this->is_cell_blocked(curr_cell)) {
        *this->cell_ptr(curr_cell) = -(state.current_player + 1);
        this->mark_board_dirty();
        state.current_player = (state.current_player + 1) % state.players_count;
        state.penguins_left_to_place -= 1;
        if (state.penguins_left_to_place <= 0) {
          state.game_phase = PHASE_MOVEMENT;
          state.current_player = 0;
        }
        this->update_blocked_cells();
        this->Refresh();
      }
    }
  } else if (state.game_phase == PHASE_MOVEMENT) {
    if (this->is_cell_in_bounds(curr_cell) && this->is_cell_in_bounds(prev_cell)) {
      MovementError result = validate_movement(prev_cell, curr_cell);
      if (result == VALID_INPUT) {
        move_penguin(
          state.board.get(),
          *reinterpret_cast<Coords*>(&prev_cell),
          *reinterpret_cast<Coords*>(&curr_cell),
          state.current_player + 1
        );
        this->mark_board_dirty();
        state.current_player = (state.current_player + 1) % state.players_count;
      }
    }
    this->update_blocked_cells();
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_enter_leave(wxMouseEvent& event) {
  this->mark_board_dirty();
  this->Refresh();
}
