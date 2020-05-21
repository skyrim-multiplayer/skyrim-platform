#include "ConsoleApi.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include <RE/CommandTable.h>
#include <RE/ConsoleLog.h>
#include <RE/Script.h>
#include <RE/TESForm.h>
#include <RE/TESObjectREFR.h>
#include <cstdlib>
#include <ctpl\ctpl_stl.h>
#include <skse64_common\SafeWrite.h>
#include <vector>

extern ctpl::thread_pool g_pool;
extern TaskQueue g_taskQueue;

JsValue ConsoleApi::PrintConsole(const JsFunctionArguments& args)
{
  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    throw NullPointerException("console");

  std::string s;

  for (size_t i = 1; i < args.GetSize(); ++i) {
    JsValue str = args[i];
    if (args[i].GetType() == JsValue::Type::Object &&
        !args[i].GetExternalData()) {

      JsValue json = JsValue::GlobalObject().GetProperty("JSON");
      str = json.GetProperty("stringify").Call({ json, args[i] });
    }
    s += str.ToString() + ' ';
  }

  int maxSize = 128;
  if (s.size() > maxSize) {
    s.resize(maxSize);
    s += "...";
  }
  console->Print("[Script] %s", s.data());

  return JsValue::Undefined();
}
namespace {
bool IsCommandNameEqual(const std::string& first, const std::string& second)
{
  return first.size() > 0 && second.size() > 0
    ? stricmp(first.data(), second.data()) == 0
    : false;
}
}

void ConsoleApi::Clear()
{
  for (auto& item : replacedConsoleCmd) {
    SafeWriteBuf((uintptr_t)item.second.myIter, &(item.second.myOriginalData),
                 sizeof(item.second.myOriginalData));
  }

  replacedConsoleCmd.clear();
}

JsValue ConsoleApi::GetConsoleHookApi()
{
  JsValue cmdHookApi = JsValue::Object();

  cmdHookApi.SetProperty("findCommand",
                         JsValue::Function(ConsoleApi::FindConsoleComand));

  return cmdHookApi;
}

ConsoleApi::ConsoleComand ConsoleApi::FillCmdInfo(ObScriptCommand* cmd)
{
  ConsoleApi::ConsoleComand cmdInfo;
  if (cmd) {
    cmdInfo.longName = cmd->longName;
    cmdInfo.shortName = cmd->shortName;
    cmdInfo.numArgs = cmd->numParams;
    cmdInfo.execute = cmd->execute;
    cmdInfo.myIter = cmd;
    cmdInfo.myOriginalData = *cmd;
    cmdInfo.jsExecute = JsValue::Function(
      [](const JsFunctionArguments& args) { return JsValue::Bool(true); });
  }
  return cmdInfo;
}

