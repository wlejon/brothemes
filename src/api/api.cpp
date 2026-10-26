#include "api.h"
#include "native_themes.h"
#include "object_builder.h"
#include "embed/embed_access.h"

#include <filesystem>

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

std::string gatedPath(const std::string& path, bool write) {
    std::string resolved = resolvePath(path);
    if (!ev::accessGateActive()) return resolved;
    namespace fs = std::filesystem;
    fs::path p(std::u8string(resolved.begin(), resolved.end()));
    std::error_code ec;
    fs::path abs = fs::absolute(p, ec);
    if (ec) abs = p;
    const std::u8string u8 = abs.lexically_normal().u8string();
    const std::string subject(u8.begin(), u8.end());
    if (std::string refused = ev::checkAccess(write ? ev::AccessKind::Write : ev::AccessKind::Read, subject);
        !refused.empty()) {
        ev::throwAccessError(refused, "EACCES", subject);
    }
    return resolved;
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
