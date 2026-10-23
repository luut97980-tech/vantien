#pragma once
#include "mobile_menu.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>
namespace vt::ios {
struct EspItem {float x=0,y=0,headY=0,distance=0;std::string label;};
// True only while a feature that needs the game runtime is switched on.
// The renderer uses it to keep the backend completely silent at the login
// screen and while the menu is just sitting there.
bool runtimeRequested(const Model&);
class GameBackend {
    struct State;
    std::unique_ptr<State> state;
public:
    using Resolver=std::function<void*(const char*)>;
    explicit GameBackend(Resolver resolver);
    ~GameBackend();
    void tick(Model&,double time);
    // Drops every managed handle and holds the next runtime call for a moment.
    // Called from layout/orientation/app-lifecycle edges, where the game is
    // rebuilding its scene and its objects are not safe to touch.
    void suspend(double time);
    void toggle(Model&,int feature);
    void stop(Model&);
    std::string diagnostic() const;
    const std::vector<EspItem>& esp() const;
};
}
