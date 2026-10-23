#include "mobile_menu.hpp"
#include "imgui_internal.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <map>
#include <string>
static std::map<ImGuiID,ImRect> rects;
static std::map<ImGuiID,std::string> names;
static std::map<std::string,ImRect> items;
void ImGuiTestEngineHook_ItemAdd(ImGuiContext*,ImGuiID id,const ImRect&r,const ImGuiLastItemData*){rects[id]=r;if(names.contains(id))items[names[id]]=r;}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext*,ImGuiID id,const char*label,ImGuiItemStatusFlags){names[id]=label;if(rects.contains(id))items[label]=rects[id];}
void ImGuiTestEngineHook_Log(ImGuiContext*,const char*,...){}
const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*,ImGuiID){return "two-features";}
int main(){
    using namespace vt::ios;static_assert(vt::FeatureCount==2);
    Model m;Actions a;int toggles=0,last=-1,saves=0,stops=0,copies=0;
    a.diagnostic=[&]{++copies;};
    a.toggle=[&](int id){++toggles;last=id;};a.save=[&]{++saves;};a.stop=[&]{++stops;};
    auto*context=ImGui::CreateContext();context->TestEngineHookItems=true;
    auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={414,808};io.DeltaTime=1.f/60;
    unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);io.Fonts->SetTexID(1);style();
    auto frame=[&]{items.clear();rects.clear();ImGui::NewFrame();draw(m,a);ImGui::Render();};
    auto click=[&](const char*label){assert(items.contains(label));auto p=items[label].GetCenter();io.AddMousePosEvent(p.x,p.y);io.AddMouseButtonEvent(0,false);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();frame();};
    frame();frame();assert(!m.visible&&items.contains("Menu"));
    assert(acceptsPoint(m,items["Menu"].GetCenter())&&!acceptsPoint(m,{0,0}));
    click("Menu");frame();frame();assert(items.contains("ESP")&&items.contains("Spam nhảy"));
    for(const char*removed:{"Speed","Nhảy cao","Nhảy đôi","Sương","Ready","Búa","Line","TCP nhanh","Xuyên"})assert(!items.contains(removed));
    click("ESP");assert(toggles==0);m.connected=m.controlsAllowed=true;m.available.fill(true);frame();
    click("ESP");assert(toggles==1&&last==vt::Esp);click("Spam nhảy");assert(toggles==2&&last==vt::SpamJump);
    m.suppressActions=true;click("ESP");assert(toggles==2);m.suppressActions=false;
    click("Lưu");assert(saves==1);click("Tắt hết");assert(stops==1);
    assert(!items.contains("Sao chép chẩn đoán"));
    click("Chi tiết");frame();frame();assert(m.showDetails&&items.contains("Sao chép chẩn đoán"));
    click("Sao chép chẩn đoán");assert(copies==1);
    click("Chi tiết");frame();frame();assert(!m.showDetails&&!items.contains("Sao chép chẩn đoán"));
    click("Ẩn");frame();assert(!m.visible);
    m.visible=true;m.player="42";
    for(ImVec2 size:{ImVec2(375,724),ImVec2(808,375),ImVec2(1024,1320),ImVec2(1366,980),ImVec2(507,1000),ImVec2(896,414),ImVec2(414,896),ImVec2(1024,768),ImVec2(768,1024)}){
        io.DisplaySize=size;frame();frame();assert(items.contains("Lưu"));assert(items["Lưu"].Max.y<=size.y+1);
        assert(acceptsPoint(m,items["Lưu"].GetCenter())&&!acceptsPoint(m,{0,0}));
        assert(m.windowMax.x-m.windowMin.x>m.windowMax.y-m.windowMin.y);
        assert(items["ESP"].Max.x<=m.windowMax.x&&items["Spam nhảy"].Max.y<=m.windowMax.y);
    }
    io.DisplaySize={896,414};m.insetMin={44,0};m.insetMax={44,21};m.scale=1.25f;
    frame();frame();assert(m.windowMin.x>=44&&m.windowMax.x<=852&&m.windowMax.y<=393);
    click("Spam nhảy");assert(last==vt::SpamJump);
    assert(items["Lưu"].Max.y<=393&&!io.MouseDrawCursor);
    m.jumpIntervalMs=std::numeric_limits<double>::quiet_NaN();sanitize(m);assert(m.jumpIntervalMs==20);
    m.jumpIntervalMs=-1;sanitize(m);assert(m.jumpIntervalMs==0);
    m.jumpIntervalMs=99999;sanitize(m);assert(m.jumpIntervalMs==1000);
    ImGui::DestroyContext(context);
}
