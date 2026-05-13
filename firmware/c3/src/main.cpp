#include <Arduino.h>
#include <Wire.h>

namespace Pins {
constexpr uint8_t I2C_SDA = 8;
constexpr uint8_t I2C_SCL = 9;
constexpr uint8_t MOTOR_PWM_FWD = 4;
constexpr uint8_t MOTOR_PWM_REV = 5;
constexpr uint8_t MOTOR_ENABLE = 6;
constexpr uint8_t FRONT_LIGHTS = 2;
constexpr uint8_t REAR_LIGHTS = 3;
}  // namespace Pins

constexpr uint8_t I2C_ADDRESS = 0x31;
constexpr uint8_t PWM_CHANNEL_FWD = 0;
constexpr uint8_t PWM_CHANNEL_REV = 1;
constexpr uint32_t PWM_FREQ_HZ = 20000;
constexpr uint8_t PWM_RESOLUTION_BITS = 10;
constexpr int16_t MAX_SPEED_CM_S = 100;

struct MovementCommand {
  int16_t targetSpeedCmS = 0;
  int8_t direction = 0;
  uint8_t frontLights = 0;
  uint8_t rearLights = 0;
};

volatile bool hasNewCommand = false;
MovementCommand latestCommand;

void applyMotorCommand(const MovementCommand& command) {
  const int16_t clampedSpeed = constrain(command.targetSpeedCmS, 0, MAX_SPEED_CM_S);
  const uint32_t duty = map(clampedSpeed, 0, MAX_SPEED_CM_S, 0, (1 << PWM_RESOLUTION_BITS) - 1);

  digitalWrite(Pins::MOTOR_ENABLE, command.direction == 0 ? LOW : HIGH);

  if (command.direction > 0) {
    ledcWrite(PWM_CHANNEL_FWD, duty);
    ledcWrite(PWM_CHANNEL_REV, 0);
  } else if (command.direction < 0) {
    ledcWrite(PWM_CHANNEL_FWD, 0);
    ledcWrite(PWM_CHANNEL_REV, duty);
  } else {
    ledcWrite(PWM_CHANNEL_FWD, 0);
    ledcWrite(PWM_CHANNEL_REV, 0);
  }

  digitalWrite(Pins::FRONT_LIGHTS, command.frontLights ? HIGH : LOW);
  digitalWrite(Pins::REAR_LIGHTS, command.rearLights ? HIGH : LOW);
}

void onReceive(int byteCount) {
  if (byteCount != static_cast<int>(sizeof(MovementCommand))) {
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  uint8_t* raw = reinterpret_cast<uint8_t*>(&latestCommand);
  for (size_t i = 0; i < sizeof(MovementCommand) && Wire.available(); ++i) {
    raw[i] = Wire.read();
  }
  hasNewCommand = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(Pins::MOTOR_ENABLE, OUTPUT);
  pinMode(Pins::FRONT_LIGHTS, OUTPUT);
  pinMode(Pins::REAR_LIGHTS, OUTPUT);

  digitalWrite(Pins::MOTOR_ENABLE, LOW);
  digitalWrite(Pins::FRONT_LIGHTS, LOW);
  digitalWrite(Pins::REAR_LIGHTS, LOW);

  ledcSetup(PWM_CHANNEL_FWD, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcSetup(PWM_CHANNEL_REV, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttachPin(Pins::MOTOR_PWM_FWD, PWM_CHANNEL_FWD);
  ledcAttachPin(Pins::MOTOR_PWM_REV, PWM_CHANNEL_REV);

  Wire.begin(I2C_ADDRESS, Pins::I2C_SDA, Pins::I2C_SCL);
  Wire.onReceive(onReceive);
}

void loop() {
  if (hasNewCommand) {
    noInterrupts();
    const MovementCommand command = latestCommand;
    hasNewCommand = false;
    interrupts();

    applyMotorCommand(command);
  }

  delay(5);
}

