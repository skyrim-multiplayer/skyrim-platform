#include "EventSinks.h"
#include "EventsApi.h"
#include "JsEngine.h"
#include "NativeValueCasts.h"
#include <RE/ActiveEffect.h>
#include <RE/Actor.h>
#include <RE/TESObjectCELL.h>

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
  auto targetRefr = event_ ? event_->target.get() : nullptr;
  auto casterRefr = event_ ? event_->caster.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto casterId = casterRefr ? casterRefr->formID : 0;

  g_taskQueue.AddTask([targetId, casterId] {
    auto obj = JsValue::Object();

    auto target = RE::TESForm::LookupByID(targetId);
    obj.SetProperty("target", CreateObject("ObjectReference", target));
    obj.SetProperty(
      "caster",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(casterId)));

    auto targetRefr = target && target->formType == RE::FormType::Reference
      ? reinterpret_cast<RE::TESObjectREFR*>(target)
      : nullptr;

    obj.SetProperty("isCrimeToActivate",
                    targetRefr ? JsValue::Bool(targetRefr->IsCrimeToActivate())
                               : JsValue::Undefined());

    EventsApi::SendEvent("activate", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* event_,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource)
{
  auto movedRef = event_ ? event_->movedRef.get() : nullptr;

  auto targetId = movedRef ? movedRef->formID : 0;
  auto isCellAttached = event_ ? event_->isCellAttached : 0;

  g_taskQueue.AddTask([targetId, isCellAttached] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "movedRef",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(targetId)));

    obj.SetProperty("isCellAttached", JsValue::Bool(isCellAttached));
    EventsApi::SendEvent("moveAttachDetach", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESWaitStopEvent* event_,
  RE::BSTEventSource<RE::TESWaitStopEvent>* eventSource)
{
  auto interrupted = event_ ? event_->interrupted : 0;

  g_taskQueue.AddTask([interrupted] {
    auto obj = JsValue::Object();

    obj.SetProperty("isInterrupted", JsValue::Bool(interrupted));

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESObjectLoadedEvent* event_,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>* eventSource)
{
  auto objectId = event_ ? event_->formID : 0;
  auto loaded = event_ ? event_->loaded : 0;

  g_taskQueue.AddTask([objectId, loaded] {
    auto obj = JsValue::Object();

    obj.SetProperty("object",
                    CreateObject("Form", RE::TESForm::LookupByID(objectId)));

    obj.SetProperty("isLoaded", JsValue::Bool(loaded));

    EventsApi::SendEvent("objectLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESLockChangedEvent* event_,
  RE::BSTEventSource<RE::TESLockChangedEvent>* eventSource)
{
  auto lockedObject = event_ ? event_->lockedObject : nullptr;
  auto lockedObjectId = lockedObject ? lockedObject->formID : 0;

  g_taskQueue.AddTask([lockedObjectId] {
    auto obj = JsValue::Object();

    obj.SetProperty("lockedObject",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(lockedObjectId)));

    EventsApi::SendEvent("lockChanged", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* event_,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* eventSource)
{
  auto cell = event_ ? event_->cell : nullptr;
  auto cellId = cell ? cell->formID : 0;

  g_taskQueue.AddTask([cellId] {
    auto obj = JsValue::Object();

    obj.SetProperty("cell",
                    CreateObject("Cell", RE::TESForm::LookupByID(cellId)));

    EventsApi::SendEvent("cellFullyLoaded", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESGrabReleaseEvent* event_,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>* eventSource)
{
  auto ref = event_ ? event_->ref.get() : nullptr;
  auto refId = ref ? ref->formID : 0;
  auto grabbed = event_ ? event_->grabbed : 0;

  g_taskQueue.AddTask([refId, grabbed] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "refr", CreateObject("ObjectReference", RE::TESForm::LookupByID(refId)));

    obj.SetProperty("isGrabbed", JsValue::Bool(grabbed));

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
  auto subject = event_ ? event_->subject.get() : nullptr;
  auto subjectId = subject ? subject->formID : 0;

  g_taskQueue.AddTask([subjectId] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "subject",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(subjectId)));

    EventsApi::SendEvent("switchRaceComplete", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* event_,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* eventSource)
{
  auto oldUniqueID = event_ ? event_->oldUniqueID : 0;
  auto newUniqueID = event_ ? event_->newUniqueID : 0;

  auto oldBaseID = event_ ? event_->oldBaseID : 0;
  auto newBaseID = event_ ? event_->newBaseID : 0;

  g_taskQueue.AddTask([oldUniqueID, newUniqueID, oldBaseID, newBaseID] {
    auto obj = JsValue::Object();

    obj.SetProperty("oldBaseID", JsValue::Double(oldBaseID));
    obj.SetProperty("newBaseID", JsValue::Double(newBaseID));
    obj.SetProperty("oldUniqueID", JsValue::Double(oldUniqueID));
    obj.SetProperty("newUniqueID", JsValue::Double(newUniqueID));

    EventsApi::SendEvent("uniqueIdChange", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESTrackedStatsEvent* event_,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>* eventSource)
{
  std::string statName = event_ ? event_->stat.data() : "";
  auto value = event_ ? event_->value : 0;

  g_taskQueue.AddTask([statName, value] {
    auto obj = JsValue::Object();

    obj.SetProperty("statName", JsValue::String(statName));
    obj.SetProperty("newValue", JsValue::Double(value));

    EventsApi::SendEvent("trackedStats", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESInitScriptEvent* event_,
  RE::BSTEventSource<RE::TESInitScriptEvent>* eventSource)
{
  auto objectInitialized = event_ ? event_->objectInitialized.get() : nullptr;
  auto objectInitializedId = objectInitialized ? objectInitialized->formID : 0;

  g_taskQueue.AddTask([objectInitializedId] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "initializedObject",
      CreateObject("ObjectReference",
                   RE::TESForm::LookupByID(objectInitializedId)));

    EventsApi::SendEvent("scriptInit", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESResetEvent* event_,
  RE::BSTEventSource<RE::TESResetEvent>* eventSource)
{
  auto object = event_ ? event_->object.get() : nullptr;
  auto objectId = object ? object->formID : 0;

  g_taskQueue.AddTask([objectId] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "object",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(objectId)));

    EventsApi::SendEvent("reset", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kStop;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESCombatEvent* event_,
  RE::BSTEventSource<RE::TESCombatEvent>* eventSource)
{
  auto targetActorRefr = event_ ? event_->targetActor.get() : nullptr;
  auto targetActorId = targetActorRefr ? targetActorRefr->formID : 0;

  auto actorRefr = event_ ? event_->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto state = event_ ? (uint32_t)event_->state : 0;

  g_taskQueue.AddTask([targetActorId, actorId, state] {
    auto obj = JsValue::Object();
    obj.SetProperty(
      "target",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(targetActorId)));

    obj.SetProperty(
      "actor",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(actorId)));

    obj.SetProperty(
      "isCombat",
      JsValue::Bool(state & (uint32_t)RE::ACTOR_COMBAT_STATE::kCombat));

    obj.SetProperty(
      "isSearching",
      JsValue::Bool(state & (uint32_t)RE::ACTOR_COMBAT_STATE::kSearching));

    EventsApi::SendEvent("combatState", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESDeathEvent* event_,
  RE::BSTEventSource<RE::TESDeathEvent>* eventSource)
{
  auto actorDyingRefr = event_ ? event_->actorDying.get() : nullptr;
  auto actorDyingId = actorDyingRefr ? actorDyingRefr->formID : 0;

  auto actorKillerRefr = event_ ? event_->actorKiller.get() : nullptr;
  auto actorKillerId = actorKillerRefr ? actorKillerRefr->formID : 0;

  auto dead = event_ ? event_->dead : 0;

  g_taskQueue.AddTask([actorDyingId, actorKillerId, dead] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "actorDying",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(actorDyingId)));

    obj.SetProperty(
      "actorKiller",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(actorKillerId)));

    dead ? EventsApi::SendEvent("deathEnd", { JsValue::Undefined(), obj })
         : EventsApi::SendEvent("deathStart", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESContainerChangedEvent* event_,
  RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource)
{
  auto oldContainerId = event_ ? event_->oldContainer : 0;
  auto newContainerId = event_ ? event_->newContainer : 0;
  auto baseObjId = event_ ? event_->baseObj : 0;
  auto itemCount = event_ ? event_->itemCount : 0;
  auto uniqueID = event_ ? event_->uniqueID : 0;

  auto reference = event_ ? event_->reference.get() : nullptr;
  auto referenceId = reference ? reference->formID : 0;

  g_taskQueue.AddTask([oldContainerId, newContainerId, baseObjId, itemCount,
                       uniqueID, referenceId] {
    auto obj = JsValue::Object();

    obj.SetProperty("oldContainer",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(oldContainerId)));

    obj.SetProperty("newContainer",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(newContainerId)));

    obj.SetProperty("baseObj",
                    CreateObject("Form", RE::TESForm::LookupByID(baseObjId)));

    obj.SetProperty("numItems", JsValue::Double(itemCount));
    obj.SetProperty("uniqueID", JsValue::Double(uniqueID));

    obj.SetProperty(
      "reference",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(referenceId)));

    EventsApi::SendEvent("containerChanged", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESHitEvent* event_,
  RE::BSTEventSource<RE::TESHitEvent>* eventSource)
{
  auto targetRefr = event_ ? event_->target.get() : nullptr;
  auto causeRefr = event_ ? event_->cause.get() : nullptr;

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto causeId = causeRefr ? causeRefr->formID : 0;

  auto sourceId = event_ ? event_->source : 0;
  auto projectileId = event_ ? event_->projectile : 0;
  uint8_t flags = event_ ? (uint8_t)event_->flags : 0;

  g_taskQueue.AddTask([targetId, causeId, sourceId, projectileId, flags] {
    auto obj = JsValue::Object();
    obj.SetProperty(
      "target",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(targetId)));

    obj.SetProperty(
      "agressor",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(causeId)));

    obj.SetProperty("source",
                    CreateObject("Form", RE::TESForm::LookupByID(sourceId)));

    obj.SetProperty(
      "projectile",
      CreateObject("Form", RE::TESForm::LookupByID(projectileId)));

    obj.SetProperty(
      "isPowerAttack",
      JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kPowerAttack));

    obj.SetProperty(
      "isSneakAttack",
      JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kSneakAttack));

    obj.SetProperty(
      "isBashAttack",
      JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kBashAttack));

    obj.SetProperty(
      "isHitBlocked",
      JsValue::Bool(flags & (uint8_t)RE::TESHitEvent::Flag::kHitBlocked));

    EventsApi::SendEvent("hit", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESEquipEvent* event_,
  RE::BSTEventSource<RE::TESEquipEvent>* eventSource)
{
  auto actorRefr = event_ ? event_->actor.get() : nullptr;
  auto actorId = actorRefr ? actorRefr->formID : 0;

  auto originalRefrId = event_ ? event_->originalRefr : 0;
  auto baseObjectId = event_ ? event_->baseObject : 0;
  auto equipped = event_ ? event_->equipped : 0;
  auto uniqueId = event_ ? event_->uniqueID : 0;

  g_taskQueue.AddTask([actorId, baseObjectId, equipped, uniqueId,
                       originalRefrId] {
    auto obj = JsValue::Object();

    obj.SetProperty(
      "actor",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(actorId)));

    obj.SetProperty(
      "baseObj", CreateObject("Form", RE::TESForm::LookupByID(baseObjectId)));

    obj.SetProperty("originalRefr",
                    CreateObject("ObjectReference",
                                 RE::TESForm::LookupByID(originalRefrId)));

    obj.SetProperty("uniqueId", JsValue::Double(uniqueId));

    equipped ? EventsApi::SendEvent("equip", { JsValue::Undefined(), obj })
             : EventsApi::SendEvent("unEquip", { JsValue::Undefined(), obj });
  });

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* event_,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* eventSource)
{
  auto caster = event_->caster.get() ? event_->caster.get() : nullptr;
  auto target = event_->target.get() ? event_->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  auto isApplied = event_ ? event_->isApplied : 0;
  auto activeEffectUniqueID = event_ ? event_->activeEffectUniqueID : 0;

  g_taskQueue.AddTask([casterId, targetId, isApplied, activeEffectUniqueID] {
    auto actor =
      reinterpret_cast<RE::Actor*>(RE::TESForm::LookupByID(targetId));
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
      if (effect && effect->usUniqueID == activeEffectUniqueID) {
        activeEffect = effect;
        break;
      }
    }

    /// TODO when add support for types that are not descendants of the Form
    // obj.SetProperty("effect", CreateObject("ActiveMagicEffect",
    // activeEffect));

    obj.SetProperty(
      "caster",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(casterId)));

    obj.SetProperty(
      "target",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(targetId)));

    isApplied
      ? EventsApi::SendEvent("effectStart", { JsValue::Undefined(), obj })
      : EventsApi::SendEvent("effectFinish", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl EventSinks::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* event_,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource)
{
  auto effectId = event_ ? event_->magicEffect : 0;

  auto caster = event_->caster.get() ? event_->caster.get() : nullptr;
  auto target = event_->target.get() ? event_->target.get() : nullptr;

  auto casterId = caster ? caster->formID : 0;
  auto targetId = target ? target->formID : 0;

  g_taskQueue.AddTask([effectId, casterId, targetId] {
    auto obj = JsValue::Object();

    auto effect = RE::TESForm::LookupByID(effectId);

    if (effect && effect->formType != RE::FormType::MagicEffect)
      return;

    obj.SetProperty("effect", CreateObject("MagicEffect", effect));

    obj.SetProperty(
      "caster",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(casterId)));

    obj.SetProperty(
      "target",
      CreateObject("ObjectReference", RE::TESForm::LookupByID(targetId)));

    EventsApi::SendEvent("magicEffectApply", { JsValue::Undefined(), obj });
  });
  return RE::BSEventNotifyControl::kContinue;
}