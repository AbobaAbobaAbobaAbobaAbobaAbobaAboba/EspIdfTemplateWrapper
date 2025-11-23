#include "core.hpp"
#include "../scheduler/scheduler.hpp"
Core::Core(const Credentials& _creds)
    : creds_(_creds),
      config_wrapper_("/settings.ini") {
    if (instance_ != nullptr) {
        throw std::runtime_error("Core instance already exists!");
    }
    instance_ = this;
    init();
    applyRotationFromConfigAboba();
}

bool isTimeInBracket(
  int startHour,
  int startMin,
  int startSec,
  int endHour,
  int endMin,
  int endSec) {
    time_t now = time(nullptr);
    tm now_time = *localtime(&now);

    // Convert current time to seconds since midnight
    int nowSeconds = now_time.tm_hour * 3600 + now_time.tm_min * 60 + now_time.tm_sec;
    int startSeconds = startHour * 3600 + startMin * 60 + startSec;
    int endSeconds = endHour * 3600 + endMin * 60 + endSec;

    if (startSeconds <= endSeconds) {
        // Normal interval (doesn't cross midnight)
        return nowSeconds >= startSeconds && nowSeconds < endSeconds;
    } else {
        // Interval crosses midnight
        return nowSeconds >= startSeconds || nowSeconds < endSeconds;
    }
}

