#include "gui/main.hh"
#include "gui/new_game_dialog.hh"
#include <cmath>
#include <wx/dcclient.h>
#include <wx/gdicmn.h>
#include <wx/graphics.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

extern "C" {
#include "random.h"
}

wxIMPLEMENT_APP(PenguinsApp);

PenguinsApp::PenguinsApp() {}

bool PenguinsApp::OnInit() {
  random_init();

  this->game_frame = new GameFrame(nullptr, wxID_ANY);
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

GameFrame::GameFrame() : wxFrame(nullptr, wxID_ANY, "Penguins game") {
  auto menuFile = new wxMenu();
  menuFile->Append(ID_NEW_GAME, "&New Game...\tCtrl-N", "Start a new game");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  auto menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);

  auto menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  auto vbox = new wxBoxSizer(wxVERTICAL);
  auto hbox = new wxBoxSizer(wxHORIZONTAL);

  this->board_panel = new BoardPanel(this);
  vbox->Add(this->board_panel, wxSizerFlags(1).Centre().Border());
  hbox->Add(vbox, wxSizerFlags(1).Centre().Border());

  this->SetSizerAndFit(hbox);
  this->SetMenuBar(menuBar);

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
  NewGameDialog dialog(this, wxID_ANY);
  int result = dialog.ShowModal();
  if (result != wxID_OK) {
    return;
  }

  Board* board_ptr = new Board;
  *board_ptr = init_board(dialog.get_board_width(), dialog.get_board_height());
  std::shared_ptr<Board> board(board_ptr, free_board);
  wxGetApp().board = board;

  generate_random_board(board.get());

  for (int player = 1; player <= dialog.get_number_of_players(); player++) {
    for (int penguin = 0; penguin < dialog.get_penguins_per_player(); penguin++) {
      int y = random_range(0, board->height - 1);
      int x = random_range(0, board->width - 1);
      board->grid[y][x] = -player;
    }
  }

  this->board_panel->InvalidateBestSize();
  this->GetSizer()->SetSizeHints(this);
  this->Refresh();
}

// clang-format off
wxBEGIN_EVENT_TABLE(BoardPanel, wxPanel)
  EVT_PAINT(BoardPanel::OnPaint)
wxEND_EVENT_TABLE();
// clang-format on

BoardPanel::BoardPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {}

wxSize BoardPanel::DoGetBestClientSize() const {
  if (auto& board = wxGetApp().board) {
    return CELL_SIZE * wxSize(board->width, board->height);
  } else {
    return wxSize(640, 640);
  }
}

void BoardPanel::OnPaint(wxPaintEvent& event) {
  wxPaintDC dc(this);

  auto& board = wxGetApp().board;
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
      int cell = board->grid[y][x];
      wxRect cell_rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
      wxPoint cell_centre = cell_rect.GetPosition() + cell_rect.GetSize() / 2;

      dc.SetPen(*wxBLACK_PEN);

      if (cell > 0) {
        // an ice floe with fish on it
        int fish_count = cell;
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(cell_rect);

        dc.SetPen(wxNullPen);
        dc.SetBrush(*wxGREY_BRUSH);
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

      } else if (cell < 0) {
        // a penguin
        int player = -cell;
        dc.SetBrush(*wxYELLOW_BRUSH);
        dc.DrawRectangle(cell_rect);

        wxString str;
        str << player;
        dc.DrawLabel(str, cell_rect, wxALIGN_CENTRE);

      } else {
        // a water tile
        dc.SetBrush(*wxCYAN_BRUSH);
        dc.DrawRectangle(cell_rect);
      }
    }
  }
}
