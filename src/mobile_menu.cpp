// Simple touch UI. Original project attribution is retained in third_party.
#include "mobile_menu.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace vt::ios {
namespace {
const ImVec4 accent{.24f,.64f,1.f,1.f};
constexpr const char* names[]={"ESP","Spam nhảy"};
bool button(Model& m,const char* title,ImVec2 size={0,40}){return ImGui::Button(title,size)&&!m.suppressActions;}
float finite(float value,float fallback,float lo,float hi){return std::isfinite(value)?std::clamp(value,lo,hi):fallback;}
void feature(Model&m,const Actions&a,int id){
    ImGui::PushID(id);const bool enabled=m.connected&&m.controlsAllowed&&m.available[id];ImGui::BeginDisabled(!enabled);
    const auto p=ImGui::GetCursorScreenPos();const float width=ImGui::GetContentRegionAvail().x;
    const bool pressed=ImGui::InvisibleButton(names[id],{width,44})&&!m.suppressActions;
    auto*d=ImGui::GetWindowDrawList();
    if(ImGui::IsItemHovered())d->AddRectFilled(p,{p.x+width,p.y+44},IM_COL32(34,36,40,255),5);
    d->AddText({p.x+2,p.y+(44-ImGui::GetFontSize())*.5f},enabled?IM_COL32(237,239,243,255):IM_COL32(126,130,138,255),names[id]);
    const ImVec2 q{p.x+width-46,p.y+10};
    d->AddRectFilled(q,{q.x+44,q.y+24},m.enabled[id]?IM_COL32(61,163,255,255):IM_COL32(62,65,71,255),12);
    d->AddCircleFilled({q.x+(m.enabled[id]?32.f:12.f),q.y+12},9,IM_COL32(247,249,252,enabled?255:160));
    ImGui::EndDisabled();if(pressed&&a.toggle)a.toggle(id);ImGui::PopID();
}
void interval(Model&m,const Actions&a){
    ImGui::TextUnformatted("Nhịp nhảy");float v=float(m.jumpIntervalMs);ImGui::SetNextItemWidth(-1);
    const bool changed=ImGui::SliderFloat("##interval",&v,0,1000,"%.0f ms",ImGuiSliderFlags_NoInput);
    m.sliderActive=ImGui::IsItemActive();
    if(changed&&!m.suppressActions){m.jumpIntervalMs=std::floor(v);if(a.number)a.number(SpamJump,m.jumpIntervalMs);}
}
}
void sanitize(Model&m){
    m.scale=finite(m.scale,1,.85f,1.25f);m.opacity=finite(m.opacity,.96f,.65f,1.f);
    m.jumpIntervalMs=std::isfinite(m.jumpIntervalMs)?std::clamp(m.jumpIntervalMs,0.,1000.):20.;
}
bool acceptsPoint(const Model&m,ImVec2 p){
    return p.x>=m.windowMin.x&&p.y>=m.windowMin.y&&p.x<m.windowMax.x&&p.y<m.windowMax.y;
}
void style(){
    ImGui::StyleColorsDark();auto&s=ImGui::GetStyle();
    s.WindowRounding=8;s.ChildRounding=0;s.FrameRounding=5;s.GrabRounding=5;s.ScrollbarRounding=5;
    s.WindowPadding={12,12};s.FramePadding={8,8};s.ItemSpacing={6,6};s.ScrollbarSize=12;
    s.WindowBorderSize=1;s.Colors[ImGuiCol_WindowBg]={.065f,.075f,.09f,1};
    s.Colors[ImGuiCol_ChildBg]={0,0,0,0};s.Colors[ImGuiCol_Border]={.22f,.25f,.29f,1};
    s.Colors[ImGuiCol_Button]={.17f,.19f,.22f,1};s.Colors[ImGuiCol_ButtonHovered]={.22f,.27f,.33f,1};
    s.Colors[ImGuiCol_ButtonActive]={.24f,.36f,.49f,1};s.Colors[ImGuiCol_CheckMark]=accent;
    s.Colors[ImGuiCol_SliderGrab]=accent;s.Colors[ImGuiCol_SliderGrabActive]={.54f,.78f,1,1};
    s.Colors[ImGuiCol_FrameBg]={.15f,.18f,.22f,1};s.Colors[ImGuiCol_Text]={.93f,.94f,.96f,1};
    s.Colors[ImGuiCol_TextDisabled]={.53f,.57f,.63f,1};
}
void draw(Model&m,const Actions&a){
    sanitize(m);auto&io=ImGui::GetIO();io.MouseDrawCursor=false;io.FontGlobalScale=m.scale;
    m.sliderActive=false;ImGui::GetStyle().Alpha=m.opacity;
    const ImVec2 origin=m.insetMin;
    const ImVec2 available={io.DisplaySize.x-origin.x-m.insetMax.x,io.DisplaySize.y-origin.y-m.insetMax.y};
    if(m.time<m.toastUntil&&!m.toast.empty()){
        auto*d=ImGui::GetForegroundDrawList();const ImVec2 p{origin.x+12,origin.y+12};auto size=ImGui::CalcTextSize(m.toast.c_str());
        d->AddRectFilled(p,{p.x+size.x+20,p.y+size.y+14},IM_COL32(24,31,40,240),5);
        d->AddText({p.x+10,p.y+7},IM_COL32(213,232,255,255),m.toast.c_str());
    }
    if(available.x<220||available.y<180){m.windowMin=m.windowMax={};return;}
    // Play Together runs both landscape and portrait screens. Reflow from the
    // live interface size instead of assuming one aspect; the safe-area insets
    // keep the panel clear of the notch, home indicator and rounded corners.
    const bool landscape=available.x>available.y;
    if(!m.visible){
        const ImVec2 size{landscape?132.f:100.f,landscape?48.f:56.f};
        m.windowMin={origin.x+12,origin.y+std::max(12.f,available.y-size.y-12)};
        m.windowMax={m.windowMin.x+size.x,m.windowMin.y+size.y};
        ImGui::SetNextWindowPos(m.windowMin);ImGui::SetNextWindowSize(size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8,6});
        ImGui::Begin("##Open",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
        if(button(m,"Menu",{size.x-16,size.y-12})){m.visible=true;m.suppressActions=false;}
        ImGui::End();ImGui::PopStyleVar();return;
    }
    const float width=std::min(landscape?std::min(420.f,available.x*.55f):380.f,available.x-16.f);
    const float maxHeight=landscape?(m.showDetails?330.f:260.f):(m.showDetails?440.f:300.f);
    const float height=std::min(maxHeight,available.y-16.f);
    m.windowMin={origin.x+(available.x-width)*.5f,origin.y+(available.y-height)*.5f};m.windowMax={m.windowMin.x+width,m.windowMin.y+height};
    ImGui::SetNextWindowPos(m.windowMin);ImGui::SetNextWindowSize({width,height});
    ImGui::Begin("PTG iOS##Main",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetCursorPos({12,18});ImGui::TextUnformatted("PTG | R5.3");ImGui::SameLine();
    ImGui::TextColored(m.connected?ImVec4(.35f,.8f,.55f,1):ImVec4(.72f,.75f,.8f,1),"%s",m.connected?"Đã kết nối":"Chờ");
    ImGui::SetCursorPos({width-64,10});if(button(m,"Ẩn",{52,32}))m.visible=false;
    ImGui::SetCursorPos({12,50});const float contentWidth=ImGui::GetContentRegionAvail().x;
    const float bodyHeight=std::max(34.f,ImGui::GetContentRegionAvail().y-46);
    ImGui::BeginChild("Content",{0,bodyHeight},ImGuiChildFlags_None);
    m.contentMin=ImGui::GetWindowPos();m.contentMax={m.contentMin.x+ImGui::GetWindowWidth(),m.contentMin.y+ImGui::GetWindowHeight()};
    if(m.scrollDelta!=0){ImGui::SetScrollY(std::max(0.f,ImGui::GetScrollY()-m.scrollDelta));m.scrollDelta=0;}
    feature(m,a,Esp);feature(m,a,SpamJump);interval(m,a);
    if(m.showDetails){
        ImGui::Spacing();ImGui::Separator();
        if(button(m,"Sao chép chẩn đoán",{-1,40})&&a.diagnostic)a.diagnostic();
        ImGui::TextWrapped("%s",m.status.c_str());
        ImGui::TextDisabled("Target VNG 2.32.0 (227628)");
        if(!m.player.empty())ImGui::TextWrapped("UID: %s",m.player.c_str());
        ImGui::TextWrapped("0 ms: tối đa một yêu cầu nhảy mỗi khung hình.");
    }
    ImGui::EndChild();
    const float footerWidth=(contentWidth-12)/3.f;
    if(button(m,"Lưu",{footerWidth,40})&&a.save)a.save();ImGui::SameLine();
    if(button(m,"Tắt hết",{footerWidth,40})&&a.stop)a.stop();ImGui::SameLine();
    if(button(m,"Chi tiết",{footerWidth,40}))m.showDetails=!m.showDetails;
    ImGui::End();
}
}
