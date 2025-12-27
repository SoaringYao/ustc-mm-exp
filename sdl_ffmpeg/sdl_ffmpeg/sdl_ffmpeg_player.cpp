#include <filesystem>
#include <iostream>
#include <string>

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
}

/**
 * @brief 打印 FFmpeg 错误信息。
 * @param msg    自定义提示信息。
 * @param errnum FFmpeg 返回的错误码。
 */
static void log_ffmpeg_error(const char* msg, int errnum) {
    char buf[256];
    av_strerror(errnum, buf, sizeof(buf));
    std::cerr << msg << ": " << buf << " (" << errnum << ")" << std::endl;
}

/**
 * @brief 保存播放器相关的所有资源并在析构时自动释放。
 */
struct PlayerResources {
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* vctx = nullptr;
    AVCodecContext* actx = nullptr;
    SwsContext* sws_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_AudioStream* audio_stream = nullptr;

    AVFrame* yuv_frame = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;

    bool sdl_inited = false;

    ~PlayerResources() {
        // FFmpeg 帧/包
        if (packet != nullptr) {
            av_packet_free(&packet);
        }
        if (frame != nullptr) {
            av_frame_free(&frame);
        }
        if (yuv_frame != nullptr) {
            av_frame_free(&yuv_frame);
        }

        // SDL 音频与重采样
        if (audio_stream != nullptr) {
            SDL_DestroyAudioStream(audio_stream);
            audio_stream = nullptr;
        }
        if (swr_ctx != nullptr) {
            swr_free(&swr_ctx);
        }

        // 视频缩放
        if (sws_ctx != nullptr) {
            sws_freeContext(sws_ctx);
        }

        // SDL 图形资源
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        if (sdl_inited) {
            SDL_Quit();
            sdl_inited = false;
        }

        // FFmpeg 解码上下文与格式上下文
        if (actx != nullptr) {
            avcodec_free_context(&actx);
        }
        if (vctx != nullptr) {
            avcodec_free_context(&vctx);
        }
        if (format_ctx != nullptr) {
            avformat_close_input(&format_ctx);
        }
    }
};

/**
 * @brief 仅做 SDL 初始化自检。
 * @return 0 表示成功，非 0 表示失败。
 */
static int RunSDLCheck() {
    // SDL3: SDL_Init 返回 bool，true = 成功，false = 失败
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Log("SDL initialized successfully.");
    SDL_Quit();
    return 0;
}

/**
 * @brief 仅输出 FFmpeg 编译配置，快速检查链接是否正常。
 * @return 0。
 */
static int RunFFmpegCheck() {
    std::cout << "FFmpeg configuration:\n"
        << avcodec_configuration() << "\nOK!" << std::endl;
    return 0;
}

/**
 * @brief 使用 SDL3 + FFmpeg 播放指定视频文件。
 * @param input_path_str 视频路径，空字符串则使用 ./demo.mp4。
 * @return 0 表示播放过程正常结束，非 0 表示出现错误。
 */
