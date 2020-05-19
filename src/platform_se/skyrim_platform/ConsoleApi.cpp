#include "ConsoleApi.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include <RE/ConsoleLog.h>
#include <RE/Script.h>
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
  auto log = RE::ConsoleLog::GetSingleton();
  for (ObScriptCommand* iter = g_firstConsoleCommand;
       iter->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase;
       ++iter) {

    if (!iter || !iter->longName || !log)
      continue;
    std::string commandName = iter->longName;
    for (auto& item : replacedConsoleCmd) {
      if (IsCommandNameEqual(item.second.longName, commandName) ||
          IsCommandNameEqual(item.second.longName, commandName)) {
        log->Print(iter->longName);
        SafeWriteBuf((uintptr_t)iter, &(item.second.myOriginalData),
                     sizeof(item.second.myOriginalData));
        break;
      }
    }
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
  auto log = RE::ConsoleLog::GetSingleton();
  log->Print("find start %s", comandName.data());

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
}
#include <RE/TESObjectREFR.h>
bool ConsoleApi::ConsoleComand_Execute(const ObScriptParam* paramInfo,
                                       ScriptData* scriptData,
                                       TESObjectREFR* thisObj,
                                       TESObjectREFR* containingObj,
                                       Script* scriptObj, ScriptLocals* locals,
                                       double& result, UInt32& opcodeOffsetPtr)
{
  if (!scriptObj)
    throw NullPointerException("scriptObj");
  auto log = RE::ConsoleLog::GetSingleton();
  RE::Script* script = reinterpret_cast<RE::Script*>(scriptObj);
  std::string comand = script->GetCommand();
  std::vector<std::string> params;
  GetArgs(comand, params);
  std::pair<const std::string, ConsoleComand>* iterator = nullptr;

  auto func = [&](int) {
    try {
      for (auto& item : replacedConsoleCmd) {
        if (IsCommandNameEqual(item.second.longName, params.at(0)) ||
            IsCommandNameEqual(item.second.shortName, params.at(0))) {

          std::vector<JsValue> args;
          auto refr = reinterpret_cast<RE::TESObjectREFR*>(thisObj);
          args.push_back(JsValue::Undefined());
          args.push_back(JsValue::String(std::to_string(refr->formID)));

          for (size_t i = 1; i < params.size(); ++i) {
            args.push_back(JsValue::String(params[i]));
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
