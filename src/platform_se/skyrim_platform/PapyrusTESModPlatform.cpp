#include "PapyrusTESModPlatform.h"
#include "CallNativeApi.h"
#include "NullPointerException.h"
#include <RE/BSScript/IFunctionArguments.h>
#include <RE/BSScript/IStackCallbackFunctor.h>
#include <RE/BSScript/NativeFunction.h>
#include <RE/ConsoleLog.h>
#include <RE/Offsets.h>
#include <RE/PlayerCharacter.h>
#include <RE/PlayerControls.h>
#include <RE/ScriptEventSourceHolder.h>
#include <RE/SkyrimVM.h>
#include <RE/TESNPC.h>
#include <atomic>
#include <mutex>
#include <skse64/GameForms.h> // IFormFactory::GetFactoryForType
#include <skse64/GameReferences.h>
#include <unordered_map>
#include <unordered_set>

#define COLOR_ALPHA(in) ((in >> 24) & 0xFF)
#define COLOR_RED(in) ((in >> 16) & 0xFF)
#define COLOR_GREEN(in) ((in >> 8) & 0xFF)
#define COLOR_BLUE(in) ((in >> 0) & 0xFF)

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace TESModPlatform {
bool papyrusUpdateAllowed = false;
bool vmCallAllowed = true;
std::atomic<bool> moveRefrBlocked = false;
std::function<void(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId)>
  onPapyrusUpdate = nullptr;
std::atomic<uint64_t> numPapyrusUpdates = 0;
struct
{
  std::unordered_map<uint32_t, int> weapDrawnMode;
  std::recursive_mutex m;
} share;

class FunctionArguments : public RE::BSScript::IFunctionArguments
{
public:
  bool operator()(
    RE::BSScrapArray<RE::BSScript::Variable>& a_dst) const override
  {
    a_dst.resize(12);
    for (int i = 0; i < 12; i++) {
      a_dst[i].SetSInt(100 + i);
    }
    return true;
  }
};

class StackCallbackFunctor : public RE::BSScript::IStackCallbackFunctor
{
public:
  void operator()(RE::BSScript::Variable a_result) override {}
  bool CanSave() const override { return false; }
  void SetObject(
    const RE::BSTSmartPointer<RE::BSScript::Object>& a_object) override{};
};

// This class has been added as an issue 52 workaround
class LoadGameEvent : public RE::BSTEventSink<RE::TESLoadGameEvent>
{
public:
  LoadGameEvent()
  {
    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder)
      throw NullPointerException("holder");

    holder->AddEventSink(this);
  }

private:
  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESLoadGameEvent* event_,
    RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource) override
  {
    vmCallAllowed = true;
    return RE::BSEventNotifyControl::kContinue;
  }
};
}

SInt32 TESModPlatform::Add(RE::BSScript::IVirtualMachine* vm,
                           RE::VMStackID stackId, RE::StaticFunctionTag*,
                           SInt32, SInt32, SInt32, SInt32, SInt32, SInt32,
                           SInt32, SInt32, SInt32, SInt32, SInt32, SInt32)
{
  if (!papyrusUpdateAllowed)
    return 0;
  papyrusUpdateAllowed = false;

  try {
    ++numPapyrusUpdates;
    if (!onPapyrusUpdate)
      throw NullPointerException("onPapyrusUpdate");
    onPapyrusUpdate(vm, stackId);

  } catch (std::exception& e) {
    if (auto console = RE::ConsoleLog::GetSingleton())
      console->Print("Papyrus context exception: %s", e.what());
  }

  vmCallAllowed = true;

  return 0;
}

void TESModPlatform::MoveRefrToPosition(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  RE::StaticFunctionTag*, RE::TESObjectREFR* refr, RE::TESObjectCELL* cell,
  RE::TESWorldSpace* world, float posX, float posY, float posZ, float rotX,
  float rotY, float rotZ)
{
  if (!refr || (!cell && !world) || moveRefrBlocked)
    return;

  NiPoint3 pos = { posX, posY, posZ }, rot = { rotX, rotY, rotZ };
  auto nullHandle = *g_invalidRefHandle;

  auto f = ::MoveRefrToPosition.operator _MoveRefrToPosition();
  f(reinterpret_cast<TESObjectREFR*>(refr), &nullHandle, cell, world, &pos,
    &rot);
}

void TESModPlatform::BlockMoveRefrToPosition(bool blocked)
{
  moveRefrBlocked = blocked;
}

