#ifndef ISR_H
#define ISR_H

void start_isr(void (*isr_func)(void));
void* timer_callback_thread(void* arg);

#endif // ISR_H