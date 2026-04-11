#include "state_machine.h"

#include "em_cmu.h"
#include "em_timer.h"
volatile int state_machine_interrupt_flag = 0;
state_t *state_machine_current_state;

#define TIMER_PRESCALE timerPrescale1024


void state_machine_init() {
    CMU_ClockEnable(cmuClock_WTIMER0, true);

    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    timerInit.enable = false;
    timerInit.prescale = TIMER_PRESCALE;
    timerInit.oneShot = true; // Generate only one pulse
    TIMER_Init(WTIMER0, &timerInit);

    // Enable WTIMER0 interrupts
    TIMER_IntEnable(WTIMER0, WTIMER_IEN_OF);
    NVIC_EnableIRQ(WTIMER0_IRQn);
}

void state_machine_start_timer(uint32_t timeout_ms) {

    // Set the first compare value
    uint32_t compareValue1 = CMU_ClockFreqGet(cmuClock_WTIMER0)
                      * timeout_ms / 1000
                      / (1 << TIMER_PRESCALE);
    TIMER_TopSet(WTIMER0, compareValue1);


    // Enable the TIMER
    TIMER_Enable(WTIMER0, true);

    state_machine_interrupt_flag = 0;
}

void state_machine_worker() {
    state_machine_current_state->task();
}

void state_machine_next_state(state_t *next_state) {
    state_machine_current_state = next_state;
    state_machine_current_state->init_task();
}

void state_machine_state_null() {
};


void WTIMER0_IRQHandler(void)
{
    // Acknowledge the interrupt
    uint32_t flags = TIMER_IntGet(WTIMER0);
    TIMER_IntClear(WTIMER0, flags);

    state_machine_current_state->interrupt_task();
    state_machine_interrupt_flag = 1;
}