void Core::applyRotationFromConfigAboba() {
    // rotation
    auto rotation_on_time =
      config_wrapper_.get<std::string>("rotation", "rotation_on_time");
    auto rotation_off_time =
      config_wrapper_.get<std::string>("rotation", "rotation_off_time");

    int dir = config_wrapper_.get<int>("rotation", "dir");
    int step_mode = config_wrapper_.get<int>("rotation", "step_mode");
    int steps = config_wrapper_.get<int>("rotation", "steps");
    int rotation_interval = config_wrapper_.get<int>("rotation", "rotation_interval");

    //
    // light

    auto light_on_time = config_wrapper_.get<std::string>("light", "light_on_time");
    auto light_off_time = config_wrapper_.get<std::string>("light", "light_off_time");

    {
        auto [isOk_on, hour_on, minute_on, second_on] = parseTime(rotation_on_time);
        auto [isOk_off, hour_off, minute_off, second_off] =
          parseTime(rotation_off_time);

        if (!isOk_on || !isOk_off) {
            throw std::runtime_error("Failed to parse time!");
        }

        auto callback_on = [steps, rotation_interval, dir, step_mode, this]() {
            scheduleRotation(
              steps,
              rotation_interval,
              static_cast<RotationController::Direction>(dir),
              static_cast<RotationController::StepMode>(step_mode));
        };
        auto callback_off = [this]() { unscheduleRotation(); };

        auto [status1, id1] = Scheduler::Scheduler::schedule_at(
          hour_on,
          minute_on,
          second_on,
          false,
          callback_on,
          nullptr);

        auto [status2, id2] = Scheduler::Scheduler::schedule_at(
          hour_off,
          minute_off,
          second_off,
          false,
          callback_off,
          nullptr);

        esp_log(kInfoLogConfig, "ABOBA", "Id of rotation on %d!", id1);
        esp_log(
          kInfoLogConfig,
          "ABOBA",
          "Status of rotation on %d!",
          static_cast<int>(status1));

        esp_log(kInfoLogConfig, "ABOBA", "Id of rotation off %d!", id2);
        esp_log(
          kInfoLogConfig,
          "ABOBA",
          "Status of rotation off %d!",
          static_cast<int>(status2));

        bool isOn = isTimeInBracket(
          hour_on,
          minute_on,
          second_on,
          hour_off,
          minute_off,
          second_off);
        if (isOn) {
            callback_on();
        }
    }
}
void Core::init() {
    wifi_handler_.connect(creds_.ssid, creds_.password, 20);
    wifi_handler_.waitForConnected();

    if (wifi_handler_.isConnected()) {
        SntpWrapper::start();
    }

    config_wrapper_.load();
    config_wrapper_.printContents();

    http_server_.startWebserver();
    http_server_.registerCallback(&Core::applyPacket);
}
void Core::blinkCrap() {
    global::led_pin->write(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    global::led_pin->write(false);
    vTaskDelay(pdMS_TO_TICKS(100));

    global::led_pin->write(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    global::led_pin->write(false);
    vTaskDelay(pdMS_TO_TICKS(100));
}
void Core::applyPacket(
  std::shared_ptr<WSPacket::PacketBase> packet,
  [[maybe_unused]] int sockfd) {
    static_assert(
      6 == static_cast<int>(WSPacket::PacketType::COUNT),
      "PacketType::COUNT mismatch. Check if enough Packet structs exist");
    switch (static_cast<WSPacket::PacketType>(packet->packet_type)) {
        case WSPacket::PacketType::LED: {
            WSPacket::PacketLED* led_packet =
              static_cast<WSPacket::PacketLED*>(packet.get());
            instance_->applyLED(led_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::SERVO: {
            WSPacket::PacketServo* servo_packet =
              static_cast<WSPacket::PacketServo*>(packet.get());
            instance_->applyServo(servo_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::TASK_START: {
            WSPacket::PacketTaskStart* temp_packet =
              static_cast<WSPacket::PacketTaskStart*>(packet.get());
            instance_->applyTaskStart(temp_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::TASK_STOP: {
            WSPacket::PacketTaskStop* temp_packet =
              static_cast<WSPacket::PacketTaskStop*>(packet.get());
            instance_->applyTaskStop(temp_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::ROTATION_START: {
            WSPacket::PacketRotationStart* temp_packet =
              static_cast<WSPacket::PacketRotationStart*>(packet.get());
            instance_->applyRotationStart(temp_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::ROTATION_STOP: {
            WSPacket::PacketRotationStop* temp_packet =
              static_cast<WSPacket::PacketRotationStop*>(packet.get());
            instance_->applyRotationStop(temp_packet, sockfd);
            break;
        }
        default:
            esp_log(
              kErrorLogConfig,
              "CORE",
              "Received unknown packet type: %d",
              static_cast<int>(packet->packet_type));
            break;
    }
}

void Core::applyLED(
  const WSPacket::PacketLED* led_packet,
  [[maybe_unused]] int sockfd) {
    // global::led_pin->write(led_packet->state);
    Scheduler::Scheduler::schedule(TimeConv::sec(3), true, [&]() {
        this->blinkCrap();
    });
    esp_log(
      kInfoLogConfig,
      "CORE",
      "LED state changed to: %s",
      led_packet->state ? "ON" : "OFF");
    std::unique_ptr<WSPacket::PacketRespBase> packet =
      std::make_unique<WSPacket::PacketRespLED>(WSPacket::RespType::OK);
    http_server_.sendPacket(packet.get(), sockfd);
}

void Core::applyServo(
  const WSPacket::PacketServo* servo_packet,
  [[maybe_unused]] int sockfd) {
    constexpr static int angle = (2500 - 500) / 180;
    throw std::runtime_error("not implemented servo");
    // global::pwm_pin->write(500 + servo_packet->value * angle);

    esp_log(kInfoLogConfig, "CORE", "Servo value changed to: %d", servo_packet->value);
    std::unique_ptr<WSPacket::PacketRespBase> packet =
      std::make_unique<WSPacket::PacketRespServo>(WSPacket::RespType::OK);
    http_server_.sendPacket(packet.get(), sockfd);
}

void Core::applyTaskStart(
  const WSPacket::PacketTaskStart* packet,
  [[maybe_unused]] int sockfd) {
    auto exec_callback = []() { time_t now = time(nullptr); };
    auto error_callback = [](esp_err_t error) {
        std::cout << "received error: " << error << std::endl;
    };

    Scheduler::Scheduler::schedule_at(
      packet->hour,
      packet->minute,
      packet->second,
      false,
      exec_callback,
      error_callback);
    std::unique_ptr<WSPacket::PacketRespBase> resp_packet =
      std::make_unique<WSPacket::PacketRespTaskStart>(WSPacket::RespType::OK);
    http_server_.sendPacket(resp_packet.get(), sockfd);
}

void Core::applyTaskStop(
  const WSPacket::PacketTaskStop* packet,
  [[maybe_unused]] int sockfd) {
    esp_log(
      kErrorLogConfig,
      TAG,
      "ABOBA UNSCHEDULED %d %d",
      packet->id,
      Scheduler::Scheduler::unschedule(packet->id));

    std::unique_ptr<WSPacket::PacketRespBase> resp_packet =
      std::make_unique<WSPacket::PacketRespTaskStart>(WSPacket::RespType::OK);
    http_server_.sendPacket(resp_packet.get(), sockfd);
}
