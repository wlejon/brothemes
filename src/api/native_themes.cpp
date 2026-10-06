#include "native_themes.h"

namespace bro::themes::api {

void installThemesOnto(ObjectBuilder& root) {
    installContrastOnto(root);
    installFormatOnto(root);
    installColorOnto(root);
}

} // namespace bro::themes::api
