#include "EventsApi.h"

#include "InvalidArgumentException.h"
#include "MyUpdateTask.h"
#include "NativeObject.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include <map>
#include <set>
#include <unordered_map>

#include <RE\ConsoleLog.h>
#include <skse64\ObScript.h>
#include <skse64_common\SafeWrite.h>

extern ctpl::thread_pool g_pool;
extern TaskQueue g_taskQueue;

struct EventsGlobalState
{
  using Callbacks = std::map<std::string, std::vector<JsValue>>;
  Callbacks callbacks;
  Callbacks callbacksOnce;

  class Handler
  {
  public:
    Handler() = default;

    Handler(const JsValue& handler_)
      : enter(handler_.GetProperty("enter"))
      , leave(handler_.GetProperty("leave"))
    {
    }

    JsValue enter, leave;

    struct PerThread
    {
      JsValue storage, context;
    };

    std::unordered_map<DWORD, PerThread> perThread;
  };

  struct HookInfo
  {
    std::set<DWORD> inProgressThreads;
    std::vector<Handler> handlers;
  };

  struct ConsoleComand
  {
    std::string originalName = "";
    std::string longName = "";
    std::string shortName = "";
    uint16_t numArgs = 0;
    JsValue execute = JsValue::Undefined();
  };

  std::map<std::string, ConsoleComand> replacedConsoleCmd;
  HookInfo sendAnimationEvent;
} g;

namespace {
struct SendAnimationEventTag
{
  static constexpr auto name = "sendAnimationEvent";
};
}

namespace {
void CallCalbacks(const char* eventName, const std::vector<JsValue>& arguments,
                  bool isOnce = false)
{
  EventsGlobalState::Callbacks callbacks =
    isOnce ? g.callbacksOnce : g.callbacks;

  if (isOnce)
    g.callbacksOnce[eventName].clear();

  for (auto& f : callbacks[eventName]) {
    f.Call(arguments);
  }
}
}

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<JsValue>& arguments)
{
  CallCalbacks(eventName, arguments);
  CallCalbacks(eventName, arguments, true);
}

void EventsApi::Clear()
{
  g = {};
}

namespace {
enum class ClearStorage
{
  Yes,
  No
};

void PrepareContext(EventsGlobalState::Handler::PerThread& h,
                    ClearStorage clearStorage)
{
  if (h.context.GetType() != JsValue::Type::Object) {
    h.context = JsValue::Object();
  }

  thread_local auto g_standardMap = JsValue::GlobalObject().GetProperty("Map");
  thread_local auto g_clear =
    g_standardMap.GetProperty("prototype").GetProperty("clear");
  if (h.storage.GetType() != JsValue::Type::Object) {
    h.storage = g_standardMap.Constructor({ g_standardMap });
    h.context.SetProperty("storage", h.storage);
  }

  if (clearStorage == ClearStorage::Yes)
    g_clear.Call({ h.storage });
}
}

void EventsApi::SendAnimationEventEnter(uint32_t selfId,
                                        std::string& animEventName) noexcept
{
  DWORD owningThread = GetCurrentThreadId();
  auto f = [&](int) {
    try {
      if (g.sendAnimationEvent.inProgressThreads.count(owningThread))
        throw std::runtime_error("'sendAnimationEvent' is already processing");

      // This should always be done before calling throwing functions
      g.sendAnimationEvent.inProgressThreads.insert(owningThread);

      for (auto& h : g.sendAnimationEvent.handlers) {
        auto& perThread = h.perThread[owningThread];
        PrepareContext(perThread, ClearStorage::Yes);

        perThread.context.SetProperty("selfId", (double)selfId);
        perThread.context.SetProperty("animEventName", animEventName);

        h.enter.Call({ JsValue::Undefined(), perThread.context });

        animEventName =
          (std::string)perThread.context.GetProperty("animEventName");
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in SendAnimationEventEnter)");
      });
    }
  };
  g_pool.push(f).wait();
}

void EventsApi::SendAnimationEventLeave(bool animationSucceeded) noexcept
{
  DWORD owningThread = GetCurrentThreadId();
  auto f = [&](int) {
    try {
      if (!g.sendAnimationEvent.inProgressThreads.count(owningThread))
        throw std::runtime_error("'sendAnimationEvent' is not processing");
      g.sendAnimationEvent.inProgressThreads.erase(owningThread);

      for (auto& h : g.sendAnimationEvent.handlers) {
        auto& perThread = h.perThread.at(owningThread);
        PrepareContext(perThread, ClearStorage::No);

        perThread.context.SetProperty("animationSucceeded",
                                      JsValue::Bool(animationSucceeded));
        h.leave.Call({ JsValue::Undefined(), perThread.context });

        h.perThread.erase(owningThread);
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in SendAnimationEventLeave)");
      });
    }
  };
  g_pool.push(f).wait();
}

namespace {
JsValue CreateHook(EventsGlobalState::HookInfo* hookInfo)
{
  auto hook = JsValue::Object();
  hook.SetProperty(
    "add", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      auto handlerObj = args[1];
      hookInfo->handlers.push_back(EventsGlobalState::Handler(handlerObj));
      return JsValue::Undefined();
    }));
  return hook;
}
}

JsValue EventsApi::GetHooks()
{
  std::map<std::string, EventsGlobalState::HookInfo*> hooksMap = {
    { "sendAnimationEvent", &g.sendAnimationEvent }
  };

  auto hooks = JsValue::Object();
  for (auto [name, hookInfo] : hooksMap)
    hooks.SetProperty(name, CreateHook(hookInfo));
  return hooks;
}

namespace {
JsValue AddCallback(const JsFunctionArguments& args, bool isOnce = false)
{
  auto eventName = args[1].ToString();
  auto callback = args[2];

  std::set<std::string> events = { "tick", "update" };

  if (events.count(eventName) == 0)
    throw InvalidArgumentException("eventName", eventName);

  isOnce ? g.callbacksOnce[eventName].push_back(callback)
         : g.callbacks[eventName].push_back(callback);
  return JsValue::Undefined();
}
}

JsValue EventsApi::On(const JsFunctionArguments& args)
{
  return AddCallback(args);
}

JsValue EventsApi::Once(const JsFunctionArguments& args)
{
  return AddCallback(args, true);
}
#include <GameForms.h>

namespace {
bool ConsoleComand_Execute(const ObScriptParam* paramInfo,
                           ScriptData* scriptData, TESObjectREFR* thisObj,
                           TESObjectREFR* containingObj, Script* scriptObj,
                           ScriptLocals* locals, double& result,
                           UInt32& opcodeOffsetPtr)
{
  auto log = RE::ConsoleLog::GetSingleton();
   
  if (log && scriptObj) {
      auto formId = std::to_string(scriptObj->formID);
      auto type = std::to_string(scriptObj->formType);
      std::string f = "scriptObj: formId = " + formId +" type = "+ type;
      log->Print(f.data());
  }
  log->Print("end");
  return true;
}

EventsGlobalState::ConsoleComand FillCmdInfo(const JsFunctionArguments& args) noexcept 
{
  EventsGlobalState::ConsoleComand cmd;

 
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
}
}

JsValue EventsApi::ReplaceConsoleCommand(const JsFunctionArguments& args)
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
}
