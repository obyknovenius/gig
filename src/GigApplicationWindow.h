#pragma once

#include <peel/Adw/ApplicationWindow.h>
#include <peel/Gtk/Application.h>
#include <peel/WebKit/WebView.h>
#include <peel/class.h>

namespace Gig {

class ApplicationWindow final : public peel::Adw::ApplicationWindow {
    PEEL_SIMPLE_CLASS(ApplicationWindow, peel::Adw::ApplicationWindow);

public:
    static ApplicationWindow* create(peel::Gtk::Application* app)
    {
        return peel::Object::create<ApplicationWindow>(
            prop_application (), app
        );
    }

private:
    void init(Class*);

    peel::WebKit::WebView* m_web_view;
};

} // namespace Gig
