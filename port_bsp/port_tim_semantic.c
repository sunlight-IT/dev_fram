#include "port_tim_semantic.h"

#include <limits.h>
#include <stddef.h>

static bool port_tim_counter_limit(uint8_t counter_bits, uint64_t *limit)
{
  if (limit == NULL)
  {
    return false;
  }
  if (counter_bits == 16u)
  {
    *limit = 0x10000ull;
    return true;
  }
  if (counter_bits == 32u)
  {
    *limit = 0x100000000ull;
    return true;
  }
  return false;
}

static uint64_t port_tim_div_round(uint64_t numerator, uint64_t denominator)
{
  return (numerator / denominator) +
         (((numerator % denominator) * 2u >= denominator) ? 1u : 0u);
}

static uint64_t port_tim_abs_diff(uint64_t lhs, uint64_t rhs)
{
  return (lhs >= rhs) ? (lhs - rhs) : (rhs - lhs);
}

static uint64_t port_tim_period_error(uint64_t timer_ticks,
                                      uint64_t requested_cycles)
{
  uint64_t requested_ticks = requested_cycles / 1000000ull;
  uint64_t requested_remainder = requested_cycles % 1000000ull;
  uint64_t whole_tick_delta;

  if (timer_ticks <= requested_ticks)
  {
    whole_tick_delta = requested_ticks - timer_ticks;
    return (whole_tick_delta * 1000000ull) + requested_remainder;
  }
  whole_tick_delta = timer_ticks - requested_ticks;
  return (whole_tick_delta * 1000000ull) - requested_remainder;
}

static bool port_tim_fraction_less(uint64_t lhs_numerator,
                                   uint64_t lhs_denominator,
                                   uint64_t rhs_numerator,
                                   uint64_t rhs_denominator)
{
  bool reverse = false;

  while (true)
  {
    uint64_t lhs_integer = lhs_numerator / lhs_denominator;
    uint64_t rhs_integer = rhs_numerator / rhs_denominator;
    uint64_t lhs_remainder;
    uint64_t rhs_remainder;

    if (lhs_integer != rhs_integer)
    {
      return reverse ? (lhs_integer > rhs_integer) :
                       (lhs_integer < rhs_integer);
    }
    lhs_remainder = lhs_numerator % lhs_denominator;
    rhs_remainder = rhs_numerator % rhs_denominator;
    if ((lhs_remainder == 0u) || (rhs_remainder == 0u))
    {
      if (lhs_remainder == rhs_remainder)
      {
        return false;
      }
      return (lhs_remainder == 0u) ? !reverse : reverse;
    }
    lhs_numerator = lhs_denominator;
    lhs_denominator = lhs_remainder;
    rhs_numerator = rhs_denominator;
    rhs_denominator = rhs_remainder;
    reverse = !reverse;
  }
}

static bool port_tim_ticks_to_us(uint64_t timer_ticks,
                                 uint32_t timer_clock_hz,
                                 uint32_t *period_us)
{
  uint64_t seconds;
  uint64_t remainder;
  uint64_t result;

  if ((timer_clock_hz == 0u) || (period_us == NULL))
  {
    return false;
  }
  seconds = timer_ticks / timer_clock_hz;
  remainder = timer_ticks % timer_clock_hz;
  result = (seconds * 1000000ull) +
           port_tim_div_round(remainder * 1000000ull, timer_clock_hz);
  if (result > UINT32_MAX)
  {
    return false;
  }
  *period_us = (uint32_t)result;
  return true;
}

