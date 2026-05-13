#include <Arduino.h>
#include <Wire.h>

namespace Pins {
constexpr uint8_t I2C_SDA = 8;
constexpr uint8_t I2C_SCL = 9;
constexpr uint8_t IR_POSITION_RX = 4;
constexpr uint8_t DYNAMO_MOTOR_ENABLE = 5;
constexpr uint8_t MAIN_POWER_GOOD = 6;
constexpr uint8_t C3_ENABLE = 7;
constexpr uint8_t MOTOR_TEMP_ADC = 1;
constexpr uint8_t MOSFET_TEMP_ADC = 2;
}  // namespace Pins

constexpr uint8_t C3_I2C_ADDRESS = 0x31;
constexpr uint32_t CONTROL_PERIOD_MS = 20;

enum class PowerState : uint8_t {
  Standby,
  Starting,
  Running,
  Fault,
};

struct MovementCommand {
  int16_t targetSpeedCmS = 0;
  int8_t direction = 0;
  uint8_t frontLights = 0;
  uint8_t rearLights = 0;
};

PowerState powerState = PowerState::Standby;
MovementCommand currentCommand;
uint16_t lastMarkerId = 0;
uint32_t lastControlTick = 0;

void sendCommandToC3(const MovementCommand& command) {
  Wire.beginTransmission(C3_I2C_ADDRESS);
  Wire.write(reinterpret_cast<const uint8_t*>(&command), sizeof(command));
  Wire.endTransmission();
}

void startMainPower() {
  powerState = PowerState::Starting;
  digitalWrite(Pins::DYNAMO_MOTOR_ENABLE, HIGH);
}

void updatePowerState() {
  if (powerState == PowerState::Starting && digitalRead(Pins::MAIN_POWER_GOOD) == HIGH) {
    digitalWrite(Pins::C3_ENABLE, HIGH);
    powerState = PowerState::Running;
  }
}

void highPriorityControlTask(void*) {
  for (;;) {
    const uint32_t now = millis();
    if (now - lastControlTick >= CONTROL_PERIOD_MS) {
      lastControlTick = now;

      updatePowerState();
      if (powerState == PowerState::Running) {
        sendCommandToC3(currentCommand);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void telemetryAndCommsTask(void*) {
  for (;;) {
    // TODO: inizializzare ESP-NOW e inviare telemetria al gateway S2.
    Serial.printf(
        "power=%u marker=%u target=%d dir=%d\n",
        static_cast<uint8_t>(powerState),
        lastMarkerId,
        currentCommand.targetSpeedCmS,
        currentCommand.direction);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(Pins::DYNAMO_MOTOR_ENABLE, OUTPUT);
  pinMode(Pins::MAIN_POWER_GOOD, INPUT);
  pinMode(Pins::C3_ENABLE, OUTPUT);
  pinMode(Pins::IR_POSITION_RX, INPUT);

  digitalWrite(Pins::DYNAMO_MOTOR_ENABLE, LOW);
  digitalWrite(Pins::C3_ENABLE, LOW);

  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

  // Prima versione: avvio automatico. Poi verra sostituito da comando ESP-NOW.
  startMainPower();

  xTaskCreatePinnedToCore(highPriorityControlTask, "control", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(telemetryAndCommsTask, "telemetry", 4096, nullptr, 1, nullptr, 0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

