/**
 * @file Coop_Scheduler.h
 * @brief Cooperative scheduler interface for task management
 * 
 * This header defines the task structure and scheduler interface
 * for a simple cooperative multitasking system.
 */

#ifndef COOP_SCHEDULER_H_
#define COOP_SCHEDULER_H_

#include <stdint.h>

/**
 * @brief Task structure for scheduler
 * 
 * Each task has a period, last run time, and a function to execute.
 * Tasks are executed periodically when their period has elapsed.
 */
typedef struct {
    uint32_t period;      /**< Task period in milliseconds */
    uint32_t lastRun;     /**< Last execution timestamp */
    void (*sched)(void);  /**< Task function to execute */
} Task_t;

/**
 * @brief Update and execute pending tasks
 * 
 * Checks each task in the array and executes it if its period
 * has elapsed since the last run.
 * 
 * @param task[]    Array of tasks to manage
 * @param numTasks  Number of tasks in the array
 * @param now       Current system time in milliseconds
 * 
 * @note Must be called continuously in main super loop
 * @note All task functions must be non-blocking
 */
void Task_update(Task_t task[], uint32_t const numTasks, uint32_t now);

#endif /* COOP_SCHEDULER_H_ */