#include "skse64/PluginAPI.h"
#include "skse64_common/SafeWrite.h"
#include "skse64_common/skse_version.h"

#include "skse64/GameAPI.h"
#include "skse64/GameData.h"
#include "skse64/GameEvents.h"
#include "skse64/GameExtraData.h"
#include "skse64/GameObjects.h"
#include "skse64/GameRTTI.h"

#include "skse64/PapyrusEvents.h"
#include "skse64/PapyrusVM.h"

#include "skse64/NiMaterial.h"
#include "skse64/NiNodes.h"
#include "skse64/NiProperties.h"
#include "skse64/NiRTTI.h"

#include "skse64/ScaleformCallbacks.h"
#include "skse64/ScaleformMovie.h"

/*#include "PluginInterface.h"
#include "ActorUpdateManager.h"
#include "BodyMorphInterface.h"
#include "ItemDataInterface.h"
#include "NiTransformInterface.h"
#include "OverlayInterface.h"
#include "OverrideInterface.h"
#include "SkeletonExtender.h"
#include "TintMaskInterface.h"

#include "FaceMorphInterface.h"
#include "PartHandler.h"

#include "ScaleformCharGenFunctions.h"
#include "ScaleformFunctions.h"
#include "ShaderUtilities.h"*/

#include "SKMain.h"
#include "SKStringTable.h"

#include <chrono>
#include <shlobj.h>
#include <string>

/*#include "PapyrusNiOverride.h"
#include "PapyrusCharGen.h"
#include "SKEEHooks.h"*/

// IDebugLog	gLog;

// PluginHandle					g_pluginHandle =
// kPluginHandle_Invalid;

// Interfaces
SKSESerializationInterface* g_serialization = nullptr;
SKSEScaleformInterface* g_scaleform = nullptr;
SKSETaskInterface* g_task = nullptr;
SKSEMessagingInterface* g_messaging = nullptr;
SKSEPapyrusInterface* g_papyrus = nullptr;
// SKSETrampolineInterface			* g_trampoline = nullptr;

#include "SKItemDataInterface.h"
#include "SKTintMaskInterface.h"

// Handlers
/*InterfaceMap				g_interfaceMap;
DyeMap						g_dyeMap;
OverrideInterface			g_overrideInterface;*/
TintMaskInterface g_tintMaskInterface; /*
OverlayInterface g_overlayInterface;
BodyMorphInterface g_bodyMorphInterface;*/
ItemDataInterface g_itemDataInterface; /*
NiTransformInterface		g_transformInterface;
FaceMorphInterface			g_morphInterface;
SkeletonExtenderInterface	g_skeletonExtenderInterface;
ActorUpdateManager			g_actorUpdateManager;
PartSet	g_partSet;*/

StringTable g_stringTable;

// Feature Toggles
bool g_enableOverlays = true;
bool g_enableSculpting = true;
bool g_enableBodyGen = true;
bool g_enableAutoTransforms = true;
bool g_enableHeadExport = true;
bool g_enableBodyMorph = true;
bool g_enableTintSync = true;
bool g_enableTintInventory = true;
bool g_enableTintHairSlot = true;

bool g_playerOnly = true;
UInt32 g_numBodyOverlays = 3;
UInt32 g_numHandOverlays = 3;
UInt32 g_numFeetOverlays = 3;
UInt32 g_numFaceOverlays = 3;
UInt32 g_numSpellBodyOverlays = 1;
UInt32 g_numSpellHandOverlays = 1;
UInt32 g_numSpellFeetOverlays = 1;
UInt32 g_numSpellFaceOverlays = 1;
UInt32 g_tintHairSlot = 1;

bool g_alphaOverride = true;
UInt16 g_alphaFlags = 4845;
UInt16 g_alphaThreshold = 0;

UInt16 g_loadMode = 0;

bool g_enableBodyInit = true;
bool g_firstLoad = false;
bool g_immediateArmor = true;
bool g_enableFaceOverlays = true;
bool g_immediateFace = false;
bool g_enableEquippableTransforms = true;
bool g_parallelMorphing = true;
bool g_deferredBodyMorph = false;
UInt16 g_scaleMode = 0;
UInt16 g_bodyMorphMode = 0;

bool g_externalHeads = false;
bool g_extendedMorphs = true;
bool g_allowAllMorphs = true;
bool g_allowAnyRacePart = false;
bool g_allowAnyGenderPart = false;
bool g_disableFaceGenCache = true;
bool g_exportSkinToBone = true;
float g_sliderMultiplier = 1.0f;
float g_sliderInterval = 0.01f;
UInt32 g_numPresets = 10;
UInt32 g_customDataMax = 10;
std::string g_raceTemplate = "NordRace";

// Compact DirectX vars
/*#include "CDXCamera.h"
#include "CDXBrush.h"
#include "CDXNifScene.h"

CDXD3DDevice*		g_Device = nullptr;
CDXModelViewerCamera	g_Camera;
CDXNifScene				g_World;*/

