#include <peel/Adw/Adw.h>
#include <peel/Gio/ApplicationFlags.h>
#include <peel/Gtk/Gtk.h>

using namespace peel;

int main(int argc, char **argv)
{
  auto app = Adw::Application::create("com.github.obyknovenius.Gig", Gio::Application::Flags::DEFAULT_FLAGS);
  app->connect_activate([](Gio::Application *app) {
    auto window = Gtk::ApplicationWindow::create(app->cast<Gtk::Application> ());
    window->set_default_size(200, 200);

    auto child = Gtk::Label::create("Hello world!");
    window->set_child(std::move(child));

    window->present();
  });
  app->run(argc, argv);
}
