#include "MpClientPluginApi.h"
#include <DbgHelp.h>
#include <map>
#include <memory>
#include <stdexcept>

namespace {
const char* GetPacketTypeName(int32_t type)
{
  switch (type) {
    case 0:
      return "message";
    case 1:
      return "disconnect";
    case 2:
      return "connectionAccepted";
    case 3:
      return "connectionFailed";
    case 4:
      return "connectionDenied";
    default:
      return "";
  }
}

class MpClientPlugin
{
public:
  MpClientPlugin()
  {
    hModule = LoadLibraryA("Data/SKSE/Plugins/MpClientPlugin.dll");
    if (!hModule) {
      throw std::runtime_error("Unable to load MpClientPlugin, error code " +
                               std::to_string(GetLastError()));
    }
  }

  void* GetFunction(const char* funcName)
  {
    auto addr = GetProcAddress(hModule, funcName);
    if (!addr) {
      throw std::runtime_error(
        "Unable to find function with name '" + std::string(funcName) +
        "' in MpClientPlugin, error code " + std::to_string(GetLastError()));
    }
    return addr;
  }

  HMODULE hModule = nullptr;
};

MpClientPlugin* GetMpClientPlugin()
{
  static MpClientPlugin instance;
  return &instance;
}
}

JsValue MpClientPluginApi::GetVersion(const JsFunctionArguments& args)
{
  typedef const char* (*GetVersion)();
  auto f = (GetVersion)GetMpClientPlugin()->GetFunction("MpCommonGetVersion");
  return JsValue::String(f());
}

JsValue MpClientPluginApi::CreateClient(const JsFunctionArguments& args)
{
  auto hostName = (std::string)args[1];
  auto port = (int)args[2];

  if (port < 0 || port > 65535)
    throw std::runtime_error(std::to_string(port) + " is not a valid port");

  typedef void (*CreateClient)(const char* hostName, uint16_t port);
  auto f = (CreateClient)GetMpClientPlugin()->GetFunction("CreateClient");
  f(hostName.data(), (uint16_t)port);
  return JsValue::Undefined();
}

JsValue MpClientPluginApi::DestroyClient(const JsFunctionArguments& args)
{
  typedef void (*DestroyClient)();
  auto f = (DestroyClient)GetMpClientPlugin()->GetFunction("DestroyClient");
  f();
  return JsValue::Undefined();
}

JsValue MpClientPluginApi::IsConnected(const JsFunctionArguments& args)
{
  typedef bool (*IsConnected)();
  auto f = (IsConnected)GetMpClientPlugin()->GetFunction("IsConnected");
  return JsValue::Bool(f());
}

JsValue MpClientPluginApi::Tick(const JsFunctionArguments& args)
{
  auto onPacket = args[1];

  typedef void (*OnPacket)(int32_t type, const char* jsonContent,
                           const char* error, void* state);
  typedef void (*Tick)(OnPacket onPacket, void* state);

  auto f = (Tick)GetMpClientPlugin()->GetFunction("Tick");
  f(
    [](int32_t type, const char* jsonContent, const char* error, void* state) {
      auto onPacket = reinterpret_cast<JsValue*>(state);
      onPacket->Call(
        { JsValue::Undefined(), GetPacketTypeName(type), jsonContent, error });
    },
    &onPacket);
  return JsValue::Undefined();
}

JsValue MpClientPluginApi::Send(const JsFunctionArguments& args)
{
  typedef void (*SendString)(const char* jsonContent, bool reliable);
  typedef void (*SendData)(uint8_t* data, size_t dataSize, bool reliable);

  // Call old function
  if (args[1].GetType() == JsValue::Type::String) {
    auto jsonContent = (std::string)args[1];
    auto reliable = (bool)args[2];

    auto f = (SendString)GetMpClientPlugin()->GetFunction("SendString");
    f(jsonContent.data(), reliable);
    return JsValue::Undefined();
  }

  auto data = args[0].GetArrayBufferData();
  auto len = args[0].GetArrayBufferLength();

  if (!data || len == 0) // return if data is empty
    return JsValue::Undefined();

  auto f = (SendData)GetMpClientPlugin()->GetFunction("SendData");
  f((uint8_t*)data, len, (bool)args[2]);
  return JsValue::Undefined();
}