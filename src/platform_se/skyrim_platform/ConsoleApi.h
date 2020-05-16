#pragma once
#include "JsEngine.h"
#include <skse64\ObScript.h>
#include <map>

namespace ConsoleApi {
struct ConsoleComand
{
  std::string longName = "";
  std::string shortName = "";
  uint16_t numArgs = 0;
  ObScript_Execute execute;
  JsValue jsExecute = JsValue::Undefined();
  ObScriptCommand* myIter;
};
static std::map<std::string, ConsoleComand> replacedConsoleCmd;
JsValue PrintConsole(const JsFunctionArguments& args);

JsValue GetConsoleHookApi();
JsValue FindConsoleComand(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("printConsole", JsValue::Function(PrintConsole));
  exports.SetProperty("tesConsole", GetConsoleHookApi());
}
JsValue ReplaceConsoleCommand(const JsFunctionArguments& args);

ConsoleComand FillCmdInfo(ObScriptCommand* cmd);

bool ConsoleComand_Execute(const ObScriptParam* paramInfo,
                           ScriptData* scriptData, TESObjectREFR* thisObj,
                           TESObjectREFR* containingObj, Script* scriptObj,
                           ScriptLocals* locals, double& result,
                           UInt32& opcodeOffsetPtr);
}