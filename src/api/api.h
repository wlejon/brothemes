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

// ---- Access -----------------------------------------------------------------
//
// Which files a program may touch is the host's decision, not brothemes's:
// the natives handed a path ask the host's one access gate,
// bronze::embed::checkAccess (embed/embed_access.h), with the path resolved
// (setPathResolver), absolute and lexically normal (UTF-8), on the JS thread
// that made the call, before the file is opened. import / export take text
// and ask nothing. With no gate installed every path is allowed.
//
//   load   the path Read
//   save   the path Write
//
// A refusal is bronze::embed::newAccessError, thrown: an Error whose `message`
// is the gate's sentence (named PermissionRevoked for a grant taken back),
// `code` is "EACCES" and `path` the absolute path asked about. Nothing is read
// or written.

} // namespace bro::themes::api

namespace brothemes::api {
    using bro::themes::api::installThemes;
    using bro::themes::api::setPathResolver;
    using bro::themes::api::resolvePath;
}

using bro::themes::api::installThemes;
using bro::themes::api::setPathResolver;
using bro::themes::api::resolvePath;