float g_panSpeed = 0.01f;
float g_cameraFOV = 45.0f;
SInt32 g_viewWidth = 1024;
SInt32 g_viewHeight = 1024;
float g_sculptBackgroundA = 0.0f;
float g_sculptBackgroundR = 0.0f;
float g_sculptBackgroundG = 0.0f;
float g_sculptBackgroundB = 0.0f;
float g_sculptOffsetX = 0.0f;
float g_sculptOffsetY = 0.0f;
float g_sculptOffsetZ = 0.0f;
bool g_sculptDrawBrushPoint = true;
bool g_sculptDrawBrushRadius = true;

// extern double
// g_brushProperties[CDXBrush::kBrushTypes][CDXBrush::kBrushProperties][CDXBrush::kBrushPropertyValues];

#define MIN_SERIALIZATION_VERSION 1
#define MIN_TASK_VERSION 2
#define MIN_SCALEFORM_VERSION 1
#define MIN_PAPYRUS_VERSION 1

const std::string& F4EEGetRuntimeDirectory(void)
{
  static std::string s_runtimeDirectory;

  if (s_runtimeDirectory.empty()) {
    // can't determine how many bytes we'll need, hope it's not more than
    // MAX_PATH
    char runtimePathBuf[MAX_PATH];
    UInt32 runtimePathLength = GetModuleFileName(
      GetModuleHandle(NULL), runtimePathBuf, sizeof(runtimePathBuf));

    if (runtimePathLength && (runtimePathLength < sizeof(runtimePathBuf))) {
      std::string runtimePath(runtimePathBuf, runtimePathLength);

      // truncate at last slash
      std::string::size_type lastSlash = runtimePath.rfind('\\');
      if (lastSlash != std::string::npos) // if we don't find a slash something
                                          // is VERY WRONG
      {
        s_runtimeDirectory = runtimePath.substr(0, lastSlash + 1);

        _DMESSAGE("runtime root = %s", s_runtimeDirectory.c_str());
      } else {
        _WARNING("no slash in runtime path? (%s)", runtimePath.c_str());
      }
    } else {
      _WARNING("couldn't find runtime path (len = %d, err = %08X)",
               runtimePathLength, GetLastError());
    }
  }

  return s_runtimeDirectory;
}

const std::string& SKEE64GetConfigPath(bool custom = false)
{
  static std::string s_configPath;
  static std::string s_configPathCustom;

  if (s_configPath.empty()) {
    std::string runtimePath = F4EEGetRuntimeDirectory();
    if (!runtimePath.empty()) {
      s_configPath = runtimePath + "Data\\SKSE\\Plugins\\skee64.ini";

      _MESSAGE("default config path = %s", s_configPath.c_str());
    }
  }
  if (s_configPathCustom.empty()) {
    std::string runtimePath = F4EEGetRuntimeDirectory();
    if (!runtimePath.empty()) {
      s_configPathCustom =
        runtimePath + "Data\\SKSE\\Plugins\\skee64_custom.ini";

      _MESSAGE("custom config path = %s", s_configPathCustom.c_str());
    }
  }

  return custom ? s_configPathCustom : s_configPath;
}

std::string SKEE64GetConfigOption(const char* section, const char* key)
{
  std::string result;

  const std::string& configPath = SKEE64GetConfigPath();
  const std::string& configPathCustom = SKEE64GetConfigPath(true);

  char resultBuf[256];
  resultBuf[0] = 0;

  if (!configPath.empty()) {
    UInt32 resultLen = GetPrivateProfileString(
      section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());
    result = resultBuf;
  }
  if (!configPathCustom.empty()) {
    UInt32 resultLen =
      GetPrivateProfileString(section, key, NULL, resultBuf, sizeof(resultBuf),
                              configPathCustom.c_str());
    if (resultLen > 0) // Only take custom if we have it
      result = resultBuf;
  }

  return result;
}

template <typename T>
const char* SKEE64GetTypeFormatting(T* dataOut)
{
  return false;
}

template <>
const char* SKEE64GetTypeFormatting(double* dataOut)
{
  return "%lf";
}
template <>
const char* SKEE64GetTypeFormatting(float* dataOut)
{
  return "%f";
}
template <>
const char* SKEE64GetTypeFormatting(bool* dataOut)
{
  return "%c";
}
template <>
const char* SKEE64GetTypeFormatting(SInt16* dataOut)
{
  return "%hd";
}
template <>
const char* SKEE64GetTypeFormatting(UInt16* dataOut)
{
  return "%hu";
}
template <>
const char* SKEE64GetTypeFormatting(SInt32* dataOut)
{
  return "%d";
}
template <>
const char* SKEE64GetTypeFormatting(UInt32* dataOut)
{
  return "%u";
}
template <>
const char* SKEE64GetTypeFormatting(UInt64* dataOut)
{
  return "%I64u";
}

template <typename T>
bool SKEE64GetConfigValue(const char* section, const char* key, T* dataOut)
{
  std::string data = SKEE64GetConfigOption(section, key);
  if (data.empty())
    return false;

  T tmp;
  bool res =
    (sscanf_s(data.c_str(), SKEE64GetTypeFormatting(dataOut), &tmp) == 1);
  if (res) {
    *dataOut = tmp;
  }
  return res;
}

