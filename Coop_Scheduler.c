/**
 * @file Coop_Scheduler.c
 * @brief Implementation of a cooperative scheduler for task management
 * 
 * This scheduler runs tasks periodically based on their defined periods.
 * It's called "cooperative" because tasks must voluntarily yield control
 * by returning (no preemption).
 */

#include "Coop_Scheduler.h"

/**
 * @brief Update and run pending tasks
 * 
 * This function checks each task in the task array to see if its period
 * has elapsed since its last run. If so, the task function is executed
 * and its lastRun timestamp is updated.
 * 
 * @param task[]    Array of Task_t structures containing task definitions
 * @param numTasks  Number of tasks in the array (calculated with sizeof)
 * @param now       Current system time in milliseconds (from SysTick)
 * 
 * @note This function should be called continuously in the main super loop
 * @note Uses 32-bit timestamps which wrap safely due to unsigned arithmetic
 * 
 * @warning All task functions must be non-blocking and return quickly
 * @warning Task periods should be multiples of the SysTick period (1ms)
 */
void Task_update(Task_t task[], uint32_t const numTasks, uint32_t now){
    
    // Iterate through all tasks in the array
    for(uint8_t i = 0; i < numTasks; i++)
    {  
        // Check if it's time to run this task
        // Unsigned arithmetic handles timer overflow automatically
        // Example: if now wraps from 0xFFFFFFFF to 0, the subtraction still works!
        if((now - task[i].lastRun) >= task[i].period)
        {
            // Run the task function (via function pointer)
            task[i].sched();
            
            // Update last run time for next period
            // Adding period instead of setting to 'now' prevents drift
            task[i].lastRun += task[i].period;
        }
    }
}