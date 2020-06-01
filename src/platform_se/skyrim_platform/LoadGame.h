#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace SaveFile_ {
struct PlayerLocation;
struct RefID;
struct SaveFile;
struct ChangeForm;
struct ChangeFormNPC_;
struct Weather;
struct GlobalVariables;
}

class LoadGame
{
  class Time;

public:
  static void Run(const std::array<float, 3>& pos,
                  const std::array<float, 3>& angle, uint32_t cellOrWorld,
                  Time* time = nullptr, SaveFile_::Weather* _weather = nullptr,
                  SaveFile_::ChangeFormNPC_* changeFormNPC = nullptr);

  static std::wstring GetPathToMyDocuments();

private:
  static std::wstring StringToWstring(std::string s);

  static std::string GenerateGuid();

  static std::filesystem::path GetSaveFullPath(const std::string& name);

  static SaveFile_::PlayerLocation* FindSectionWithPlayerLocation(
    std::shared_ptr<SaveFile_::SaveFile> save);

  static SaveFile_::PlayerLocation CreatePlayerLocation(
    const std::array<float, 3>& pos, const SaveFile_::RefID& world);

  static std::vector<uint8_t> Decompress(
    const SaveFile_::ChangeForm& changeForm);

  static void EditChangeForm(std::vector<uint8_t>& data,
                             const std::array<float, 3>& pos,
                             const std::array<float, 3>& angle,
                             const SaveFile_::RefID& world);

  static std::vector<uint8_t> Compress(
    const std::vector<uint8_t>& uncompressed);

  static void WriteChangeForm(std::shared_ptr<SaveFile_::SaveFile> save,
                              SaveFile_::ChangeForm& changeForm,
                              const std::vector<uint8_t>& compressed,
                              size_t uncompressedSize);

  static void ModifyEssStructure(std::shared_ptr<SaveFile_::SaveFile> save,
                                 std::array<float, 3> pos,
                                 std::array<float, 3> angle,
                                 uint32_t cellOrWorld);

  static void ModifyPluginInfo(std::shared_ptr<SaveFile_::SaveFile>& save);

  static void ModifySaveTime(std::shared_ptr<SaveFile_::SaveFile>& save,
                             Time* time);

  static void ModifySaveWeather(std::shared_ptr<SaveFile_::SaveFile>& save,
                                SaveFile_::Weather* _weather);

  static void ModifyPlayerFormNPC(std::shared_ptr<SaveFile_::SaveFile> save,
                                  SaveFile_::ChangeFormNPC_* changeFormNPC);

  static void FillChangeForm(
    std::shared_ptr<SaveFile_::SaveFile> save, SaveFile_::ChangeForm* form,
    std::pair<uint32_t, std::vector<uint8_t>>& newValues);
};