template <>
bool SKEE64GetConfigValue(const char* section, const char* key, bool* dataOut)
{
  std::string data = SKEE64GetConfigOption(section, key);
  if (data.empty())
    return false;

  UInt32 tmp;
  bool res =
    (sscanf_s(data.c_str(), SKEE64GetTypeFormatting(&tmp), &tmp) == 1);
  if (res) {
    *dataOut = (tmp > 0);
  }
  return res;
}

// This event isnt hooked up SKSE end
/*void SKEE64Serialization_FormDelete(UInt64 handle)
{
        g_overrideInterface.RemoveAllReferenceOverrides(handle);
        g_overrideInterface.RemoveAllReferenceNodeOverrides(handle);
}

void SKEE64Serialization_Revert(SKSESerializationInterface * intfc)
{
        _MESSAGE("Reverting...");

        g_overlayInterface.Revert();
        g_overrideInterface.Revert();
        g_bodyMorphInterface.Revert();
        g_itemDataInterface.Revert();
        g_dyeMap.Revert();
        g_transformInterface.Revert();
        g_morphInterface.Revert();
        g_stringTable.Revert();
}*/

class StopWatch
{
public:
  StopWatch() {}

  void Start() { start = std::chrono::system_clock::now(); }

  long long Stop()
  {
    end = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
      .count();
  }

private:
  std::chrono::system_clock::time_point start;
  std::chrono::system_clock::time_point end;
};

