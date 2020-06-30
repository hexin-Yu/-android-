#include <jni.h>
#include <string>
#include <thread>
#include <android/log.h>


const char* JNIREG_CLASS = "com/yu/threeaudioplayers/FFmpegMainActivity";
#define NELEM(X) ((int)(sizeof((X))/sizeof((X)[0])))
#define LOGE(X)  __android_log_print(ANDROID_LOG_ERROR, "ERROR", (X));
#define LOGD(X)  __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", (X));

extern "C"{

#include "libavcodec/avcodec.h"     //编码
#include "libavformat/avformat.h"     //封装格式
#include "libswscale/swscale.h"          //缩放处理
#include "libswresample/swresample.h"    //重采样
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"

/*
#include <libavcodec/jni.h>
JNIEXPORT jint JNI_OnLoad(JavaVM * vm, void * res) // jni 初始化时会调用此函数
{
    // ffmpeg要使用硬解码，需要将java虚拟机环境传给ffmpeg
    av_jni_set_java_vm(vm, 0);
    return JNI_VERSION_1_4; // 选用jni 1.4 版本
}
*/

}


AVPacket packet;
int audioStream;
AVFrame *aFrame;
SwrContext *swr;
AVFormatContext *aFormatCtx;
AVCodecContext *aCodecCtx;
AVStream *aStream;

uint8_t *outputBuffer;
size_t outputBufferSize;
int rate;
int channel;

static void *buffer;
static size_t bufferSize;
static bool bStop = false;
static double audio_clock;
static JNIEnv *new_env;
static JNIEnv *this_env;
static JavaVM *this_vm;
static jclass instance;

static jclass thiz;
static jmethodID method_id;

const char *file_name;

void play_back();

int init(){

    const char *method_name     = "playback";
    const char *method_sign     = "([BI)V";

    method_id = this_env->GetMethodID(thiz, method_name, method_sign);
    if (method_id == NULL){
        __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "method id null. ");
    } else{
        __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "method id not null. ");
    }

    av_register_all();     //注册所有容器格式和CODEC

    aFormatCtx = avformat_alloc_context();  //属于libavformat库，创建一个avformatcontext对象

    if (file_name == NULL){
        LOGE("file name is null. ");
        return -1;
    }

    // Open audio file
    if (avformat_open_input(&aFormatCtx, file_name, NULL, NULL) != 0) {
        LOGE("Couldn't open file:%s\n");
        return -1; // Couldn't open file
    }

    // Retrieve stream information 从文件中提取流信息
    if (avformat_find_stream_info(aFormatCtx, NULL) < 0) {
        LOGE("Couldn't find stream information.");
        return -1;
    }

    // Find the first audio stream
    int i;
    audioStream = -1;
    for (i = 0; i < aFormatCtx->nb_streams; i++) {
        if (aFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
            audioStream < 0) {
            audioStream = i;
        }
    }
    if (audioStream == -1) {
        LOGE("Couldn't find audio stream!");
        return -1;
    }

    // Get a pointer to the codec context for the video stream
    aStream = aFormatCtx->streams[audioStream];
    aCodecCtx = aFormatCtx->streams[audioStream]->codec;

    // Find the decoder for the audio stream
     //aCodec =avcodec_find_decoder_by_name("h264_mediacodec");   //硬解接口调用
    AVCodec *aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
        LOGE("Could not open codec.");
        return -1; // Could not open codec
    }
    //按解码帧分配内存？
    aFrame = av_frame_alloc();

    // 设置格式转换
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_layout",  aCodecCtx->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", aCodecCtx->channel_layout,  0);
    av_opt_set_int(swr, "in_sample_rate",     aCodecCtx->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate",    aCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  aCodecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
    swr_init(swr);

    // 分配PCM数据缓存
    outputBufferSize = 8196;
    outputBuffer = (uint8_t *) malloc(sizeof(uint8_t) * outputBufferSize);

    // 返回sample rate和channels
    rate = aCodecCtx->sample_rate;
    channel = aCodecCtx->channels;
    return 0;

}

