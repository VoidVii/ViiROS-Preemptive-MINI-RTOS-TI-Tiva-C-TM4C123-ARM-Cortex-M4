/**
 * @file GPIO.c
 * @brief GPIO (General Purpose Input/Output) hardware abstraction layer
 * 
 * This module provides low-level access to the GPIO peripherals on the
 * TM4C123GH6PM microcontroller. It handles port enabling, pin configuration,
 * and digital read/write operations using the AHB (Advanced High-performance
 * Bus) for faster access.
 */

#include "GPIO.h"
#include "TM4C123GH6PM.h"

/**
 * @brief Enable clock and AHB for a GPIO port
 * 
 * This function performs three essential steps for GPIO operation:
 * 1. Enables AHB (Advanced High-performance Bus) for faster GPIO access
 * 2. Enables the clock for the specified port (without clock, GPIO won't work)
 * 3. Waits for the port to be ready (peripheral ready flag)
 * 
 * @param port Port number (GPIO_PORTA through GPIO_PORTF)
 * 
 * @note Must be called before any other GPIO operations on that port
 * @note Without clock enable, register writes will have no effect
 * @see GPIO_AHB_BASE definition in GPIO.h for AHB base addresses
 */
void GPIO_EnablePort(uint8_t port)
{
    /*
     * Activate AHB (Advanced High-performance Bus)
     * GPIO High-Performance Bus Control (GPIOHBCTL)
     * Base: 0x400FE000, Offset: 0x06C
     * 
     * Enabling AHB allows faster access to GPIO registers via the
     * AHB bus instead of the slower APB bus.
     */
  
    //SYSCTL_GPIOHBCTL_R |= (1U << port);
    SYSCTL->GPIOHBCTL |= (1U << port);
    
    /*
     * Enable clock for the specified GPIO port
     * RCGCGPIO = Run Mode Clock Gating Control for GPIO
     * Base: 0x400FE000, Offset: 0x608
     * 
     * Each peripheral needs its clock enabled before use.
     * Without clock, register writes are ignored.
     */
    
    //SYSCTL_RCGCGPIO_R |= (1U << port);
    SYSCTL->RCGCGPIO |= (1U << port);
    
    /*
     * Wait for the port to be ready
     * PRGPIO = Peripheral Ready GPIO
     * Base: 0x400FE000, Offset: 0xA08
     * 
     * After enabling clock, we must wait for the peripheral
     * to become ready before accessing its registers.
     */
    
    while ((SYSCTL->PRGPIO & (1U << port)) == 0U) {}
}

/**
 * @brief Configure a GPIO pin as digital output
 * 
 * Sets up a pin for digital output operation by:
 * 1. Configuring the pin direction as output (GPIODIR)
 * 2. Enabling digital functionality (GPIODEN)
 * 
 * @param port Port number (GPIO_PORTA through GPIO_PORTF)
 * @param pin  Pin number (0-7) to configure as output
 * 
 * @note GPIO_EnablePort() must be called first for this port
 * @note Pins default to input after reset
 * 
 * @see GPIO_ConfigureInput() for input configuration
 */
void GPIO_ConfigureOutput(uint8_t port, uint32_t pin)
{
    volatile uint32_t* dir;  /* Pointer to GPIODIR register */
    volatile uint32_t* den;  /* Pointer to GPIODEN register */

    /*
     * Calculate register addresses
     * 
     * GPIO Port A (AHB) base: 0x4005.8000
     * GPIO Port B (AHB) base: 0x4005.9000
     * GPIO Port C (AHB) base: 0x4005.A000
     * GPIO Port D (AHB) base: 0x4005.B000
     * GPIO Port E (AHB) base: 0x4005.C000
     * GPIO Port F (AHB) base: 0x4005.D000
     * 
     * Each GPIO port occupies 0x1000 bytes (4 KB) of address space.
     * Register offsets are added to the base address.
     */
    dir = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_DIR_OFFSET);

    den = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_DEN_OFFSET);

    /* Configure pin as output (1 = output) */
    *dir |= (1U << pin);
    
    /* Enable digital functionality for this pin */
    *den |= (1U << pin);
}

