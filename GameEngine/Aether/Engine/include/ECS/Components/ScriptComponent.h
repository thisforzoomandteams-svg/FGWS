#pragma once

#include <sol/sol.hpp>

#include <string>

namespace Aether {

struct ScriptComponent {
  std::string ScriptPath;
  sol::table Instance;
};

} // namespace Aether

