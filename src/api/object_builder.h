#pragma once

#include "embed/embed.h"

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace bro::themes::api {

namespace ev = bronze::embed;
using Value = bronze::Value;

/// Helper to build objects and namespaces property by property using bronze::embed.
/// Handles moving GC by rooting the target in an ev::Persistent and keeping all intermediate
/// allocations strictly sequenced.
struct ObjectBuilder {
    ev::Persistent obj;

    ObjectBuilder() : obj(ev::createObject()) {}
    explicit ObjectBuilder(Value existing) : obj(existing) {}

    void set(std::string_view name, Value v) {
        obj.set(ev::setProperty(obj.get(), name, v));
    }

    void set(std::string_view name, double d) {
        ev::Persistent v(ev::fromDouble(d));
        obj.set(ev::setProperty(obj.get(), name, v.get()));
    }

    void set(std::string_view name, bool b) {
        ev::Persistent v(ev::fromBool(b));
        obj.set(ev::setProperty(obj.get(), name, v.get()));
    }

    void set(std::string_view name, const std::string& s) {
        ev::Persistent v(ev::fromUtf8(s));
        obj.set(ev::setProperty(obj.get(), name, v.get()));
    }

    void set(std::string_view name, const char* s) {
        ev::Persistent v(ev::fromUtf8(s));
        obj.set(ev::setProperty(obj.get(), name, v.get()));
    }

    void def(std::string_view name, uint32_t arity, ev::NativeFn fn) {
        ev::Persistent f(ev::makeFunction(std::move(fn), arity, name));
        obj.set(ev::setProperty(obj.get(), name, f.get()));
    }

    void accessor(std::string_view name, ev::NativeFn getter, ev::NativeFn setter = nullptr) {
        const std::string getName = "get " + std::string(name);
        const std::string setName = "set " + std::string(name);
        ev::Persistent g(ev::makeFunction(std::move(getter), 0, getName));
        ev::Persistent s;
        if (setter) {
            s.set(ev::makeFunction(std::move(setter), 1, setName));
        } else {
            s.set(ev::undefined());
        }
        obj.set(ev::defineAccessor(obj.get(), name, g.get(), s.get(), /*enumerable=*/true));
    }

    Value get() const { return obj.get(); }
    Value build() const { return obj.get(); }
};

} // namespace bro::themes::api
