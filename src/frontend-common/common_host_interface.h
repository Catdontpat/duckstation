#pragma once
#include "common/string.h"
#include "core/host_interface.h"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

class ControllerInterface;

namespace FrontendCommon {
class SaveStateSelectorUI;
}

class CommonHostInterface : public HostInterface
{
public:
  friend ControllerInterface;

  enum : s32
  {
    PER_GAME_SAVE_STATE_SLOTS = 10,
    GLOBAL_SAVE_STATE_SLOTS = 10
  };

  using HostKeyCode = s32;
  using HostMouseButton = s32;

  using InputButtonHandler = std::function<void(bool)>;
  using InputAxisHandler = std::function<void(float)>;
  using ControllerRumbleCallback = std::function<void(const float*, u32)>;

  struct HotkeyInfo
  {
    String category;
    String name;
    String display_name;
    InputButtonHandler handler;
  };

  using HotkeyInfoList = std::vector<HotkeyInfo>;

  struct SaveStateInfo
  {
    std::string path;
    u64 timestamp;
    s32 slot;
    bool global;
  };

  struct ExtendedSaveStateInfo
  {
    std::string path;
    u64 timestamp;
    s32 slot;
    bool global;

    std::string title;
    std::string game_code;

    u32 screenshot_width;
    u32 screenshot_height;
    std::vector<u32> screenshot_data;
  };

  using HostInterface::LoadState;
  using HostInterface::SaveState;

  /// Returns the name of the frontend.
  virtual const char* GetFrontendName() const = 0;

  virtual bool Initialize() override;
  virtual void Shutdown() override;

  virtual bool BootSystem(const SystemBootParameters& parameters) override;
  virtual void PowerOffSystem() override;
  virtual void DestroySystem() override;

  /// Returns the game list.
  ALWAYS_INLINE const GameList* GetGameList() const { return m_game_list.get(); }

  /// Returns a list of all available hotkeys.
  ALWAYS_INLINE const HotkeyInfoList& GetHotkeyInfoList() const { return m_hotkeys; }

  /// Access to current controller interface.
  ALWAYS_INLINE ControllerInterface* GetControllerInterface() const { return m_controller_interface.get(); }

  /// Returns true if running in batch mode, i.e. exit after emulation.
  ALWAYS_INLINE bool InBatchMode() const { return m_batch_mode; }

  void PauseSystem(bool paused);

  /// Parses command line parameters for all frontends.
  bool ParseCommandLineParameters(int argc, char* argv[], std::unique_ptr<SystemBootParameters>* out_boot_params);

  /// Loads the current emulation state from file. Specifying a slot of -1 loads the "resume" game state.
  bool LoadState(bool global, s32 slot);

  /// Saves the current emulation state to a file. Specifying a slot of -1 saves the "resume" save state.
  bool SaveState(bool global, s32 slot);

  /// Loads the resume save state for the given game. Optionally boots the game anyway if loading fails.
  bool ResumeSystemFromState(const char* filename, bool boot_on_failure);

  /// Loads the most recent resume save state. This may be global or per-game.
  bool ResumeSystemFromMostRecentState();

  /// Saves the resume save state, call when shutting down. Not called automatically on DestroySystem() since that can
  /// be called as a result of an error.
  bool SaveResumeSaveState();

  /// Returns a list of save states for the specified game code.
  std::vector<SaveStateInfo> GetAvailableSaveStates(const char* game_code) const;

  /// Returns save state info if present. If game_code is null or empty, assumes global state.
  std::optional<SaveStateInfo> GetSaveStateInfo(const char* game_code, s32 slot);

  /// Returns save state info if present. If game_code is null or empty, assumes global state.
  std::optional<ExtendedSaveStateInfo> GetExtendedSaveStateInfo(const char* game_code, s32 slot);

  /// Deletes save states for the specified game code. If resume is set, the resume state is deleted too.
  void DeleteSaveStates(const char* game_code, bool resume);

