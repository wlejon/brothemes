#pragma once

#include "embed/embed.h"

#include <functional>
#include <string>

namespace bro::themes::api {

/// Mounts `bro.themes` onto `bro` in current Bronze realm.
void installThemes();

/// Sets process-wide path resolver for file operations (load / save).
void setPathResolver(std::function<std::string(const std::string&)> resolver);

/// Resolves a path using the active resolver (or identity if unset).
std::string resolvePath(const std::string& path);

} // namespace bro::themes::api

namespace brothemes::api {
    using bro::themes::api::installThemes;
    using bro::themes::api::setPathResolver;
    using bro::themes::api::resolvePath;
}

using bro::themes::api::installThemes;
using bro::themes::api::setPathResolver;
using bro::themes::api::resolvePath;