bool port_tim_calc_period(uint32_t timer_clock_hz, uint8_t counter_bits,
                          uint32_t period_us,
                          port_tim_calc_result_t *result)
{
  uint64_t counter_limit;
  uint64_t requested_cycles;
  uint64_t divisor;
  uint64_t counts;
  uint64_t total_ticks;
  uint64_t error;
  uint64_t best_error = UINT64_MAX;
  uint64_t best_divisor = 0u;
  uint64_t best_counts = 0u;

  if ((timer_clock_hz == 0u) || (period_us == 0u) || (result == NULL) ||
      !port_tim_counter_limit(counter_bits, &counter_limit))
  {
    return false;
  }
  requested_cycles = (uint64_t)timer_clock_hz * period_us;
  for (divisor = 1u; divisor <= 0x10000ull; divisor++)
  {
    counts = port_tim_div_round(requested_cycles,
                                1000000ull * divisor);
    if ((counts == 0u) || (counts > counter_limit))
    {
      continue;
    }
    total_ticks = counts * divisor;
    error = port_tim_period_error(total_ticks, requested_cycles);
    if (error < best_error)
    {
      best_error = error;
      best_divisor = divisor;
      best_counts = counts;
    }
  }
  if ((best_divisor == 0u) ||
      !port_tim_ticks_to_us(best_counts * best_divisor, timer_clock_hz,
                            &result->actual_period_us))
  {
    return false;
  }
  result->prescaler = (uint16_t)(best_divisor - 1u);
  result->auto_reload = (uint32_t)(best_counts - 1u);
  result->counter_hz = timer_clock_hz / (uint32_t)best_divisor;
  result->actual_frequency_hz = (uint32_t)port_tim_div_round(
      timer_clock_hz, best_counts * best_divisor);
  result->quantized = (best_error != 0u);
  return true;
}

bool port_tim_calc_frequency(uint32_t timer_clock_hz, uint8_t counter_bits,
                             uint32_t frequency_hz,
                             port_tim_calc_result_t *result)
{
  uint64_t counter_limit;
  uint64_t divisor;
  uint64_t denominator;
  uint64_t counts;
  uint64_t total_divisor;
  uint64_t error;
  uint64_t best_error = 0u;
  uint64_t best_total_divisor = 1u;
  uint64_t best_divisor = 0u;
  uint64_t best_counts = 0u;

  if ((timer_clock_hz == 0u) || (frequency_hz == 0u) ||
      (frequency_hz > timer_clock_hz) || (result == NULL) ||
      !port_tim_counter_limit(counter_bits, &counter_limit))
  {
    return false;
  }
  for (divisor = 1u; divisor <= 0x10000ull; divisor++)
  {
    denominator = (uint64_t)frequency_hz * divisor;
    counts = port_tim_div_round(timer_clock_hz, denominator);
    if ((counts == 0u) || (counts > counter_limit))
    {
      continue;
    }
    total_divisor = counts * divisor;
    error = port_tim_abs_diff(timer_clock_hz,
                              (uint64_t)frequency_hz * total_divisor);
    if ((best_divisor == 0u) ||
        port_tim_fraction_less(error, total_divisor,
                               best_error, best_total_divisor))
    {
      best_error = error;
      best_total_divisor = total_divisor;
      best_divisor = divisor;
      best_counts = counts;
    }
  }
  if ((best_divisor == 0u) ||
      !port_tim_ticks_to_us(best_total_divisor, timer_clock_hz,
                            &result->actual_period_us))
  {
    return false;
  }
  result->prescaler = (uint16_t)(best_divisor - 1u);
  result->auto_reload = (uint32_t)(best_counts - 1u);
  result->counter_hz = timer_clock_hz / (uint32_t)best_divisor;
  result->actual_frequency_hz = (uint32_t)port_tim_div_round(
      timer_clock_hz, best_total_divisor);
  result->quantized = (best_error != 0u);
  return true;
}

uint32_t port_tim_pwm_ccr(uint32_t auto_reload, uint16_t duty_permille)
{
  uint64_t period_counts = (uint64_t)auto_reload + 1u;
  uint64_t pulse;

  if (duty_permille > 1000u)
  {
    duty_permille = 1000u;
  }
  pulse = ((period_counts * duty_permille) + 500u) / 1000u;
  if (pulse > auto_reload)
  {
    pulse = auto_reload;
  }
  return (uint32_t)pulse;
}

bool port_tim_capture_ring_push_isr(
    port_tim_capture_ring_t *ring,
    const port_tim_capture_raw_event_t *event)
{
  uint8_t next;

  if ((ring == NULL) || (event == NULL))
  {
    return false;
  }
  if (ring->count >= PORT_TIM_CAPTURE_RING_CAPACITY)
  {
    ring->dropped_count++;
    return false;
  }
  ring->events[ring->write_index] = *event;
  next = (uint8_t)(ring->write_index + 1u);
  ring->write_index = (next >= PORT_TIM_CAPTURE_RING_CAPACITY) ? 0u : next;
  ring->count++;
  return true;
}