  /// Adds OSD messages, duration is in seconds.
  void AddOSDMessage(std::string message, float duration = 2.0f) override;

  /// Displays a loading screen with the logo, rendered with ImGui. Use when executing possibly-time-consuming tasks
  /// such as compiling shaders when starting up.
  void DisplayLoadingScreen(const char* message, int progress_min = -1, int progress_max = -1,
                            int progress_value = -1) override;

  /// Retrieves information about specified game from game list.
  void GetGameInfo(const char* path, CDImage* image, std::string* code, std::string* title) override;

  /// Returns true if currently dumping audio.
  bool IsDumpingAudio() const;

  /// Starts dumping audio to a file. If no file name is provided, one will be generated automatically.
  bool StartDumpingAudio(const char* filename = nullptr);

  /// Stops dumping audio to file if it has been started.
  void StopDumpingAudio();

  /// Saves a screenshot to the specified file. IF no file name is provided, one will be generated automatically.
  bool SaveScreenshot(const char* filename = nullptr, bool full_resolution = true, bool apply_aspect_ratio = true);

protected:
  enum : u32
  {
    SETTINGS_VERSION = 2
  };

  struct OSDMessage
  {
    std::string text;
    Common::Timer time;
    float duration;
  };

  struct InputProfileEntry
  {
    std::string name;
    std::string path;
  };
  using InputProfileList = std::vector<InputProfileEntry>;

  CommonHostInterface();
  ~CommonHostInterface();

  /// Request the frontend to exit.
  virtual void RequestExit() = 0;

  /// Executes per-frame tasks such as controller polling.
  virtual void PollAndUpdate();

  virtual bool IsFullscreen() const;
  virtual bool SetFullscreen(bool enabled);

  virtual std::unique_ptr<AudioStream> CreateAudioStream(AudioBackend backend) override;
  virtual std::unique_ptr<ControllerInterface> CreateControllerInterface();

  virtual void OnSystemCreated() override;
  virtual void OnSystemPaused(bool paused);
  virtual void OnSystemDestroyed() override;
  virtual void OnRunningGameChanged() override;
  virtual void OnControllerTypeChanged(u32 slot) override;

  virtual std::optional<HostKeyCode> GetHostKeyCode(const std::string_view key_code) const;

  virtual bool AddButtonToInputMap(const std::string& binding, const std::string_view& device,
                                   const std::string_view& button, InputButtonHandler handler);
  virtual bool AddAxisToInputMap(const std::string& binding, const std::string_view& device,
                                 const std::string_view& axis, InputAxisHandler handler);
  virtual bool AddRumbleToInputMap(const std::string& binding, u32 controller_index, u32 num_motors);

  /// Reloads the input map from config. Callable from controller interface.
  virtual void UpdateInputMap() = 0;

  /// Returns a path where an input profile with the specified name would be saved.
  std::string GetSavePathForInputProfile(const char* name) const;

  /// Returns a list of all input profiles. first - name, second - path
  InputProfileList GetInputProfileList() const;

  /// Applies the specified input profile.
  void ApplyInputProfile(const char* profile_path, SettingsInterface& si);

  /// Saves the current input configuration to the specified profile name.
  bool SaveInputProfile(const char* profile_path, SettingsInterface& si);

  void RegisterHotkey(String category, String name, String display_name, InputButtonHandler handler);
  bool HandleHostKeyEvent(HostKeyCode code, bool pressed);
  bool HandleHostMouseEvent(HostMouseButton button, bool pressed);
  void UpdateInputMap(SettingsInterface& si);

  void AddControllerRumble(u32 controller_index, u32 num_motors, ControllerRumbleCallback callback);
  void UpdateControllerRumble();
  void StopControllerRumble();

  /// Returns the path to a save state file. Specifying an index of -1 is the "resume" save state.
  std::string GetGameSaveStateFileName(const char* game_code, s32 slot) const;

