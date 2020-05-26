#include "EventSinks.h"
#include "EventsApi.h"
#include "JsEngine.h"
#include "NativeValueCasts.h"
#include <RE/ActiveEffect.h>
#include <RE/Actor.h>

extern TaskQueue g_taskQueue;

struct RE::TESActivateEvent
{
  NiPointer<TESObjectREFR> target;
  NiPointer<TESObjectREFR> caster;
};

namespace {
JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESActivateEvent* event_,
  RE::BSTEventSource<RE::TESActivateEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto target = CreateObject("ObjectReference", event.target.get());
    auto caster = CreateObject("ObjectReference", event.caster.get());

    EventsApi::SendEvent("activate", { JsValue::Undefined(), target, caster });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event_,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto movedRef = CreateObject("ObjectReference", event.movedRef.get());
    auto isCellAttached = JsValue::Bool(event.isCellAttached);

    EventsApi::SendEvent("moveAttachDetach",
                         { JsValue::Undefined(), movedRef, isCellAttached });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESWaitStopEvent* event_,
  RE::BSTEventSource<RE::TESWaitStopEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto interrupted = JsValue::Bool(event.interrupted);

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), interrupted });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESObjectLoadedEvent* event_,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto objectForm = RE::TESForm::LookupByID(event.formID);
    auto object = CreateObject("Form", objectForm);
    auto isLoaded = JsValue::Bool(event.loaded);

    EventsApi::SendEvent("objectLoaded",
                         { JsValue::Undefined(), object, isLoaded });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESLockChangedEvent* event_,
  RE::BSTEventSource<RE::TESLockChangedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto lockedObject =
      CreateObject("ObjectReference", event.lockedObject.get());
    EventsApi::SendEvent("lockChanged",
                         { JsValue::Undefined(), lockedObject });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event_,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto cell = CreateObject("Cell", event.cell);

    EventsApi::SendEvent("cellFullyLoaded", { JsValue::Undefined(), cell });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESGrabReleaseEvent* event_,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto ref = CreateObject("ObjectReference", event.ref.get());
    auto isGrabbed = JsValue::Bool(event.grabbed);

    EventsApi::SendEvent("grabRelease",
                         { JsValue::Undefined(), ref, isGrabbed });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESLoadGameEvent* event_,
  RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource)
{
  g_taskQueue.AddTask(
    [] { EventsApi::SendEvent("loadGame", { JsValue::Undefined() }); });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* event_,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto subject = CreateObject("ObjectReference", event.subject.get());
    EventsApi::SendEvent("switchRaceComplete",
                         { JsValue::Undefined(), subject });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event_,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto oldBaseID = JsValue::Double(event.oldBaseID);
    auto newBaseID = JsValue::Double(event.newBaseID);
    EventsApi::SendEvent("uniqueIdChange",
                         { JsValue::Undefined(), oldBaseID, newBaseID });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESTrackedStatsEvent* event_,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto stat = JsValue::String(event.stat.data());
    auto value = JsValue::Double(event.value);
    EventsApi::SendEvent("trackedStats",
                         { JsValue::Undefined(), stat, value });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESInitScriptEvent* event_,
  RE::BSTEventSource<RE::TESInitScriptEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto objectInitialized =
      CreateObject("ObjectReference", event.objectInitialized.get());

    EventsApi::SendEvent("scriptInit",
                         { JsValue::Undefined(), objectInitialized });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESResetEvent* event_,
  RE::BSTEventSource<RE::TESResetEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto object = CreateObject("ObjectReference", event.object.get());

    EventsApi::SendEvent("reset", { JsValue::Undefined(), object });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCombatEvent* event_,
  RE::BSTEventSource<RE::TESCombatEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto target = CreateObject("ObjectReference", event.targetActor.get());
    auto actor = CreateObject("ObjectReference", event.actor.get());

    auto isCombat = JsValue::Bool((uint8_t)event.state &
                                  (uint8_t)RE::ACTOR_COMBAT_STATE::kCombat);
    auto isSearching = JsValue::Bool(
      (uint8_t)event.state & (uint8_t)RE::ACTOR_COMBAT_STATE::kSearching);

    EventsApi::SendEvent(
      "combatState",
      { JsValue::Undefined(), actor, target, isCombat, isSearching });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESDeathEvent* event_,
  RE::BSTEventSource<RE::TESDeathEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto actorDying = CreateObject("ObjectReference", event.actorDying.get());
    auto actorKiller =
      CreateObject("ObjectReference", event.actorKiller.get());

    auto dead = JsValue::Bool(event.dead);

    EventsApi::SendEvent(
      "death", { JsValue::Undefined(), actorDying, actorKiller, dead });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESContainerChangedEvent* event_,
  RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto oldContainerRef = RE::TESForm::LookupByID(event.oldContainer);
    auto oldContainer = CreateObject("ObjectReference", oldContainerRef);

    auto newContainerRef = RE::TESForm::LookupByID(event.newContainer);
    auto newContainer = CreateObject("ObjectReference", newContainerRef);

    auto baseObjForm = RE::TESForm::LookupByID(event.baseObj);
    auto baseObj = CreateObject("Form", baseObjForm);

    auto itemCount = JsValue::Double(event.itemCount);

    EventsApi::SendEvent("containerChanged",
                         { JsValue::Undefined(), oldContainer, newContainer,
                           baseObj, itemCount });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESHitEvent* event_,
  RE::BSTEventSource<RE::TESHitEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto target = CreateObject("ObjectReference", event.target.get());
    auto agressor = CreateObject("ObjectReference", event.cause.get());

    auto sourceForm = RE::TESForm::LookupByID(event.source);
    auto source = CreateObject("Form", sourceForm);

    auto projectileForm = RE::TESForm::LookupByID(event.projectile);
    auto projectile = CreateObject("Projectile", projectileForm);

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

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESEquipEvent* event_,
  RE::BSTEventSource<RE::TESEquipEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto actor = CreateObject("ObjectReference", event.actor.get());

    auto baseObjectForm = RE::TESForm::LookupByID(event.baseObject);
    auto baseObject = CreateObject("Form", baseObjectForm);

    auto equipped = JsValue::Bool(event.equipped);

    EventsApi::SendEvent(
      "equip", { JsValue::Undefined(), actor, baseObject, equipped });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event_,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* eventSource)
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

    auto activeMgef = CreateObject("ActiveMagicEffect", activeEffect);
    auto caster = CreateObject("ObjectReference", event.caster.get());
    auto target = CreateObject("ObjectReference", event.target.get());

    auto isApplied = JsValue::Bool(event.isApplied);

    EventsApi::SendEvent(
      "activeEffectApplyRemove",
      { JsValue::Undefined(), activeMgef, caster, target, isApplied });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event_,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto magicEffect =
      CreateObject("MagicEffect", RE::TESForm::LookupByID(event.magicEffect));

    auto caster = CreateObject("ObjectReference", event.caster.get());
    auto target = CreateObject("ObjectReference", event.target.get());

    EventsApi::SendEvent(
      "magicEffectApply",
      { JsValue::Undefined(), magicEffect, caster, target });
  });
  return RE::BSEventNotifyControl::kContinue;
}