#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H


#include <stdint.h>

struct state {
    void (*init_task)(void);

    void (*task)(void);

    void (*interrupt_task)(void);
};

typedef struct state state_t;

extern state_t *state_machine_current_state;

extern volatile int state_machine_interrupt_flag;


void state_machine_init(void);

void state_machine_state_null(void);

void state_machine_start_timer(uint32_t timeout_ms);

void state_machine_worker(void);

void state_machine_next_state(state_t *next_state);


#endif //STATE_MACHINE_H