void TESModPlatform::SetWeaponDrawnMode(RE::BSScript::IVirtualMachine* vm,
                                        RE::VMStackID stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor, SInt32 weapDrawnMode)
{
  if (!actor || weapDrawnMode < WEAP_DRAWN_MODE_MIN ||
      weapDrawnMode > WEAP_DRAWN_MODE_MAX)
    return;

  if (g_nativeCallRequirements.gameThrQ) {
    auto formId = actor->formID;
    g_nativeCallRequirements.gameThrQ->AddTask([=] {
      if (LookupFormByID(formId) != (void*)actor)
        return;

      if (!actor->IsWeaponDrawn() &&
          weapDrawnMode == WEAP_DRAWN_MODE_ALWAYS_TRUE)
        actor->DrawWeaponMagicHands(true);

      if (actor->IsWeaponDrawn() &&
          weapDrawnMode == WEAP_DRAWN_MODE_ALWAYS_FALSE)
        actor->DrawWeaponMagicHands(false);
    });
  }

  std::lock_guard l(share.m);
  share.weapDrawnMode[actor->formID] = weapDrawnMode;
}

SInt32 TESModPlatform::GetNthVtableElement(RE::BSScript::IVirtualMachine* vm,
                                           RE::VMStackID stackId,
                                           RE::StaticFunctionTag*,
                                           RE::TESForm* pointer,
                                           SInt32 pointerOffset,
                                           SInt32 elementIndex)
{
  static auto getNthVTableElement = [](void* obj, size_t idx) {
    using VTable = size_t*;
    auto vtable = *(VTable*)obj;
    return vtable[idx];
  };

  if (pointer && elementIndex >= 0) {
    __try {
      return getNthVTableElement(reinterpret_cast<uint8_t*>(pointer) +
                                   pointerOffset,
                                 elementIndex) -
        REL::Module::BaseAddr();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return -1;
}

bool TESModPlatform::IsPlayerRunningEnabled(RE::BSScript::IVirtualMachine* vm,
                                            RE::VMStackID stackId,
                                            RE::StaticFunctionTag*)
{
  if (auto controls = RE::PlayerControls::GetSingleton())
    return controls->data.running;
  return false;
}

RE::BGSColorForm* TESModPlatform::GetSkinColor(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  RE::StaticFunctionTag*, RE::TESNPC* base)
{
  auto factory = IFormFactory::GetFactoryForType(BGSColorForm::kTypeID);
  if (!factory)
    return nullptr;
  auto col = (RE::BGSColorForm*)factory->Create();
  if (!col)
    return nullptr;
  col->color = base->bodyTintColor;
  return col;
}

inline uint32_t AllocateFormId()
{
  static uint32_t g_formId = 0xffffffff;
  return g_formId--;
}

RE::TESNPC* TESModPlatform::CreateNpc(RE::BSScript::IVirtualMachine* vm,
                                      RE::VMStackID stackId,
                                      RE::StaticFunctionTag*)
{
  auto factory = IFormFactory::GetFactoryForType(TESNPC::kTypeID);
  if (!factory)
    return nullptr;

  auto npc = (RE::TESNPC*)factory->Create();
  if (!npc)
    return nullptr;

  enum
  {
    AADeleteWhenDoneTestJeremyRegular = 0x0010D13E
  };
  const auto srcNpc =
    (TESNPC*)LookupFormByID(AADeleteWhenDoneTestJeremyRegular);
  assert(srcNpc);
  assert(srcNpc->formType == kFormType_NPC);
  if (!srcNpc || srcNpc->formType != kFormType_NPC)
    return nullptr;
  auto backup = npc->formID;
  memcpy(npc, srcNpc, sizeof TESNPC);
  npc->formID = backup;
  return (RE::TESNPC*)npc;

  /* auto factory = IFormFactory::GetFactoryForType(TESNPC::kTypeID);
   if (!factory)
     return nullptr;

   return (RE::TESNPC*)factory->Create();*/

  /*auto npc = (TESNPC*)malloc(sizeof TESNPC);
  if (!npc)
    return nullptr;

  enum
  {
    AADeleteWhenDoneTestJeremyRegular = 0x0010D13E
  };
  const auto srcNpc =
    (TESNPC*)LookupFormByID(AADeleteWhenDoneTestJeremyRegular);
  assert(srcNpc);
  assert(srcNpc->formType == kFormType_NPC);
  if (!srcNpc || srcNpc->formType != kFormType_NPC)
    return nullptr;
  memcpy(npc, srcNpc, sizeof TESNPC);

  npc->actorData.voiceType = (BGSVoiceType*)LookupFormByID(0x0002F7C3);
  npc->formID = 0;

  npc->SetFormID(AllocateFormId(), 1);
  return (RE::TESNPC*)npc;*/
}

void TESModPlatform::SetNpcSex(RE::BSScript::IVirtualMachine* vm,
                               RE::VMStackID stackId, RE::StaticFunctionTag*,
                               RE::TESNPC* npc, SInt32 sex)
{
  if (npc) {
    if (sex == 1)
      npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kFemale;
    else
      npc->actorData.actorBaseFlags &= ~RE::ACTOR_BASE_DATA::Flag::kFemale;
  }
}

void TESModPlatform::SetNpcRace(RE::BSScript::IVirtualMachine* vm,
                                RE::VMStackID stackId, RE::StaticFunctionTag*,
                                RE::TESNPC* npc, RE::TESRace* race)
{
  if (npc && race)
    npc->race = race;
}

void TESModPlatform::SetNpcSkinColor(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                                     SInt32 color)
{
  if (!npc)
    return;
  npc->bodyTintColor.red = COLOR_RED(color);
  npc->bodyTintColor.green = COLOR_GREEN(color);
  npc->bodyTintColor.blue = COLOR_BLUE(color);
}

void TESModPlatform::SetNpcHairColor(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESNPC* npc,
                                     SInt32 color)
{
  if (!npc)
    return;
  if (!npc->headRelatedData) {
    npc->headRelatedData =
      (RE::TESNPC::HeadRelatedData*)malloc(sizeof RE::TESNPC::HeadRelatedData);
    if (!npc->headRelatedData)
      return;
    npc->headRelatedData->faceDetails = nullptr;
  }

  auto factory = IFormFactory::GetFactoryForType(BGSColorForm::kTypeID);
  if (!factory)
    return;

  npc->headRelatedData->hairColor = (RE::BGSColorForm*)factory->Create();
  if (!npc->headRelatedData->hairColor)
    return;

  auto& c = npc->headRelatedData->hairColor->color;
  c.red = COLOR_RED(color);
  c.green = COLOR_GREEN(color);
  c.blue = COLOR_BLUE(color);
}

void TESModPlatform::ResizeHeadpartsArray(RE::BSScript::IVirtualMachine* vm,
                                          RE::VMStackID stackId,
                                          RE::StaticFunctionTag*,
                                          RE::TESNPC* npc, SInt32 newSize)
{
  if (!npc)
    return;
  if (newSize <= 0) {
    npc->headParts = nullptr;
    npc->numHeadParts = 0;
  } else {
    npc->headParts = new RE::BGSHeadPart*[newSize];
    npc->numHeadParts = (uint8_t)newSize;
    for (SInt8 i = 0; i < npc->numHeadParts; ++i)
      npc->headParts[i] = nullptr;
  }

  /*if (!npc)
    return;
  if (index < 0) {
    npc->headParts = nullptr;
    npc->numHeadParts = 0;
  } else {
    int newHeadpartsCount = index + 1;

    std::vector<RE::BGSHeadPart*> prev;
    if (npc->headParts)
      for (size_t i = 0; i < npc->numHeadParts; ++i)
        prev.push_back(npc->headParts[i]);

    npc->headParts = new RE::BGSHeadPart*[newHeadpartsCount];
    npc->numHeadParts = (uint8_t)newHeadpartsCount;
  }*/

  /*if (!npc ||
      std::find(headparts.begin(), headparts.end(), nullptr) !=
        headparts.end())
    return;

  if (headparts.size() > 0) {
    npc->headParts = new RE::BGSHeadPart*[headparts.size()];
    npc->numHeadParts = (uint8_t)headparts.size();
  } else {
    npc->headParts = nullptr;
    npc->numHeadParts = 0;
  }

  for (size_t i = 0; i < headparts.size(); ++i)
    npc->headParts[i] = headparts[i];*/
}

void TESModPlatform::ResizeTintsArray(RE::BSScript::IVirtualMachine* vm,
                                      RE::VMStackID stackId,
                                      RE::StaticFunctionTag*, SInt32 newSize)
{
  PlayerCharacter* pc = *g_thePlayer;
  RE::PlayerCharacter* rePc = (RE::PlayerCharacter*)pc;
  if (!rePc)
    return;
  if (newSize < 0 || newSize > 1024)
    return;
  auto prevSize = rePc->tintMasks.size();
  rePc->tintMasks.resize(newSize);
  for (size_t i = prevSize; i < rePc->tintMasks.size(); ++i) {
    rePc->tintMasks[i] = (RE::TintMask*)new TintMask;
  }
}

void TESModPlatform::SetFormIdUnsafe(RE::BSScript::IVirtualMachine* vm,
                                     RE::VMStackID stackId,
                                     RE::StaticFunctionTag*, RE::TESForm* form,
                                     UInt32 newId)
{
  form->formID = newId;
}

int TESModPlatform::GetWeapDrawnMode(uint32_t actorId)
{
  std::lock_guard l(share.m);
  auto it = share.weapDrawnMode.find(actorId);
  return it == share.weapDrawnMode.end() ? WEAP_DRAWN_MODE_DEFAULT
                                         : it->second;
}

void TESModPlatform::Update()
{
  if (!vmCallAllowed)
    return;
  vmCallAllowed = false;

  papyrusUpdateAllowed = true;

  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    return;

  auto vm = RE::SkyrimVM::GetSingleton();
  if (!vm || !vm->impl)
    return console->Print("VM was nullptr");

  FunctionArguments args;
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> functor(
    new StackCallbackFunctor);

  RE::BSFixedString className("TESModPlatform");
  RE::BSFixedString funcName("Add");
  vm->impl->DispatchStaticCall(className, funcName, &args, functor);
}

uint64_t TESModPlatform::GetNumPapyrusUpdates()
{
  return numPapyrusUpdates;
}

bool TESModPlatform::Register(RE::BSScript::IVirtualMachine* vm)
{
  TESModPlatform::onPapyrusUpdate = onPapyrusUpdate;

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(Add), SInt32,
                                     RE::StaticFunctionTag*, SInt32, SInt32,
                                     SInt32, SInt32, SInt32, SInt32, SInt32,
                                     SInt32, SInt32, SInt32, SInt32, SInt32>(
      "Add", "TESModPlatform", Add));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<
      true, decltype(MoveRefrToPosition), void, RE::StaticFunctionTag*,
      RE::TESObjectREFR*, RE::TESObjectCELL*, RE::TESWorldSpace*, float, float,
      float, float, float, float>("MoveRefrToPosition", "TESModPlatform",
                                  MoveRefrToPosition));
  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetWeaponDrawnMode), void,
                                     RE::StaticFunctionTag*, RE::Actor*, int>(
      "SetWeaponDrawnMode", "TESModPlatform", SetWeaponDrawnMode));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(GetNthVtableElement),
                                     SInt32, RE::StaticFunctionTag*,
                                     RE::TESForm*, int, int>(
      "GetNthVtableElement", "TESModPlatform", GetNthVtableElement));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(IsPlayerRunningEnabled),
                                     bool, RE::StaticFunctionTag*>(
      "IsPlayerRunningEnabled", "TESModPlatform", IsPlayerRunningEnabled));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(GetSkinColor),
                                     RE::BGSColorForm*, RE::StaticFunctionTag*,
                                     RE::TESNPC*>(
      "GetSkinColor", "TESModPlatform", GetSkinColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(CreateNpc), RE::TESNPC*,
                                     RE::StaticFunctionTag*>(
      "CreateNpc", "TESModPlatform", CreateNpc));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcSex), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>("SetNpcSex", "TESModPlatform",
                                             SetNpcSex));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcRace), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     RE::TESRace*>(
      "SetNpcRace", "TESModPlatform", SetNpcRace));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcSkinColor), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>(
      "SetNpcSkinColor", "TESModPlatform", SetNpcSkinColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetNpcHairColor), void,
                                     RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>(
      "SetNpcHairColor", "TESModPlatform", SetNpcHairColor));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResizeHeadpartsArray),
                                     void, RE::StaticFunctionTag*, RE::TESNPC*,
                                     SInt32>(
      "ResizeHeadpartsArray", "TESModPlatform", ResizeHeadpartsArray));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(ResizeTintsArray), void,
                                     RE::StaticFunctionTag*, SInt32>(
      "ResizeTintsArray", "TESModPlatform", ResizeTintsArray));

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(SetFormIdUnsafe), void,
                                     RE::StaticFunctionTag*, RE::TESForm*,
                                     SInt32>(
      "SetFormIdUnsafe", "TESModPlatform", SetFormIdUnsafe));

  static LoadGameEvent loadGameEvent;

  return true;
}