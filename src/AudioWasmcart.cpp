/*
 * AudioWasmcart.cpp — Audio mixer for wasmcart
 *
 * Loads WAV files from .wasc assets, mixes multiple channels into PCM output.
 */

#include "AudioWasmcart.h"
#include "wasmcart.h"
#include "Config.h"

#include <cstring>
#include <cstdlib>

/* Scratch buffer for loading WAV assets */
static unsigned char wav_load_buf[AW_MAX_SOUND_SIZE];

/* Parse a WAV file header and return pointer to PCM data */
static int parseWav(const unsigned char *data, int size,
                    int16_t **pcm_out, int *length_out,
                    int *channels_out, int *sample_rate_out)
{
    if (size < 44) return -1;
    /* Check RIFF header */
    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F') return -1;
    if (data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E') return -1;

    /* Find fmt chunk */
    int pos = 12;
    int fmt_found = 0;
    int num_channels = 1;
    int sample_rate = 44100;
    int bits_per_sample = 16;

    while (pos + 8 <= size) {
        int chunk_size = data[pos+4] | (data[pos+5]<<8) | (data[pos+6]<<16) | (data[pos+7]<<24);
        if (data[pos] == 'f' && data[pos+1] == 'm' && data[pos+2] == 't' && data[pos+3] == ' ') {
            if (pos + 8 + 16 > size) return -1;
            int audio_format = data[pos+8] | (data[pos+9]<<8);
            num_channels = data[pos+10] | (data[pos+11]<<8);
            sample_rate = data[pos+12] | (data[pos+13]<<8) | (data[pos+14]<<16) | (data[pos+15]<<24);
            bits_per_sample = data[pos+22] | (data[pos+23]<<8);
            (void)audio_format; /* We assume PCM (format 1) */
            fmt_found = 1;
        }
        if (data[pos] == 'd' && data[pos+1] == 'a' && data[pos+2] == 't' && data[pos+3] == 'a') {
            if (!fmt_found) return -1;
            int data_start = pos + 8;
            int data_size = chunk_size;
            if (data_start + data_size > size) data_size = size - data_start;

            int bytes_per_sample = bits_per_sample / 8;
            int total_samples = data_size / bytes_per_sample;
            int samples_per_channel = total_samples / num_channels;

            /* Allocate and convert to int16 */
            int16_t *pcm = (int16_t*)malloc(total_samples * sizeof(int16_t));
            if (!pcm) return -1;

            if (bits_per_sample == 16) {
                memcpy(pcm, data + data_start, total_samples * 2);
            } else if (bits_per_sample == 8) {
                for (int i = 0; i < total_samples; i++) {
                    pcm[i] = ((int16_t)data[data_start + i] - 128) * 256;
                }
            } else {
                free(pcm);
                return -1;
            }

            *pcm_out = pcm;
            *length_out = samples_per_channel;
            *channels_out = num_channels;
            *sample_rate_out = sample_rate;
            return 0;
        }
        pos += 8 + chunk_size;
        if (chunk_size & 1) pos++; /* chunks are word-aligned */
    }
    return -1;
}

AudioWasmcart::AudioWasmcart()
{
    soundVolume = 0.9f;
    musicVolume = 0.5f;
    musicPaused = 0;
    currentMusicMode = MusicMenu;
    soundsLoaded = 0;

    for (int i = 0; i < (int)NumSoundTypes; i++) {
        sounds[i].samples = 0;
        sounds[i].length = 0;
        sounds[i].channels = 1;
        sounds[i].sample_rate = 44100;
    }
    for (int i = 0; i < AW_MAX_CHANNELS; i++) {
        channels[i].sound = 0;
        channels[i].position = 0;
        channels[i].volume = 1.0f;
        channels[i].active = 0;
    }
    musicChannel.sound = 0;
    musicChannel.position = 0;
    musicChannel.fpos = 0;
    musicChannel.volume = musicVolume;
    musicChannel.active = 0;

    initSound();
}

AudioWasmcart::~AudioWasmcart()
{
    for (int i = 0; i < (int)NumSoundTypes; i++) {
        if (sounds[i].samples) {
            free(sounds[i].samples);
            sounds[i].samples = 0;
        }
    }
}

void AudioWasmcart::loadWav(SoundType type, const char *path)
{
    int path_len = 0;
    const char *p = path;
    while (*p) { path_len++; p++; }

    int asset_len = wc_asset_size(path, path_len);
    if (asset_len <= 0 || asset_len > AW_MAX_SOUND_SIZE) return;

    int loaded = wc_load_asset(path, path_len, wav_load_buf, AW_MAX_SOUND_SIZE);
    if (loaded <= 0) return;

    int16_t *pcm = 0;
    int length = 0, ch = 1, sr = 44100;
    if (parseWav(wav_load_buf, loaded, &pcm, &length, &ch, &sr) == 0) {
        sounds[type].samples = pcm;
        sounds[type].length = length;
        sounds[type].channels = ch;
        sounds[type].sample_rate = sr;
    }
}

void AudioWasmcart::initSound()
{
    Config *config = Config::instance();
    if (!config->audioEnabled()) return;

    /* Load sound effects and music from assets */
    loadWav(HeroAmmo00,  "wav/boom.wav");
    loadWav(PowerUp,     "wav/power.wav");
    loadWav(Explosion,   "wav/exploStd.wav");
    loadWav(ExploPop,    "wav/exploPop.wav");
    loadWav(ExploBig,    "wav/exploBig.wav");
    loadWav(LoseLife,    "wav/life_lose.wav");
    loadWav(AddLife,     "wav/life_add.wav");
    loadWav(MusicGame,   "wav/music_game.wav");
    loadWav(MusicMenu,   "wav/music_menu.wav");

    soundsLoaded = 1;
}

void AudioWasmcart::update()
{
    /* Mixing happens in mixAudio(), called from wc_audio_buf */
}

void AudioWasmcart::playSound(SoundType type, float *pos, int age)
{
    (void)pos; (void)age;
    if (type >= NumSoundTypes || !sounds[type].samples) return;

    /* Find a free channel */
    for (int i = 0; i < AW_MAX_CHANNELS; i++) {
        if (!channels[i].active) {
            channels[i].sound = &sounds[type];
            channels[i].position = 0;
            channels[i].fpos = 0;
            channels[i].volume = soundVolume;
            channels[i].active = 1;
            return;
        }
    }
    /* All channels busy — reuse oldest (channel 0) */
    channels[0].sound = &sounds[type];
    channels[0].position = 0;
    channels[0].fpos = 0;
    channels[0].volume = soundVolume;
    channels[0].active = 1;
}

void AudioWasmcart::stopMusic()
{
    musicChannel.active = 0;
}

void AudioWasmcart::pauseGameMusic(bool pause)
{
    musicPaused = pause ? 1 : 0;
}

void AudioWasmcart::setMusicMode(SoundType mode)
{
    currentMusicMode = mode;
    AW_Sound *s = 0;
    if (mode == MusicGame && sounds[MusicGame].samples) {
        s = &sounds[MusicGame];
    } else if (mode == MusicMenu && sounds[MusicMenu].samples) {
        s = &sounds[MusicMenu];
    }
    if (s) {
        musicChannel.sound = s;
        musicChannel.position = 0;
    musicChannel.fpos = 0;
        musicChannel.volume = musicVolume;
        musicChannel.active = 1;
        musicPaused = 0;
    }
}

void AudioWasmcart::setMusicVolume(float vol)
{
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    musicVolume = vol;
    musicChannel.volume = vol;
}

void AudioWasmcart::setSoundVolume(float vol)
{
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    soundVolume = vol;
}

void AudioWasmcart::setMusicIndex(int index)
{
    (void)index;
}

void AudioWasmcart::nextMusicIndex()
{
}

void AudioWasmcart::mixAudio(float *output, int frames)
{
    const float OUTPUT_RATE = 44100.0f;

    /* Clear output buffer (stereo float) */
    memset(output, 0, frames * 2 * sizeof(float));

    /* Mix sound effect channels */
    for (int ch = 0; ch < AW_MAX_CHANNELS; ch++) {
        if (!channels[ch].active || !channels[ch].sound) continue;
        AW_Sound *s = channels[ch].sound;
        float vol = channels[ch].volume;
        float rate = (float)s->sample_rate / OUTPUT_RATE;

        for (int f = 0; f < frames; f++) {
            int pos = (int)channels[ch].fpos;
            if (pos >= s->length) {
                channels[ch].active = 0;
                break;
            }
            if (s->channels == 2) {
                float l = (float)s->samples[pos * 2] / 32768.0f;
                float r = (float)s->samples[pos * 2 + 1] / 32768.0f;
                output[f * 2]     += l * vol;
                output[f * 2 + 1] += r * vol;
            } else {
                float sample = (float)s->samples[pos] / 32768.0f * vol;
                output[f * 2]     += sample;
                output[f * 2 + 1] += sample;
            }
            channels[ch].fpos += rate;
        }
        channels[ch].position = (int)channels[ch].fpos;
    }

    /* Mix music channel (looping) */
    if (musicChannel.active && !musicPaused && musicChannel.sound) {
        AW_Sound *s = musicChannel.sound;
        float vol = musicChannel.volume;
        float rate = (float)s->sample_rate / OUTPUT_RATE;

        for (int f = 0; f < frames; f++) {
            int pos = (int)musicChannel.fpos;
            if (pos >= s->length) {
                musicChannel.fpos = 0;
                pos = 0;
            }
            if (s->channels == 2) {
                float l = (float)s->samples[pos * 2] / 32768.0f;
                float r = (float)s->samples[pos * 2 + 1] / 32768.0f;
                output[f * 2]     += l * vol;
                output[f * 2 + 1] += r * vol;
            } else {
                float sample = (float)s->samples[pos] / 32768.0f * vol;
                output[f * 2]     += sample;
                output[f * 2 + 1] += sample;
            }
            musicChannel.fpos += rate;
        }
        musicChannel.position = (int)musicChannel.fpos;
    }

    /* Clamp mixed output to [-1.0, 1.0] to prevent distortion */
    for (int f = 0; f < frames; f++) {
        float l = output[f * 2];
        float r = output[f * 2 + 1];
        if (l >  1.0f) l =  1.0f;
        if (l < -1.0f) l = -1.0f;
        if (r >  1.0f) r =  1.0f;
        if (r < -1.0f) r = -1.0f;
        output[f * 2]     = l;
        output[f * 2 + 1] = r;
    }
}
