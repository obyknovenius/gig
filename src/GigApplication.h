#pragma once

#include <peel/Adw/Application.h>
#include <peel/Gio/ApplicationFlags.h>
#include <peel/class.h>

namespace Gig {

class Application final : public peel::Adw::Application {
    PEEL_SIMPLE_CLASS(Application, peel::Adw::Application);
    friend class peel::Gio::Application;

public:
    static peel::RefPtr<Application> create()
    {
        return peel::Object::create<Application>(
            prop_application_id(), "com.github.obyknovenius.Gig",
            prop_flags(), peel::Gio::Application::Flags::DEFAULT_FLAGS
        );
    }

private:
    void vfunc_activate();
};

} // namespace Gig
