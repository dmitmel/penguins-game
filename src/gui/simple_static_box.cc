#include "gui/simple_static_box.hh"
#include <wx/dcclient.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/stattext.h>

bool SimpleStaticBox::Create(
  wxWindow* parent,
  wxWindowID id,
  const wxPoint& pos,
  const wxSize& size,
  long style,
  const wxString& name
) {
#ifdef __WXGTK__
  auto fake_label = new wxStaticText(parent, wxID_ANY, wxEmptyString);
  if (!wxStaticBox::Create(parent, id, fake_label, pos, size, style, name)) {
    return false;
  }
  // This tricks wxGTK into removing the top border of the static box.
  fake_label->Destroy();
  this->m_labelWin = nullptr;
  return true;
#else
  return wxStaticBox::Create(parent, id, fake_label, pos, size, style, name);
#endif
}

#ifdef __WXGTK__
// Get rid of annoying warnings caused by the removal of the GtkFrame's label.
bool SimpleStaticBox::GTKWidgetNeedsMnemonic() const {
  return false;
}
void SimpleStaticBox::GTKWidgetDoSetMnemonic(GtkWidget* WXUNUSED(w)) {}
#endif
