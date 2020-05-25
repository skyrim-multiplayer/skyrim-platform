#include "EventsApi.h"

#include "InvalidArgumentException.h"
#include "MyUpdateTask.h"
#include "NativeObject.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include <RE/Actor.h>
#include <RE/ScriptEventSourceHolder.h>
#include <RE/TESActiveEffectApplyRemoveEvent.h>
#include <RE/TESContainerChangedEvent.h>
#include <RE/TESDeathEvent.h>
#include <RE/TESEquipEvent.h>
#include <RE/TESHitEvent.h>
#include <RE/TESMagicEffectApplyEvent.h>
#include <RE\ActiveEffect.h>
#include <RE\ConsoleLog.h>
#include <map>
#include <set>
#include <unordered_map>
extern ctpl::thread_pool g_pool;
extern TaskQueue g_taskQueue;

struct EventsGlobalState
{
  using Callbacks = std::map<std::string, std::vector<JsValue>>;
  Callbacks callbacks;
  Callbacks callbacksOnce;

  class Handler
  {
  public:
    Handler() = default;

    Handler(const JsValue& handler_)
      : enter(handler_.GetProperty("enter"))
      , leave(handler_.GetProperty("leave"))
    {
    }

    JsValue enter, leave;

    struct PerThread
    {
      JsValue storage, context;
    };

    std::unordered_map<DWORD, PerThread> perThread;
  };

  struct HookInfo
  {
    std::set<DWORD> inProgressThreads;
    std::vector<Handler> handlers;
  };
  HookInfo sendAnimationEvent;
} g;

namespace {
struct SendAnimationEventTag
{
  static constexpr auto name = "sendAnimationEvent";
};
}

namespace {
void CallCalbacks(const char* eventName, const std::vector<JsValue>& arguments,
                  bool isOnce = false)
{
  EventsGlobalState::Callbacks callbacks =
    isOnce ? g.callbacksOnce : g.callbacks;

  if (isOnce)
    g.callbacksOnce[eventName].clear();

  for (auto& f : callbacks[eventName]) {
    f.Call(arguments);
  }
}
}

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<JsValue>& arguments)
{
  CallCalbacks(eventName, arguments);
  CallCalbacks(eventName, arguments, true);
}

void EventsApi::Clear()
{
  g = {};
}

namespace {
enum class ClearStorage
{
  Yes,
  No
};

void PrepareContext(EventsGlobalState::Handler::PerThread& h,
                    ClearStorage clearStorage)
{
  if (h.context.GetType() != JsValue::Type::Object) {
    h.context = JsValue::Object();
  }

  thread_local auto g_standardMap = JsValue::GlobalObject().GetProperty("Map");
  thread_local auto g_clear =
    g_standardMap.GetProperty("prototype").GetProperty("clear");
  if (h.storage.GetType() != JsValue::Type::Object) {
    h.storage = g_standardMap.Constructor({ g_standardMap });
    h.context.SetProperty("storage", h.storage);
  }

  if (clearStorage == ClearStorage::Yes)
    g_clear.Call({ h.storage });
}
}

void EventsApi::SendAnimationEventEnter(uint32_t selfId,
                                        std::string& animEventName) noexcept
{
  DWORD owningThread = GetCurrentThreadId();
  auto f = [&](int) {
    try {
      if (g.sendAnimationEvent.inProgressThreads.count(owningThread))
        throw std::runtime_error("'sendAnimationEvent' is already processing");

      // This should always be done before calling throwing functions
      g.sendAnimationEvent.inProgressThreads.insert(owningThread);

      for (auto& h : g.sendAnimationEvent.handlers) {
        auto& perThread = h.perThread[owningThread];
        PrepareContext(perThread, ClearStorage::Yes);

        perThread.context.SetProperty("selfId", (double)selfId);
        perThread.context.SetProperty("animEventName", animEventName);

        h.enter.Call({ JsValue::Undefined(), perThread.context });

        animEventName =
          (std::string)perThread.context.GetProperty("animEventName");
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in SendAnimationEventEnter)");
      });
    }
  };
  g_pool.push(f).wait();
}

void EventsApi::SendAnimationEventLeave(bool animationSucceeded) noexcept
{
  DWORD owningThread = GetCurrentThreadId();
  auto f = [&](int) {
    try {
      if (!g.sendAnimationEvent.inProgressThreads.count(owningThread))
        throw std::runtime_error("'sendAnimationEvent' is not processing");
      g.sendAnimationEvent.inProgressThreads.erase(owningThread);

      for (auto& h : g.sendAnimationEvent.handlers) {
        auto& perThread = h.perThread.at(owningThread);
        PrepareContext(perThread, ClearStorage::No);

        perThread.context.SetProperty("animationSucceeded",
                                      JsValue::Bool(animationSucceeded));
        h.leave.Call({ JsValue::Undefined(), perThread.context });

        h.perThread.erase(owningThread);
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in SendAnimationEventLeave)");
      });
    }
  };
  g_pool.push(f).wait();
}

