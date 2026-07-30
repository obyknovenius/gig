#include "GigApplication.h"

int main(int argc, char** argv)
{
  auto app = Gig::Application::create();
  return app->run(argc, argv);
}
