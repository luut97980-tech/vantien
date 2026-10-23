#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
namespace fake {
struct Field {std::string name;int type;std::size_t size;};
struct Class {std::string name,ns;Class*parent=nullptr;std::map<std::string,Field> fields;};
struct Object {Class*klass;bool alive=true;std::map<Field*,std::array<unsigned char,16>> data;std::array<unsigned char,16> boxed{};explicit Object(Class*c):klass(c){}};
struct Method{std::string name;};
inline std::map<std::string,Class> classes;
inline std::map<std::string,Method> methods;
inline std::map<std::uintptr_t,Object*> roots;
inline std::uintptr_t nextRoot=std::uintptr_t(1)<<33;
inline Object *system=nullptr,*actor=nullptr;
inline Object boolean{nullptr},box{nullptr};
inline Object*camera=nullptr;inline std::uint64_t jumps=0;inline Object*lastJumpTarget=nullptr;inline bool jumpException=false;
inline Class& cls(const char*n,const char*ns=""){auto&c=classes[n];c.name=n;c.ns=ns;return c;}
inline Field&field(Class&c,const char*n,int type,std::size_t size){auto&f=c.fields[n];f={n,type,size};return f;}
template<class T>void set(Object&o,Field&f,T value){assert(sizeof(T)==f.size);std::memcpy(o.data[&f].data(),&value,sizeof(value));}
template<class T>T get(Object&o,Field&f){T value{};std::memcpy(&value,o.data[&f].data(),sizeof(value));return value;}
inline void*domain_get(){return &classes;}
inline void**domain_get_assemblies(void*,std::size_t*n){static void*arr[]={reinterpret_cast<void*>(1),reinterpret_cast<void*>(2)};*n=2;return arr;}
inline void*assembly_get_image(void*p){return p;}
inline const char*image_get_name(void*p){return p==reinterpret_cast<void*>(1)?"Assembly-CSharp.dll":"UnityEngine.CoreModule.dll";}
inline void*class_from_name(void*,const char*ns,const char*n){return &cls(n,ns);}
inline void*class_get_parent(void*c){return static_cast<Class*>(c)->parent;}
inline const char*class_get_name(void*c){return static_cast<Class*>(c)->name.c_str();}
inline const char*class_get_namespace(void*c){return static_cast<Class*>(c)->ns.c_str();}
inline const void*class_get_method_from_name(void*,const char*n,int){auto&m=methods[n];m.name=n;return &m;}
inline void*class_get_field_from_name(void*c,const char*n){auto&fields=static_cast<Class*>(c)->fields;auto i=fields.find(n);return i==fields.end()?nullptr:&i->second;}
inline const void*field_get_type(void*f){return f;}
inline int type_get_type(const void*f){return static_cast<const Field*>(f)->type;}
inline void field_get_value(void*o,void*f,void*out){auto&obj=*static_cast<Object*>(o);auto&field=*static_cast<Field*>(f);std::memcpy(out,obj.data[&field].data(),field.size);}
inline void field_static_get_value(void*,void*out){std::memcpy(out,&system,sizeof(system));}
inline void*runtime_invoke(const void*m,void*object,void**args,void**ex){
    *ex=nullptr;const auto&name=static_cast<const Method*>(m)->name;
    bool result=false;
    if(name=="op_Implicit")result=args[0]&&static_cast<Object*>(args[0])->alive;
    else if(name=="get_MyActorCharacter")return actor;
    else if(name=="OnJump"){lastJumpTarget=static_cast<Object*>(object);if(jumpException){*ex=&boolean;return nullptr;}++jumps;return nullptr;}
    else if(name=="get_position"){box.boxed.fill(0);return &box;}
    else if(name=="get_main")return camera;
    else if(name=="get_rect"){float rect[4]={0,0,1,1};std::memcpy(box.boxed.data(),rect,16);return &box;}
    else if(name=="get_OtherActorCharacter")return nullptr;
    else{assert(false&&"Unexpected managed method");}
    std::memcpy(boolean.boxed.data(),&result,sizeof(result));return &boolean;
}
inline void*object_get_class(void*o){return static_cast<Object*>(o)->klass;}
inline void*object_unbox(void*o){return static_cast<Object*>(o)->boxed.data();}
inline std::uintptr_t gchandle_new(void*o,bool){auto id=nextRoot++;roots[id]=static_cast<Object*>(o);return id;}
inline void*gchandle_get_target(std::uintptr_t id){return roots.at(id);}
inline void gchandle_free(std::uintptr_t id){assert(roots.erase(id)==1);}
inline void*array_new(void*,std::size_t){assert(false);return nullptr;}
inline std::size_t array_length(void*){assert(false);return 0;}
inline std::uint32_t array_object_header_size(){return 32;}
inline void*thread_current(){return &classes;}
inline void*thread_attach(void*){return &classes;}
inline void thread_detach(void*){assert(false);}
inline void*resolve(const char*n){
#define FN(x) if(std::strcmp(n,"il2cpp_" #x)==0)return reinterpret_cast<void*>(&x)
    FN(domain_get);FN(domain_get_assemblies);FN(assembly_get_image);FN(image_get_name);FN(class_from_name);
    FN(class_get_parent);FN(class_get_name);FN(class_get_namespace);FN(class_get_method_from_name);FN(class_get_field_from_name);
    FN(field_get_type);FN(type_get_type);FN(field_get_value);FN(field_static_get_value);FN(runtime_invoke);
    FN(object_get_class);FN(object_unbox);FN(gchandle_new);FN(gchandle_get_target);FN(gchandle_free);
    FN(array_new);FN(array_length);FN(array_object_header_size);FN(thread_current);FN(thread_attach);FN(thread_detach);
#undef FN
    return nullptr;
}
}
