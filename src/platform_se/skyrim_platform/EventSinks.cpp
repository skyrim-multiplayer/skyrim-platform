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
    auto obj = JsValue::Object();

    obj.SetProperty("target",
                    CreateObject("ObjectReference", event.target.get()));
    obj.SetProperty("caster",
                    CreateObject("ObjectReference", event.caster.get()));

    EventsApi::SendEvent("activate", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event_,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();

    obj.SetProperty("movedRef",
                    CreateObject("ObjectReference", event.movedRef.get()));

    obj.SetProperty("isCellAttached", JsValue::Bool(event.isCellAttached));
    EventsApi::SendEvent("moveAttachDetach", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESWaitStopEvent* event_,
  RE::BSTEventSource<RE::TESWaitStopEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("isInterrupted", JsValue::Bool(event.interrupted));

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESObjectLoadedEvent* event_,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty(
      "object", CreateObject("Form", RE::TESForm::LookupByID(event.formID)));
    obj.SetProperty("isLoaded", JsValue::Bool(event.loaded));

    EventsApi::SendEvent("objectLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESLockChangedEvent* event_,
  RE::BSTEventSource<RE::TESLockChangedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("lockedObject",
                    CreateObject("ObjectReference", event.lockedObject.get()));

    EventsApi::SendEvent("lockChanged", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event_,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("cell", CreateObject("Cell", event.cell));

    EventsApi::SendEvent("cellFullyLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESGrabReleaseEvent* event_,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("refr", CreateObject("ObjectReference", event.ref.get()));
    obj.SetProperty("isGrabbed", JsValue::Bool(event.grabbed));

    EventsApi::SendEvent("grabRelease", { JsValue::Undefined(), obj });
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
    auto obj = JsValue::Object();
    obj.SetProperty("subject",
                    CreateObject("ObjectReference", event.subject.get()));

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event_,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("oldBaseID", JsValue::Double(event.oldBaseID));
    obj.SetProperty("newBaseID", JsValue::Double(event.newBaseID));

    EventsApi::SendEvent("uniqueIdChange", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESTrackedStatsEvent* event_,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("statName", JsValue::String(event.stat.data()));
    obj.SetProperty("newValue", JsValue::Double(event.value));

    EventsApi::SendEvent("trackedStats", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESInitScriptEvent* event_,
  RE::BSTEventSource<RE::TESInitScriptEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty(
      "initializedObject",
      CreateObject("ObjectReference", event.objectInitialized.get()));

    EventsApi::SendEvent("scriptInit", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESResetEvent* event_,
  RE::BSTEventSource<RE::TESResetEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("object",
                    CreateObject("ObjectReference", event.object.get()));

    EventsApi::SendEvent("reset", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCombatEvent* event_,
  RE::BSTEventSource<RE::TESCombatEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("target",
                    CreateObject("ObjectReference", event.targetActor.get()));
    obj.SetProperty("actor",
                    CreateObject("ObjectReference", event.actor.get()));
    obj.SetProperty("isCombat",
                    JsValue::Bool((uint8_t)event.state &
                                  (uint8_t)RE::ACTOR_COMBAT_STATE::kCombat));
    obj.SetProperty(
      "isSearching",
      JsValue::Bool((uint8_t)event.state &
                    (uint8_t)RE::ACTOR_COMBAT_STATE::kSearching));

    EventsApi::SendEvent("combatState", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESDeathEvent* event_,
  RE::BSTEventSource<RE::TESDeathEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("actorDying",
                    CreateObject("ObjectReference", event.actorDying.get()));
    obj.SetProperty("actorKiller",
                    CreateObject("ObjectReference", event.actorKiller.get()));

    event.dead
      ? EventsApi::SendEvent("deathEnd", { JsValue::Undefined(), obj })
      : EventsApi::SendEvent("deathStart", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESContainerChangedEvent* event_,
  RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();

    auto oldContainerRef = RE::TESForm::LookupByID(event.oldContainer);
    obj.SetProperty("oldContainer",
                    CreateObject("ObjectReference", oldContainerRef));

    auto newContainerRef = RE::TESForm::LookupByID(event.newContainer);
    obj.SetProperty("newContainer",
                    CreateObject("ObjectReference", newContainerRef));

    auto baseObjForm = RE::TESForm::LookupByID(event.baseObj);
    obj.SetProperty("baseObj", CreateObject("Form", baseObjForm));
    obj.SetProperty("numItems", JsValue::Double(event.itemCount));

    EventsApi::SendEvent("containerChanged", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESHitEvent* event_,
  RE::BSTEventSource<RE::TESHitEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("target",
                    CreateObject("ObjectReference", event.target.get()));
    obj.SetProperty("agressor",
                    CreateObject("ObjectReference", event.cause.get()));

    auto sourceForm = RE::TESForm::LookupByID(event.source);
    obj.SetProperty("source", CreateObject("Form", sourceForm));

    auto projectileForm = RE::TESForm::LookupByID(event.projectile);
    obj.SetProperty("projectile", CreateObject("Form", projectileForm));

    obj.SetProperty(
      "isPowerAttack",
      JsValue::Bool((uint8_t)event.flags &
                    (uint8_t)RE::TESHitEvent::Flag::kPowerAttack));

    obj.SetProperty(
      "isSneakAttack",
      JsValue::Bool((uint8_t)event.flags &
                    (uint8_t)RE::TESHitEvent::Flag::kSneakAttack));

    obj.SetProperty(
      "isBashAttack",
      JsValue::Bool((uint8_t)event.flags &
                    (uint8_t)RE::TESHitEvent::Flag::kBashAttack));

    obj.SetProperty(
      "isHitBlocked",
      JsValue::Bool((uint8_t)event.flags &
                    (uint8_t)RE::TESHitEvent::Flag::kHitBlocked));

    EventsApi::SendEvent("hit", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESEquipEvent* event_,
  RE::BSTEventSource<RE::TESEquipEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();
    obj.SetProperty("actor",
                    CreateObject("ObjectReference", event.actor.get()));

    auto baseObjectForm = RE::TESForm::LookupByID(event.baseObject);
    obj.SetProperty("baseObj", CreateObject("Form", baseObjectForm));

    event.equipped
      ? EventsApi::SendEvent("equip", { JsValue::Undefined(), obj })
      : EventsApi::SendEvent("unEquip", { JsValue::Undefined(), obj });
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

    if (actor->formType != RE::FormType::ActorCharacter)
      return;

    auto obj = JsValue::Object();

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

    obj.SetProperty("effect", CreateObject("ActiveMagicEffect", activeEffect));
    obj.SetProperty("caster",
                    CreateObject("ObjectReference", event.caster.get()));
    obj.SetProperty("target",
                    CreateObject("ObjectReference", event.target.get()));

    event.isApplied
      ? EventsApi::SendEvent("effectStart", { JsValue::Undefined(), obj })
      : EventsApi::SendEvent("effectFinish", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event_,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource)
{
  auto event = *event_;
  g_taskQueue.AddTask([event] {
    auto obj = JsValue::Object();

  auto effect = RE::TESForm::LookupByID(event.magicEffect);

  if (effect && effect->formType != RE::FormType::MagicEffect)
    return;

    obj.SetProperty(
      "effect", CreateObject("MagicEffect", effect));

    obj.SetProperty("caster",
                    CreateObject("ObjectReference", event.caster.get()));
    obj.SetProperty("target",
                    CreateObject("ObjectReference", event.target.get()));

    EventsApi::SendEvent("magicEffectApply", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}