#pragma once

#include "gui/game_state.hh"
#include <wx/defs.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/grid.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/window.h>

class GameEndDialogGrid : public wxGrid {
public:
  GameEndDialogGrid(
    wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxASCII_STR(wxGridNameStr)
  );

  void equally_size_columns();

protected:
  virtual bool TryBefore(wxEvent& event) override;
  void on_resize(wxSizeEvent& event);
};

class GameEndDialog : public wxDialog {
public:
  GameEndDialog(wxWindow* parent, wxWindowID id, GuiGameState& state);

protected:
  wxStaticText* header_label;
  wxStaticText* winner_label;
  GameEndDialogGrid* grid;
  wxStdDialogButtonSizer* buttons_sizer;

  void on_ok(wxCommandEvent& event);
  void on_key_down(wxKeyEvent& event);
};
