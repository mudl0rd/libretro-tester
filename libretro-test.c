#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "libretro.h"
#include <audio/audio_mixer.h>

#define BUFSIZE 44100/60

retro_environment_t environ_cb            = NULL;
retro_video_refresh_t video_cb            = NULL;
retro_audio_sample_t audio_cb             = NULL;
retro_audio_sample_batch_t audio_batch_cb = NULL;
retro_input_poll_t poller_cb              = NULL;
retro_input_state_t input_state_cb        = NULL;

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

#ifndef EXPORT
#if defined(CPPCLI)
#define EXPORT EXTERNC
#elif defined(_WIN32)
#define EXPORT EXTERNC __declspec(dllexport)
#else
#define EXPORT EXTERNC __attribute__((visibility("default")))
#endif
#endif

EXPORT void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

EXPORT void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

EXPORT void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

EXPORT void retro_set_input_poll(retro_input_poll_t cb)
{
   poller_cb = cb;
}

EXPORT void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

EXPORT void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
}

EXPORT void retro_deinit(void) {}

EXPORT unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

EXPORT void retro_init(void)
{
   audio_mixer_init(44100);
}

EXPORT void retro_get_system_info(struct retro_system_info* info)
{
   const struct retro_system_info myinfo={ "WAV player", "v1", "wav", false, false };
   memcpy(info, &myinfo, sizeof(myinfo));
}

EXPORT void retro_get_system_av_info(struct retro_system_av_info* info)
{
   const struct retro_system_av_info myinfo={
      { 320, 240, 320, 240, 0.0 },
      { 60.0, 44100 }
   };
   memcpy(info, &myinfo, sizeof(myinfo));
}

EXPORT void retro_reset(void)
{
}

audio_mixer_sound_t *wavfile = NULL;
audio_mixer_voice_t * voice1 = NULL;

void convert_float_to_s16(int16_t *out,
	const float *in, size_t samples)
{
	size_t i = 0;
	for (; i < samples; i++)
	{
		int32_t val = (int32_t)(in[i] * 0x8000);
		out[i] = (val > 0x7FFF) ? 0x7FFF :
			(val < -0x8000 ? -0x8000 : (int16_t)val);
	}
}

EXPORT void retro_run(void)
{
   static uint16_t pixels[240][320];
   float samples[BUFSIZE * 2] = { 0 };
   int16_t samples2[2 * BUFSIZE] = { 0 };
   audio_mixer_mix(samples, BUFSIZE, 1.0, false);
   convert_float_to_s16(samples2,samples, 2 * BUFSIZE);
   audio_batch_cb(samples2, BUFSIZE);

   poller_cb();
   memset(pixels, 0xFF, sizeof(pixels));
   video_cb(pixels, 320, 240, sizeof(uint16_t) * 320);
}

EXPORT size_t retro_serialize_size(void)
{
   return 0;
}

EXPORT bool retro_serialize(void* data, size_t size)
{
  
   return true;
}

EXPORT bool retro_unserialize(const void* data, size_t size)
{
  
   return true;
}

EXPORT bool retro_load_game(const struct retro_game_info* game)
{
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      return false;
   wavfile = audio_mixer_load_wav(game->data,game->size);
   voice1 = audio_mixer_play(wavfile,true,1.0,NULL);
   return true;
}

EXPORT bool retro_load_game_special(unsigned game_type,
      const struct retro_game_info* info, size_t num_info)
{
   return false;
}

EXPORT void retro_unload_game(void)
{
   audio_mixer_stop(voice1);
   audio_mixer_destroy(wavfile);
   audio_mixer_done();
}

EXPORT unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

EXPORT void* retro_get_memory_data(unsigned id)  { return NULL; }
EXPORT size_t retro_get_memory_size(unsigned id) { return 0; }
EXPORT void retro_cheat_reset(void) {}
EXPORT void retro_cheat_set(unsigned index, bool enabled, const char* code) {}
EXPORT void retro_set_controller_port_device(unsigned port, unsigned device) {}