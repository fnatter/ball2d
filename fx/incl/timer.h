#ifndef __TIMER_H
#define __TIMER_H

#define PIT0defaultfreq         18.2067597
#define PIT_CHANNEL0_PORT       0x40
#define PIT_CHANNEL1_PORT       0x41
#define PIT_CHANNEL2_PORT       0x42
#define PIT_CONTROL_PORT        0x43
#define PIT_FREQUENCY           0x001234DD // clocks per second
#define IRQ0_INT                0x08
#define PIT_CONTROL_SETTINGS    0x43
#define PIT_SETTINGS_RESET      0x36



volatile unsigned long Timercount;
float TPS = PIT0defaultfreq; // TicksPerSecond
bool TimerHandlerInstalled = false;


void setfrequency(unsigned short freq);
void installtimerhandler(unsigned short freq = 0);
void restoretimerhandler();

#endif
