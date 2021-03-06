#pragma once
#include "core/host_interface.h"
#include "core/system.h"
#include "libretro.h"

class LibretroHostInterface : public HostInterface
{
public:
  LibretroHostInterface();
  ~LibretroHostInterface() override;

  static void InitLogging();
  static bool SetCoreOptions();
  static bool HasCoreVariablesChanged();

  ALWAYS_INLINE u32 GetResolutionScale() const { return m_settings.gpu_resolution_scale; }

  bool Initialize() override;
  void Shutdown() override;

  void ReportError(const char* message) override;
  void ReportMessage(const char* message) override;
  bool ConfirmMessage(const char* message) override;
  void AddOSDMessage(std::string message, float duration = 2.0f) override;

  void GetGameInfo(const char* path, CDImage* image, std::string* code, std::string* title) override;
  std::string GetSharedMemoryCardPath(u32 slot) const override;
  std::string GetGameMemoryCardPath(const char* game_code, u32 slot) const override;
  std::string GetShaderCacheBasePath() const override;
  std::string GetSettingValue(const char* section, const char* key, const char* default_value = "") override;

  // Called by frontend
  void retro_get_system_av_info(struct retro_system_av_info* info);
  bool retro_load_game(const struct retro_game_info* game);
  void retro_run_frame();
  unsigned retro_get_region();
  size_t retro_serialize_size();
  bool retro_serialize(void* data, size_t size);
  bool retro_unserialize(const void* data, size_t size);

protected:
  bool AcquireHostDisplay() override;
  void ReleaseHostDisplay() override;
  std::unique_ptr<AudioStream> CreateAudioStream(AudioBackend backend) override;
  void OnSystemDestroyed() override;
  void CheckForSettingsChanges(const Settings& old_settings) override;

private:
  void LoadSettings();
  void UpdateSettings();
  void UpdateControllers();
  void UpdateControllersDigitalController(u32 index);
  void UpdateControllersAnalogController(u32 index);
  void GetSystemAVInfo(struct retro_system_av_info* info, bool use_resolution_scale);
  void UpdateSystemAVInfo(bool use_resolution_scale);
  void UpdateGeometry();
  void UpdateLogging();

  // Hardware renderer setup.
  bool RequestHardwareRendererContext();
  void SwitchToHardwareRenderer();
  void SwitchToSoftwareRenderer();

  static void HardwareRendererContextReset();
  static void HardwareRendererContextDestroy();

  retro_hw_render_callback m_hw_render_callback = {};
  std::unique_ptr<HostDisplay> m_hw_render_display;
  bool m_hw_render_callback_valid = false;
  bool m_using_hardware_renderer = false;
};

extern LibretroHostInterface g_libretro_host_interface;

// libretro callbacks
extern retro_environment_t g_retro_environment_callback;
extern retro_video_refresh_t g_retro_video_refresh_callback;
extern retro_audio_sample_t g_retro_audio_sample_callback;
extern retro_audio_sample_batch_t g_retro_audio_sample_batch_callback;
extern retro_input_poll_t g_retro_input_poll_callback;
extern retro_input_state_t g_retro_input_state_callback;