static int RunPlayer(const std::string& input_path_str) {
    PlayerResources res;  // RAII 资源管理

    // 1. 解析输入路径
    const std::filesystem::path input_path =
        std::filesystem::absolute(
            input_path_str.empty()
            ? std::filesystem::path("./demo.mp4")
            : std::filesystem::path(input_path_str));

    const std::string filename = input_path.string();
    std::cout << "Opening: " << filename << std::endl;

    // 2. 打开媒体文件
    int ret = avformat_open_input(&res.format_ctx, filename.c_str(), nullptr, nullptr);
    if (ret < 0) {
        log_ffmpeg_error("avformat_open_input failed", ret);
        return 1;
    }

    ret = avformat_find_stream_info(res.format_ctx, nullptr);
    if (ret < 0) {
        log_ffmpeg_error("avformat_find_stream_info failed", ret);
        return 1;
    }

    const int video_stream_index = av_find_best_stream(
        res.format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index < 0) {
        std::cerr << "No video stream found." << std::endl;
        return 1;
    }

    const int audio_stream_index = av_find_best_stream(
        res.format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        std::cerr << "No audio stream found. Video-only mode." << std::endl;
    }

    AVStream* const vstream = res.format_ctx->streams[video_stream_index];
    AVStream* const astream =
        (audio_stream_index >= 0) ? res.format_ctx->streams[audio_stream_index] : nullptr;

    // 3. 初始化视频解码器
    {
        AVCodecParameters* const vpar = vstream->codecpar;
        const AVCodec* const vcodec = avcodec_find_decoder(vpar->codec_id);
        if (vcodec == nullptr) {
            std::cerr << "Video decoder not found." << std::endl;
            return 1;
        }

        res.vctx = avcodec_alloc_context3(vcodec);
        if (res.vctx == nullptr) {
            std::cerr << "Failed to allocate video codec context." << std::endl;
            return 1;
        }

        ret = avcodec_parameters_to_context(res.vctx, vpar);
        if (ret < 0) {
            log_ffmpeg_error("avcodec_parameters_to_context (video) failed", ret);
            return 1;
        }

        ret = avcodec_open2(res.vctx, vcodec, nullptr);
        if (ret < 0) {
            log_ffmpeg_error("avcodec_open2 (video) failed", ret);
            return 1;
        }
    }

    const int frame_width = res.vctx->width;
    const int frame_height = res.vctx->height;

    // 4. 初始化音频解码器（如存在）
    if (audio_stream_index >= 0 && astream != nullptr) {
        AVCodecParameters* const apar = astream->codecpar;
        const AVCodec* const acodec = avcodec_find_decoder(apar->codec_id);
        if (acodec == nullptr) {
            std::cerr << "Audio decoder not found. Will play video only." << std::endl;
        }
        else {
            res.actx = avcodec_alloc_context3(acodec);
            if (res.actx == nullptr) {
                std::cerr << "Failed to allocate audio codec context. Video only." << std::endl;
            }
            else {
                ret = avcodec_parameters_to_context(res.actx, apar);
                if (ret < 0) {
                    log_ffmpeg_error("avcodec_parameters_to_context (audio) failed", ret);
                    avcodec_free_context(&res.actx);
                }
                else {
                    ret = avcodec_open2(res.actx, acodec, nullptr);
                    if (ret < 0) {
                        log_ffmpeg_error("avcodec_open2 (audio) failed", ret);
                        avcodec_free_context(&res.actx);
                    }
                }
            }
        }
    }

    // 5. 初始化 SDL3（SDL3 返回 bool）
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    res.sdl_inited = true;

    // 6. 创建窗口和渲染器
    res.window = SDL_CreateWindow(
        filename.c_str(), frame_width, frame_height, SDL_WINDOW_RESIZABLE);
    if (res.window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    res.renderer = SDL_CreateRenderer(res.window, nullptr);
    if (res.renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    res.texture = SDL_CreateTexture(
        res.renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        frame_width,
        frame_height);
    if (res.texture == nullptr) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_SetWindowPosition(res.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // 7. 创建 SwsContext 与目标 YUV420P 帧
    res.sws_ctx = sws_getContext(
        res.vctx->width,
        res.vctx->height,
        res.vctx->pix_fmt,
        frame_width,
        frame_height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
    if (res.sws_ctx == nullptr) {
        std::cerr << "sws_getContext failed." << std::endl;
        return 1;
    }

    res.yuv_frame = av_frame_alloc();
    if (res.yuv_frame == nullptr) {
        std::cerr << "av_frame_alloc (yuv_frame) failed." << std::endl;
        return 1;
    }

    res.yuv_frame->format = AV_PIX_FMT_YUV420P;
    res.yuv_frame->width = frame_width;
    res.yuv_frame->height = frame_height;

    ret = av_frame_get_buffer(res.yuv_frame, 32);
    if (ret < 0) {
        log_ffmpeg_error("av_frame_get_buffer (yuv_frame) failed", ret);
        return 1;
    }

    // 8. 初始化 SDL 音频与重采样
    if (res.actx != nullptr) {
        SDL_AudioSpec desired{};
        desired.freq = res.actx->sample_rate;
        desired.format = SDL_AUDIO_S16;
        desired.channels = static_cast<Uint8>(
            res.actx->ch_layout.nb_channels != 0
            ? res.actx->ch_layout.nb_channels
            : 2);

        res.audio_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &desired,
            nullptr,
            nullptr);
        if (res.audio_stream == nullptr) {
            std::cerr << "SDL_OpenAudioDeviceStream failed: "
                << SDL_GetError() << "\nAudio disabled." << std::endl;
        }
        else {
            SDL_ResumeAudioStreamDevice(res.audio_stream);

            AVChannelLayout out_ch_layout;
            av_channel_layout_default(&out_ch_layout, desired.channels);

            int swr_err = swr_alloc_set_opts2(
                &res.swr_ctx,
                &out_ch_layout,
                AV_SAMPLE_FMT_S16,
                res.actx->sample_rate,
                &res.actx->ch_layout,
                res.actx->sample_fmt,
                res.actx->sample_rate,
                0,
                nullptr);

            if (swr_err < 0 || res.swr_ctx == nullptr) {
                log_ffmpeg_error("swr_alloc_set_opts2 failed", swr_err);
                swr_free(&res.swr_ctx);
                SDL_DestroyAudioStream(res.audio_stream);
                res.audio_stream = nullptr;
            }
            else {
                swr_err = swr_init(res.swr_ctx);
                if (swr_err < 0) {
                    log_ffmpeg_error("swr_init failed", swr_err);
                    swr_free(&res.swr_ctx);
                    SDL_DestroyAudioStream(res.audio_stream);
                    res.audio_stream = nullptr;
                }
            }
        }
    }

    // 9. 打印基本信息
    double fps = 0.0;
    if (vstream != nullptr &&
        vstream->avg_frame_rate.num != 0 &&
        vstream->avg_frame_rate.den != 0) {
        fps = av_q2d(vstream->avg_frame_rate);
    }
    const double frame_duration_ms =
        (fps > 0.0)
        ? (1000.0 / fps)
        : (vstream != nullptr ? (1000.0 * av_q2d(vstream->time_base)) : 0.0);

    std::cout << "Video Stream Index: " << video_stream_index << std::endl;
    if (audio_stream_index >= 0) {
        std::cout << "Audio Stream Index: " << audio_stream_index << std::endl;
    }
    std::cout << "FPS: " << fps << std::endl;
    std::cout << "Width: " << frame_width
        << ", Height: " << frame_height << std::endl;
    if (res.actx != nullptr) {
        std::cout << "Audio Sample Rate: " << res.actx->sample_rate
            << ", Channels: " << res.actx->ch_layout.nb_channels << std::endl;
    }

    // 10. 分配解码用的 packet 与 frame
    res.packet = av_packet_alloc();
    res.frame = av_frame_alloc();
    if (res.packet == nullptr || res.frame == nullptr) {
        std::cerr << "Failed to allocate packet/frame." << std::endl;
        return 1;
    }

    // 11. 主播放循环
    bool running = true;
    Uint64 last_frame_ticks = SDL_GetTicks();

    SDL_Event ev{};
    SDL_FRect dst_rect{
        0.0F,
        0.0F,
        static_cast<float>(frame_width),
        static_cast<float>(frame_height) };

    while (running && av_read_frame(res.format_ctx, res.packet) >= 0) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        if (!running) {
            break;
        }

        if (res.packet->stream_index == video_stream_index) {
            // 视频解码
            ret = avcodec_send_packet(res.vctx, res.packet);
            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                log_ffmpeg_error("avcodec_send_packet (video) failed", ret);
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(res.vctx, res.frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                if (ret < 0) {
                    log_ffmpeg_error("avcodec_receive_frame (video) failed", ret);
                    break;
                }

                sws_scale(
                    res.sws_ctx,
                    res.frame->data,
                    res.frame->linesize,
                    0,
                    res.vctx->height,
                    res.yuv_frame->data,
                    res.yuv_frame->linesize);

                SDL_UpdateYUVTexture(
                    res.texture,
                    nullptr,
                    res.yuv_frame->data[0], res.yuv_frame->linesize[0],
                    res.yuv_frame->data[1], res.yuv_frame->linesize[1],
                    res.yuv_frame->data[2], res.yuv_frame->linesize[2]);

                SDL_RenderClear(res.renderer);
                SDL_RenderTexture(res.renderer, res.texture, nullptr, &dst_rect);
                SDL_RenderPresent(res.renderer);

                const Uint64 now = SDL_GetTicks();
                const double elapsed = static_cast<double>(now - last_frame_ticks);
                if (frame_duration_ms > 0.0 && elapsed < frame_duration_ms) {
                    SDL_Delay(static_cast<Uint32>(frame_duration_ms - elapsed));
                }
                last_frame_ticks = SDL_GetTicks();
            }
        }
        else if (res.actx != nullptr &&
            res.audio_stream != nullptr &&
            res.packet->stream_index == audio_stream_index) {
            // 音频解码
            ret = avcodec_send_packet(res.actx, res.packet);
            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                log_ffmpeg_error("avcodec_send_packet (audio) failed", ret);
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(res.actx, res.frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                if (ret < 0) {
                    log_ffmpeg_error("avcodec_receive_frame (audio) failed", ret);
                    break;
                }

                if (res.swr_ctx == nullptr || res.audio_stream == nullptr) {
                    continue;
                }

                const int64_t delay = swr_get_delay(res.swr_ctx, res.actx->sample_rate);
                const int64_t total = delay + res.frame->nb_samples;
                const int dst_nb_samples = static_cast<int>(
                    av_rescale_rnd(
                        total,
                        res.actx->sample_rate,
                        res.actx->sample_rate,
                        AV_ROUND_UP));

                uint8_t** dst_data = nullptr;
                int dst_linesize = 0;
                const int channels = res.actx->ch_layout.nb_channels;

                const int alloc_ret = av_samples_alloc_array_and_samples(
                    &dst_data,
                    &dst_linesize,
                    channels,
                    dst_nb_samples,
                    AV_SAMPLE_FMT_S16,
                    0);
                if (alloc_ret < 0) {
                    log_ffmpeg_error("av_samples_alloc_array_and_samples failed", alloc_ret);
                    break;
                }

                const int converted = swr_convert(
                    res.swr_ctx,
                    dst_data,
                    dst_nb_samples,
                    const_cast<const uint8_t**>(res.frame->data),
                    res.frame->nb_samples);

                if (converted < 0) {
                    log_ffmpeg_error("swr_convert failed", converted);
                    av_freep(&dst_data[0]);
                    av_freep(&dst_data);
                    break;
                }

                const int data_size = av_samples_get_buffer_size(
                    &dst_linesize,
                    channels,
                    converted,
                    AV_SAMPLE_FMT_S16,
                    1);
                if (data_size > 0) {
                    SDL_PutAudioStreamData(res.audio_stream, dst_data[0], data_size);
                }

                av_freep(&dst_data[0]);
                av_freep(&dst_data);
            }
        }

        av_packet_unref(res.packet);
    }

    return 0;
}

//------------------- main：模式分发 -------------------
int main(int argc, char* argv[]) {
    std::string mode;
    std::string video_path;

    // 命令行参数：
    //   --check-sdl
    //   --check-ffmpeg
    //   其他参数视为视频文件路径（最后一个为准）
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--check-sdl") {
            mode = "check-sdl";
        }
        else if (arg == "--check-ffmpeg") {
            mode = "check-ffmpeg";
        }
        else {
            video_path = arg;
        }
    }

    if (mode == "check-sdl") {
        return RunSDLCheck();
    }
    if (mode == "check-ffmpeg") {
        return RunFFmpegCheck();
    }

    // 默认：播放视频
    return RunPlayer(video_path);
}