  /// Returns the path to a save state file. Specifying an index of -1 is the "resume" save state.
  std::string GetGlobalSaveStateFileName(s32 slot) const;

  /// Sets the base path for the user directory. Can be overridden by platform/frontend/command line.
  virtual void SetUserDirectory();

  /// Performs the initial load of settings. Should call CheckSettings() and LoadSettings(SettingsInterface&).
  virtual void LoadSettings() = 0;

  /// Updates logging settings.
  virtual void UpdateLogSettings(LOGLEVEL level, const char* filter, bool log_to_console, bool log_to_debug,
                                 bool log_to_window, bool log_to_file);

  /// Returns the path of the settings file.
  std::string GetSettingsFileName() const;

  /// Returns the most recent resume save state.
  std::string GetMostRecentResumeSaveStatePath() const;

  /// Ensures the settings is valid and the correct version. If not, resets to defaults.
  void CheckSettings(SettingsInterface& si);

  /// Restores all settings to defaults.
  virtual void SetDefaultSettings(SettingsInterface& si) override;

  /// Loads settings to m_settings and any frontend-specific parameters.
  virtual void LoadSettings(SettingsInterface& si) override;

  /// Saves current settings variables to ini.
  virtual void SaveSettings(SettingsInterface& si) override;

  /// Checks for settings changes, std::move() the old settings away for comparing beforehand.
  virtual void CheckForSettingsChanges(const Settings& old_settings) override;

  /// Increases timer resolution when supported by the host OS.
  void SetTimerResolutionIncreased(bool enabled);

  void UpdateSpeedLimiterState();

  void RecreateSystem() override;

  virtual void DrawImGuiWindows();

  void DrawFPSWindow();
  void DrawOSDMessages();
  void DrawDebugWindows();
  void DoFrameStep();

  std::unique_ptr<GameList> m_game_list;

  std::unique_ptr<ControllerInterface> m_controller_interface;

  std::deque<OSDMessage> m_osd_messages;
  std::mutex m_osd_messages_lock;

  bool m_paused = false;
  bool m_frame_step_request = false;
  bool m_speed_limiter_temp_disabled = false;
  bool m_speed_limiter_enabled = false;
  bool m_timer_resolution_increased = false;

private:
  void InitializeUserDirectory();
  void RegisterGeneralHotkeys();
  void RegisterGraphicsHotkeys();
  void RegisterSaveStateHotkeys();
  void RegisterAudioHotkeys();
  void FindInputProfiles(const std::string& base_path, InputProfileList* out_list) const;
  void UpdateControllerInputMap(SettingsInterface& si);
  void UpdateHotkeyInputMap(SettingsInterface& si);
  void ClearAllControllerBindings(SettingsInterface& si);

#ifdef WITH_DISCORD_PRESENCE
  void SetDiscordPresenceEnabled(bool enabled);
  void InitializeDiscordPresence();
  void ShutdownDiscordPresence();
  void UpdateDiscordPresence();
  void PollDiscordPresence();
#endif

  HotkeyInfoList m_hotkeys;

  std::unique_ptr<FrontendCommon::SaveStateSelectorUI> m_save_state_selector_ui;

  // input key maps
  std::map<HostKeyCode, InputButtonHandler> m_keyboard_input_handlers;
  std::map<HostMouseButton, InputButtonHandler> m_mouse_input_handlers;

  // controller vibration motors/rumble
  struct ControllerRumbleState
  {
    enum : u32
    {
      MAX_MOTORS = 2
    };

    u32 controller_index;
    u32 num_motors;
    std::array<float, MAX_MOTORS> last_strength;
    ControllerRumbleCallback update_callback;
  };
  std::vector<ControllerRumbleState> m_controller_vibration_motors;

  // running in batch mode? i.e. exit after stopping emulation
  bool m_batch_mode = false;

#ifdef WITH_DISCORD_PRESENCE
  // discord rich presence
  bool m_discord_presence_enabled = false;
  bool m_discord_presence_active = false;
#endif
};
