#include "CallNative.h"
#include "GetNativeFunctionAddr.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include <RE/BSScript/PackUnpack.h>
#include <RE/BSScript/StackFrame.h>
#include <RE/SkyrimVM.h>
#include <bitset>

template <class T>
inline size_t GetIndexFor()
{
  static const CallNative::AnySafe v = T();
  return v.index();
}

RE::BSScript::Variable AnySafeToVariable(const CallNative::AnySafe& v,
                                         bool treatNumberAsInt)
{
  return std::visit(
    overloaded{
      [&](double f) {
        RE::BSScript::Variable res;
        treatNumberAsInt ? res.SetSInt((int)floor(f)) : res.SetFloat((float)f);
        return res;
      },
      [](bool b) {
        RE::BSScript::Variable res;
        res.SetBool(b);
        return res;
      },
      [](const std::string& s) -> RE::BSScript::Variable {
        throw std::runtime_error("Unable to cast String to Variable");
      },
      [](const CallNative::ObjectPtr& obj) {
        RE::BSScript::Variable res;
        res.SetNone();
        if (!obj) {
          return res;
        }
        auto nativePtrRaw = (RE::TESForm*)obj->GetNativeObjectPtr();
        if (!nativePtrRaw)
          throw NullPointerException("nativePtrRaw");

        RE::BSScript::PackHandle(
          &res, nativePtrRaw,
          static_cast<RE::VMTypeID>(nativePtrRaw->formType));
        return res;
      },
      [](auto) -> RE::BSScript::Variable {
        throw std::runtime_error(
          "Unable to cast the argument to RE::BSScript::Variable");
      } },
    v);
}

class VariableAccess : public RE::BSScript::Variable
{
public:
  VariableAccess() = delete;

  static RE::BSScript::TypeInfo GetType(const RE::BSScript::Variable& v)
  {
    return ((VariableAccess&)v).varType;
  }
};

