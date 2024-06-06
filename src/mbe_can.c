#include "can2040.h"
#include "pico/stdlib.h"
#include "pico/time.h"

// can2040 stuff.
//

#define PIO_NUM 0
#define PIO_IRQ PIO0_IRQ_0
#define SYS_CLOCK 125000000
#define BITRATE 500000
#define GPIO_RX 27
#define GPIO_TX 26

static struct can2040 canbus;

static bool got_msg = false;
static struct can2040_msg rx_msg;

// MBE ECU
//

#define MBE_ID_EASIMAP 0xcbe1101lu
#define MBE_ID_ECU     0xcbe0111lu

static struct can2040_msg tx_msg_part1 = {
  .id = (CAN2040_ID_EFF | MBE_ID_EASIMAP),
  .dlc = 8,
  .data = { 0x10, 0xc, 0x1, 0x0, 0x0, 0x0, 0x0, 0xf8 },
};

static struct can2040_msg tx_msg_part2 = {
  .id = (CAN2040_ID_EFF | MBE_ID_EASIMAP),
  .dlc = 8,
  // RPM - ( 0x7d, 0x7c )
  // Coolant temp - ( 0x45, 0x44 )
  // Voltage - ( 0x9f, 0x9e )
  .data = { 0x21, 0x7d, 0x7c, 0x45, 0x44, 0x9f, 0x9e },
};

static uint16_t rpm = 0;
static float temp = 0;
static float volts = 0;

#define POLL_INTERVAL_MS 100
#define POLL_TIMEOUT_MS 500
#define DATA_VALIDITY_MICROS 2000000

static absolute_time_t zero_ts;
static absolute_time_t send_ts;
static absolute_time_t next_send_ts;
static absolute_time_t recv_ts;
static absolute_time_t recv_timeout_ts;

static void can_callback(struct can2040 *cb, uint32_t notify,
                            struct can2040_msg *msg) {
  if (notify == CAN2040_NOTIFY_RX) {
    rx_msg = *msg;
    got_msg = true;
  }
}

static void pio_irq_handler() {
  can2040_pio_irq_handler(&canbus);
}

void mbe_can_setup() {
  can2040_setup(&canbus, PIO_NUM);
  can2040_callback_config(&canbus, can_callback);

  irq_set_exclusive_handler(PIO_IRQ, pio_irq_handler);
  irq_set_priority(PIO_IRQ, 1);
  irq_set_enabled(PIO_IRQ, true);

  can2040_start(&canbus, SYS_CLOCK, BITRATE, GPIO_RX, GPIO_TX);

  zero_ts = from_us_since_boot(0);
  send_ts = from_us_since_boot(0);
  next_send_ts = from_us_since_boot(0);
  recv_ts = from_us_since_boot(0);
  recv_timeout_ts = from_us_since_boot(0);
}

static uint16_t rx_data_u16(uint8_t i1, uint8_t i2) {
  return 0x100 * rx_msg.data[i1] + rx_msg.data[i2];
}

void mbe_can_update() {
  absolute_time_t ts = get_absolute_time();

  if (got_msg) {
    got_msg = false;
    if (rx_msg.id == (CAN2040_ID_EFF | MBE_ID_ECU) && rx_msg.data[1] == 0x81) {
      rpm = rx_data_u16(2, 3);
      temp = rx_data_u16(4, 5) * 160.0f / 65535.0f - 30.0f;
      volts = rx_data_u16(6, 7) * 20.0f / 65535.0f;
    }
    recv_ts = ts;
  }

  if (recv_ts >= send_ts && ts >= next_send_ts
      || ts >= recv_timeout_ts) {
    if (can2040_check_transmit(&canbus)) {
      can2040_transmit(&canbus, &tx_msg_part1);
      can2040_transmit(&canbus, &tx_msg_part2);
    }
    send_ts = ts;
    next_send_ts = delayed_by_ms(ts, POLL_INTERVAL_MS);
    recv_timeout_ts = delayed_by_ms(ts, POLL_TIMEOUT_MS);
  }
}

bool mbe_can_is_data_valid() {
  return recv_ts != zero_ts 
    && absolute_time_diff_us(recv_ts, send_ts) <= DATA_VALIDITY_MICROS;
}

uint16_t mbe_can_rpm() {
  return rpm;
}

float mbe_can_volts() {
  return volts;
}

float mbe_can_temp() {
  return temp;
}
