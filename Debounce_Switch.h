/**
 * @file Debounce_Switch.h
 * @brief Software debouncing interface for mechanical switches
 * 
 * This header defines the switch object structure and debouncing
 * interface for handling mechanical switch bounce.
 */

#ifndef DEBOUNCE_SWITCH_H_
#define DEBOUNCE_SWITCH_H_

#include <stdint.h>

/**
 * @brief Debounce time in milliseconds
 * 
 * Typical mechanical switches require 10-20ms debounce time.
 * This value can be adjusted based on switch characteristics.
 */
#define DEBOUNCE_TIME_MS 20U

/**
 * @brief Switch object structure
 * 
 * Encapsulates all data needed for debouncing a single switch:
 * - Read function to get raw hardware state
 * - Timestamp for debounce timing
 * - Previous raw reading
 * - Current debounced state
 */
typedef struct {
    uint32_t lastDebounceTime;  /**< Timestamp of last state change */
    uint32_t lastReading;        /**< Last raw reading from hardware */
    uint8_t switchState;         /**< Current debounced state (0=pressed, 1=released) */
    uint8_t (*read)(void);       /**< Function pointer to read hardware */
} Switch_t;

/**
 * @brief Debounce a switch and return stable state
 * 
 * Implements a timer-based debouncing algorithm.
 * 
 * @param sw Pointer to switch object to debounce
 * @return Current debounced state (0=pressed, 1=released)
 * 
 * @note Should be called periodically (every 10-20ms)
 * @see DEBOUNCE_TIME_MS for debounce period
 */
uint32_t debounce_switch(Switch_t *sw);

#endif /* DEBOUNCE_SWITCH_H_ */