namespace {
JsValue CreateHook(EventsGlobalState::HookInfo* hookInfo)
{
  auto hook = JsValue::Object();
  hook.SetProperty(
    "add", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      auto handlerObj = args[1];
      hookInfo->handlers.push_back(EventsGlobalState::Handler(handlerObj));
      return JsValue::Undefined();
    }));
  return hook;
}
}

JsValue EventsApi::GetHooks()
{
  std::map<std::string, EventsGlobalState::HookInfo*> hooksMap = {
    { "sendAnimationEvent", &g.sendAnimationEvent }
  };

  auto hooks = JsValue::Object();
  for (auto [name, hookInfo] : hooksMap)
    hooks.SetProperty(name, CreateHook(hookInfo));
  return hooks;
}

namespace {
class EventSinks
  : public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>
  , public RE::BSTEventSink<RE::TESLoadGameEvent>
  , public RE::BSTEventSink<RE::TESEquipEvent>
  , public RE::BSTEventSink<RE::TESHitEvent>
  , public RE::BSTEventSink<RE::TESContainerChangedEvent>
  , public RE::BSTEventSink<RE::TESDeathEvent>
  , public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>
{
public:
  EventSinks()
  {
    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder)
      throw NullPointerException("holder");

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>*>(
        this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESLoadGameEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESEquipEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESHitEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESContainerChangedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESDeathEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESMagicEffectApplyEvent>*>(this));
  }

private:
  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESLoadGameEvent* event_,
    RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource) override
  {
    g_taskQueue.AddTask(
      [] { EventsApi::SendEvent("loadGame", { JsValue::Undefined() }); });

    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESDeathEvent* event_,
    RE::BSTEventSource<RE::TESDeathEvent>* eventSource) override
  {
    auto event = *event_;
    g_taskQueue.AddTask([event] {
      auto actorDying = event.actorDying
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("ObjectReference",
                                                 event.actorDying.get()))
        : JsValue::Null();
      auto actorKiller = event.actorKiller
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("ObjectReference",
                                                 event.actorKiller.get()))
        : JsValue::Null();

      auto dead = JsValue::Bool(event.dead);

      EventsApi::SendEvent(
        "death", { JsValue::Undefined(), actorDying, actorKiller, dead });
    });
    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESContainerChangedEvent* event_,
    RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource) override
  {
    auto event = *event_;
    g_taskQueue.AddTask([event] {
      auto oldContainerRef = RE::TESForm::LookupByID(event.oldContainer);
      auto oldContainer = oldContainerRef
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("ObjectReference",
                                                 oldContainerRef))
        : JsValue::Null();

      auto newContainerRef = RE::TESForm::LookupByID(event.newContainer);
      auto newContainer = newContainerRef
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("ObjectReference",
                                                 newContainerRef))
        : JsValue::Null();

      auto baseObjForm = RE::TESForm::LookupByID(event.baseObj);
      auto baseObj = baseObjForm
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("Form", baseObjForm))
        : JsValue::Null();

      auto itemCount = JsValue::Double(event.itemCount);

      EventsApi::SendEvent("containerChanged",
                           { JsValue::Undefined(), oldContainer, newContainer,
                             baseObj, itemCount });
    });
    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESHitEvent* event_,
    RE::BSTEventSource<RE::TESHitEvent>* eventSource) override
  {
    auto event = *event_;
    g_taskQueue.AddTask([event] {
      auto target = event.target ? NativeValueCasts::NativeObjectToJsObject(
                                     std::make_shared<CallNative::Object>(
                                       "ObjectReference", event.target.get()))
                                 : JsValue::Null();
      auto agressor = event.cause ? NativeValueCasts::NativeObjectToJsObject(
                                      std::make_shared<CallNative::Object>(
                                        "ObjectReference", event.cause.get()))
                                  : JsValue::Null();
      auto sourceForm = RE::TESForm::LookupByID(event.source);
      auto source = sourceForm
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("Form", sourceForm))
        : JsValue::Null();

      auto projectileForm = RE::TESForm::LookupByID(event.projectile);
      auto projectile = projectileForm
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("Projectile", projectileForm))
        : JsValue::Null();

      auto isPowerAttack = JsValue::Bool(
        (uint8_t)event.flags & (uint8_t)RE::TESHitEvent::Flag::kPowerAttack);
      auto isSneakAttack = JsValue::Bool(
        (uint8_t)event.flags & (uint8_t)RE::TESHitEvent::Flag::kSneakAttack);
      auto isBashAttack = JsValue::Bool(
        (uint8_t)event.flags & (uint8_t)RE::TESHitEvent::Flag::kBashAttack);
      auto isHitBlocked = JsValue::Bool(
        (uint8_t)event.flags & (uint8_t)RE::TESHitEvent::Flag::kHitBlocked);

      EventsApi::SendEvent("hit",
                           { JsValue::Undefined(), target, agressor, source,
                             projectile, isPowerAttack, isSneakAttack,
                             isBashAttack, isHitBlocked });
    });
    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESEquipEvent* event_,
    RE::BSTEventSource<RE::TESEquipEvent>* eventSource) override
  {
    auto event = *event_;
    g_taskQueue.AddTask([event] {
      auto actor = event.actor ? NativeValueCasts::NativeObjectToJsObject(
                                   std::make_shared<CallNative::Object>(
                                     "ObjectReference", event.actor.get()))
                               : JsValue::Null();

      auto baseObjectForm = RE::TESForm::LookupByID(event.baseObject);
      auto baseObject = baseObjectForm
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("Form", baseObjectForm))
        : JsValue::Null();

      auto equipped = JsValue::Bool(event.equipped);

      EventsApi::SendEvent(
        "equip", { JsValue::Undefined(), actor, baseObject, equipped });
    });

    return RE::BSEventNotifyControl::kContinue;
  }

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESActiveEffectApplyRemoveEvent* event_,
    RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* eventSource)
    override
  {
    auto event = *event_;
    g_taskQueue.AddTask([event] {
      auto actor = reinterpret_cast<RE::Actor*>(event.target.get());
      if (!actor)
        throw NullPointerException("actor");

      auto effectList = actor->GetActiveEffectList();
      if (!effectList)
        throw NullPointerException("effectList");

      RE::ActiveEffect* activeEffect = nullptr;
      for (auto effect : *effectList) {
        if (effect && effect->usUniqueID == event.activeEffectUniqueID) {
          activeEffect = effect;
          break;
        }
      }

      auto activeMgef = activeEffect
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("ActiveMagicEffect",
                                                 activeEffect))
        : JsValue::Null();

      auto caster = event.caster ? NativeValueCasts::NativeObjectToJsObject(
                                     std::make_shared<CallNative::Object>(
                                       "ObjectReference", event.caster.get()))
                                 : JsValue::Null();
      auto target = event.target ? NativeValueCasts::NativeObjectToJsObject(
                                     std::make_shared<CallNative::Object>(
                                       "ObjectReference", event.target.get()))
                                 : JsValue::Null();

      auto isApplied = JsValue::Bool(event.isApplied);

      EventsApi::SendEvent(
        "activeEffectApplyRemove",
        { JsValue::Undefined(), activeMgef, caster, target, isApplied });
    });
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event_,
    RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource) override
  {
    auto event = *event_;
    g_taskQueue.AddTask([event] {
      auto effect = reinterpret_cast<RE::EffectSetting*>(
        RE::TESForm::LookupByID(event.magicEffect));

      auto magicEffect = effect
        ? NativeValueCasts::NativeObjectToJsObject(
            std::make_shared<CallNative::Object>("MagicEffect", effect))
        : JsValue::Null();

      auto caster = event.caster ? NativeValueCasts::NativeObjectToJsObject(
                                     std::make_shared<CallNative::Object>(
                                       "ObjectReference", event.caster.get()))
                                 : JsValue::Null();
      auto target = event.target ? NativeValueCasts::NativeObjectToJsObject(
                                     std::make_shared<CallNative::Object>(
                                       "ObjectReference", event.target.get()))
                                 : JsValue::Null();

      EventsApi::SendEvent(
        "magicEffectApply",
        { JsValue::Undefined(), magicEffect, caster, target });
    });
    return RE::BSEventNotifyControl::kContinue;
  }
};

JsValue AddCallback(const JsFunctionArguments& args, bool isOnce = false)
{
  static EventSinks g_sinks;

  auto eventName = args[1].ToString();
  auto callback = args[2];

  std::set<std::string> events = {
    "tick",    "update", "activeEffectApplyRemove", "magicEffectApply",
    "equip",   "hit",    "containerChanged",        "death",
    "loadGame"
  };

  if (events.count(eventName) == 0)
    throw InvalidArgumentException("eventName", eventName);

  isOnce ? g.callbacksOnce[eventName].push_back(callback)
         : g.callbacks[eventName].push_back(callback);
  return JsValue::Undefined();
}
}

JsValue EventsApi::On(const JsFunctionArguments& args)
{
  return AddCallback(args);
}

JsValue EventsApi::Once(const JsFunctionArguments& args)
{
  return AddCallback(args, true);
}