void _play(JNIEnv *env, jclass clazz, jstring object, jobject obj){

    file_name = env->GetStringUTFChars(object, NULL);
    __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "file %s", file_name);
    init();
    instance = (jclass)env->NewGlobalRef(obj);

//    env->CallVoidMethod(instance, method_id);

    std::thread data_thread(play_back);
    data_thread.detach();
}

int getPCM(void **pcm, size_t *pcmSize, jbyteArray jarray);
void play_back(){

    this_vm->AttachCurrentThread(&new_env, NULL);

    __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "playback thread is run. ");

    jbyteArray jarray = NULL;
    while (!bStop){
        if (getPCM(&buffer, &bufferSize, jarray) != 0){
            break;
        }
    }
    __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "playback thread is end. ");
    this_vm->DetachCurrentThread();
}

int getPCM(void **pcm, size_t *pcmSize, jbyteArray jarray) {
    LOGD("getPcm");
     //不断从码流中提取帧数据
    while (av_read_frame(aFormatCtx, &packet) >= 0) {

        int frameFinished = 0;
        // Is this a packet from the audio stream?
        if (packet.stream_index == audioStream) {
            /*
             * 计算时间戳
             *
             *
             */
            aFrame->pts = av_frame_get_best_effort_timestamp(aFrame);
            audio_clock = aFrame->pts * av_q2d(aStream->time_base);  //计算显示时间
            __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "time base is %lf", (av_q2d(aStream->time_base)));
            __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "pts is aFrame->pts %ld", aFrame->pts);
            __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "audio timestamp is %f", audio_clock);

            //判断帧的类型，对于音频的调用
            avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);

            if (frameFinished) {
                // data_size为音频数据所占的字节数
                int data_size = av_samples_get_buffer_size(
                        aFrame->linesize, aCodecCtx->channels,
                        aFrame->nb_samples, aCodecCtx->sample_fmt, 1);

                // 这里内存再分配可能存在问题
                if (data_size > outputBufferSize) {
                    outputBufferSize = data_size;
                    outputBuffer = (uint8_t *) realloc(outputBuffer,
                                                       sizeof(uint8_t) * outputBufferSize);
                }

                // 音频格式转换
                swr_convert(swr, &outputBuffer, aFrame->nb_samples,
                            (uint8_t const **) (aFrame->extended_data),
                            aFrame->nb_samples);

                // 返回pcm数据
                *pcm = outputBuffer;
                *pcmSize = data_size;

                if (jarray == NULL){
                    __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", "jarray is null. ");
                    jbyteArray jbyteArray1 = new_env->NewByteArray(data_size + 1);
                    jarray = (jbyteArray)new_env->NewGlobalRef(jbyteArray1);
                    new_env->DeleteLocalRef(jbyteArray1);
                } else if (new_env->GetArrayLength(jarray) < (data_size+1)){
                    new_env->DeleteGlobalRef(jarray);
                    jbyteArray jbyteArray1 = new_env->NewByteArray(data_size + 1);
                    jarray = (jbyteArray)new_env->NewGlobalRef(jbyteArray1);
                    new_env->DeleteLocalRef(jbyteArray1);
                }

                new_env->SetByteArrayRegion(jarray, 0, data_size, (jbyte*)outputBuffer);

                new_env->CallVoidMethod(instance, method_id, jarray, data_size);

                return 0;
            }
        }
    }
    return -1;
}

static JNINativeMethod g_methods[] = {
        { "native_play",    "(Ljava/lang/String;Lcom/yu/threeaudioplayers/FFmpegMainActivity;)V",    (void *) _play },
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }

    this_env = env;
    jclass clazz = NULL;
    clazz = env->FindClass(JNIREG_CLASS);
    thiz = (jclass)env->NewGlobalRef(clazz);

    this_vm = vm;

    env->RegisterNatives(clazz, g_methods, NELEM(g_methods) );

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved)

{
    
}