/*
 * AudioWasmcart.h — Audio via wasmcart PCM ring buffer
 * Subclass of Audio that loads WAV files and mixes to PCM output.
 */
#ifndef AudioWasmcart_h
#define AudioWasmcart_h

#include "Audio.h"

#define AW_MAX_CHANNELS 8
#define AW_SAMPLE_RATE 44100
#define AW_MAX_SOUND_SIZE (2 * 1024 * 1024) /* 2MB max per sound */

struct AW_Sound {
    int16_t *samples;   /* PCM data (mono or stereo interleaved) */
    int length;          /* total samples (per channel) */
    int channels;        /* 1 or 2 */
    int sample_rate;     /* original sample rate */
};

struct AW_Channel {
    AW_Sound *sound;
    int position;       /* current sample position (integer) */
    float fpos;         /* fractional position for resampling */
    float volume;
    int active;
};

class AudioWasmcart : public Audio
{
public:
    AudioWasmcart();
    virtual ~AudioWasmcart();

    virtual void update();
    virtual void playSound(SoundType type, float *pos, int age = 0);
    virtual void stopMusic();
    virtual void pauseGameMusic(bool);
    virtual void setMusicMode(SoundType);
    virtual void setMusicVolume(float);
    virtual void setSoundVolume(float);
    virtual void setMusicIndex(int);
    virtual void nextMusicIndex();

    /* Called by wc_audio_buf to fill the output buffer */
    void mixAudio(float *output, int frames);

private:
    void loadWav(SoundType type, const char *path);
    void initSound();

    AW_Sound sounds[NumSoundTypes];
    AW_Channel channels[AW_MAX_CHANNELS];

    /* Music channel (separate from sfx) */
    AW_Channel musicChannel;
    SoundType currentMusicMode;

    float soundVolume;
    float musicVolume;
    int musicPaused;
    int soundsLoaded;
};

#endif // AudioWasmcart_h
