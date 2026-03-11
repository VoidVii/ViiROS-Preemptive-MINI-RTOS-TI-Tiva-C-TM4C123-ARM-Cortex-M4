/**
 * @file GPIO.h
 * @brief GPIO hardware abstraction layer interface
 * 
 * This header defines the interface for GPIO operations on the
 * TM4C123GH6PM microcontroller using AHB for faster access.
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>

/*==============================================================================
 *                                REGISTER ADDRESSES
 *==============================================================================*/

/** @name GPIO Base Addresses
 *  @brief AHB base addresses for GPIO ports
 *  @{
 */
#define GPIO_AHB_BASE           0x40058000U  /**< Port A base address */
/** @} */

/** @name Register Offsets
 *  @brief Offsets from port base address
 *  @{
 */
#define GPIO_DIR_OFFSET         0x400U       /**< Direction register */
#define GPIO_DEN_OFFSET         0x51CU       /**< Digital enable register */
#define GPIO_PUR_OFFSET         0x510U       /**< Pull-up resistor register */
#define GPIO_LOCK_OFFSET        0x520U       /**< Lock register */
#define GPIO_CR_OFFSET          0x524U       /**< Commit register */
#define GPIO_DATA_OFFSET        0x000U       /**< Data register */
/** @} */

/*==============================================================================
 *                                PORT DEFINITIONS
 *==============================================================================*/

/** @name Port Numbers
 *  @brief Port identifiers for GPIO functions
 *  @{
 */
#define GPIO_PORTA 0U  /**< Port A */
#define GPIO_PORTB 1U  /**< Port B */
#define GPIO_PORTC 2U  /**< Port C */
#define GPIO_PORTD 3U  /**< Port D */
#define GPIO_PORTE 4U  /**< Port E */
#define GPIO_PORTF 5U  /**< Port F */
/** @} */

/*==============================================================================
 *                                FUNCTION PROTOTYPES
 *==============================================================================*/

/**
 * @brief Enable clock and AHB for a GPIO port
 * @param port Port number (GPIO_PORTA - GPIO_PORTF)
 * @note Must be called before any other GPIO functions for that port
 */
void GPIO_EnablePort(uint8_t port);

/**
 * @brief Configure a pin as digital output
 * @param port Port number
 * @param pin Pin number (0-7)
 * @note Pin must be enabled first with GPIO_EnablePort()
 */
void GPIO_ConfigureOutput(uint8_t port, uint32_t pin);

/**
 * @brief Configure a pin as digital input with pull-up
 * @param port Port number
 * @param pin Pin number (0-7)
 * @note For protected pins (like PF0), unlock sequence is handled
 */
void GPIO_ConfigureInput(uint8_t port, uint32_t pin);

/**
 * @brief Write a digital value to a pin
 * @param port Port number
 * @param pin Pin number (0-7)
 * @param value 1 = HIGH, 0 = LOW
 * @note Pin must be configured as output first
 */
void GPIO_WritePin(uint8_t port, uint32_t pin, uint8_t value);

/**
 * @brief Read a digital value from a pin
 * @param port Port number
 * @param pin Pin number (0-7)
 * @return 1 = HIGH, 0 = LOW
 */
uint32_t GPIO_ReadPin(uint8_t port, uint32_t pin);

#endif /* GPIO_H_ */