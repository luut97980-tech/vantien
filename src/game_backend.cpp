#include "game_backend.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <tuple>

namespace vt::ios {
namespace {
using Obj=void*;using Klass=void*;using Field=void*;using Method=const void*;
struct Vec3 {float x,y,z;};struct Rect{float x,y,w,h;};
struct API {
    void*(*domain_get)()=nullptr;
    void**(*domain_get_assemblies)(void*,std::size_t*)=nullptr;
    void*(*assembly_get_image)(void*)=nullptr;
    const char*(*image_get_name)(void*)=nullptr;
    Klass(*class_from_name)(void*,const char*,const char*)=nullptr;
    Klass(*class_get_parent)(Klass)=nullptr;
    const char*(*class_get_name)(Klass)=nullptr;
    const char*(*class_get_namespace)(Klass)=nullptr;
    Method(*class_get_method_from_name)(Klass,const char*,int)=nullptr;
    Field(*class_get_field_from_name)(Klass,const char*)=nullptr;
    const void*(*field_get_type)(Field)=nullptr;
    int(*type_get_type)(const void*)=nullptr;
    void(*field_get_value)(Obj,Field,void*)=nullptr;
    void(*field_static_get_value)(Field,void*)=nullptr;
    Obj(*runtime_invoke)(Method,Obj,void**,Obj*)=nullptr;
    Klass(*object_get_class)(Obj)=nullptr;
    void*(*object_unbox)(Obj)=nullptr;
    std::uintptr_t(*gchandle_new)(Obj,bool)=nullptr;
    Obj(*gchandle_get_target)(std::uintptr_t)=nullptr;
    void(*gchandle_free)(std::uintptr_t)=nullptr;
    Obj(*array_new)(Klass,std::size_t)=nullptr;
    std::size_t(*array_length)(Obj)=nullptr;
    std::uint32_t(*array_object_header_size)()=nullptr;
    void*(*thread_current)()=nullptr;
    void*(*thread_attach)(void*)=nullptr;
    void(*thread_detach)(void*)=nullptr;
    explicit API(const GameBackend::Resolver& r){
#define LOAD(n) n=reinterpret_cast<decltype(n)>(r("il2cpp_" #n));if(!n)throw std::runtime_error("Missing il2cpp_" #n)
        LOAD(domain_get);LOAD(domain_get_assemblies);LOAD(assembly_get_image);LOAD(image_get_name);
        LOAD(class_from_name);LOAD(class_get_parent);LOAD(class_get_name);LOAD(class_get_namespace);
        LOAD(class_get_method_from_name);LOAD(class_get_field_from_name);LOAD(field_get_type);LOAD(type_get_type);
        LOAD(field_get_value);LOAD(field_static_get_value);LOAD(runtime_invoke);
        LOAD(object_get_class);LOAD(object_unbox);LOAD(gchandle_new);LOAD(gchandle_get_target);LOAD(gchandle_free);
        LOAD(array_new);LOAD(array_length);LOAD(array_object_header_size);
        LOAD(thread_current);LOAD(thread_attach);LOAD(thread_detach);
#undef LOAD
    }
};
struct Root {
    API* a=nullptr;std::uintptr_t h=0;
    Root()=default;Root(API&api,Obj o):a(&api),h(o?api.gchandle_new(o,false):0){if(o&&!h)throw std::runtime_error("GC handle failed");}
    Root(const Root&)=delete;Root&operator=(const Root&)=delete;
    ~Root(){reset();}
    Obj get()const{return h?a->gchandle_get_target(h):nullptr;}
    void reset(){if(h){a->gchandle_free(h);h=0;}}
    void set(API&api,Obj o){auto next=o?api.gchandle_new(o,false):0;if(o&&!next)throw std::runtime_error("GC handle failed");reset();a=&api;h=next;}
};
bool valid(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
}
bool runtimeRequested(const Model&m){return m.enabled[Esp]||m.enabled[SpamJump];}
struct GameBackend::State {
    API a;void* ownThread=nullptr;
    std::map<std::string,Klass> classes;
    std::map<std::tuple<Klass,std::string,int>,Method> methods;
    std::map<std::pair<Klass,std::string>,Field> fields;
    Root actorRoot,controlRoot;
    std::string actorUID;
    double nextJump=0,lastJump=-1,lastTick=-1;
    // Scene-change / failure gates. Both keep managed objects untouched while
    // the game is between scenes, which is exactly when the local character,
    // its controller and the camera are being destroyed.
    double settleUntil=0,retryAfter=0;
    bool idle=true,needsSettle=false;
    std::uint64_t jumpRequests=0;
    std::vector<EspItem> points;double espTime=-1;
    std::string status="Waiting for IL2CPP",controllerName;
    explicit State(Resolver r):a(r){}
    ~State(){controlRoot.reset();actorRoot.reset();if(ownThread)a.thread_detach(ownThread);}
    Klass klass(const char*image,const char*ns,const char*name){
        std::string key=std::string(image)+":"+ns+":"+name;
        if(auto it=classes.find(key);it!=classes.end())return it->second;
        auto domain=a.domain_get();if(!domain)throw std::runtime_error("Waiting for IL2CPP domain");
        if(!a.thread_current()&&!ownThread){ownThread=a.thread_attach(domain);if(!ownThread)throw std::runtime_error("IL2CPP attach failed");}
        std::size_t count=0;auto list=a.domain_get_assemblies(domain,&count);
        if(!list||count>4096)throw std::runtime_error("Waiting for assemblies");
        for(std::size_t i=0;i<count;++i){auto im=a.assembly_get_image(list[i]);const char*n=a.image_get_name(im);
            if(n&&std::strcmp(n,image)==0){auto c=a.class_from_name(im,ns,name);if(c){classes[key]=c;return c;}}}
        throw std::runtime_error(std::string("Class unavailable: ")+name);
    }
    Klass game(const char*name){return klass("Assembly-CSharp.dll","",name);}
    Klass unity(const char*name){return klass("UnityEngine.CoreModule.dll","UnityEngine",name);}
    Method method(Klass c,const char*name,int argc){
        auto key=std::make_tuple(c,std::string(name),argc);if(auto i=methods.find(key);i!=methods.end())return i->second;
        for(auto k=c;k;k=a.class_get_parent(k)){auto m=a.class_get_method_from_name(k,name,argc);if(m){methods[key]=m;return m;}}
        throw std::runtime_error(std::string("Method unavailable: ")+name);
    }
    Field field(Klass c,const char*name){
        auto key=std::make_pair(c,std::string(name));if(auto i=fields.find(key);i!=fields.end())return i->second;
        for(auto k=c;k;k=a.class_get_parent(k)){auto f=a.class_get_field_from_name(k,name);if(f){fields[key]=f;return f;}}
        throw std::runtime_error(std::string("Field unavailable: ")+name);
    }
    Obj invoke(Method m,Obj o,void**args=nullptr){Obj ex=nullptr;auto r=a.runtime_invoke(m,o,args,&ex);if(ex)throw std::runtime_error("Managed call raised an exception");return r;}
    Obj call(Obj o,const char*n,int argc=0,void**args=nullptr){if(!o)throw std::runtime_error("Waiting for object");return invoke(method(a.object_get_class(o),n,argc),o,args);}
    template<class T>T unbox(Obj o){if(!o)throw std::runtime_error("Missing boxed return value");auto p=a.object_unbox(o);if(!p)throw std::runtime_error("Cannot unbox result");T v{};std::memcpy(&v,p,sizeof(v));return v;}
    template<class T>T read(Obj o,Field f,int expected=0){
        if(!o)throw std::runtime_error("Null field owner");
        if(expected&&a.type_get_type(a.field_get_type(f))!=expected)throw std::runtime_error("Field type changed");
        T v{};a.field_get_value(o,f,&v);return v;
    }
    Obj ref(Obj o,const char*n){return read<Obj>(o,field(a.object_get_class(o),n));}
    bool alive(Obj o){if(!o)return false;void*args[]={o};return unbox<bool>(invoke(method(unity("Object"),"op_Implicit",1),nullptr,args));}
    Obj actorSystem(){Obj o=nullptr;a.field_static_get_value(field(game("ActorSystem"),"Self"),&o);return o;}
    Obj actor(){auto o=actorSystem();return o?call(o,"get_MyActorCharacter"):nullptr;}
    Obj currentControl(Obj actor){
        auto control=ref(actor,"actorControl");if(!control)return nullptr;
        if(ref(control,"actorBase")!=actor)throw std::runtime_error("Local controller link changed");
        auto klass=a.object_get_class(control);bool supported=false;
        for(auto c=klass;c;c=a.class_get_parent(c)){
            const char*n=a.class_get_name(c);const char*ns=a.class_get_namespace(c);
            if(n&&ns&&!*ns&&std::strcmp(n,"ActorControl")==0){supported=true;break;}
        }
        if(!supported)throw std::runtime_error("Local controller is not an ActorControl");
        if(read<bool>(control,field(klass,"_terminate"),2))return nullptr;
        if(!alive(control))return nullptr;
        controllerName=a.class_get_name(klass);return control;
    }
    void resetFocus(){controlRoot.reset();actorRoot.reset();actorUID.clear();nextJump=0;lastJump=-1;}
    void jumpTick(Obj actor,double time,double intervalMs){
        if(!std::isfinite(intervalMs)||intervalMs<0||intervalMs>1000)throw std::runtime_error("Invalid spam interval");
        auto control=currentControl(actor);
        if(!control){controlRoot.reset();nextJump=0;lastJump=-1;return;}
        if(control!=controlRoot.get()){controlRoot.set(a,control);nextJump=0;lastJump=-1;}
        if(time<nextJump||time==lastJump)return;
        // Invoke the real controller's virtual jump action, as a normal jump request.
        // Do not modify speed, jump height, double-jump flags or round permissions.
        call(control,"OnJump");++jumpRequests;lastJump=time;nextJump=time+intervalMs/1000.;
    }
    Vec3 position(Obj actor){auto tr=ref(actor,"baseTransfrom");if(!alive(tr))throw std::runtime_error("Transform unavailable");return unbox<Vec3>(call(tr,"get_position"));}
    void espTick(double time){
        if(time-espTime<.1)return;
        espTime=time;points.clear();
        auto own=actor();if(!alive(own))return;Root ownRoot(a,own);auto origin=position(own);
        auto cam=invoke(method(unity("Camera"),"get_main",0),nullptr);
        // No camera means the scene is mid-swap; hold the next actor read too.
        if(!alive(cam)){needsSettle=true;return;}Root camRoot(a,cam);
        Rect viewport=unbox<Rect>(call(cam,"get_rect"));
        if(!std::isfinite(viewport.w)||!std::isfinite(viewport.h)||viewport.w<=0||viewport.h<=0)return;
        auto dict=call(actorSystem(),"get_OtherActorCharacter");if(!dict)return;Root dictRoot(a,dict);
        int count=unbox<int>(call(dict,"get_Count"));if(count<=0)return;if(count>128)throw std::runtime_error("ESP actor count exceeds 128");
        auto values=call(dict,"get_Values");Root valuesRoot(a,values);
        auto arr=a.array_new(game("ActorCharacter"),std::size_t(count));Root arrayRoot(a,arr);
        if(!arr)throw std::runtime_error("ESP array allocation failed");
        int index=0;void*args[]={arr,&index};call(values,"CopyTo",2,args);
        if(a.array_length(arr)!=std::size_t(count))throw std::runtime_error("ESP array size mismatch");
        auto head=a.array_object_header_size();if(head<16||head>128)throw std::runtime_error("Array layout unavailable");
        auto project=[&](Vec3 p){void*v[]={&p};return unbox<Vec3>(call(cam,"WorldToViewportPoint",1,v));};
        for(int i=0;i<count;++i){
            // One bad actor must not abort the whole overlay: during a scene load
            // half the dictionary can be destroyed while CopyTo already ran.
            try{
                Obj other=nullptr;std::memcpy(&other,static_cast<char*>(arr)+head+i*sizeof(Obj),sizeof(other));
                if(!other||other==own||!alive(other))continue;
                auto pos=position(other);Vec3 top=pos;top.y+=1.65f;if(!valid(pos)||!valid(origin))continue;
                auto feet=project(pos),above=project(top);if(!valid(feet)||!valid(above)||feet.z<=.1f||above.z<=.1f)continue;
                float x=viewport.x+feet.x*viewport.w,y=viewport.y+feet.y*viewport.h,hy=viewport.y+above.y*viewport.h;
                if(x<-.2f||x>1.2f||y<-.2f||y>1.2f)continue;
                float distance=std::sqrt((pos.x-origin.x)*(pos.x-origin.x)+(pos.y-origin.y)*(pos.y-origin.y)+(pos.z-origin.z)*(pos.z-origin.z));
                if(!std::isfinite(distance)||distance>10000)continue;
                auto uid=read<std::int64_t>(other,field(a.object_get_class(other),"<SUID>k__BackingField"),0x0a);
                points.push_back({x,1-y,1-hy,distance,std::to_string(uid)});
            }catch(const std::exception&){continue;}
        }
    }
};
GameBackend::GameBackend(Resolver r):state(std::make_unique<State>(std::move(r))){}
GameBackend::~GameBackend()=default;
const std::vector<EspItem>&GameBackend::esp()const{return state->points;}
std::string GameBackend::diagnostic()const{
    auto&s=*state;
    return s.status+"\nController: "+s.controllerName+
        "\nJump requests sent (not confirmed jumps): "+std::to_string(s.jumpRequests)+
        "\nESP: "+std::to_string(s.points.size())+
        "\nRuntime gate: "+(s.idle?"idle (no game calls)":"active");
}
void GameBackend::stop(Model&m){
    auto&s=*state;m.enabled.fill(false);s.points.clear();s.resetFocus();s.espTime=-1;
    s.settleUntil=0;s.retryAfter=0;s.idle=true;
    m.status="Đã tắt ESP và spam nhảy";s.status=m.status;
}
void GameBackend::suspend(double time){
    auto&s=*state;s.points.clear();s.resetFocus();s.espTime=-1;s.retryAfter=0;
    s.needsSettle=true;
    s.settleUntil=std::isfinite(time)?time+.75:0;
}
void GameBackend::toggle(Model&m,int id){
    if(id<0||id>=FeatureCount||!m.available[id]||!m.connected||!m.controlsAllowed)return;
    bool on=!m.enabled[id];m.enabled[id]=on;
    if(id==SpamJump){state->nextJump=0;state->lastJump=-1;if(!on)state->controlRoot.reset();}
    if(id==Esp)state->points.clear();
    state->settleUntil=0;
    m.toast=std::string(id==Esp?"ESP":"Spam nhảy")+(on?" • BẬT":" • TẮT");m.toastUntil=m.time+1.;
}
void GameBackend::tick(Model&m,double time){
    auto&s=*state;
    try{
        if(!m.connected||!m.controlsAllowed){
            if(!s.idle)stop(m);
            else{m.status="Chờ kết nối game";s.status=m.status;}
            return;
        }
        if(!std::isfinite(time)||time<0){s.points.clear();m.status="Thời gian cập nhật không hợp lệ";return;}
        if(time<s.lastTick){s.nextJump=0;s.lastJump=-1;s.espTime=-1;s.settleUntil=0;s.retryAfter=0;}
        s.lastTick=time;
        if(!runtimeRequested(m)){
            // Both switches off: stay out of the game runtime entirely. R5 kept
            // invoking ActorSystem/Object every frame and that is what crashed
            // the app while the login scene was being built.
            s.points.clear();
            if(!s.idle){s.resetFocus();s.espTime=-1;s.idle=true;m.player.clear();}
            s.status="Chờ bật ESP hoặc Spam nhảy";m.status=s.status;
            return;
        }
        s.idle=false;
        if(time<s.retryAfter){m.status=s.status;return;}
        auto actor=s.actor();
        if(!s.alive(actor)){
            s.points.clear();s.resetFocus();m.player.clear();
            // A scene gap (login, loading, respawn) always earns a settle hold
            // before the next actor is used.
            s.needsSettle=true;
            s.status="Chờ nhân vật local";m.status=s.status;return;
        }
        auto uid=std::to_string(s.read<std::int64_t>(actor,s.field(s.a.object_get_class(actor),"<SUID>k__BackingField"),0x0a));
        if(actor!=s.actorRoot.get()||uid!=s.actorUID){
            const bool hadActor=s.actorRoot.get()!=nullptr;
            s.resetFocus();s.actorRoot.set(s.a,actor);s.actorUID=uid;
            // Character hand-off means a scene/controller change; let the new
            // scene finish before touching cameras, transforms or controllers.
            if(hadActor||s.needsSettle){s.settleUntil=time+.75;s.needsSettle=false;}
        }
        m.player=uid;
        if(time<s.settleUntil){s.points.clear();s.status="Đang chờ cảnh ổn định";m.status=s.status;return;}
        std::string detail;
        if(m.enabled[SpamJump]){
            try{s.jumpTick(actor,time,m.jumpIntervalMs);}
            catch(const std::exception&e){s.nextJump=time+.25;detail=std::string("Spam nhảy: ")+e.what();}
        }else s.controlRoot.reset();
        if(m.enabled[Esp]){
            try{s.espTick(time);}
            catch(const std::exception&e){s.points.clear();s.needsSettle=true;if(!detail.empty())detail+=" | ";detail+=std::string("ESP: ")+e.what();}
        }else s.points.clear();
        s.retryAfter=0;
        m.status=detail.empty()?"ESP / Spam nhảy • input gốc của game":detail;s.status=m.status;
    }catch(const std::exception&e){
        // Any failure at the scene/camera level: drop every managed handle and
        // back off for a moment instead of hammering a half-destroyed scene.
        s.points.clear();s.resetFocus();m.player.clear();
        s.needsSettle=true;
        s.status=std::string("Chờ cảnh: ")+e.what();m.status=s.status;
        s.retryAfter=std::isfinite(time)?time+.75:0;
    }catch(...){
        s.points.clear();s.resetFocus();m.player.clear();
        s.needsSettle=true;
        s.status="Backend lỗi không xác định";m.status=s.status;
        s.retryAfter=std::isfinite(time)?time+.75:0;
    }
}
}
