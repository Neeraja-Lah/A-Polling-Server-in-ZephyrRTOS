#include <zephyr.h>
#include <sys/printk.h>
#include <kernel.h>
#include <stdbool.h>
#include <stdlib.h>
#include <timing/timing.h>
#include <sys/util.h>
#include <shell/shell_uart.h>
#include "task_model_p4.h"
#define STACK_SIZE  4096

// Flag value to set threads running
static bool running = true;

//Stack area 
static K_THREAD_STACK_DEFINE(thread_stack_area, STACK_SIZE * (NUM_THREADS+1));

// Thread structures and declarations 
static struct k_thread threads_data[NUM_THREADS];
static k_tid_t t_ids[NUM_THREADS];
static int thread_num[NUM_THREADS];
static int done[NUM_THREADS];
static struct k_sem com_sem[NUM_THREADS];
//timer to track left budget
static struct k_timer budget_check_timer; 
//aperiodic requests
extern struct k_timer req_timer;

//polling server replenishment timer
static struct k_timer ps_timer_thread; 
//Entry Function Declaration
static void periodic_entry_func(void *, void *, void *);

//Periodic thread for Polling Server
static k_tid_t ps_thread;
static struct k_thread my_thread_data;

void aperiodic_switched_in(){
    if (k_current_get()==ps_thread)
    k_timer_start(&budget_check_timer, K_NSEC(poll_info.left_budget), K_NO_WAIT);
}
void aperiodic_switched_out(){
    if (k_current_get()==ps_thread)
    {
        k_timer_stop(&budget_check_timer);
    }
}

//Periodic Timer Callback func
static void timer_expiry_function(struct k_timer *timer_exp)
{
    int id = *(int *)timer_exp->user_data;
    int ret;
    ret = done[id];  
    if (ret==1) {
        k_sem_give(&com_sem[id]);
    }
    else {
        k_sem_give(&com_sem[id]);
    }
}

// Entry Function for each thread in task set
static void periodic_entry_func(void *periodic_data, void *periodic_tid, void *unused)
{
    struct task_s *periodic_info = (struct task_s *)periodic_data;
    int thread_id = *(int *)periodic_tid;
	ARG_UNUSED(unused);
	uint32_t period;
    struct k_timer timer_thread;

    k_timer_init(&timer_thread, timer_expiry_function, NULL);
    timer_thread.user_data = periodic_tid;
	period = periodic_info->period; 
    k_timer_start(&timer_thread,K_MSEC(period), K_MSEC(period));

    while (running) {
        // Loop to emulate task execution
        done[thread_id]=0;
        volatile int t1 = periodic_info->loop_iter;
        looping(t1);
        done[thread_id]=1;
        k_sem_take(&com_sem[thread_id], K_FOREVER);
    }
}

//Timer callback for tracking left budget
static void budget_expiry_function(struct k_timer *budget_timer_exp){
    poll_info.left_budget = 14;

}


//Timer Callback for budget replenishment
static void ps_timer_expiry_function(struct k_timer *ps_timer_exp){
	poll_info.left_budget = 1000000*BUDGET;
	//k_thread_priority_set(ps_thread, 6);		
}

//Entry Function for Polling Server
static void ps_entryfunc(void *DUMMY1, void *DUMMY2, void *unused){
    struct req_type aperiodic;
    uint32_t period;
	 

	period = poll_info.period;
    k_timer_start(&ps_timer_thread,K_MSEC(period), K_MSEC(period));
	
 	// Loop to emulate task execution
	while (running) {
		if(!k_msgq_get(&req_msgq,&aperiodic,K_NO_WAIT))
		{
            k_timer_start(&budget_check_timer, K_NSEC(poll_info.left_budget), K_NO_WAIT);
			int task = aperiodic.iterations;
			looping(task);
		}
		else {			
			poll_info.left_budget = 0;		
		}       
	}
	k_timer_stop(&ps_timer_thread);
}

//Threads begin to execute
static void create_threads(void)
{
	//Semaphore Initialisation
    for (int i = 0; i < NUM_THREADS; i++) {
        k_sem_init(&com_sem[i], 0, 1);
        done[i]=0;
    }

    // Thread Creation
    for (int i = 0; i < NUM_THREADS; i++) {
		thread_num[i]=i;
        t_ids[i] = k_thread_create(&threads_data[i], &thread_stack_area[STACK_SIZE * i],STACK_SIZE * sizeof(k_thread_stack_t),
                                         periodic_entry_func, (void *)&threads[i],(void *)&thread_num[i], NULL, threads[i].priority,0, K_MSEC(10));

        k_thread_name_set(t_ids[i], threads[i].t_name);
    }
    k_timer_start(&req_timer, K_USEC(ARR_TIME), K_NO_WAIT); 
    ps_thread = k_thread_create(&my_thread_data, &thread_stack_area[STACK_SIZE*NUM_THREADS],STACK_SIZE * sizeof(k_thread_stack_t),
                                         ps_entryfunc, NULL, NULL, NULL, 14,0, K_MSEC(10));
    k_thread_name_set(ps_thread, poll_info.t_name);
    k_timer_init(&ps_timer_thread, ps_timer_expiry_function, NULL);
}

void main(void)
{
    //Spawning threads
    create_threads();
    k_timer_init(&budget_check_timer, budget_expiry_function, NULL);
    
    k_sleep(K_MSEC(TOTAL_TIME));

    running = false;

    for (int i = 0; i < NUM_THREADS; ++i) {
        k_sem_give(&com_sem[i]);
    }
    
    //wait for threads to exit
	int j;
    for (j = 0; j < NUM_THREADS; ++j) {
        k_thread_join(&threads_data[j],K_FOREVER);
    }
}

