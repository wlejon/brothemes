#include "api.h"
#include "native_themes.h"
#include "object_builder.h"

namespace bro::themes::api {

namespace ev = bronze::embed;
using Value = bronze::Value;

namespace {

std::function<std::string(const std::string&)>& pathResolverSlot() {
    static std::function<std::string(const std::string&)> resolver;
    return resolver;
}

} // namespace

std::string resolvePath(const std::string& path) {
    auto& r = pathResolverSlot();
    return r ? r(path) : path;
}

void setPathResolver(std::function<std::string(const std::string&)> resolver) {
    pathResolverSlot() = std::move(resolver);
}

void installThemes() {
    ev::Persistent globalThisVal;
    auto gt = ev::globalValue("globalThis");
    if (gt.found && ev::isObject(gt.value)) {
        globalThisVal.set(gt.value);
    }

    ev::Persistent broP;
    auto bro = ev::globalValue("bro");
    if (bro.found && ev::isObject(bro.value)) broP.set(bro.value);
    if (!ev::isObject(broP.get()) && ev::isObject(globalThisVal.get())) {
        Value candidate = ev::getProperty(globalThisVal.get(), "bro");
        if (ev::isObject(candidate)) broP.set(candidate);
    }
    if (!ev::isObject(broP.get())) {
        broP.set(ev::createObject());
        ev::registerGlobal("bro", broP.get());
        if (ev::isObject(globalThisVal.get())) {
            globalThisVal.set(ev::setProperty(globalThisVal.get(), "bro", broP.get()));
        }
    }

    ObjectBuilder themes;
    installThemesOnto(themes);

    ev::Persistent themesP(themes.build());
    broP.set(ev::setProperty(broP.get(), "themes", themesP.get()));
    if (ev::isObject(globalThisVal.get())) {
        globalThisVal.set(ev::setProperty(globalThisVal.get(), "bro", broP.get()));
    }
}

} // namespace bro::themes::api
