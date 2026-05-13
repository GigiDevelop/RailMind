#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#define IR_LED_PIN PB0
#define CHAIN_IN_PIN PB1
#define CHAIN_OUT_PIN PB2
#define RESET_PIN PB3

#define BIT_US 208
#define EEPROM_EMPTY 0xFFFF
#define DEFAULT_MARKER_ID 1

uint16_t EEMEM ee_marker_id = EEPROM_EMPTY;

static void carrier_38khz_for_bit(void) {
  for (uint8_t i = 0; i < 8; ++i) {
    PORTB |= (1 << IR_LED_PIN);
    _delay_us(13);
    PORTB &= ~(1 << IR_LED_PIN);
    _delay_us(13);
  }
}

static void space_for_bit(void) {
  PORTB &= ~(1 << IR_LED_PIN);
  _delay_us(BIT_US);
}

static void send_bit(uint8_t bit_value) {
  if (bit_value) {
    space_for_bit();
  } else {
    carrier_38khz_for_bit();
  }
}

static void send_byte(uint8_t value) {
  send_bit(0);
  for (uint8_t i = 0; i < 8; ++i) {
    send_bit((value >> i) & 0x01);
  }
  send_bit(1);
}

static uint8_t crc8_update(uint8_t crc, uint8_t value) {
  crc ^= value;
  for (uint8_t i = 0; i < 8; ++i) {
    if (crc & 0x80) {
      crc = (uint8_t)((crc << 1) ^ 0x07);
    } else {
      crc <<= 1;
    }
  }
  return crc;
}

static uint8_t packet_crc(uint8_t id_low, uint8_t id_high) {
  uint8_t crc = 0;
  crc = crc8_update(crc, 0xAA);
  crc = crc8_update(crc, 0x55);
  crc = crc8_update(crc, id_low);
  crc = crc8_update(crc, id_high);
  return crc;
}

static void send_marker_packet(uint16_t marker_id) {
  const uint8_t id_low = (uint8_t)(marker_id & 0xFF);
  const uint8_t id_high = (uint8_t)(marker_id >> 8);
  const uint8_t crc = packet_crc(id_low, id_high);

  send_byte(0xAA);
  send_byte(0x55);
  send_byte(id_low);
  send_byte(id_high);
  send_byte(crc);
}

static uint8_t pin_is_high(uint8_t pin) {
  return (PINB & (1 << pin)) != 0;
}

static uint16_t load_or_assign_marker_id(void) {
  if (pin_is_high(RESET_PIN)) {
    eeprom_update_word(&ee_marker_id, EEPROM_EMPTY);
  }

  uint16_t marker_id = eeprom_read_word(&ee_marker_id);
  if (marker_id != EEPROM_EMPTY) {
    return marker_id;
  }

  while (!pin_is_high(CHAIN_IN_PIN)) {
    _delay_ms(10);
  }

  // TODO: sostituire DEFAULT_MARKER_ID con valore ricevuto da linea dati.
  marker_id = DEFAULT_MARKER_ID;
  eeprom_update_word(&ee_marker_id, marker_id);

  PORTB |= (1 << CHAIN_OUT_PIN);
  _delay_ms(100);
  PORTB &= ~(1 << CHAIN_OUT_PIN);

  return marker_id;
}

int main(void) {
  DDRB |= (1 << IR_LED_PIN) | (1 << CHAIN_OUT_PIN);
  DDRB &= ~((1 << CHAIN_IN_PIN) | (1 << RESET_PIN));
  PORTB &= ~((1 << IR_LED_PIN) | (1 << CHAIN_OUT_PIN));

  const uint16_t marker_id = load_or_assign_marker_id();

  for (;;) {
    send_marker_packet(marker_id);
    _delay_ms(2);
  }
}

