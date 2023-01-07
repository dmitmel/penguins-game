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
#include <wx/graphics.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/pen.h>
#include <wx/sizer.h>
#include <wx/types.h>
#include <wx/window.h>

extern "C" {
#include "board.h"
#include "gamestate.h"
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

GameFrame::GameFrame(wxWindow* parent, wxWindowID id, GameState& state)
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
  state.player_names = new wxString[state.players_count];
  for (int i = 0; i < state.players_count; i++) {
    state.player_names[i] = dialog->get_player_name(i);
  }
  state.board.reset(new Board(init_board(dialog->get_board_width(), dialog->get_board_height())));

  state.game_phase = PHASE_PLACEMENT;
  state.current_player = 0;
  state.penguins_left_to_place = 0;

  Board* board = state.board.get();
  generate_random_board(board);

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

CanvasPanel::CanvasPanel(wxWindow* parent, wxWindowID id, GameState& state)
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

bool CanvasPanel::is_cell_blocked(wxPoint cell) const {
  wxASSERT(this->is_cell_in_bounds(cell));
  return !is_spot_valid_for_placement(state.board.get(), cell.x, cell.y);
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
  if (!board) {
    return;
  }

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
      wxPoint cell_centre = cell_rect.GetPosition() + cell_rect.GetSize() / 2;

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

void CanvasPanel::paint_overlay(wxDC& dc) {
  wxPoint current_cell = this->get_cell_by_coords(this->mouse_pos);
  if (this->mouse_within_window && this->is_cell_in_bounds(current_cell)) {
    dc.SetBrush(wxNullBrush);
    dc.SetPen(wxPen(this->is_cell_blocked(current_cell) ? *wxRED : *wxGREEN, 5));
    dc.DrawRectangle(this->get_cell_rect(current_cell));
  }
}

void CanvasPanel::on_any_mouse_event(wxMouseEvent& event) {
  this->mouse_pos = event.GetPosition();
  if (event.ButtonDown()) {
    this->mouse_drag_pos = this->mouse_pos;
  }
  if (event.Entering()) {
    this->mouse_within_window = true;
  } else if (event.Leaving()) {
    this->mouse_within_window = false;
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
  wxPoint cell = this->get_cell_by_coords(this->mouse_pos);
  if (this->is_cell_in_bounds(cell) && !this->is_cell_blocked(cell)) {
    *this->cell_ptr(cell) = -(state.current_player + 1);
    state.current_player = (state.current_player + 1) % state.players_count;
    this->mark_board_dirty();
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_move(wxMouseEvent& event) {
  this->Refresh();
}

void CanvasPanel::on_mouse_up(wxMouseEvent& event) {}

void CanvasPanel::on_mouse_enter_leave(wxMouseEvent& event) {
  this->mark_board_dirty();
  this->Refresh();
}
