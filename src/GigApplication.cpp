#include "GigApplication.h"

#include <peel/Gtk/Gtk.h>
#include "GigApplicationWindow.h"

namespace Gig {

PEEL_CLASS_IMPL (Application, "GigApplication", peel::Adw::Application)

void Application::Class::init()
{
    override_vfunc_activate<Application>();
}

void Application::vfunc_activate()
{
    parent_vfunc_activate<Application>();

    auto* window = Gig::ApplicationWindow::create(this);

    auto child = peel::Gtk::Label::create("Hello world!");
    window->set_content(std::move(child));

    window->present();
}

} // namespace Gig
