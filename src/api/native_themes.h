#pragma once

#include "embed/embed.h"
#include "object_builder.h"

namespace bro::themes::api {

void installContrastOnto(ObjectBuilder& root);
void installFormatOnto(ObjectBuilder& root);
void installColorOnto(ObjectBuilder& root);
void installThemesOnto(ObjectBuilder& root);

} // namespace bro::themes::api
