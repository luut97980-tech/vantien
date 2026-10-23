#include "game_backend.hpp"
#include "fake_il2cpp.hpp"
#include <cassert>
#include <stdexcept>
#include <string>
int main(){
    static_assert(vt::FeatureCount==2);
    bool rejected=false;
    try{vt::ios::GameBackend missing([](const char*)->void*{return nullptr;});}
    catch(const std::exception&e){rejected=std::string(e.what()).find("Missing il2cpp_domain_get")!=std::string::npos;}
    assert(rejected);
    using namespace fake;
    auto&ac=cls("ActorCharacter");auto&base=cls("ActorControl");auto&cc=cls("ActorDefaultControlPlayer");cc.parent=&base;
    auto&actorControl=field(ac,"actorControl",0x12,8);auto&uid=field(ac,"<SUID>k__BackingField",0x0a,8);
    auto&actorBase=field(base,"actorBase",0x12,8);auto&terminate=field(base,"_terminate",2,1);
    auto&transform=field(ac,"baseTransfrom",0x12,8);field(cls("ActorSystem"),"Self",0x12,8);
    Object sys{&cls("ActorSystem")},act{&ac},control{&cc},nextActor{&ac},nextControl{&cc};
    Object tr{&cls("Transform","UnityEngine")},cam{&cls("Camera","UnityEngine")};camera=&cam;
    fake::system=&sys;actor=&act;set(act,actorControl,&control);set(act,uid,std::int64_t(42));
    set(control,actorBase,&act);set(control,terminate,false);set(act,transform,&tr);
    set(nextActor,actorControl,&nextControl);set(nextActor,uid,std::int64_t(43));
    set(nextControl,actorBase,&nextActor);set(nextControl,terminate,false);set(nextActor,transform,&tr);
    {
        vt::ios::GameBackend backend(resolve);vt::ios::Model m;
        m.available.fill(true);m.connected=m.controlsAllowed=true;m.jumpIntervalMs=20;
        backend.toggle(m,vt::SpamJump);backend.tick(m,0);assert(jumps==1&&lastJumpTarget==&control&&m.player=="42");
        backend.tick(m,.01);assert(jumps==1);backend.tick(m,.02);assert(jumps==2);
        backend.tick(m,20);assert(jumps==3); // No catch-up burst after a long gap.
        backend.tick(m,20);assert(jumps==3);
        m.jumpIntervalMs=0;backend.tick(m,20.02);assert(jumps==4);
        backend.tick(m,20.02);assert(jumps==4); // Zero delay still never spins.
        backend.tick(m,20.04);assert(jumps==5);
        backend.toggle(m,vt::SpamJump);backend.tick(m,21);assert(jumps==5);
        backend.toggle(m,vt::Esp);backend.tick(m,22);assert(jumps==5&&backend.esp().empty());
        backend.toggle(m,vt::SpamJump);backend.tick(m,23);assert(jumps==6);
        set(control,terminate,true);backend.tick(m,24);assert(jumps==6);
        set(control,terminate,false);backend.tick(m,25);assert(jumps==7);
        act.alive=false;backend.tick(m,26);assert(jumps==7&&m.player.empty()&&roots.empty());
        actor=&nextActor;backend.tick(m,27);assert(jumps==7&&m.player=="43"); // scene gap holds the new scene for a moment
        backend.tick(m,27.8);assert(jumps==8&&lastJumpTarget==&nextControl);
        jumpException=true;backend.tick(m,28);assert(jumps==8&&m.enabled[vt::SpamJump]);
        assert(m.status.find("raised an exception")!=std::string::npos);
        jumpException=false;backend.tick(m,28.1);assert(jumps==8);backend.tick(m,28.3);assert(jumps==9);
        backend.tick(m,.1);assert(jumps==10); // Clock rollback resets the schedule.
        m.jumpIntervalMs=-1;backend.tick(m,.2);assert(jumps==10&&m.status.find("Invalid spam interval")!=std::string::npos);
        backend.stop(m);backend.tick(m,30);assert(jumps==10&&!m.enabled[vt::Esp]&&!m.enabled[vt::SpamJump]);
        backend.stop(m);assert(roots.empty());
        backend.toggle(m,vt::FeatureCount);assert(!m.enabled[0]&&!m.enabled[1]);
        m.connected=false;backend.tick(m,31);assert(roots.empty());
    }
    assert(roots.empty());
}