/*void SKEE64Serialization_Save(SKSESerializationInterface * intfc)
{
        _MESSAGE("Saving...");

        StopWatch sw;
        UInt32 strCount = 0;

        sw.Start();
        g_stringTable.Save(intfc, StringTable::kSerializationVersion);
        _DMESSAGE("%s - Serialized string table %dms", __FUNCTION__,
sw.Stop());

        sw.Start();
        g_morphInterface.Save(intfc,
FaceMorphInterface::kSerializationVersion); _DMESSAGE("%s - Player morph data
%dms", __FUNCTION__, sw.Stop());

        sw.Start();
        g_transformInterface.Save(intfc,
NiTransformInterface::kSerializationVersion); _DMESSAGE("%s - Serialized
transforms %dms", __FUNCTION__, sw.Stop());

        sw.Start();
        g_overlayInterface.Save(intfc,
OverlayInterface::kSerializationVersion); _DMESSAGE("%s - Serialized overlays
%dms", __FUNCTION__, sw.Stop());

        sw.Start();
        g_overrideInterface.Save(intfc,
OverrideInterface::kSerializationVersion); _DMESSAGE("%s - Serialized overrides
%dms", __FUNCTION__, sw.Stop());

        sw.Start();
        g_bodyMorphInterface.Save(intfc,
BodyMorphInterface::kSerializationVersion); _DMESSAGE("%s - Serialized body
morphs %dms", __FUNCTION__, sw.Stop());

        sw.Start();
        g_itemDataInterface.Save(intfc,
ItemDataInterface::kSerializationVersion); _DMESSAGE("%s - Serialized item data
%dms", __FUNCTION__, sw.Stop());
}

void SKEE64Serialization_Load(SKSESerializationInterface * intfc)
{
        _MESSAGE("Loading...");

        UInt32 type, length, version;
        bool error = false;

        std::unordered_map<UInt32, StringTableItem> stringTable;

        StopWatch sw;
        sw.Start();
        while (intfc->GetNextRecordInfo(&type, &version, &length))
        {
                switch (type)
                {
                        case 'STTB':	g_stringTable.Load(intfc, version,
stringTable);						break; case 'MRST':
g_morphInterface.LoadMorphData(intfc, version, stringTable);
break; case 'SCDT':	g_morphInterface.LoadSculptData(intfc, version,
stringTable);			break; case 'AOVL':
g_overlayInterface.Load(intfc, version);
break; case 'ACEN':	g_overrideInterface.LoadOverrides(intfc, version,
stringTable);			break; case 'NDEN':
g_overrideInterface.LoadNodeOverrides(intfc, version, stringTable);
break; case 'WPEN':	g_overrideInterface.LoadWeaponOverrides(intfc, version,
stringTable);	break; case 'SKNR':
g_overrideInterface.LoadSkinOverrides(intfc, version, stringTable);
break; case 'MRPH':	g_bodyMorphInterface.Load(intfc, version, stringTable);
break; case 'ITEE':	g_itemDataInterface.Load(intfc, version, stringTable);
break; case 'ACTM':	g_transformInterface.Load(intfc, version, stringTable);
break; default: _MESSAGE("unhandled type %08X (%.4s)", type, &type); error =
true; break;
                }
        }
        _DMESSAGE("%s - Loaded %dms", __FUNCTION__, sw.Stop());

        PlayerCharacter * player = (*g_thePlayer);
        g_task->AddTask(new SKSETaskApplyMorphs(player));

        g_firstLoad = true;

#ifdef _DEBUG
        g_overrideInterface.DumpMap();
        g_overlayInterface.DumpMap();
#endif
}

bool RegisterNiOverrideScaleform(GFxMovieView * view, GFxValue * root)
{
        GFxValue obj;
        RegisterBool(root, "bEnableOverlays", g_enableOverlays);

        view->CreateObject(&obj);
        RegisterNumber(&obj, "iNumOverlays", g_numBodyOverlays);
        RegisterNumber(&obj, "iSpellOverlays", g_numSpellBodyOverlays);
        root->SetMember("body", &obj);

        obj.SetNull();
        view->CreateObject(&obj);
        RegisterNumber(&obj, "iNumOverlays", g_numHandOverlays);
        RegisterNumber(&obj, "iSpellOverlays", g_numSpellHandOverlays);
        root->SetMember("hand", &obj);

        obj.SetNull();
        view->CreateObject(&obj);
        RegisterNumber(&obj, "iNumOverlays", g_numFeetOverlays);
        RegisterNumber(&obj, "iSpellOverlays", g_numSpellFeetOverlays);
        root->SetMember("feet", &obj);

        obj.SetNull();
        view->CreateObject(&obj);
        RegisterNumber(&obj, "iNumOverlays", g_numFaceOverlays);
        RegisterNumber(&obj, "iSpellOverlays", g_numSpellFaceOverlays);
        root->SetMember("face", &obj);

        RegisterBool(root, "bPlayerOnly", g_playerOnly);

        RegisterFunction <SKSEScaleform_GetDyeItems>(root, view,
"GetDyeItems"); RegisterFunction <SKSEScaleform_GetDyeableItems>(root, view,
"GetDyeableItems"); RegisterFunction <SKSEScaleform_SetItemDyeColor>(root,
view, "SetItemDyeColor"); RegisterFunction
<SKSEScaleform_SetItemDyeColors>(root, view, "SetItemDyeColors");

        return true;
}

bool RegisterCharGenScaleform(GFxMovieView * view, GFxValue * root)
{
        RegisterBool(root, "bEnableSculpting", g_enableSculpting);
        RegisterBool(root, "bEnableHeadExport", g_enableHeadExport);

        RegisterFunction <SKSEScaleform_ImportHead>(root, view, "ImportHead");
        RegisterFunction <SKSEScaleform_ExportHead>(root, view, "ExportHead");
        RegisterFunction <SKSEScaleform_SavePreset>(root, view, "SavePreset");
        RegisterFunction <SKSEScaleform_LoadPreset>(root, view, "LoadPreset");
        RegisterFunction <SKSEScaleform_ReadPreset>(root, view, "ReadPreset");
        RegisterFunction <SKSEScaleform_ReloadSliders>(root, view,
"ReloadSliders"); RegisterFunction <SKSEScaleform_GetSliderData>(root, view,
"GetSliderData"); RegisterFunction <SKSEScaleform_GetModName>(root, view,
"GetModName");

        RegisterFunction <SKSEScaleform_GetPlayerPosition>(root, view,
"GetPlayerPosition"); RegisterFunction <SKSEScaleform_GetPlayerRotation>(root,
view, "GetPlayerRotation"); RegisterFunction
<SKSEScaleform_SetPlayerRotation>(root, view, "SetPlayerRotation");

        RegisterFunction <SKSEScaleform_GetRaceSexCameraRot>(root, view,
"GetRaceSexCameraRot"); RegisterFunction
<SKSEScaleform_GetRaceSexCameraPos>(root, view, "GetRaceSexCameraPos");
        RegisterFunction <SKSEScaleform_SetRaceSexCameraPos>(root, view,
"SetRaceSexCameraPos");

        RegisterFunction <SKSEScaleform_CreateMorphEditor>(root, view,
"CreateMorphEditor"); RegisterFunction <SKSEScaleform_ReleaseMorphEditor>(root,
view, "ReleaseMorphEditor");

        RegisterFunction <SKSEScaleform_LoadImportedHead>(root, view,
"LoadImportedHead"); RegisterFunction <SKSEScaleform_ReleaseImportedHead>(root,
view, "ReleaseImportedHead");

        RegisterFunction <SKSEScaleform_BeginRotateMesh>(root, view,
"BeginRotateMesh"); RegisterFunction <SKSEScaleform_DoRotateMesh>(root, view,
"DoRotateMesh"); RegisterFunction <SKSEScaleform_EndRotateMesh>(root, view,
"EndRotateMesh");

        RegisterFunction <SKSEScaleform_BeginPanMesh>(root, view,
"BeginPanMesh"); RegisterFunction <SKSEScaleform_DoPanMesh>(root, view,
"DoPanMesh"); RegisterFunction <SKSEScaleform_EndPanMesh>(root, view,
"EndPanMesh");

        RegisterFunction <SKSEScaleform_BeginPaintMesh>(root, view,
"BeginPaintMesh"); RegisterFunction <SKSEScaleform_DoPaintMesh>(root, view,
"DoPaintMesh"); RegisterFunction <SKSEScaleform_EndPaintMesh>(root, view,
"EndPaintMesh");

        RegisterFunction <SKSEScaleform_DoHoverMesh>(root, view,
"DoHoverMesh");

        RegisterFunction <SKSEScaleform_GetCurrentBrush>(root, view,
"GetCurrentBrush"); RegisterFunction <SKSEScaleform_SetCurrentBrush>(root,
view, "SetCurrentBrush");

        RegisterFunction <SKSEScaleform_GetBrushes>(root, view, "GetBrushes");
        RegisterFunction <SKSEScaleform_SetBrushData>(root, view,
"SetBrushData");

        RegisterFunction <SKSEScaleform_GetMeshes>(root, view, "GetMeshes");
        RegisterFunction <SKSEScaleform_SetMeshData>(root, view,
"SetMeshData");

        RegisterFunction <SKSEScaleform_UndoAction>(root, view, "UndoAction");
        RegisterFunction <SKSEScaleform_RedoAction>(root, view, "RedoAction");
        RegisterFunction <SKSEScaleform_GoToAction>(root, view, "GoToAction");
        RegisterFunction <SKSEScaleform_GetActionLimit>(root, view,
"GetActionLimit"); RegisterFunction <SKSEScaleform_ClearSculptData>(root, view,
"ClearSculptData");

        RegisterFunction <SKSEScaleform_GetMeshCameraRadius>(root, view,
"GetMeshCameraRadius"); RegisterFunction
<SKSEScaleform_SetMeshCameraRadius>(root, view, "SetMeshCameraRadius");

        RegisterFunction <SKSEScaleform_GetExternalFiles>(root, view,
"GetExternalFiles"); return true;
}*/

