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

PenguinsApp::~PenguinsApp() {
  if (this->board_inited) {
    free_board(&this->board);
  }
}

bool PenguinsApp::OnInit() {
  random_init();

  this->board = init_board(20, 20);
  this->board_inited = true;
  generate_random_board(&this->board);

  for (int player = 1; player <= 3; player++) {
    for (int penguin = 0; penguin < 2; penguin++) {
      int y = random_range(0, this->board.height - 1);
      int x = random_range(0, this->board.width - 1);
      this->board.grid[y][x] = -player;
    }
  }

  this->game_frame = new GameFrame();
  this->game_frame->Centre();
  this->game_frame->Show();
  return true;
}

enum {
  ID_NEW_GAME = wxID_HIGHEST + 1,
};

// clang-format off
wxBEGIN_EVENT_TABLE(GameFrame, wxFrame)
  EVT_MENU(ID_NEW_GAME, GameFrame::OnNewGame)
  EVT_MENU(wxID_ABOUT, GameFrame::OnAbout)
  EVT_MENU(wxID_EXIT, GameFrame::OnExit)
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

void GameFrame::OnNewGame(wxCommandEvent& event) {
  NewGameDialog dialog(this, wxID_ANY);
  int result = dialog.ShowModal();
}

void GameFrame::OnExit(wxCommandEvent& event) {
  this->Close(true);
}

void GameFrame::OnAbout(wxCommandEvent& event) {
  wxMessageBox(
    "This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION
  );
}

// clang-format off
wxBEGIN_EVENT_TABLE(BoardPanel, wxPanel)
  EVT_PAINT(BoardPanel::OnPaint)
wxEND_EVENT_TABLE();
// clang-format on

BoardPanel::BoardPanel(wxFrame* parent) : wxPanel(parent) {
  this->SetBackgroundColour(*wxWHITE);
}

wxSize BoardPanel::DoGetBestClientSize() const {
  auto board = &wxGetApp().board;
  return CELL_SIZE * wxSize(board->width, board->height);
}

void BoardPanel::OnPaint(wxPaintEvent& event) {
  wxPaintDC dc(this);

  dc.SetTextForeground(*wxBLACK);
  const wxFont default_font = dc.GetFont();
  wxFont cell_font(default_font);
  cell_font.MakeBold();
  cell_font.SetPointSize(CELL_FONT_SIZE);
  dc.SetFont(cell_font);

  auto board = &wxGetApp().board;
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
