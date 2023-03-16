

// This file reimplements Android/media/AudioTrack class.
// Reference: https://developer.android.com/reference/android/media/AudioTrack


// https://developer.android.com/reference/android/media/AudioTrack#AudioTrack(int,%20int,%20int,%20int,%20int,%20int)
jobject audioTrack_init(jmethodID id, va_list args) {
    int streamType = va_arg(args, int);
    int sampleRateInHz = va_arg(args, int);
    int channelConfig = va_arg(args, int);
    int audioFormat = va_arg(args, int);
    int bufferSizeInBytes = va_arg(args, int);
    int mode = va_arg(args, int);

    logv_info("audioTrack_init(streamType: %i, sampleRateInHz: %i, "
              "channelConfig: %i, audioFormat: %i, bufferSizeInBytes: %i, "
              "mode: %i)", streamType, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mode);

    return 0xfeed; // anything non-null works
}


// https://developer.android.com/reference/android/media/AudioTrack#getMinBufferSize(int,%20int,%20int)
jint audioTrack_getMinBufferSize(jmethodID id, va_list args) {
    int sampleRateInHz = va_arg(args, int);
    int channelConfig = va_arg(args, int);
    int audioFormat = va_arg(args, int);

    logv_info("audioTrack_getMinBufferSize(sampleRateInHz: %i, channelConfig: %i, "
              "audioFormat: %i", sampleRateInHz, channelConfig, audioFormat);

    return 256; // stub value, no thought behind it
}


// https://developer.android.com/reference/android/media/AudioTrack#play()
void audioTrack_play(jmethodID id, va_list args) {
    // no arguments
    log_info("audioTrack_play()");
}


// https://developer.android.com/reference/android/media/AudioTrack#pause()
void audioTrack_pause(jmethodID id, va_list args) {
    // no arguments
    log_info("audioTrack_pause()");
}


// https://developer.android.com/reference/android/media/AudioTrack#stop()
void audioTrack_stop(jmethodID id, va_list args) {
    // no arguments
    log_info("audioTrack_stop()");
}


// https://developer.android.com/reference/android/media/AudioTrack#release()
void audioTrack_release(jmethodID id, va_list args) {
    // no arguments
    log_info("audioTrack_release()");
}


// https://developer.android.com/reference/android/media/AudioTrack#write(byte[],%20int,%20int)
jint audioTrack_write(jmethodID id, va_list args) {
    jbyteArray _audioData = va_arg(args, jbyteArray);
    int offsetInBytes = va_arg(args, int);
    int sizeInBytes = va_arg(args, int);

    // Before accessing/changing the array elements, we have to do the following:
    JavaDynArray * jda = jda_find(_audioData);
    if (!jda) {
        log_error("[audioTrack_write] Provided buffer is not a valid JDA.");
        return 0;
    }

    uint8_t * audioData = jda->array; // Now this array we can work with

    // Do something

    return 0;
}
