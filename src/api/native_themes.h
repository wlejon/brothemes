#pragma once

#include "embed/embed.h"
#include "object_builder.h"

#include <string>

namespace bro::themes::api {

/// `path` resolved (resolvePath), once the host's access gate (embed/embed_access.h;
/// api.h, "Access") allows reading it, or writing it when `write`, asked with the
/// path absolute and lexically normal. A refusal is thrown (an Error, code EACCES,
/// the path) and never returns, so nothing is opened (api.cpp).
std::string gatedPath(const std::string& path, bool write);

void installContrastOnto(ObjectBuilder& root);
void installFormatOnto(ObjectBuilder& root);
void installColorOnto(ObjectBuilder& root);
void installThemesOnto(ObjectBuilder& root);

} // namespace bro::themes::api
