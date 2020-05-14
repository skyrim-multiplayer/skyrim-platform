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

JsValue ConsoleApi::GetConsoleHookApi()
{
  JsValue cmdHookApi = JsValue::Object();

  cmdHookApi.SetProperty("findCommand",
                         JsValue::Function(ConsoleApi::FindConsoleComand));

  return cmdHookApi;
}

/*ConsoleApi::ConsoleComand& ConsoleApi::FillCmdInfo(
  const JsFunctionArguments& args)
{
  ConsoleApi::ConsoleComand cmd;

  auto comandName = args[1].ToString();
  auto comandNewLongName = args[2].ToString();
  auto comandNewShortName = args[3].ToString();
  double numArgs = 0;
  JsValue func = args[5];

  try {
    numArgs =
      std::get<double>(NativeValueCasts::JsValueToNativeValue(args[4]));
  } catch (const std::bad_variant_access& f) {
    std::string what = f.what();
    throw std::runtime_error("ReplaceConsoleCommand " + what);
  }

  if (numArgs < 0 || numArgs > 5)
    throw std::runtime_error(
      "ReplaceConsoleCommand argument count parameter too large ");
  auto log = RE::ConsoleLog::GetSingleton();
  cmd.originalName = comandName;
  cmd.longName = comandNewLongName;
  cmd.shortName = comandNewShortName;
  cmd.numArgs = (uint16_t)numArgs;
  cmd.execute = func;

  return cmd;
}*/

/*JsValue ConsoleApi::ReplaceConsoleCommand(const JsFunctionArguments& args)
{
  bool success = false;
  auto cmdInfo = FillCmdInfo(args);
  g.replacedConsoleCmd[cmdInfo.originalName] = cmdInfo;
  auto& myIter = g.replacedConsoleCmd[cmdInfo.originalName];
  auto log = RE::ConsoleLog::GetSingleton();

  for (ObScriptCommand* iter = g_firstConsoleCommand;
       iter->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase;
       ++iter) {
    if (iter->longName == myIter.originalName) {
      success = true;

      ObScriptCommand cmd = *iter;

      cmd.longName = myIter.longName.data();
      cmd.shortName = myIter.shortName.data();
      cmd.helpText = "";
      cmd.needsParent = 0;
      cmd.numParams = myIter.numArgs;
      cmd.execute = ConsoleComand_Execute;
      cmd.flags = 0;

      SafeWriteBuf((uintptr_t)iter, &cmd, sizeof(cmd));
    }
  }

  if (!success)
    throw std::runtime_error("wrong ConsoleCommand name to replace");

  return JsValue::Undefined();
}*/

JsValue GetterForLongName(const JsFunctionArguments& args)
{
  throw std::runtime_error("Getter work!");
  return JsValue::String("Getter work!");
}
void SetterForLongName(JsValue& newValue)
{

}

JsValue ConsoleApi::FindConsoleComand(const JsFunctionArguments& args)
{
  auto comandName = args[1].ToString();
  auto log = RE::ConsoleLog::GetSingleton();
  for (ObScriptCommand* iter = g_firstConsoleCommand;
       iter->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase;
       ++iter) {
    if (!stricmp(iter->longName, comandName.data())) {
      JsValueRef obj;
      JsCreateObject(&obj);

      JsValue::SetProperty(obj, "longName", GetterForLongName,
                           JsValue::Type::Getter);

      auto myObj = std::make_shared<CallNative::Object>(CallNative::Object("Object", obj));
      auto nativeObj = NativeValueCasts::NativeObjectToJsObject(myObj);
      return nativeObj;
     // return JsValue::ExternalObject((JsExternalObjectBase*)&obj);
    }
  }

  return JsValue::Null();
}

/*bool ConsoleApi::ConsoleComand_Execute(const ObScriptParam* paramInfo,
                                       ScriptData* scriptData,
                                       TESObjectREFR* thisObj,
                                       TESObjectREFR* containingObj,
                                       Script* scriptObj, ScriptLocals* locals,
                                       double& result, UInt32& opcodeOffsetPtr)
{
  auto log = RE::ConsoleLog::GetSingleton();

  if (log && scriptObj) {
    auto func = [&](int) {
      try {
        RE::Script* script = reinterpret_cast<RE::Script*>(scriptObj);
        auto comand = script->GetCommand();
        std::string f = "start exequte comand: " + comand;
        log->Print(f.data());
        const char* data = comand.data();
        for (auto& item : g.replacedConsoleCmd) {
          if (!stricmp(item.second.longName.data(), data) ||
              !stricmp(item.second.shortName.data(), data)) {
            log->Print("waiting for a response from the server");
            item.second.execute.Call({});
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
    return true;
  }
}*/
