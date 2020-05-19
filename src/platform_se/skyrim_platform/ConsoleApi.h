#pragma once
#include "JsEngine.h"
#include <map>
#include <skse64\ObScript.h>

namespace ConsoleApi {
struct ConsoleComand
{
  std::string longName = "";
  std::string shortName = "";
  uint16_t numArgs = 0;
  ObScript_Execute execute;
  JsValue jsExecute;
  ObScriptCommand* myIter;
  ObScriptCommand myOriginalData;
};
static std::map<std::string, ConsoleComand> replacedConsoleCmd;
JsValue PrintConsole(const JsFunctionArguments& args);
void ClearComandHooks(ObScriptCommand* iter, uint32_t end);
void Clear();
JsValue GetConsoleHookApi();
bool FindComand(ObScriptCommand* iter,
                               const std::string& comandName, uint32_t end, JsValue& res);
JsValue FindConsoleComand(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("printConsole", JsValue::Function(PrintConsole));
  exports.SetProperty("tesConsole", GetConsoleHookApi());
}

ConsoleComand FillCmdInfo(ObScriptCommand* cmd);

bool ConsoleComand_Execute(const ObScriptParam* paramInfo,
                           ScriptData* scriptData, TESObjectREFR* thisObj,
                           TESObjectREFR* containingObj, Script* scriptObj,
                           ScriptLocals* locals, double& result,
                           UInt32& opcodeOffsetPtr);
}