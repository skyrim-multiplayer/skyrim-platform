#include "ConsoleApi.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include <RE/ConsoleLog.h>
#include <RE/Script.h>
#include <ctpl\ctpl_stl.h>
#include <skse64_common\SafeWrite.h>

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

void ConsoleApi::Clear()
{
  auto log = RE::ConsoleLog::GetSingleton();
  for (ObScriptCommand* iter = g_firstConsoleCommand;
       iter->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase;
       ++iter) {

    if (!iter || !iter->longName || !log)
      continue;

    for (auto& item : replacedConsoleCmd) {
      if (!stricmp(item.second.longName.data(), iter->longName) ||
          !stricmp(item.second.shortName.data(), iter->shortName)) {
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

JsValue ConsoleApi::FindConsoleComand(const JsFunctionArguments& args)
{
  auto comandName = args[1].ToString();
  auto log = RE::ConsoleLog::GetSingleton();

  for (ObScriptCommand* iter = g_firstConsoleCommand;
       iter->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase;
       ++iter) {
    if (!stricmp(iter->longName, comandName.data()) ||
        !stricmp(iter->shortName, comandName.data())) {
      JsValue obj = JsValue::Object();

      replacedConsoleCmd[comandName] = FillCmdInfo(iter);
      auto replaced = replacedConsoleCmd.find(comandName);

      CreateLongNameProperty(obj, &(*replaced));
      CreateShortNameProperty(obj, &(*replaced));
      CreateNumArgsProperty(obj, &(*replaced));
      CreateExecuteProperty(obj, &(*replaced));

      ObScriptCommand cmd = *iter;
      cmd.execute = ConsoleApi::ConsoleComand_Execute;
      SafeWriteBuf((uintptr_t)iter, &cmd, sizeof(cmd));
      return obj;
    }
  }
  return JsValue::Null();
}

bool ConsoleApi::ConsoleComand_Execute(const ObScriptParam* paramInfo,
                                       ScriptData* scriptData,
                                       TESObjectREFR* thisObj,
                                       TESObjectREFR* containingObj,
                                       Script* scriptObj, ScriptLocals* locals,
                                       double& result, UInt32& opcodeOffsetPtr)
{
  if (!scriptObj)
    throw NullPointerException("scriptObj");

  RE::Script* script = reinterpret_cast<RE::Script*>(scriptObj);
  std::string comandName = script->GetCommand();
  std::pair<const std::string, ConsoleComand>* iterator = nullptr;
  
  auto log = RE::ConsoleLog::GetSingleton();

  //script->data->GetChunk()->AsString()->GetString().c_str();
  /* log->Print("start");
  std::string resS = script->data->GetChunk()->AsString()->GetString();
   int  resI = script->data->GetChunk()->AsInteger()->GetInteger();
  auto res = "res = " + resS + std::to_string(resI);
  log->Print(res.data());*/

  auto func = [&](int) {
    try {
      for (auto& item : replacedConsoleCmd) {
        if (!stricmp(item.second.longName.data(), comandName.data()) ||
            !stricmp(item.second.shortName.data(), comandName.data())) {
          
            
          //log->Print(param.paramData);
          //log->Print("end");
          if (item.second.jsExecute.Call({}))
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
