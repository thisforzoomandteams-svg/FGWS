#include "Scripting/ScriptRegistry.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace Aether {
namespace fs = std::filesystem;

void ScriptRegistry::AddSearchPath(fs::path path) {
  if (path.empty()) {
    return;
  }

  for (const DirState& s : m_dirs) {
    if (s.path == path) {
      return;
    }
  }

  DirState st;
  st.path = std::move(path);
  m_dirs.push_back(std::move(st));
}

void ScriptRegistry::ClearSearchPaths() {
  m_dirs.clear();
  m_scripts.clear();
}

std::string ScriptRegistry::ToLower(std::string s) {
  std::transform(
    s.begin(),
    s.end(),
    s.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
  );
  return s;
}

std::string ScriptRegistry::ExtractDescription(const fs::path& filePath) {
  // Best-effort: read a small prefix and extract the first meaningful comment line.
  std::ifstream in(filePath, std::ios::binary);
  if (!in) {
    return {};
  }

  std::string line;
  while (std::getline(in, line)) {
    // Stop at the first non-comment line.
    if (line.rfind("--", 0) != 0) {
      break;
    }

    // Trim leading "--" and whitespace.
    std::string text = line.substr(2);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
      text.erase(text.begin());
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
      text.pop_back();
    }

    if (text.empty()) {
      continue;
    }

    const std::string lower = ToLower(text);
    constexpr const char* kDescPrefix = "@desc";
    if (lower.rfind(kDescPrefix, 0) == 0) {
      // "@desc: something"
      std::string rest = text.substr(std::char_traits<char>::length(kDescPrefix));
      if (!rest.empty() && (rest.front() == ':' || std::isspace(static_cast<unsigned char>(rest.front())))) {
        while (!rest.empty() && (rest.front() == ':' || std::isspace(static_cast<unsigned char>(rest.front())))) {
          rest.erase(rest.begin());
        }
      }
      return rest;
    }

    return text;
  }

  return {};
}

void ScriptRegistry::Refresh() {
  m_scripts.clear();

  // Capture directory timestamps for RefreshIfNeeded.
  for (DirState& d : m_dirs) {
    std::error_code ec;
    d.lastWriteTime = fs::last_write_time(d.path, ec);
    d.hasWriteTime = !ec;
  }

  const fs::path cwd = fs::current_path();

  for (const DirState& d : m_dirs) {
    std::error_code ec;
    if (!fs::exists(d.path, ec)) {
      continue;
    }

    fs::recursive_directory_iterator it(d.path, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    while (!ec && it != end) {
      const fs::directory_entry& entry = *it;
      std::error_code ec2;
      if (entry.is_regular_file(ec2)) {
        const fs::path p = entry.path();
        std::string ext = ToLower(p.extension().string());
        if (ext == ".lua") {
          ScriptInfo info;
          info.name = p.stem().string();

          std::error_code ec3;
          fs::path rel = fs::relative(p, cwd, ec3);
          info.path = (!ec3) ? rel.generic_string() : p.generic_string();
          info.description = ExtractDescription(p);

          m_scripts.push_back(std::move(info));
        }
      }

      it.increment(ec);
    }
  }

  std::sort(m_scripts.begin(), m_scripts.end(), [](const ScriptInfo& a, const ScriptInfo& b) {
    return a.path < b.path;
  });

  // De-dup by path.
  m_scripts.erase(
    std::unique(m_scripts.begin(), m_scripts.end(), [](const ScriptInfo& a, const ScriptInfo& b) {
      return a.path == b.path;
    }),
    m_scripts.end()
  );
}

bool ScriptRegistry::RefreshIfNeeded() {
  bool need = m_scripts.empty();

  for (DirState& d : m_dirs) {
    std::error_code ec;
    const fs::file_time_type t = fs::last_write_time(d.path, ec);
    if (ec) {
      continue;
    }

    if (!d.hasWriteTime || t != d.lastWriteTime) {
      need = true;
      break;
    }
  }

  if (need) {
    Refresh();
    return true;
  }
  return false;
}

const std::vector<ScriptInfo>& ScriptRegistry::GetAvailableScripts(const Scene* /*scene*/) const {
  return m_scripts;
}

} // namespace Aether

