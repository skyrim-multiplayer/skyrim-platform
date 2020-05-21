#pragma once
#include "JsEngine.h"

class ObScriptCommand;

namespace ConsoleApi {

JsValue PrintConsole(const JsFunctionArguments& args);
JsValue FindConsoleComand(const JsFunctionArguments& args);

JsValue FindComand(ObScriptCommand* iter, size_t size,
                   const std::string& comandName);
void Clear();

inline void Register(JsValue& exports)
{
  exports.SetProperty("printConsole", JsValue::Function(PrintConsole));
  exports.SetProperty("findConsoleCommand",
                      JsValue::Function(ConsoleApi::FindConsoleComand));
}
}