/**
 * @brief Configure a GPIO pin as digital input with pull-up
 * 
 * Sets up a pin for digital input operation with internal pull-up resistor.
 * This is typically used for switches and buttons.
 * 
 * The configuration steps are:
 * 1. Unlock the pin (if necessary, e.g., for PF0)
 * 2. Commit the configuration (GPIOCR)
 * 3. Enable pull-up resistor (GPIOPUR)
 * 4. Set direction as input (GPIODIR)
 * 5. Enable digital functionality (GPIODEN)
 * 
 * @param port Port number (GPIO_PORTA through GPIO_PORTF)
 * @param pin  Pin number (0-7) to configure as input
 * 
 * @note Some pins (like PF0) are locked by default and need unlocking
 * @note Pull-up is enabled for use with switches (active low)
 * @warning For PF0, the unlock sequence is critical - see datasheet
 * 
 * @see GPIO_ConfigureOutput() for output configuration
 */
void GPIO_ConfigureInput(uint8_t port, uint32_t pin)
{
    volatile uint32_t* dir;   /* Pointer to GPIODIR register */
    volatile uint32_t* den;   /* Pointer to GPIODEN register */
    volatile uint32_t* lock;  /* Pointer to GPIOLOCK register */
    volatile uint32_t* cr;    /* Pointer to GPIOCR (Commit) register */
    volatile uint32_t* pur;   /* Pointer to GPIOPUR (Pull-Up) register */

    /*
     * Calculate register addresses
     * Protected pins (like PF0) require unlocking before configuration
     */
    lock = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_LOCK_OFFSET);

    cr = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_CR_OFFSET);

    dir = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_DIR_OFFSET);

    den = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_DEN_OFFSET);
    
    pur = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000U)) + GPIO_PUR_OFFSET);

    /*
     * Unlock the pin if it's protected
     * The GPIOLOCK register enables write access to GPIOCR.
     * Writing the magic key 0x4C4F434B ("LOCK") unlocks it.
     */
    *lock = 0x4C4F434BU;
    *cr |= (1U << pin);        /* Allow configuration changes */
    
    /* Enable internal pull-up resistor */
    *pur |= (1U << pin);
    
    /* Configure pin as input (0 = input) */
    *dir &= ~(1U << pin);
    
    /* Enable digital functionality */
    *den |= (1U << pin);
}

/**
 * @brief Write a digital value to a GPIO pin
 * 
 * Sets the output state of a GPIO pin to HIGH or LOW.
 * Uses the GPIO_DATA register with bit-specific addressing for
 * atomic pin access.
 * 
 * @param port  Port number (GPIO_PORTA through GPIO_PORTF)
 * @param pin   Pin number (0-7) to write to
 * @param value 1 = HIGH, 0 = LOW
 * 
 * @note Pin must be configured as output first
 * @note Uses bit-specific addressing for atomic operations
 * 
 * @see GPIO_ConfigureOutput() to set up the pin first
 */
void GPIO_WritePin(uint8_t port, uint32_t pin, uint8_t value)
{
    volatile uint32_t* data;
    
    /*
     * Calculate address using bit-specific addressing
     * The formula: base + DATA_OFFSET + ((1<<pin) << 2)
     * This creates a unique address for each pin combination,
     * allowing atomic read-modify-write operations.
     */
    data = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000)) + GPIO_DATA_OFFSET + ((1U << pin) << 2U));
      
    if(value)
    {
        *data = (1U << pin);   /* Set pin HIGH */
    }
    else
    {
        *data = 0U;             /* Set pin LOW */
    }
}

/**
 * @brief Read the digital value from a GPIO pin
 * 
 * Reads the current state of a GPIO pin.
 * Uses the same bit-specific addressing as GPIO_WritePin().
 * 
 * @param port Port number (GPIO_PORTA through GPIO_PORTF)
 * @param pin  Pin number (0-7) to read from
 * 
 * @return 1 if pin is HIGH, 0 if pin is LOW
 * 
 * @note Pin should be configured as input for meaningful results
 * @note For output pins, returns the value currently being driven
 * 
 * @see GPIO_ConfigureInput() to set up the pin first
 */
uint32_t GPIO_ReadPin(uint8_t port, uint32_t pin)
{
    volatile uint32_t* data;
    
    /*
     * Calculate address using bit-specific addressing
     * Same addressing scheme as GPIO_WritePin()
     */
    data = (volatile uint32_t*)
        ((GPIO_AHB_BASE + (port * 0x1000)) + GPIO_DATA_OFFSET + ((1U << pin) << 2U));
      
    /* Read the register and return 1 if any bit is set */
    return ((*data) ? 1U : 0U);
}