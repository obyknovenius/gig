#include "GigApplicationWindow.h"

namespace Gig {

PEEL_CLASS_IMPL (ApplicationWindow, "GigApplicationWindow", peel::Adw::ApplicationWindow)

void ApplicationWindow::Class::init()
{
}

void ApplicationWindow::init(Class*)
{
    set_title("Gig");
    set_default_size(1024, 768);

    auto web_view = peel::WebKit::WebView::create();
    m_web_view = web_view;
    set_content(std::move(web_view));

    m_web_view->load_uri("https://github.com/obyknovenius/gig");
}

} // namespace Gig
