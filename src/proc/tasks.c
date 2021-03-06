#include <stdint.h>
#include <memory/mem.h>
#include <proc/irq_ctl.h>
#include <proc/tasks.h>

// Global variables
static task_t init_task = INIT_TASK_STRUCT;
uint32_t number_tasks = 1;
task_t *current_task = &init_task;
task_t *tasks[ MAX_TASKS ] = { &init_task, };


/*
 * Enable and disable preemption
 */
void preempt_enable() { current_task->can_preempt = 1; }
void preempt_disable(){ current_task->can_preempt = 0; }

/*
 * Called every timeslice, from the timer interrupt
 * If the current process still has lifetime or cannot be preempted
 * then do nothing, else schedule tasks with interrupts enabled
 */
void schedule_tick(){
	current_task->lifetime--;	// Decrease the current processes lifetime

	if( (current_task->lifetime > 0) || !(current_task->can_preempt) )	// If it still has life or cannot be switched
		return;		// Do nothing


	unmask_irq();	// Else, enable interrupts (we are inside an interrupt already)
	schedule();		// schedule the tasks
	mask_irq();		// disable interrupts again
}


/*
 * Schedule tasks, interrupts must be enabled here
 * Task state can be changed by an interrupt, so we need to listen for them
 */
void schedule(){
	current_task->lifetime = 0;	// Current process now has no lifetime as it is switching

	preempt_disable();			// Disable preemption for the current task (we are already switching)
	int64_t next_task, next_task_lifetime;
	task_t *task;

	while(1){
		next_task_lifetime = -1;	// Set negative so it will be overridden
		next_task = 0;				// Default to task 0
		
		for(uint32_t i = 0; i < MAX_TASKS; i++){	// For all tasks...
			task = tasks[i];
			if( task && (task->state == TASK_STATE_RUNNING) && (task->lifetime > next_task_lifetime) ){
				// If the task exists, is running and its lifetime is larger than the current largest one
				// Set this task as the next one
				next_task_lifetime = task->lifetime;
				next_task = i;
			}
		}

		if(next_task_lifetime) break;	// If we have a task that has a nonzero lifetime, break out
										// If not...
		
		for(uint32_t i = 0; i < MAX_TASKS; i++){	// For all tasks...
			task = tasks[i];
			if(task){
				// Increase the task lifetime, making sure it does not exceed 2*priority
				task->lifetime = (task->lifetime >> 1) + task->priority;
			}
		}

	}
	
	switch_task(tasks[next_task]);	// Switch to the found task
	preempt_enable();				// Enable preemption for the new task
}


/*
 * Switches the currently running task
 * Main bulk is in an assembly function
 */
void switch_task(task_t *task){
	if(task == current_task) return;		// Don't switch to the currently running task

	task_t *prev = current_task;			// Keep track of the old task
	current_task = task;					// Current task is now the new task

	cpu_switch_task(prev, task);	// Do the switch
}


/*
 * Function to exit and destroy a task after compeltion
 */
void exit_process(){
	preempt_disable();							// Don't interrupt killing a task

	for(uint32_t i = 0; i < MAX_TASKS; i++){	// Loop through all tasks
		if( tasks[i] == current_task ){			// If we are current task
			tasks[i]->state = TASK_STATE_ZOMBIE;		// Set task state to zombie
			break;								// Exit for loop
		}
	}
	if( current_task->stack )					// If we have a stack
		free_page(current_task->stack);			// Free it
	
	preempt_enable();							// Can interrupt me now
	schedule();									// Find another task to run
}
