#ifndef VIIROS_H_
#define VIIROS_H_

/*============================================================================*/
/*                           Includes & Defines                               */
/*============================================================================*/
#include <stdint.h>


#define NULL 0

/*      
*       LOG2(x) = 32 - __CLZ(x)
*       ViiROS_readyMask = 0b0000 0000 0000 0000 0000 0000 0101 0010
*                            ^ 25 zeros before the first 1  ^
*
*       __CLZ(x) = 25 --> 32 - 25 = 7 highest bit and priority 
*       works for >> x != 0 << and is undefined for x = 0  
*/
#define LOG2(x) (32U - __CLZ(x))

/*============================================================================*/
/*                       Thread Data (TCB, Arrays, etc.)                      */
/*============================================================================*/

typedef void (*ViiROS_ThreadHandler)(void);

/**
*@brief Thread Control Block (TCB)
*@{
*/
typedef struct {
  void *sp; /**<thread stack pointer // top of the stack */
  uint8_t priority; /**<thread priority */
  uint32_t blocktime; /**<time thread spends in blocked state */
  /* space for more thread attributes like state, threadHandler and more */
}ViiROS_Thread;
/**
*@}
*/

/*============================================================================*/
/*                       Function Declarations                                */
/*============================================================================*/

/* Idle function  */
static void ViiROS_onIdle(void);

/* Callback: Kernel ready? >> further initialization before ViiROS_run */
void ViiROS_lastInit(void);

/* Give ViiROS control over system */
void ViiROS_Run(void);

/* Initialization */ 
void ViiROS_Init(void);

/* Scheduler */ 
void ViiROS_Scheduler(void);

/* BlockTime */
void ViiROS_BlockTime(uint32_t time);

/* Countwatch  */
void ViiROS_BlockWatch(void);


/* Thread start */
void ViiROS_ThreadStart(
                        ViiROS_Thread *me,
                        ViiROS_ThreadHandler thread_Handler,
                        uint8_t priority, 
                        void *stk_Storage, uint32_t stk_Size);


#endif // VIIROS_H_