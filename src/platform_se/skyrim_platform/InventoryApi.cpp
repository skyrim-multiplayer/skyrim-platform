#include "InventoryApi.h"
#include "NullPointerException.h"
#include <set>
#include <skse64/GameExtraData.h>
#include <skse64/GameRTTI.h>
#include <skse64/GameReferences.h>
#include <skse64/PapyrusObjectReference.h>

namespace {

JsValue ToJsValue(ExtraHealth& extra)
{
  auto res = JsValue::Object();
  res.SetProperty("health", extra.health);
  res.SetProperty("type", "Health");
  return res;
}

JsValue ToJsValue(BSExtraData* extraData)
{
  if (extraData) {
    switch (extraData->GetType()) {
      case kExtraData_Health:
        return ToJsValue(*reinterpret_cast<ExtraHealth*>(extraData));
    }
  }
  return JsValue::Undefined();
}

JsValue ToJsValue(BaseExtraList* extraList)
{
  if (!extraList)
    return JsValue::Null();

  std::vector<JsValue> jData;

  BSReadLocker lock(&extraList->m_lock);

  for (auto data = extraList->m_data; data != nullptr; data = data->next) {
    auto extra = ToJsValue(data);
    if (extra.GetType() != JsValue::Type::Undefined)
      jData.push_back(extra);
  }

  return jData;
}

JsValue ToJsValue(tList<BaseExtraList>* extendDataList)
{
  if (!extendDataList)
    return JsValue::Null();

  auto arr = JsValue::Array(extendDataList->Count());
  for (uint32_t i = 0; i < extendDataList->Count(); ++i) {
    auto baseExtraList = extendDataList->GetNthItem(i);
    arr.SetProperty(JsValue::Int(i), ToJsValue(baseExtraList));
  }
  return arr;
}
}

JsValue InventoryApi::GetExtraContainerChanges(const JsFunctionArguments& args)
{
  auto objectReferenceId = static_cast<double>(args[1]);

  auto refr =
    reinterpret_cast<TESObjectREFR*>(LookupFormByID(objectReferenceId));
  if (!refr ||
      (refr->formType != kFormType_Character &&
       refr->formType != kFormType_Reference)) {
    return JsValue::Null();
  }

  auto extraCntainerChanges = reinterpret_cast<ExtraContainerChanges*>(
    refr->extraData.GetByType(kExtraData_ContainerChanges));
  if (!extraCntainerChanges || !extraCntainerChanges->data)
    return JsValue::Null();

  tList<InventoryEntryData>* objList = extraCntainerChanges->data->objList;
  if (!objList)
    throw NullPointerException("objList");

  auto res = JsValue::Array(objList->Count());
  for (uint32_t i = 0; i < objList->Count(); ++i) {
    auto entry = objList->GetNthItem(static_cast<int>(i));
    if (!entry)
      continue;

    const auto baseId = entry->type ? entry->type->formID : 0;

    auto jEntry = JsValue::Object();
    jEntry.SetProperty("countDelta", entry->countDelta);
    jEntry.SetProperty("baseId", JsValue::Double(baseId));
    jEntry.SetProperty("extendDataList", ToJsValue(entry->extendDataList));
    res.SetProperty(JsValue::Int(i), jEntry);
  }
  return res;
}

JsValue InventoryApi::GetContainer(const JsFunctionArguments& args)
{
  auto formId = static_cast<double>(args[1]);
  auto form = reinterpret_cast<TESObjectREFR*>(LookupFormByID(formId));

  if (!form)
    return JsValue::Array(0);

  TESContainer* pContainer = DYNAMIC_CAST(form, TESForm, TESContainer);

  if (!pContainer)
    return JsValue::Array(0);

  auto res = JsValue::Array(pContainer->numEntries);
  for (int i = 0; i < static_cast<int>(pContainer->numEntries); ++i) {
    auto e = pContainer->entries[i];

    auto jEntry = JsValue::Object();
    jEntry.SetProperty("count", JsValue::Double(e->count));
    jEntry.SetProperty("baseId",
                       JsValue::Double(e->form ? e->form->formID : 0));
    res.SetProperty(i, jEntry);
  }
  return res;
}

/*JsValue InventoryApi::GetInventory(const JsFunctionArguments& args)
{
  auto objectReferenceId = static_cast<double>(args[1]);

  auto refr =
    reinterpret_cast<TESObjectREFR*>(LookupFormByID(objectReferenceId));
  if (!refr ||
      (refr->formType != kFormType_Character &&
       refr->formType != kFormType_Reference)) {
    return JsValue::Array(0);
  }

  TESForm* pBaseForm = refr->baseForm;
  if (!pBaseForm)
    return JsValue::Array(0);
  TESContainer* pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
  if (!pContainer)
    return JsValue::Array(0);

  auto extraCntainerChanges = reinterpret_cast<ExtraContainerChanges*>(
    refr->extraData.GetByType(kExtraData_ContainerChanges));
  if (!extraCntainerChanges || !extraCntainerChanges->data)
    return JsValue::Array(0);

  tList<InventoryEntryData>* objList = extraCntainerChanges->data->objList;
  if (!objList)
    throw NullPointerException("objList");

  std::vector<JsValue> res;
  std::set<uint32_t> formIdsInRes;

  for (uint32_t i = 0; i < objList->Count(); ++i) {
    InventoryEntryData* entry = objList->GetNthItem(i);
    if (!entry)
      continue;

    std::vector<JsValue> jExtraData;

    if (auto extendDataList = entry->extendDataList) {
      for (int j = 0; j < extendDataList->Count(); ++j) {
        auto baseExtraList = extendDataList->GetNthItem(j);
        if (!baseExtraList)
          continue;
        BSReadLocker l(&baseExtraList->m_lock);

        for (auto data = baseExtraList->m_data; data != nullptr;
             data = data->next) {
          switch (data->GetType()) {
            case kExtraData_Health: {
              auto jExtra = JsValue::Object();
              jExtra.SetProperty("type", "Health");
              jExtra.SetProperty("health",
                                 reinterpret_cast<ExtraHealth*>(data)->health);
              jExtraData.push_back(jExtra);
              break;
            }
          }
        }
      }
    }

    const auto baseId = entry->type ? entry->type->formID : 0;
    const bool hasExtraData =
      entry->extendDataList && entry->extendDataList->Count() > 0;

    // marked to be ignored while base container is being processed
    if (!hasExtraData)
      formIdsInRes.insert(baseId);

    int count = entry->countDelta;
    if (jExtraData.empty() && entry->type && !hasExtraData) {
      count += pContainer->CountItem(entry->type);
    }
    if (count == 0)
      continue;

    JsValue jEntry = JsValue::Object();
    jEntry.SetProperty("baseId", JsValue::Double(baseId));
    jEntry.SetProperty("count", JsValue::Int(count));
    jEntry.SetProperty("extraData", jExtraData);
    res.push_back(jEntry);
  }

  for (auto it = pContainer->entries;
       it != pContainer->entries + pContainer->numEntries; ++it) {
    auto f = (*it)->form;
    if (!f)
      continue;
    if (!formIdsInRes.count(f->formID)) {
      JsValue jEntry = JsValue::Object();
      jEntry.SetProperty("baseId", JsValue::Double(f->formID));
      jEntry.SetProperty("count", JsValue::Int((*it)->count));
      jEntry.SetProperty("extraData", JsValue::Array(0));
      res.push_back(jEntry);
    }
  }

  return res;
}*/