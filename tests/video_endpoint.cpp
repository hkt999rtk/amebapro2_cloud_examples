/* Adapted from RTK SDK host endpoint to send this repository's actual fixture. */
#include "rtk_ameba_libdatachannel.h"
#include <rtc/rtc.h>
extern "C" {
#include "test_video.h"
}
#include "playback.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

namespace {
void RTC_API log_message(rtcLogLevel, const char *message) {
    if (message != nullptr) std::cerr << message << '\n';
}

bool read_frame(std::string &value) {
    std::string line;
    if (!std::getline(std::cin, line)) return false;
    char *end = nullptr;
    const auto parsed = std::strtoull(line.c_str(), &end, 10);
    if (end == line.c_str() || *end != '\0' || parsed == 0U || parsed > 256U * 1024U)
        return false;
    value.resize(static_cast<size_t>(parsed));
    std::cin.read(value.data(), static_cast<std::streamsize>(value.size()));
    return std::cin.gcount() == static_cast<std::streamsize>(value.size());
}

bool write_frame(const char *value) {
    const size_t size = std::strlen(value);
    std::cout << size << '\n';
    std::cout.write(value, static_cast<std::streamsize>(size));
    std::cout.flush();
    return static_cast<bool>(std::cout);
}
} // namespace

int main() {
    rtcInitLogger(RTC_LOG_WARNING, log_message);
    unsigned long iterations = 1U;
    if (const char *raw = std::getenv("RTK_TEST_ITERATIONS")) {
        char *end = nullptr;
        iterations = std::strtoul(raw, &end, 10);
        if (end == raw || *end != '\0' || iterations == 0U || iterations > 100U)
            return 2;
    }
    const char *ice_json = std::getenv("RTK_TEST_ICE_JSON_AMEBA");
    if (ice_json == nullptr) ice_json = "[]";

    for (unsigned long iteration = 0U; iteration < iterations; ++iteration) {
        std::string offer;
        if (!read_frame(offer)) return 3;
        rtk_ameba_peer_vtable_t peer{};
        rtk_ameba_libdatachannel_config_t config{};
        config.gathering_timeout_ms = 10000U;
        config.force_relay = std::getenv("RTK_TEST_FORCE_RELAY") != nullptr;
        if (rtk_ameba_libdatachannel_create(&config, &peer) != 0) return 4;
        std::vector<char> answer(256U * 1024U, '\0');
        if (peer.start_answer(peer.context, ice_json, offer.c_str(), answer.data(), answer.size()) != 0 ||
            !write_frame(answer.data())) {
            rtk_ameba_libdatachannel_destroy(&peer); return 5;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (peer.state(peer.context) != RTC_CONNECTED &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (peer.state(peer.context) != RTC_CONNECTED) {
            std::cerr << "device did not reach connected state: " << peer.state(peer.context) << '\n';
            rtk_ameba_libdatachannel_destroy(&peer); return 6;
        }
        playback_t playback{0, 0};
        for (unsigned i = 0; i < TEST_FRAME_COUNT * 2; ++i) {
            auto timestamp = playback.timestamp;
            const auto &sample = test_frames[playback.index];
            rtk_video_frame_t frame{test_video + sample.offset, sample.size,
                                   timestamp, RTK_VIDEO_FLAG_KEYFRAME};
            playback_advance(&playback, TEST_FRAME_COUNT);
            const int result = peer.send_video(peer.context, &frame);
            if (result != 0) std::cerr << "send_video failed at " << timestamp << '\n';
            std::this_thread::sleep_for(std::chrono::microseconds(66667));
        }
        rtk_ameba_libdatachannel_destroy(&peer);
    }
    std::cerr << "Ameba repeated connect/close iterations=" << iterations << '\n';
    return 0;
}
