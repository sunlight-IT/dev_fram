#ifndef __PORT_TIM_SEMANTIC_H__
#define __PORT_TIM_SEMANTIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define PORT_TIM_CAPTURE_RING_CAPACITY 8u

#define PORT_TIM_CAP_BASIC   (1u << 0)
#define PORT_TIM_CAP_CAPTURE (1u << 1)
#define PORT_TIM_CAP_PWM     (1u << 2)

typedef enum
{
  USR_PORT_TIM_OWNER_NONE = 0,
  USR_PORT_TIM_OWNER_SCHEDULER,
  USR_PORT_TIM_OWNER_BASIC,
  USR_PORT_TIM_OWNER_CAPTURE,
  USR_PORT_TIM_OWNER_PWM,
} port_tim_owner_t;

typedef struct
{
  uint16_t prescaler;
  uint32_t auto_reload;
  uint32_t counter_hz;
  uint32_t actual_period_us;
  uint32_t actual_frequency_hz;
  bool quantized;
} port_tim_calc_result_t;

typedef struct
{
  uint32_t sequence;
  uint32_t raw_counter;
  uint32_t update_epoch;
  uint8_t channel;
  uint8_t edge;
  bool overcapture;
  bool update_pending;
} port_tim_capture_raw_event_t;

typedef struct
{
  port_tim_capture_raw_event_t events[PORT_TIM_CAPTURE_RING_CAPACITY];
  volatile uint8_t read_index;
  volatile uint8_t write_index;
  volatile uint8_t count;
  volatile uint32_t dropped_count;
} port_tim_capture_ring_t;

typedef struct
{
  uint32_t last_sequence;
  uint64_t baseline_ticks;
  uint64_t latest_event_ticks;
  uint8_t baseline_edge;
  bool require_alternating_edges;
  bool baseline_valid;
} port_tim_capture_state_t;

typedef struct
{
  uint64_t timestamp_ticks;
  uint64_t period_ticks;
  uint8_t edge;
} port_tim_capture_sample_t;

bool port_tim_calc_period(uint32_t timer_clock_hz, uint8_t counter_bits,
                          uint32_t period_us,
                          port_tim_calc_result_t *result);
bool port_tim_calc_frequency(uint32_t timer_clock_hz, uint8_t counter_bits,
                             uint32_t frequency_hz,
                             port_tim_calc_result_t *result);
uint32_t port_tim_pwm_ccr(uint32_t auto_reload, uint16_t duty_permille);
bool port_tim_capture_ring_push_isr(
    port_tim_capture_ring_t *ring,
    const port_tim_capture_raw_event_t *event);
bool port_tim_capture_ring_pop(port_tim_capture_ring_t *ring,
                               port_tim_capture_raw_event_t *event);
uint32_t port_tim_capture_delta(uint32_t previous, uint32_t current,
                                uint8_t counter_bits);
uint64_t port_tim_capture_extend(uint32_t update_epoch,
                                 uint32_t raw_counter,
                                 bool update_pending,
                                 uint8_t counter_bits);
bool port_tim_capture_process_event(
    const port_tim_capture_raw_event_t *event,
    uint8_t counter_bits,
    port_tim_capture_state_t *state,
    port_tim_capture_sample_t *sample,
    uint32_t *fault_count);
bool port_tim_claim_allowed(port_tim_owner_t current_owner,
                            port_tim_owner_t requested_owner,
                            uint32_t requested_capability);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_TIM_SEMANTIC_H__ */
