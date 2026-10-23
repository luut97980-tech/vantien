#pragma once
#include "feature_keys.hpp"
#include "imgui.h"
#include <array>
#include <functional>
#include <string>
#include <vector>
namespace vt::ios {
struct Model {
    bool visible=false,connected=false,controlsAllowed=false,suppressActions=false;
    bool sliderActive=false,showDetails=false;
    float scale=1.f,opacity=.96f,scrollDelta=0.f;
    std::array<bool,FeatureCount> enabled{},available{};
    ImVec2 windowMin{},windowMax{},insetMin{},insetMax{};
    std::string toast,player;double time=0,toastUntil=0;
    double jumpIntervalMs=20.;
    ImVec2 contentMin{},contentMax{};
    std::string status="Chưa kết nối game",diagnostic="PTG iOS R5.3 • Chưa kiểm tra runtime.";
};
struct Actions {
    std::function<void(int)> toggle;
    std::function<void(int,double)> number;
    std::function<void()> stop,save,diagnostic;
};
void sanitize(Model&);
void style();
void draw(Model&,const Actions&);
bool acceptsPoint(const Model&,ImVec2);
}