CallNative::AnySafe CallNative::CallNativeSafe(
  RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId,
  const std::string& className, const std::string& classFunc,
  const AnySafe& self, const AnySafe* args, size_t numArgs,
  FunctionInfoProvider& provider)
{
  auto funcInfo = provider.GetFunctionInfo(className, classFunc);
  if (!funcInfo) {
    throw std::runtime_error("Native function not found '" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' ");
  }
  if (funcInfo->IsLatent()) {
    throw std::runtime_error("Latent functions are not supported");
  }

  if (self.index() != GetIndexFor<ObjectPtr>())
    throw std::runtime_error("Expected self to be an object");

  auto& selfObjPtr = std::get<ObjectPtr>(self);
  RE::TESForm* rawSelf =
    selfObjPtr ? (RE::TESForm*)selfObjPtr->GetNativeObjectPtr() : nullptr;

  if (rawSelf && funcInfo->IsGlobal()) {
    throw std::runtime_error("Expected self to be null ('" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' is Global function)");
  }
  if (!rawSelf && !funcInfo->IsGlobal()) {
    throw std::runtime_error("Expected self to be non-null ('" +
                             std::string(className) + "." +
                             std::string(classFunc) + "' is Member function)");
  }

  if (numArgs > g_maxArgs)
    throw std::runtime_error("Too many arguments passed (" +
                             std::to_string(numArgs) + "), the limit is " +
                             std::to_string(g_maxArgs));

  if (funcInfo->GetParamCount() != numArgs) {
    std::stringstream ss;
    ss << "Function requires " << funcInfo->GetParamCount()
       << " arguments, but " << numArgs << " passed";
    throw std::runtime_error(ss.str());
  }

  auto f = funcInfo->GetIFunction();
  auto vmImpl = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  auto stackIterator = vmImpl->allRunningStacks.find(stackId);
  if (stackIterator == vmImpl->allRunningStacks.end())
    throw std::runtime_error("Bad stackIterator");

  auto it = vmImpl->objectTypeMap.find(f->GetObjectTypeName());
  if (it == vmImpl->objectTypeMap.end())
    throw std::runtime_error("Unable to find owning object type");

  stackIterator->second->top->owningFunction = f;
  stackIterator->second->top->owningObjectType = it->second;
  stackIterator->second->top->self = AnySafeToVariable(self, false);
  stackIterator->second->top->size = numArgs;
  auto topArgs = stackIterator->second->top->args;
  for (int i = 0; i < numArgs; i++) {
    RE::BSFixedString unusedNameOut;
    RE::BSScript::TypeInfo typeOut;
    f->GetParam(i, unusedNameOut, typeOut);

    topArgs[i] = AnySafeToVariable(args[i], typeOut.IsInt());
  }

  RE::BSScript::IFunction::CallResult callResut =
    f->Call(stackIterator->second, vmImpl->GetErrorLogger(), vmImpl, false);
  if (callResut != RE::BSScript::IFunction::CallResult::kCompleted) {
    throw std::runtime_error("Bad call result " +
                             std::to_string((int)callResut));
  }

  auto& r = stackIterator->second->returnValue;

  switch (funcInfo->GetReturnType().type) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return ObjectPtr();
    case RE::BSScript::TypeInfo::RawType::kObject: {
      auto object = r.GetObject();
      if (!object)
        return ObjectPtr();

      auto policy = vmImpl->GetObjectHandlePolicy();

      void* objPtr = nullptr;

      for (int i = 0; i < (int)RE::FormType::Max; ++i)
        if (policy->HandleIsType(i, object->handle)) {
          objPtr = object->Resolve(i);
          break;
        }
      if (objPtr)
        return std::make_shared<Object>(funcInfo->GetReturnType().className,
                                        objPtr);
      else
        return ObjectPtr();
    }
    case RE::BSScript::TypeInfo::RawType::kString:
      throw std::runtime_error(
        "Functions with String return type are not supported");
    case RE::BSScript::TypeInfo::RawType::kInt:
      return (double)r.GetSInt();
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return (double)r.GetFloat();
    case RE::BSScript::TypeInfo::RawType::kBool:
      return r.GetBool();
    case RE::BSScript::TypeInfo::RawType::kNoneArray:
    case RE::BSScript::TypeInfo::RawType::kObjectArray:
    case RE::BSScript::TypeInfo::RawType::kStringArray:
    case RE::BSScript::TypeInfo::RawType::kIntArray:
    case RE::BSScript::TypeInfo::RawType::kFloatArray:
    case RE::BSScript::TypeInfo::RawType::kBoolArray:
      throw std::runtime_error(
        "Functions with Array return type are not supported");
    default:
      throw std::runtime_error("Unknown function return type");
  }
}

namespace {
bool IsInstanceOf(
  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo>& objectInfo,
  const char* parentType)
{
  RE::BSScript::ObjectTypeInfo* objectInQuestion = objectInfo.get();

  if (!objectInQuestion)
    NullPointerException("objectInQuestion");

  while (true) {

    if (!stricmp(objectInQuestion->GetName(), parentType))
      return true;

    objectInQuestion = objectInQuestion->GetParent();

    if (!objectInQuestion)
      return false;
  }
};

void GetScriptObjectType(RE::BSScript::Internal::VirtualMachine& vm, RE::VMTypeID vmTypeID,
 RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo>& objectTypeInfo)
{
  bool res = vm.GetScriptObjectType(vmTypeID, objectTypeInfo);

  if (!objectTypeInfo)
    throw NullPointerException("objectTypeInfo");
  
  if (!res)
    throw std::runtime_error("GetScriptObjectType returned false");
};

}

// I don't think this implementation covers all cases but it should work
// for TESForm and derived classes
CallNative::AnySafe CallNative::DynamicCast(const std::string& to,
                                            const AnySafe& from_)
{
  static const auto objectIndex = AnySafe(ObjectPtr()).index();
  if (from_.index() != objectIndex)
    throw std::runtime_error("Dynamic cast can only cast objects");

  auto& from = std::get<ObjectPtr>(from_);
  if (!from)
    return ObjectPtr(); // DynamicCast should return None for None argument

  auto rawPtr = from->GetNativeObjectPtr();
  if (!rawPtr)
    throw NullPointerException("rawPtr");

  if (!stricmp(from->GetType(), to.data()))
    return from_;

  auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
  if (!vm)
    throw NullPointerException("vm");

  RE::VMTypeID rawTypeId;
  RE::VMTypeID targetTypeId;

  if (!vm->GetTypeIDForScriptObject(from->GetType(), rawTypeId))
    return ObjectPtr();

  if (!vm->GetTypeIDForScriptObject(to.data(), targetTypeId))
    return ObjectPtr();

  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> rawTypeInfoPtr;
  RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> outTypeInfoPtr;

  GetScriptObjectType(*vm, rawTypeId, rawTypeInfoPtr);
  GetScriptObjectType(*vm, targetTypeId, outTypeInfoPtr);

  if (IsInstanceOf(rawTypeInfoPtr, "Form")) {

    auto form = (RE::TESForm*)rawPtr;

    const bool IsParentOf = IsInstanceOf(outTypeInfoPtr, rawTypeInfoPtr->GetName());
    const bool IsChildOf = IsInstanceOf(rawTypeInfoPtr, outTypeInfoPtr->GetName());

    if (IsChildOf ||
        IsParentOf && targetTypeId == (RE::VMTypeID)form->formType) {

      return std::shared_ptr<Object>(
        new Object(outTypeInfoPtr->GetName(), form));
    }
  }

  return ObjectPtr();
}