/*bool RegisterPapyrusFunctions(VMClassRegistry * registry)
{
        papyrusNiOverride::RegisterFuncs(registry);
        papyrusCharGen::RegisterFuncs(registry);
        return true;
}*/

/*void SKSEMessageHandler(SKSEMessagingInterface::Message * message)
{
        switch (message->type)
        {
                case SKSEMessagingInterface::kMessage_InputLoaded:
                {
                        if (g_enableAutoTransforms || g_enableBodyGen) {
                                GetEventDispatcherList()->objectLoadedDispatcher.AddEventSink(&g_actorUpdateManager);
                        }
                }
                break;
                case SKSEMessagingInterface::kMessage_PreLoadGame:
                        g_enableBodyInit = false;
                        g_tintMaskInterface.ManageTints();
                        break;
                case SKSEMessagingInterface::kMessage_PostLoadGame:
                        g_enableBodyInit = true;
                        g_tintMaskInterface.ReleaseTints();
                        break;
                case SKSEMessagingInterface::kMessage_DataLoaded:
                {
                        if (g_enableBodyGen) {
                                GetEventDispatcherList()->initScriptDispatcher.AddEventSink(&g_actorUpdateManager);

                                g_bodyMorphInterface.LoadMods();
                        }


                        GetEventDispatcherList()->uniqueIdChangeDispatcher.AddEventSink(&g_itemDataInterface);

                        g_tintMaskInterface.LoadMods();
                        g_morphInterface.LoadMods();
                }
                break;
        }
}*/

/*void InterfaceExchangeMessageHandler(SKSEMessagingInterface::Message *
message)
{
        switch (message->type)
        {
        case InterfaceExchangeMessage::kMessage_ExchangeInterface:
        {
                InterfaceExchangeMessage * exchangeMessage =
(InterfaceExchangeMessage*)message->data; exchangeMessage->interfaceMap =
&g_interfaceMap;
        }
        break;
        }
}*/

