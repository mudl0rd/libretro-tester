#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "libretro.h"
#include <audio/audio_mixer.h>

#define BUFSIZE 44100 / 60

retro_environment_t environ_cb = NULL;
retro_video_refresh_t video_cb = NULL;
retro_audio_sample_t audio_cb = NULL;
retro_audio_sample_batch_t audio_batch_cb = NULL;
retro_input_poll_t poller_cb = NULL;
retro_input_state_t input_state_cb = NULL;

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

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
uint8_t r[256], g[256], b[256];

EXPORT void retro_init(void)
{
   for (int x = 0; x < 256; x++)
   {

      r[x] = (uint8_t)(128.0 + 128 * sin(3.1415 * x / 32.0));
      g[x] = (uint8_t)(128.0 + 128 * sin(3.1415 * x / 64.0));
      b[x] = (uint8_t)(128.0 + 128 * sin(3.1415 * x / 128.0));
   }

   audio_mixer_init(44100);
}

EXPORT void retro_get_system_info(struct retro_system_info *info)
{
   const struct retro_system_info myinfo = {"libretro test", "v1", "wav|s3m|mod|xm|flac|ogg|mp3", true, false};
   memcpy(info, &myinfo, sizeof(myinfo));
}

EXPORT void retro_get_system_av_info(struct retro_system_av_info *info)
{
   const struct retro_system_av_info myinfo = {
       {SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0}, {60.0, 44100}};
   memcpy(info, &myinfo, sizeof(myinfo));
}

EXPORT void retro_reset(void)
{
}

audio_mixer_sound_t *wavfile = NULL;
audio_mixer_voice_t *voice1 = NULL;

void convert_float_to_s16(int16_t *out,
                          const float *in, size_t samples)
{
   size_t i = 0;
   for (; i < samples; i++)
   {
      int32_t val = (int32_t)(in[i] * 0x8000);
      out[i] = (val > 0x7FFF) ? 0x7FFF : (val < -0x8000 ? -0x8000 : (int16_t)val);
   }
}

union
{
   unsigned int integer;
   unsigned char byte[4];
} foo;
static uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};

EXPORT void retro_run(void)
{

   float samples[BUFSIZE * 2] = {0};
   int16_t samples2[2 * BUFSIZE] = {0};
   audio_mixer_mix(samples, BUFSIZE, 1.0, false);
   convert_float_to_s16(samples2, samples, 2 * BUFSIZE);
   audio_batch_cb(samples2, BUFSIZE);

   poller_cb();

   static double f = 0;

   memset(pixels, 0xFF, sizeof(pixels));

   for (int y = 0; y < 200; y++)
   {
      for (int x = 0; x < 320; x++)
      {
         float c1 = sin(x / 50.0 + f + y / 200.0);
         float c2 = sqrt((sin(0.8 * f) * 160 - x + 160) * (sin(0.8 * f) * 160 - x + 160) + (cos(1.2 * f) * 100 - y + 100) * (cos(1.2 * f) * 100 - y + 100));
         c2 = sin(c2 / 50.0);
         float c3 = (c1 + c2) / 2;

         int res = ceil((c3 + 1) * 127);

         foo.byte[0] = 0xFF;
         foo.byte[1] = b[res];
         foo.byte[2] = g[res];
         foo.byte[3] = r[res];

         pixels[x + 320 * y] = foo.integer;
      }
   }

   f += 0.1;
   video_cb(pixels, 320, 200, 320 * 4);
}

EXPORT size_t retro_serialize_size(void)
{
   return 0;
}

EXPORT bool retro_serialize(void *data, size_t size)
{

   return true;
}

EXPORT bool retro_unserialize(const void *data, size_t size)
{

   return true;
}
char *buffer = NULL;

const char *get_filename_ext(const char *filename)
{
   const char *dot = strrchr(filename, '.');
   if (!dot || dot == filename)
      return "";
   return dot + 1;
}

EXPORT bool retro_load_game(const struct retro_game_info *game)
{
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_XRGB8888;

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      return false;

   FILE *pFile;
   long lSize;
   size_t result;
   pFile = fopen(game->path, "rb");
   if (pFile == NULL)
      return false;
   // obtain file size:
   fseek(pFile, 0, SEEK_END);
   lSize = ftell(pFile);
   rewind(pFile);
   // allocate memory to contain the whole file:
   buffer = (char *)malloc(sizeof(char) * lSize);
   // copy the file into the buffer:
   result = fread(buffer, 1, lSize, pFile);
   /* the whole file is now loaded in the memory buffer. */
   // terminate
   fclose(pFile);

   char *ext = get_filename_ext(game->path);

   if (strcmp("wav", ext) == 0)
      wavfile = audio_mixer_load_wav(buffer, lSize, NULL, RESAMPLER_QUALITY_DONTCARE);
   else if (strcmp("mp3", ext) == 0)
      wavfile = audio_mixer_load_mp3(buffer, lSize);
   else if (strcmp("ogg", ext) == 0)
      wavfile = audio_mixer_load_ogg(buffer, lSize);
   else if (strcmp("flac", ext) == 0)
      wavfile = audio_mixer_load_flac(buffer, lSize);
   else
      wavfile = audio_mixer_load_mod(buffer, lSize);
   voice1 = audio_mixer_play(wavfile, true, 1.0, NULL, RESAMPLER_QUALITY_DONTCARE, NULL);
   return true;
}

EXPORT bool retro_load_game_special(unsigned game_type,
                                    const struct retro_game_info *info, size_t num_info)
{
   return false;
}

EXPORT void retro_unload_game(void)
{
   audio_mixer_stop(voice1);
   audio_mixer_destroy(wavfile);
   audio_mixer_done();
   if (buffer)
      free(buffer);
}

EXPORT unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

EXPORT void *retro_get_memory_data(unsigned id) { return NULL; }
EXPORT size_t retro_get_memory_size(unsigned id) { return 0; }
EXPORT void retro_cheat_reset(void) {}
EXPORT void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
EXPORT void retro_set_controller_port_device(unsigned port, unsigned device) {}