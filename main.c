#include "TM4C123GH6PM.h"       // TI - MCU - CMSIS 
#include "system_TM4C123.h"     // TI - Board - CMSIS 
#include "core_cm4.h"           // CMSIS Header
#include "SysTick.h"
#include "GPIO.h"
#include "ViiROS.h"

/*============================================================================*/
/*                             Thread Data                                    */
/*============================================================================*/

ViiROS_Thread Red;
static uint32_t stack_Red[80];

void Red_t(void)
{
  while(1){
    GPIO_WritePin(GPIO_PORTF, RED_LED, 1U);
    ViiROS_BlockTime(50U);
    
    GPIO_WritePin(GPIO_PORTF, RED_LED, 0U);
    ViiROS_BlockTime(50U);
  }
}

ViiROS_Thread Blue;
static uint32_t stack_Blue[80];

void Blue_t(void)
{
  while(1){
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, 1U);
    ViiROS_BlockTime(50U);
    
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, 0U);
    ViiROS_BlockTime(50U);
    
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, 1U);
    ViiROS_BlockTime(250U);
    
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, 0U);
    ViiROS_BlockTime(350U);
  }
}

ViiROS_Thread Green;
static uint32_t stack_Green[80];

/**
*@brief Toogle green LED via Switch 1 (debounce / edge detection)
*@note  20 ms for switch debouncing 
*@note  Edge detection via static state variable
*@warning Switch pressed: Active switch state (1: active high, 0: active low)
*/
void Green_t(void)
{
  static uint8_t lastStateSwitch = 1U; /**< save last rounds switch state */
  
  while(1)
  { 
    /** Falling edge detection: SW = 1 -> SW = 0 (active LOW) */
    if(GPIO_ReadPin(GPIO_PORTF, Switch_1) == 0U && lastStateSwitch == 1U)
    { 
      ViiROS_BlockTime(20U);/**< debounce time for switch */
      /** Check: after 20 ms still pressed? */
      if(GPIO_ReadPin(GPIO_PORTF, Switch_1) == 0U && lastStateSwitch == 1U)
      {
        /** Toogle LED */
        if(GPIO_ReadPin(GPIO_PORTF, GREEN_LED) == OFF)
        {
          GPIO_WritePin(GPIO_PORTF, GREEN_LED, ON); 
        }
        else
        {
          GPIO_WritePin(GPIO_PORTF, GREEN_LED, OFF);
        }
        /* Save last switch state for the next execution */
        lastStateSwitch = 0U;
      }
    }
    /** No events */
    if(GPIO_ReadPin(GPIO_PORTF, Switch_1) == 1U)
    {
      
      if(lastStateSwitch == 0U) /**< change switch state if nacessary  */
      {
        lastStateSwitch = 1U;
      }
      
      ViiROS_BlockTime(18U); /**< block thread to free the CPU */
    }
  }
}


/*============================================================================*/
/*                               Main                                         */
/*============================================================================*/

int main()
{
    /* Update the System Clock */
    SystemCoreClockUpdate(); 
    
    /* defensive initialization by disabling interrupts globally */
    __disable_irq();
    
    /* SysTick - Initialization */
    SysTick_Init();
    
    /* GPIO - Initialization */
    GPIO_EnablePort(GPIO_PORTF); /* Enable clock for Port F */
    
    GPIO_ConfigureInput(GPIO_PORTF, Switch_1);
    
    GPIO_ConfigureOutput(GPIO_PORTF, RED_LED);    /* Red LED pin as output */
    GPIO_ConfigureOutput(GPIO_PORTF, BLUE_LED);  /* Blue LED pin as output */
    GPIO_ConfigureOutput(GPIO_PORTF, GREEN_LED);  /* Green LED pin as output */
    
    
    GPIO_WritePin(GPIO_PORTF, RED_LED, OFF);  /* defined state == 0 */
    GPIO_WritePin(GPIO_PORTF, BLUE_LED, OFF);  /* defined state == 0 */
    GPIO_WritePin(GPIO_PORTF, GREEN_LED, OFF); /* defined state == 0 */
    
    /* ViiROS - Initialization */
    ViiROS_Init();
    
    ViiROS_ThreadStart(&Red,
                       Red_t,
                       9U,
                       stack_Red, sizeof(stack_Red));
    
    ViiROS_ThreadStart(&Blue,
                       Blue_t,
                       8U,
                       stack_Blue, sizeof(stack_Blue));
    
    ViiROS_ThreadStart(&Green,
                       Green_t,
                       7U,
                       stack_Green, sizeof(stack_Green));

    /* enable interrups after initialiaztion */
    __enable_irq();
    
    /* give up controll to ViiROS -> MSP --> PSP */
    ViiROS_Run();

}