namespace {

void CreateLongNameProperty(
  JsValue& obj,
  std::pair<const std::string, ConsoleApi::ConsoleComand>* replaced)
{
  obj.SetProperty(
    "longName",
    [=](const JsFunctionArguments& args) {
      return JsValue::String((*replaced).second.myIter->longName);
    },
    [=](const JsFunctionArguments& args) {
      ObScriptCommand cmd = *(*replaced).second.myIter;
      (*replaced).second.longName = args[1].ToString();
      cmd.longName = (*replaced).second.longName.c_str();
      SafeWriteBuf((uintptr_t)(*replaced).second.myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateShortNameProperty(
  JsValue& obj,
  std::pair<const std::string, ConsoleApi::ConsoleComand>* replaced)
{
  obj.SetProperty(
    "shortName",
    [=](const JsFunctionArguments& args) {
      return JsValue::String(replaced->second.myIter->shortName);
    },
    [=](const JsFunctionArguments& args) {
      ObScriptCommand cmd = *replaced->second.myIter;
      replaced->second.shortName = args[1].ToString();
      cmd.shortName = replaced->second.shortName.c_str();
      SafeWriteBuf((uintptr_t)replaced->second.myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateNumArgsProperty(
  JsValue& obj,
  std::pair<const std::string, ConsoleApi::ConsoleComand>* replaced)
{
  obj.SetProperty(
    "numArgs",
    [=](const JsFunctionArguments& args) {
      return JsValue::Double(replaced->second.myIter->numParams);
    },
    [=](const JsFunctionArguments& args) {
      ObScriptCommand cmd = *replaced->second.myIter;
      try {
        replaced->second.numArgs =
          std::get<double>(NativeValueCasts::JsValueToNativeValue(args[1]));
      } catch (const std::bad_variant_access& f) {
        std::string what = f.what();
        throw std::runtime_error("try to get numArgs " + what);
      }
      cmd.numParams = replaced->second.numArgs;
      SafeWriteBuf((uintptr_t)replaced->second.myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateExecuteProperty(
  JsValue& obj,
  std::pair<const std::string, ConsoleApi::ConsoleComand>* replaced)
{
  obj.SetProperty(
    "execute",
    [](const JsFunctionArguments& args) { return JsValue::Undefined(); },
    [=](const JsFunctionArguments& args) {
      replaced->second.jsExecute = args[1];
      return JsValue::Undefined();
    });
}
}

bool ConsoleApi::FindComand(ObScriptCommand* iter,
                            const std::string& comandName, uint32_t end,
                            JsValue& res)
{
  for (ObScriptCommand* _iter = iter; _iter->opcode < end; ++_iter) {
    if (IsCommandNameEqual(_iter->longName, comandName) ||
        IsCommandNameEqual(_iter->shortName, comandName)) {
      JsValue obj = JsValue::Object();

      replacedConsoleCmd[comandName] = FillCmdInfo(_iter);
      auto replaced = replacedConsoleCmd.find(comandName);

      CreateLongNameProperty(obj, &(*replaced));
      CreateShortNameProperty(obj, &(*replaced));
      CreateNumArgsProperty(obj, &(*replaced));
      CreateExecuteProperty(obj, &(*replaced));

      ObScriptCommand cmd = *_iter;
      cmd.execute = ConsoleApi::ConsoleComand_Execute;
      SafeWriteBuf((uintptr_t)_iter, &cmd, sizeof(cmd));
      res = obj;
      return true;
    }
  }
  return false;
}
JsValue ConsoleApi::FindConsoleComand(const JsFunctionArguments& args)
{
  auto comandName = args[1].ToString();
  JsValue var = JsValue::Null();

  bool res =
    FindComand(g_firstConsoleCommand, comandName,
               kObScript_NumConsoleCommands + kObScript_ConsoleOpBase, var);

  if (!res)
    FindComand(g_firstObScriptCommand, comandName,
               kObScript_NumObScriptCommands + kObScript_ScriptOpBase, var);

  return var;
}

namespace {
void GetArgs(std::string comand, std::vector<std::string>& res)
{
  static const std::string delimiterComa = ".";
  static const std::string delimiterSpase = " ";
  size_t pos = comand.find(delimiterComa);
  std::string token;

  if (pos != std::string::npos) {
    comand.erase(0, pos + delimiterComa.length());
  }

  while ((pos = comand.find(delimiterSpase)) != std::string::npos) {
    token = comand.substr(0, pos);
    res.push_back(token);
    comand.erase(0, pos + delimiterSpase.length());
  }

  if (comand.size() >= 1)
    res.push_back(comand);
}

enum class ObjectType
{
  String,
  Int_Hex,
};

ObjectType DetermineType(const std::string& param)
{
  auto id = strtoul(param.c_str(), nullptr, 16);
  auto formByEditorId = RE::TESForm::LookupByEditorID(param);
  auto formById = RE::TESForm::LookupByID(id);

  if (formByEditorId)
    return ObjectType::String;
  if (formById)
    return ObjectType::Int_Hex;

  auto err = "For param: " + param + " formId and editorId was not found";
  throw std::runtime_error(err.data());
  return ObjectType::String;
}

JsValue GetObject(const std::string& param)
{
  ObjectType type = DetermineType(param);
  return type == ObjectType::Int_Hex
    ? JsValue::Double((double)strtoll(param.c_str(), nullptr, 16))
    : JsValue::String(param);
}

JsValue GetTypedArg(RE::SCRIPT_PARAM_TYPE type, std::string param)
{
  switch (type) {
    case RE::SCRIPT_PARAM_TYPE::kStage:
    case RE::SCRIPT_PARAM_TYPE::kInt:
    case RE::SCRIPT_PARAM_TYPE::kFloat:
      return JsValue::Double((double)strtoll(param.c_str(), nullptr, 10));

    case RE::SCRIPT_PARAM_TYPE::kCoontainerRef:
    case RE::SCRIPT_PARAM_TYPE::kInvObjectOrFormList:
    case RE::SCRIPT_PARAM_TYPE::kSpellItem:
    case RE::SCRIPT_PARAM_TYPE::kInventoryObject:
    case RE::SCRIPT_PARAM_TYPE::kPerk:
    case RE::SCRIPT_PARAM_TYPE::kActorBase:
    case RE::SCRIPT_PARAM_TYPE::kObjectRef:
      return JsValue::Double((double)strtoll(param.c_str(), nullptr, 16));

    case RE::SCRIPT_PARAM_TYPE::kAxis:
    case RE::SCRIPT_PARAM_TYPE::kActorValue:
    case RE::SCRIPT_PARAM_TYPE::kChar:
      return JsValue::String(param);

    case RE::SCRIPT_PARAM_TYPE::kQuest:
      return GetObject(param);

    default:
      return JsValue::Undefined();
  }
}
}

bool ConsoleApi::ConsoleComand_Execute(const ObScriptParam* paramInfo,
                                       ScriptData* scriptData,
                                       TESObjectREFR* thisObj,
                                       TESObjectREFR* containingObj,
                                       Script* scriptObj, ScriptLocals* locals,
                                       double& result, UInt32& opcodeOffsetPtr)
{
  std::pair<const std::string, ConsoleComand>* iterator = nullptr;

  auto func = [&](int) {
    try {
      if (!scriptObj)
        throw NullPointerException("scriptObj");

      RE::Script* script = reinterpret_cast<RE::Script*>(scriptObj);
      std::string comand = script->GetCommand();
      std::vector<std::string> params;
      GetArgs(comand, params);

      for (auto& item : replacedConsoleCmd) {
        if (IsCommandNameEqual(item.second.longName, params.at(0)) ||
            IsCommandNameEqual(item.second.shortName, params.at(0))) {

          std::vector<JsValue> args;
          auto refr = reinterpret_cast<RE::TESObjectREFR*>(thisObj);
          args.push_back(JsValue::Undefined());
          if (refr)
            args.push_back(JsValue::Double((double)refr->formID));

          auto log = RE::ConsoleLog::GetSingleton();
          auto param =
            reinterpret_cast<const RE::SCRIPT_PARAMETER*>(paramInfo);
          for (size_t i = 1; i < params.size(); ++i) {
            if (!param)
              break;

            JsValue arg = GetTypedArg(param[i - 1].paramType, params[i]);

            if (arg.GetType() == JsValue::Type::Undefined) {
              auto err = " typeId " +
                std::to_string((uint32_t)param[i - 1].paramType) +
                " not yet supported";

              throw std::runtime_error(err.data());
            }

            args.push_back(arg);
          }

          if (item.second.jsExecute.Call(args))
            iterator = &item;
        }
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in ConsoleComand_Execute)");
      });
    }
  };

  g_pool.push(func).wait();
  if (iterator)
    iterator->second.execute(paramInfo, scriptData, thisObj, containingObj,
                             scriptObj, locals, result, opcodeOffsetPtr);
  return true;
}