bool port_tim_capture_ring_pop(port_tim_capture_ring_t *ring,
                               port_tim_capture_raw_event_t *event)
{
  uint8_t next;

  if ((ring == NULL) || (event == NULL) || (ring->count == 0u))
  {
    return false;
  }
  *event = ring->events[ring->read_index];
  next = (uint8_t)(ring->read_index + 1u);
  ring->read_index = (next >= PORT_TIM_CAPTURE_RING_CAPACITY) ? 0u : next;
  ring->count--;
  return true;
}

uint32_t port_tim_capture_delta(uint32_t previous, uint32_t current,
                                uint8_t counter_bits)
{
  if (counter_bits == 16u)
  {
    return (uint16_t)((uint16_t)current - (uint16_t)previous);
  }
  if (counter_bits == 32u)
  {
    return current - previous;
  }
  return 0u;
}

uint64_t port_tim_capture_extend(uint32_t update_epoch,
                                 uint32_t raw_counter,
                                 bool update_pending,
                                 uint8_t counter_bits)
{
  uint64_t modulus;
  uint64_t epoch = update_epoch;

  if (!port_tim_counter_limit(counter_bits, &modulus))
  {
    return 0u;
  }
  raw_counter &= (uint32_t)(modulus - 1u);
  if (update_pending && ((uint64_t)raw_counter < (modulus / 2u)))
  {
    epoch++;
  }
  return (epoch * modulus) + raw_counter;
}

bool port_tim_capture_process_event(
    const port_tim_capture_raw_event_t *event,
    uint8_t counter_bits,
    port_tim_capture_state_t *state,
    port_tim_capture_sample_t *sample,
    uint32_t *fault_count)
{
  uint64_t event_ticks;
  bool discontinuity;

  if ((event == NULL) || (state == NULL) || (sample == NULL) ||
      (fault_count == NULL) ||
      ((counter_bits != 16u) && (counter_bits != 32u)))
  {
    return false;
  }
  discontinuity = event->overcapture ||
                  ((state->last_sequence != 0u) &&
                   (event->sequence != (state->last_sequence + 1u)));
  if (discontinuity)
  {
    state->baseline_valid = false;
    (*fault_count)++;
  }
  state->last_sequence = event->sequence;
  event_ticks = port_tim_capture_extend(event->update_epoch,
                                        event->raw_counter,
                                        event->update_pending,
                                        counter_bits);
  state->latest_event_ticks = event_ticks;
  if (!state->baseline_valid)
  {
    state->baseline_ticks = event_ticks;
    state->baseline_edge = event->edge;
    state->baseline_valid = !event->overcapture;
    return false;
  }
  if (state->require_alternating_edges &&
      (event->edge == state->baseline_edge))
  {
    state->baseline_ticks = event_ticks;
    state->baseline_edge = event->edge;
    (*fault_count)++;
    return false;
  }
  sample->timestamp_ticks = event_ticks;
  sample->period_ticks = event_ticks - state->baseline_ticks;
  sample->edge = event->edge;
  state->baseline_ticks = event_ticks;
  state->baseline_edge = event->edge;
  return true;
}

bool port_tim_claim_allowed(port_tim_owner_t current_owner,
                            port_tim_owner_t requested_owner,
                            uint32_t requested_capability)
{
  uint32_t owner_capability;

  if ((requested_owner == USR_PORT_TIM_OWNER_SCHEDULER) ||
      (requested_owner == USR_PORT_TIM_OWNER_BASIC))
  {
    owner_capability = PORT_TIM_CAP_BASIC;
  }
  else if (requested_owner == USR_PORT_TIM_OWNER_CAPTURE)
  {
    owner_capability = PORT_TIM_CAP_CAPTURE;
  }
  else if (requested_owner == USR_PORT_TIM_OWNER_PWM)
  {
    owner_capability = PORT_TIM_CAP_PWM;
  }
  else
  {
    return false;
  }
  return ((current_owner == USR_PORT_TIM_OWNER_NONE) ||
          (current_owner == requested_owner)) &&
         ((requested_capability & owner_capability) != 0u) &&
         ((requested_capability & ~owner_capability) == 0u);
}
