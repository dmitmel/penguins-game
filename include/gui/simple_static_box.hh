#pragma once

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/statbox.h>
#include <wx/string.h>
#include <wx/window.h>

class SimpleStaticBox : public wxStaticBox {
public:
  SimpleStaticBox() {}

  SimpleStaticBox(
    wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxStaticBoxNameStr
  ) {
    this->Create(parent, id, pos, size, style, name);
  }

  bool Create(
    wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = 0,
    const wxString& name = wxStaticBoxNameStr
  );

protected:
#ifdef __WXGTK__
  virtual bool GTKWidgetNeedsMnemonic() const override;
  virtual void GTKWidgetDoSetMnemonic(GtkWidget* w) override;
#endif
};
