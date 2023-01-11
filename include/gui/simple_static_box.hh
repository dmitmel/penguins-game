#pragma once

#include <wx/event.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

class SimpleStaticBox : public wxStaticBox {
public:
  SimpleStaticBox() {}

  SimpleStaticBox(
    wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxASCII_STR(wxStaticBoxNameStr)
  ) {
    this->Create(parent, id, pos, size, style, name);
  }

  bool Create(
    wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxASCII_STR(wxStaticBoxNameStr)
  );

protected:
#ifdef __WXGTK__
  virtual bool GTKWidgetNeedsMnemonic() const wxOVERRIDE;
  virtual void GTKWidgetDoSetMnemonic(GtkWidget* w) wxOVERRIDE;
#endif
};