bool SKEE::SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info)
{
  // Loading SKEE64 into SKSEVR short circuit immediately
  if (GET_EXE_VERSION_SUB(skse->runtimeVersion) != 0) {
    return false;
  }

  if (skse->isEditor) {
    _MESSAGE("loaded in editor, marking as incompatible");
    return false;
  } else if (skse->runtimeVersion != RUNTIME_VERSION_1_5_97 &&
             skse->runtimeVersion != RUNTIME_VERSION_1_5_97) {
    UInt32 runtimeVersion = RUNTIME_VERSION_1_5_97;
    char buf[512];
    sprintf_s(
      buf,
      "RaceMenu Version Error:\nexpected game version %d.%d.%d.%d\nyour game "
      "version is %d.%d.%d.%d\nsome features may not work correctly.",
      GET_EXE_VERSION_MAJOR(runtimeVersion),
      GET_EXE_VERSION_MINOR(runtimeVersion),
      GET_EXE_VERSION_BUILD(runtimeVersion),
      GET_EXE_VERSION_SUB(runtimeVersion),
      GET_EXE_VERSION_MAJOR(skse->runtimeVersion),
      GET_EXE_VERSION_MINOR(skse->runtimeVersion),
      GET_EXE_VERSION_BUILD(skse->runtimeVersion),
      GET_EXE_VERSION_SUB(skse->runtimeVersion));
    MessageBox(NULL, buf, "Game Version Error", MB_OK | MB_ICONEXCLAMATION);
    _FATALERROR("unsupported runtime version %08X", skse->runtimeVersion);
    return false;
  }

  // get the serialization interface and query its version
  g_serialization = (SKSESerializationInterface*)skse->QueryInterface(
    kInterface_Serialization);
  if (!g_serialization) {
    _FATALERROR("couldn't get serialization interface");
    return false;
  }
  if (g_serialization->version <
      MIN_SERIALIZATION_VERSION) // SKSESerializationInterface::kVersion)
  {
    _FATALERROR("serialization interface too old (%d expected %d)",
                g_serialization->version, MIN_SERIALIZATION_VERSION);
    return false;
  }

  // get the scaleform interface and query its version
  g_scaleform =
    (SKSEScaleformInterface*)skse->QueryInterface(kInterface_Scaleform);
  if (!g_scaleform) {
    _FATALERROR("couldn't get scaleform interface");
    return false;
  }
  if (g_scaleform->interfaceVersion < MIN_SCALEFORM_VERSION) {
    _FATALERROR("scaleform interface too old (%d expected %d)",
                g_scaleform->interfaceVersion, MIN_SCALEFORM_VERSION);
    return false;
  }

  // get the papyrus interface and query its version
  g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
  if (!g_papyrus) {
    _FATALERROR("couldn't get papyrus interface");
    return false;
  }
  if (g_papyrus->interfaceVersion < MIN_PAPYRUS_VERSION) {
    _FATALERROR("scaleform interface too old (%d expected %d)",
                g_papyrus->interfaceVersion, MIN_PAPYRUS_VERSION);
    return false;
  }

  // get the task interface and query its version
  g_task = (SKSETaskInterface*)skse->QueryInterface(kInterface_Task);
  if (!g_task) {
    _FATALERROR("couldn't get task interface");
    return false;
  }
  if (g_task->interfaceVersion <
      MIN_TASK_VERSION) // SKSETaskInterface::kInterfaceVersion)
  {
    _FATALERROR("task interface too old (%d expected %d)",
                g_task->interfaceVersion, MIN_TASK_VERSION);
    return false;
  }

  g_messaging =
    (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
  if (!g_messaging) {
    _ERROR("couldn't get messaging interface");
  }

  // supported runtime version
  return true;
}

bool SKEE::SKSEPlugin_Load(const SKSEInterface* skse)
{
  _DMESSAGE("NetImmerse Override Enabled");

  SKEE64GetConfigValue("Features", "bEnableOverlays", &g_enableOverlays);
  SKEE64GetConfigValue("Features", "bEnableSculpting", &g_enableSculpting);
  SKEE64GetConfigValue("Features", "bEnableHeadExport", &g_enableHeadExport);
  SKEE64GetConfigValue("Features", "bEnableBodyGen", &g_enableBodyGen);
  SKEE64GetConfigValue("Features", "bEnableBodyMorph", &g_enableBodyMorph);
  SKEE64GetConfigValue("Features", "bEnableAutoTransforms",
                       &g_enableAutoTransforms);
  SKEE64GetConfigValue("Features", "bEnableEquippableTransforms",
                       &g_enableEquippableTransforms);
  SKEE64GetConfigValue("Features", "bEnableTintSync", &g_enableTintSync);
  SKEE64GetConfigValue("Features", "bEnableTintInventory",
                       &g_enableTintInventory);
  SKEE64GetConfigValue("Features", "bEnableTintHairSlot",
                       &g_enableTintHairSlot);

  SKEE64GetConfigValue("Overlays", "bPlayerOnly", &g_playerOnly);
  SKEE64GetConfigValue("Overlays", "bEnableFaceOverlays",
                       &g_enableFaceOverlays);
  SKEE64GetConfigValue("Overlays", "bImmediateArmor", &g_immediateArmor);
  SKEE64GetConfigValue("Overlays", "bImmediateFace", &g_immediateFace);

  SKEE64GetConfigValue("Overlays/Body", "iNumOverlays", &g_numBodyOverlays);
  SKEE64GetConfigValue("Overlays/Body", "iSpellOverlays",
                       &g_numSpellBodyOverlays);
  SKEE64GetConfigValue("Overlays/Hands", "iNumOverlays", &g_numHandOverlays);
  SKEE64GetConfigValue("Overlays/Hands", "iSpellOverlays",
                       &g_numSpellHandOverlays);
  SKEE64GetConfigValue("Overlays/Feet", "iNumOverlays", &g_numFeetOverlays);
  SKEE64GetConfigValue("Overlays/Feet", "iSpellOverlays",
                       &g_numSpellFeetOverlays);
  SKEE64GetConfigValue("Overlays/Face", "iNumOverlays", &g_numFaceOverlays);
  SKEE64GetConfigValue("Overlays/Face", "iSpellOverlays",
                       &g_numSpellFaceOverlays);

  SKEE64GetConfigValue("Overlays/Data", "bAlphaOverride", &g_alphaOverride);
  SKEE64GetConfigValue("Overlays/Data", "iAlphaFlags", &g_alphaFlags);
  SKEE64GetConfigValue("Overlays/Data", "iAlphaThreshold", &g_alphaThreshold);

  std::string defaultTexture =
    GetConfigOption("Overlays/Data", "sDefaultTexture");
  if (defaultTexture.empty()) {
    defaultTexture = "textures\\actors\\character\\overlays\\default.dds";
  }
  // g_overlayInterface.SetDefaultTexture(defaultTexture);

  SKEE64GetConfigValue("General", "iLoadMode", &g_loadMode);
  SKEE64GetConfigValue("General", "bDeferredBodyMorph", &g_deferredBodyMorph);

  SKEE64GetConfigValue("General", "iScaleMode", &g_scaleMode);
  SKEE64GetConfigValue("General", "iBodyMorphMode", &g_bodyMorphMode);
  SKEE64GetConfigValue("General", "bParallelMorphing", &g_parallelMorphing);
  SKEE64GetConfigValue("General", "uTintHairSlot", &g_tintHairSlot);

  UInt32 bodyMorphMemoryLimit = 256000000;
  if (SKEE64GetConfigValue("General", "uBodyMorphMemoryLimit",
                           &bodyMorphMemoryLimit)) {
    // g_bodyMorphInterface.SetCacheLimit(bodyMorphMemoryLimit);
  }

  if (g_numBodyOverlays > 0x7F)
    g_numBodyOverlays = 0x7F;
  if (g_numSpellBodyOverlays > 0x7F)
    g_numSpellBodyOverlays = 0x7F;

  if (g_numHandOverlays > 0x7F)
    g_numHandOverlays = 0x7F;
  if (g_numSpellHandOverlays > 0x7F)
    g_numSpellHandOverlays = 0x7F;

  if (g_numFeetOverlays > 0x7F)
    g_numFeetOverlays = 0x7F;
  if (g_numSpellFeetOverlays > 0x7F)
    g_numSpellFeetOverlays = 0x7F;

  if (g_numFaceOverlays > 0x7F)
    g_numFaceOverlays = 0x7F;
  if (g_numSpellFaceOverlays > 0x7F)
    g_numSpellFaceOverlays = 0x7F;

  if (g_alphaThreshold > 0xFF)
    g_alphaThreshold = 0xFF;

  if (!g_enableFaceOverlays) {
    g_numFaceOverlays = 0;
    g_numSpellFaceOverlays = 0;
  }

  std::string data = GetConfigOption("FaceGen", "sTemplateRace");
  if (!data.empty())
    g_raceTemplate = data;

  SKEE64GetConfigValue("FaceGen", "fSliderMultiplier", &g_sliderMultiplier);
  SKEE64GetConfigValue("FaceGen", "fSliderInterval", &g_sliderInterval);

  if (g_sliderMultiplier <= 0)
    g_sliderMultiplier = 0.01f;
  if (g_sliderInterval <= 0)
    g_sliderInterval = 0.01f;
  if (g_sliderInterval > 1.0)
    g_sliderInterval = 1.0;

  SKEE64GetConfigValue("FaceGen", "bDisableFaceGenCache",
                       &g_disableFaceGenCache);
  SKEE64GetConfigValue("FaceGen", "bExternalHeads", &g_externalHeads);
  SKEE64GetConfigValue("FaceGen", "bExtendedMorphs", &g_extendedMorphs);
  SKEE64GetConfigValue("FaceGen", "bAllowAllMorphs", &g_allowAllMorphs);
  SKEE64GetConfigValue("FaceGen", "bAllowAnyRacePart", &g_allowAnyRacePart);
  SKEE64GetConfigValue("FaceGen", "bAllowAnyGenderPart",
                       &g_allowAnyGenderPart);
  SKEE64GetConfigValue("FaceGen", "bExportSkinToBone", &g_exportSkinToBone);

  SKEE64GetConfigValue("Sculpting", "fPanSpeed", &g_panSpeed);
  SKEE64GetConfigValue("Sculpting", "fFOV", &g_cameraFOV);
  SKEE64GetConfigValue("Sculpting", "iWidth", &g_viewWidth);
  SKEE64GetConfigValue("Sculpting", "iHeight", &g_viewHeight);

  SKEE64GetConfigValue("Sculpting", "fBackgroundA", &g_sculptBackgroundA);
  SKEE64GetConfigValue("Sculpting", "fBackgroundR", &g_sculptBackgroundR);
  SKEE64GetConfigValue("Sculpting", "fBackgroundG", &g_sculptBackgroundG);
  SKEE64GetConfigValue("Sculpting", "fBackgroundB", &g_sculptBackgroundB);
  SKEE64GetConfigValue("Sculpting", "fOffsetX", &g_sculptOffsetX);
  SKEE64GetConfigValue("Sculpting", "fOffsetY", &g_sculptOffsetY);
  SKEE64GetConfigValue("Sculpting", "fOffsetZ", &g_sculptOffsetZ);
  SKEE64GetConfigValue("Sculpting", "bDrawBrushPoint",
                       &g_sculptDrawBrushPoint);
  SKEE64GetConfigValue("Sculpting", "bDrawBrushRadius",
                       &g_sculptDrawBrushRadius);

  /*std::string types[CDXBrush::kBrushTypes];
  types[CDXBrush::kBrushType_Mask_Add] = "Brush/MaskAdd/";
  types[CDXBrush::kBrushType_Mask_Subtract] = "Brush/MaskSubtract/";
  types[CDXBrush::kBrushType_Inflate] = "Brush/Inflate/";
  types[CDXBrush::kBrushType_Deflate] = "Brush/Deflate/";
  types[CDXBrush::kBrushType_Smooth] = "Brush/Smooth/";
  types[CDXBrush::kBrushType_Move] = "Brush/Move/";

  std::string properties[CDXBrush::kBrushProperties];
  properties[CDXBrush::kBrushProperty_Radius] = "Radius";
  properties[CDXBrush::kBrushProperty_Strength] = "Strength";
  properties[CDXBrush::kBrushProperty_Falloff] = "Falloff";

  std::string values[CDXBrush::kBrushPropertyValues];
  values[CDXBrush::kBrushPropertyValue_Value] = "dbDefault";
  values[CDXBrush::kBrushPropertyValue_Min] = "dbMin";
  values[CDXBrush::kBrushPropertyValue_Max] = "dbMax";
  values[CDXBrush::kBrushPropertyValue_Interval] = "dbInterval";

  CDXBrush::InitGlobals();

  for (UInt32 b = 0; b < CDXBrush::kBrushTypes; b++) {
    for (UInt32 p = 0; p < CDXBrush::kBrushProperties; p++) {
      for (UInt32 v = 0; v < CDXBrush::kBrushPropertyValues; v++) {
        std::string section = types[b] + properties[p];
        double val = 0.0;
        if (SKEE64GetConfigValue(section.c_str(), values[v].c_str(), &val))
          g_brushProperties[b][p][v] = val;
      }
    }
  }*/

  /*if (g_serialization) {
    g_serialization->SetUniqueID(g_pluginHandle, 'SKEE');
    g_serialization->SetRevertCallback(g_pluginHandle,
                                       SKEE64Serialization_Revert);
    g_serialization->SetSaveCallback(g_pluginHandle, SKEE64Serialization_Save);
    g_serialization->SetLoadCallback(g_pluginHandle, SKEE64Serialization_Load);
    g_serialization->SetFormDeleteCallback(g_pluginHandle,
                                           SKEE64Serialization_FormDelete);
  }

  // register scaleform callbacks
  if (g_scaleform) {
    g_scaleform->Register("NiOverride", RegisterNiOverrideScaleform);
    g_scaleform->Register("CharGen", RegisterCharGenScaleform);
  }

  if (g_papyrus) {
    g_papyrus->Register(RegisterPapyrusFunctions);
  }*/

  if (g_messaging) {
    /*g_messaging->RegisterListener(g_pluginHandle, "SKSE",
    SKSEMessageHandler); g_messaging->RegisterListener(g_pluginHandle, nullptr,
                                  InterfaceExchangeMessageHandler);*/

    /*if (g_enableTintHairSlot) {
      EventDispatcher<SKSENiNodeUpdateEvent>* dispatcher =
        static_cast<EventDispatcher<SKSENiNodeUpdateEvent>*>(
          g_messaging->GetEventDispatcher(
            SKSEMessagingInterface::kDispatcher_NiNodeUpdateEvent));
      if (dispatcher) {
        dispatcher->AddEventSink(&g_tintMaskInterface);
      }
    }*/
  }

  /*g_interfaceMap.AddInterface("Override", &g_overrideInterface);
  g_interfaceMap.AddInterface("Overlay", &g_overlayInterface);
  g_interfaceMap.AddInterface("NiTransform", &g_transformInterface);
  g_interfaceMap.AddInterface("BodyMorph", &g_bodyMorphInterface);
  g_interfaceMap.AddInterface("ItemData", &g_itemDataInterface);
  g_interfaceMap.AddInterface("TintMask", &g_tintMaskInterface);
  g_interfaceMap.AddInterface("FaceMorph", &g_morphInterface);
  g_interfaceMap.AddInterface("ActorUpdateManager", &g_actorUpdateManager);

  g_actorUpdateManager.AddInterface(&g_skeletonExtenderInterface);
  g_actorUpdateManager.AddInterface(&g_bodyMorphInterface);
  g_actorUpdateManager.AddInterface(&g_overlayInterface);
  g_actorUpdateManager.AddInterface(&g_overrideInterface);
  g_actorUpdateManager.AddInterface(&g_itemDataInterface);*/

  if (g_enableTintHairSlot) {
    // g_actorUpdateManager.AddInterface(&g_tintMaskInterface);
  }

  return 1;
  // return InstallSKEEHooks();
}