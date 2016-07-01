//#define TEST_TIMER

#include "timer.h"
#include <dpmi.h>

extern volatile unsigned long Timercount;
extern float TPS;
static _go32_dpmi_seginfo old_handler_rm, new_handler_rm, old_handler_pm, new_handler_pm;
static unsigned long ClockTicks;
static unsigned long ClocksPerTick;
static _go32_dpmi_registers regs;
static void rmhandler();
static void pmhandler();

void setfrequency(unsigned short freq)
{
  unsigned short counter = PIT_FREQUENCY / freq;
  TPS = (float)freq;
  ClocksPerTick = freq;
  outportb(PIT_CONTROL_PORT, PIT_CONTROL_SETTINGS);
  outportb(PIT_CHANNEL0_PORT, counter & 0x00FF);
  outportb(PIT_CHANNEL0_PORT, counter >> 8);
};

void installtimerhandler(unsigned short freq)
{
  if (!TimerHandlerInstalled)
    {
      disable();

      _go32_dpmi_get_protected_mode_interrupt_vector(IRQ0_INT, &old_handler_pm);
      new_handler_pm.pm_selector = _my_cs();
      new_handler_pm.pm_offset = (int)pmhandler;
      _go32_dpmi_chain_protected_mode_interrupt_vector(IRQ0_INT, &new_handler_pm);
      _go32_dpmi_get_real_mode_interrupt_vector(IRQ0_INT, &old_handler_rm);
      new_handler_rm.pm_selector = _my_cs();
      new_handler_rm.pm_offset = (int)rmhandler;
      __dpmi_regs regs;
      _go32_dpmi_allocate_real_mode_callback_iret(&new_handler_rm, &regs);
      _go32_dpmi_set_real_mode_interrupt_vector(IRQ0_INT, &new_handler_rm);
      if (freq > 0x12)
	setfrequency(freq);
      ClockTicks = 0;
      Timercount = 0;
      enable();
      TimerHandlerInstalled = true;
    }
};

void restoretimerhandler()
{
  if (TimerHandlerInstalled)
    {
      disable();
      outportb(PIT_CONTROL_PORT, PIT_SETTINGS_RESET);
      outportb(PIT_CHANNEL0_PORT, 0x00);
      outportb(PIT_CHANNEL0_PORT, 0x00);
      _go32_dpmi_set_real_mode_interrupt_vector(IRQ0_INT, &old_handler_rm);
      _go32_dpmi_set_protected_mode_interrupt_vector(IRQ0_INT, &old_handler_pm);
      _go32_dpmi_free_real_mode_callback(&new_handler_rm);
      enable();
      TPS = PIT0defaultfreq;
      TimerHandlerInstalled = false;
    }
};

static void rmhandler()
{
  disable();
  Timercount++;
  ClockTicks += ClocksPerTick;
  if (ClockTicks >= 0x10000)
    {
      ClockTicks = 0;
      regs.x.cs = old_handler_rm.rm_segment;
      regs.x.ip = old_handler_rm.rm_offset;
      regs.x.ss = regs.x.sp = 0;
      enable();
      _go32_dpmi_simulate_fcall_iret(&regs);
    }
  else
    outportb(0x20, 0x20);
};

static void pmhandler()
{
  disable();
  Timercount++;
  ClockTicks += ClocksPerTick;
  if (ClockTicks >= 0x10000)
    {
      ClockTicks = 0;
      enable();
    }
  else
    outportb(0x20, 0x20);
};

#ifdef TEST_TIMER

#include <stdio.h>
#include <conio.h>

int main()
{
  installtimerhandler(100);
  while (!kbhit())
    {
      printf("\nTicks: %li\n", Timercount);
    }
  restoretimerhandler();
  return 0;
}

#endif
