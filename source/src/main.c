/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include "common.h"
#include "volatile_mem.h"

// Global variable for delayed save state loading
int delayed_load_save_state = 0;


// Main Thread Params
#define PRIORITY       (32)
#define STACK_SIZE_KB  (512)
#define ATTRIBUTE      (THREAD_ATTR_USER | PSP_THREAD_ATTR_CLEAR_STACK)

PSP_MODULE_INFO("PSP TempGBA", PSP_MODULE_USER, 0, 9);
PSP_MAIN_THREAD_PARAMS(PRIORITY, STACK_SIZE_KB, ATTRIBUTE);
PSP_HEAP_SIZE_KB(-800);


u32 option_screen_scale = SCALED_USER;
u32 option_screen_mag = 170;
u32 option_screen_filter = FILTER_BILINEAR;
u32 option_sound_volume = 10;
u32 option_stack_optimize = 1;
u32 option_boot_mode = 0;
u32 option_update_backup = 0;
u32 option_screen_capture_format = 0;
u32 option_enable_analog = 0;
u32 option_analog_sensitivity = 4;
u32 option_language = 0;
u32 option_button_swap = 0;

u32 option_aspect_ratio = 0;
u32 option_overlay_enabled = 0;
u32 option_overlay_selected = 0;
u32 option_overlay_offset_x = 0;
u32 option_overlay_offset_y = 0;
u32 option_color_correction = 0;
u32 option_button_mapping = 0;
u32 option_resume_on_boot = 0;
u32 option_auto_save_state = 0;
u32 fast_forward_speed = 2;
u32 option_me_engine = 0;
u32 layer_merge_enabled = 0;

u32 option_frameskip_type = FRAMESKIP_AUTO;
u32 option_frameskip_value = 9;
u32 option_clock_speed = PSP_CLOCK_333;

char main_path[MAX_PATH];

int date_format = 0;
u32 enable_home_menu = 1;

u32 sleep_flag = 0;

u32 synchronize_flag = 1;
u32 psp_fps_debug = 0;

u32 real_frame_count = 0;
u32 virtual_frame_count = 0;


typedef enum
{
  TIMER_INACTIVE,
  TIMER_PRESCALE,
  TIMER_CASCADE
} TIMER_STATUS_TYPE;

typedef enum
{
  TIMER_NO_IRQ,
  TIMER_TRIGGER_IRQ
} TIMER_IRQ_TYPE;

typedef enum
{
  TIMER_DS_CHANNEL_NONE,
  TIMER_DS_CHANNEL_A,
  TIMER_DS_CHANNEL_B,
  TIMER_DS_CHANNEL_BOTH
} TIMER_DS_CHANNEL_TYPE;

typedef struct
{
  TIMER_DS_CHANNEL_TYPE direct_sound_channels;
  TIMER_IRQ_TYPE irq;
  TIMER_STATUS_TYPE status;
  FIXED08_24 frequency_step;
  s32 count;
  u32 reload;
  u32 reload_update;
  u32 prescale;
} TimerType;

const u8 ALIGN_DATA prescale_table[] = { 0, 6, 8, 10 };
TimerType ALIGN_DATA timer[4];

u32 dma_cycle_count = 0;

u32 cpu_ticks = 0;
s32 video_count = 272;

u32 irq_ticks = 0;
u8 cpu_init_state = 0;

u32 skip_next_frame = 0;
u32 num_skipped_frames = 0;
u32 frameskip_counter = 0;
u32 interval_skipped_frames = 0;
u32 frames = 0;
u32 vblank_count = 0;

static u8 caches_inited = 0;

static void cpu_interrupt(void);

int main(int argc, char *argv[]);
int user_main(int argc, char *argv[]);

static void init_main(void);
static void setup_main(void);
static void load_setting_cfg(void);
static void load_bios_file(void);
static int load_ku_bridge_prx(int devkit_version);

static void setup_callbacks(void);
static int exit_callback(int arg1, int arg2, void *common);
static int power_callback(int unknown, int powerInfo, void *arg);
static int callback_thread(SceSize args, void *argp);

static void vblank_interrupt_handler(u32 sub, u32 *parg);

static void synchronize(void);
static void psp_sleep_loop(void);

#define scePowerSetClockFrequency371  scePower_EBD177D6
static int (*__scePowerSetClockFrequency)(int pllfreq, int cpufreq, int busfreq);


#define SOUND_UPDATE_FREQUENCY_STEP                                           \
  FLOAT_TO_FP08_24((16777216.0 / SOUND_FREQUENCY) / timer_reload)             \

#define CHECK_COUNT(count_var)                                                \
  if (count_var < reg[EXECUTE_CYCLES])                                        \
  {                                                                           \
    reg[EXECUTE_CYCLES] = count_var;                                          \
  }                                                                           \

#define CHECK_TIMER(timer_number)                                             \
  if (timer[timer_number].status == TIMER_PRESCALE)                           \
  {                                                                           \
    CHECK_COUNT((u32)timer[timer_number].count);                              \
  }                                                                           \


void timer_control_low(u8 timer_number, u32 value)
{
  TimerType *tm = timer + timer_number;

  tm->reload = 0x10000 - value;
  tm->reload_update = 1;
}

CPU_ALERT_TYPE timer_control_high(u8 timer_number, u32 value)
{
  TimerType *tm = timer + timer_number;

  if ((value & 0x80) != 0)
  {
    if (tm->status == TIMER_INACTIVE)
    {
      if ((value & 0x04) != 0)
      {
        tm->status = TIMER_CASCADE;
        tm->prescale = 0;
      }
      else
      {
        tm->status = TIMER_PRESCALE;
        tm->prescale = prescale_table[value & 0x03];
      }

      tm->irq = (value >> 6) & 0x01;

      u32 timer_reload = tm->reload;

      pIO_REG(REG_TM0D + (timer_number << 1)) = 0x10000 - timer_reload;

      timer_reload <<= tm->prescale;
      tm->count = timer_reload;

      if (timer_number < 2)
      {
        tm->frequency_step = SOUND_UPDATE_FREQUENCY_STEP;
        tm->reload_update = 0;

        if ((tm->direct_sound_channels & 0x01) != 0)
          adjust_direct_sound_buffer(0, cpu_ticks + timer_reload);

        if ((tm->direct_sound_channels & 0x02) != 0)
          adjust_direct_sound_buffer(1, cpu_ticks + timer_reload);
      }

      return CPU_ALERT_TIMER;
    }
  }
  else
  {
    tm->status = TIMER_INACTIVE;
  }

  return CPU_ALERT_NONE;
}

void direct_sound_timer_select(u32 value)
{
  TIMER_DS_CHANNEL_TYPE timer_select;

  timer_select = ((value >> 13) & 0x02) | ((value >> 10) & 0x01);

  timer[0].direct_sound_channels = timer_select ^ 0x03;
  timer[1].direct_sound_channels = timer_select;
}


static void cpu_interrupt(void)
{
  // Interrupt handler in BIOS
  reg_mode[MODE_IRQ][6] = reg[REG_PC] + 4;
  spsr[MODE_IRQ] = reg[REG_CPSR];
  reg[REG_CPSR] = (reg[REG_CPSR] & ~0xFF) | 0x92; // set mode IRQ & disable IRQ
  reg[REG_PC] = 0x00000018;
  set_cpu_mode(MODE_IRQ);

  bios_read_protect = 0xe55ec002;

  reg[CPU_HALT_STATE] = CPU_ACTIVE;
  reg[CHANGED_PC_STATUS] = 1;
}

#define UPDATE_TIMER(timer_number)                                            \
{                                                                             \
  TimerType *tm = timer + timer_number;                                       \
                                                                              \
  if (tm->status != TIMER_INACTIVE)                                           \
  {                                                                           \
    if (tm->status != TIMER_CASCADE)                                          \
      tm->count -= reg[EXECUTE_CYCLES];                                       \
                                                                              \
    if (tm->count <= 0)                                                       \
    {                                                                         \
      if (tm->irq == TIMER_TRIGGER_IRQ)                                       \
      {                                                                       \
        irq_raised |= (IRQ_TIMER0 << timer_number);                           \
      }                                                                       \
                                                                              \
      if (timer_number != 3)                                                  \
      {                                                                       \
        if (tm[1].status == TIMER_CASCADE)                                    \
          tm[1].count--;                                                      \
      }                                                                       \
                                                                              \
      u32 timer_reload = tm->reload << tm->prescale;                          \
                                                                              \
      if (timer_number < 2)                                                   \
      {                                                                       \
        if ((tm->direct_sound_channels & 0x01) != 0)                          \
          sound_timer(tm->frequency_step, 0);                                 \
                                                                              \
        if ((tm->direct_sound_channels & 0x02) != 0)                          \
          sound_timer(tm->frequency_step, 1);                                 \
                                                                              \
        if (tm->reload_update != 0)                                           \
        {                                                                     \
          tm->frequency_step = SOUND_UPDATE_FREQUENCY_STEP;                   \
          tm->reload_update = 0;                                              \
        }                                                                     \
      }                                                                       \
                                                                              \
      tm->count += timer_reload;                                              \
    }                                                                         \
                                                                              \
    pIO_REG(REG_TM0D + (timer_number << 1)) = 0x10000 - (tm->count >> tm->prescale); \
  }                                                                           \
}                                                                             \

#define TRANSFER_HBLANK_DMA(channel)                                          \
  if (dma[channel].start_type == DMA_START_HBLANK)                            \
  {                                                                           \
    dma_transfer(dma + channel);                                              \
  }                                                                           \

#define TRANSFER_VBLANK_DMA(channel)                                          \
  if (dma[channel].start_type == DMA_START_VBLANK)                            \
  {                                                                           \
    dma_transfer(dma + channel);                                              \
  }                                                                           \

u32 update_gba(void)
{
  s32 i;
  IRQ_TYPE irq_raised = IRQ_NONE;
  static int frame_count = 0;

  do
  {
    reg[CPU_DMA_HACK] = 0;

    update_gba_loop:
    
    // Debug: Log more frames to track execution after workaround
    if (frame_count < 10) {
      FILE *debug_log = fopen("froglog.txt", "a");
      if (debug_log) {
        fprintf(debug_log, "UPDATE_GBA: Frame %d, PC=0x%08lx, cycles=%lu, CPSR=0x%08lx\n", 
                frame_count, (unsigned long)reg[REG_PC], (unsigned long)cpu_ticks, 
                (unsigned long)reg[REG_CPSR]);
        
        // Track PC changes - if it's moving away from 0x080001f8 but coming back
        static u32 last_pc = 0;
        
        // Debug: Check if PC advancement is fundamentally broken
        static u32 prev_cycles = 0;
        u32 cycles_delta = cpu_ticks - prev_cycles;
        prev_cycles = cpu_ticks;
        
        if (frame_count > 1 && cycles_delta > 0 && reg[REG_PC] == last_pc) {
          fprintf(debug_log, "  EMULATION_BUG: Executed %lu cycles but PC didn't advance!\n", (unsigned long)cycles_delta);
          fprintf(debug_log, "  EMULATION_BUG: This suggests broken PC increment in CPU emulation\n");
        }
        if (frame_count > 0 && reg[REG_PC] != last_pc) {
          fprintf(debug_log, "  PC_CHANGE: 0x%08lx -> 0x%08lx\n", 
                  (unsigned long)last_pc, (unsigned long)reg[REG_PC]);
        }
        last_pc = reg[REG_PC];
        
        // Debug the instruction at 0x080001fc after workaround
        if (frame_count > 2 && reg[REG_PC] == 0x080001fc) {
          extern u8 *memory_map_read[8192];
          u8 *rom_page = memory_map_read[0x080001fc >> 15];
          if (rom_page) {
            u32 curr_instr = *((u32*)(rom_page + (0x080001fc & 0x7FFF)));
            fprintf(debug_log, "  DEBUG: Instruction at 0x080001fc = 0x%08lx\n", (unsigned long)curr_instr);
            
            // Check if this PC is also getting stuck
            static u32 fc_stuck_count = 0;
            static u32 last_fc_frame = 0;
            if (frame_count != last_fc_frame + 1) {
              fc_stuck_count = 1;
            } else {
              fc_stuck_count++;
            }
            last_fc_frame = frame_count;
            
            if (fc_stuck_count > 2) {
              fprintf(debug_log, "  WARNING: PC stuck at 0x080001fc for %lu frames\n", (unsigned long)fc_stuck_count);
              
              // Another workaround: skip this instruction too if stuck for 3+ frames
              if (fc_stuck_count >= 3) {
                fprintf(debug_log, "  WORKAROUND2: Skipping stuck ANDS instruction at 0x080001fc\n");
                fprintf(debug_log, "  WORKAROUND2: Advancing PC from 0x080001fc to 0x08000200\n");
                reg[REG_PC] = 0x08000200; // Move to next instruction
              }
            }
          }
        }
        // Workaround: If stuck at 0x080001f8 with problematic STMDB, skip to next instruction
        if (frame_count > 0 && reg[REG_PC] == 0x080001f8) {
          // This is the problematic STMDB r0!, {r0,r7,fp,sp,lr} with r0=DMA2 register
          // Apply workaround after frame 2, but log debug info first
          if (frame_count >= 2) {
            fprintf(debug_log, "  WORKAROUND: Skipping problematic STMDB instruction at 0x080001f8\n");
            fprintf(debug_log, "  WORKAROUND: Advancing PC from 0x080001f8 to 0x080001fc\n");
            reg[REG_PC] = 0x080001fc; // Move to next instruction
          } else {
            // Debug logging for stuck PC
          extern u8 *memory_map_read[8192];
          u8 *rom_page = memory_map_read[0x080001f8 >> 15];
          if (rom_page) {
            // Show context around stuck instruction
            u32 prev_instr = *((u32*)(rom_page + ((0x080001f8 - 4) & 0x7FFF)));
            u32 curr_instr = *((u32*)(rom_page + (0x080001f8 & 0x7FFF)));
            u32 next_instr = *((u32*)(rom_page + ((0x080001f8 + 4) & 0x7FFF)));
            fprintf(debug_log, "  STUCK: 0x080001f4: 0x%08lx\n", (unsigned long)prev_instr);
            fprintf(debug_log, "  STUCK: 0x080001f8: 0x%08lx <- PC HERE (STMDB r0!, {r0,r7,fp,sp,lr})\n", (unsigned long)curr_instr);
            fprintf(debug_log, "  STUCK: 0x080001fc: 0x%08lx\n", (unsigned long)next_instr);
            fprintf(debug_log, "  STUCK: r0=0x%08lx (DMA0CNT), r7=0x%08lx, fp=0x%08lx, sp=0x%08lx, lr=0x%08lx\n",
                    (unsigned long)reg[0], (unsigned long)reg[7], (unsigned long)reg[11],
                    (unsigned long)reg[13], (unsigned long)reg[14]);
            fprintf(debug_log, "  STUCK: irq_raised=0x%08lx, condition LS should be %s\n", 
                    (unsigned long)irq_raised, 
                    ((reg[REG_CPSR] & (1<<1)) || !(reg[REG_CPSR] & (1<<2))) ? "TRUE" : "FALSE");
            
            // Check if the instruction is somehow jumping back to itself
            if (frame_count == 5) {
              fprintf(debug_log, "  ANALYSIS: PC stuck at 0x080001f8 for 5 frames\n");
              fprintf(debug_log, "  ANALYSIS: This STMDB instruction tries to store registers to I/O space\n");
              fprintf(debug_log, "  ANALYSIS: r0=0x040000d4 is DMA2 source address, not DMA0CNT\n");
              fprintf(debug_log, "  ANALYSIS: STMDB should store to decreasing addresses: 0xd4,0xd0,0xcc,0xc8,0xc4\n");
              
              // Check if I/O registers are changing
              extern u16 io_registers[0x400];
              u32 dma2_src = ADDRESS32(io_registers, 0xD4);
              u32 dma2_dst_low = ADDRESS32(io_registers, 0xD0); 
              fprintf(debug_log, "  IO_CHECK: DMA2_SRC(0xD4)=0x%08lx, DMA2_DST_LOW(0xD0)=0x%08lx\n",
                      (unsigned long)dma2_src, (unsigned long)dma2_dst_low);
            }
          }
        }
        }
        fclose(debug_log);
      }
    }
    
    // Check for delayed save state loading after a few frames
    if (delayed_load_save_state && frame_count > 60) { // Wait ~1 second (60 frames)
      delayed_load_save_state = 0; // Clear flag
      
      FILE *debug_log = fopen("froglog.txt", "a");
      if (debug_log) {
        fprintf(debug_log, "UPDATE_GBA: Loading delayed save state after %d frames\n", frame_count);
        fclose(debug_log);
      }
      
      // Load the save state now that game is running
      extern s32 load_manual_auto_save_state(void);
      int result = load_manual_auto_save_state();
      
      debug_log = fopen("froglog.txt", "a");
      if (debug_log) {
        fprintf(debug_log, "UPDATE_GBA: Save state load result=%d (0=success)\n", result);
        fclose(debug_log);
      }
    }
    frame_count++;

    if (sleep_flag != 0)
      psp_sleep_loop();

    cpu_ticks += reg[EXECUTE_CYCLES];

    if (gbc_sound_update != 0)
    {
      update_gbc_sound(cpu_ticks);
      gbc_sound_update = 0;
    }

    for (i = 0; i < 4; i++)
    {
      UPDATE_TIMER(i);
    }

    video_count -= reg[EXECUTE_CYCLES];

    if (video_count <= 0)
    {
      u16 dispstat = pIO_REG(REG_DISPSTAT);
      u16 vcount   = pIO_REG(REG_VCOUNT);

      if ((dispstat & 0x02) == 0)
      {
        // Transition from hrefresh to hblank
        video_count += 272;
        dispstat |= 0x02;

        if ((dispstat & 0x01) == 0)
        {
          if (!skip_next_frame)
            update_scanline();

          // If in visible area also fire HDMA
          for (i = 0; i < 4; i++)
          {
            TRANSFER_HBLANK_DMA(i);
          }
        }

        // H-blank interrupts do occur during v-blank (unlike hdma, which does not)
        if ((dispstat & 0x10) != 0)
        {
          irq_raised |= IRQ_HBLANK;
        }
      }
      else
      {
        // Transition from hblank to next line
        video_count += 960;
        dispstat &= ~0x02;

        vcount++;

        if (vcount == 160)
        {
          // Transition from vrefresh to vblank
          dispstat |= 0x01;

          if (update_input() != 0)
            continue;

          if (!skip_next_frame)
            (*update_screen)();

          update_gbc_sound(cpu_ticks);
          gbc_sound_update = 0;

          affine_reference_x[0] = (s32)(ADDRESS32(io_registers, 0x28) << 4) >> 4;
          affine_reference_y[0] = (s32)(ADDRESS32(io_registers, 0x2C) << 4) >> 4;
          affine_reference_x[1] = (s32)(ADDRESS32(io_registers, 0x38) << 4) >> 4;
          affine_reference_y[1] = (s32)(ADDRESS32(io_registers, 0x3C) << 4) >> 4;

          for (i = 0; i < 4; i++)
          {
            TRANSFER_VBLANK_DMA(i);
          }

          if ((dispstat & 0x08) != 0)
          {
            irq_raised |= IRQ_VBLANK;
          }
        }
        else

        if (vcount > 227)
        {
          // Transition from vblank to next screen
          dispstat &= ~0x01;

          if (option_update_backup != 0)
            update_backup();

          process_cheats();

          u32 draw_this_frame = skip_next_frame ^ 0x01;

          synchronize();
          synchronize_sound();

          __draw_volume_status(draw_this_frame);

          if (draw_this_frame != 0)
            flip_screen(0);

          vcount = 0;
        }

        if (vcount == (dispstat >> 8))
        {
          // vcount trigger
          dispstat |= 0x04;

          if ((dispstat & 0x20) != 0)
          {
            irq_raised |= IRQ_VCOUNT;
          }
        }
        else
        {
          dispstat &= ~0x04;
        }

        pIO_REG(REG_VCOUNT) = vcount;
      }

      pIO_REG(REG_DISPSTAT) = dispstat;
    }

    reg[EXECUTE_CYCLES] = video_count;

    for (i = 0; i < 4; i++)
    {
      CHECK_TIMER(i);
    }

    if (dma_cycle_count != 0)
    {
      CHECK_COUNT(dma_cycle_count);
      dma_cycle_count -= reg[EXECUTE_CYCLES];

      goto update_gba_loop;
    }

    if (irq_raised != IRQ_NONE)
    {
      pIO_REG(REG_IF) |= irq_raised;
      irq_raised = IRQ_NONE;
    }

    if ((pIO_REG(REG_IF) != 0) && GBA_IME_STATE && ARM_IRQ_STATE)
    {
      u16 irq_mask = (reg[CPU_HALT_STATE] == CPU_STOP) ? 0x3080 : 0x3FFF;

      if ((pIO_REG(REG_IE) & pIO_REG(REG_IF) & irq_mask) != 0)
      {
        if (cpu_init_state != 0)
        {
          if (irq_ticks == 0)
          {
            cpu_interrupt();
            cpu_init_state = 0;
          }
        }
        else
        {
          if (reg[CPU_HALT_STATE] != CPU_ACTIVE)
          {
            cpu_interrupt();
          }
          else
          {
            // IRQ delay - Tsyncmax=3, Texc=3, Tirq=2, Tldm=20
            //             Tsyncmin=2
            irq_ticks = 9;
            cpu_init_state = 1;
          }
        }
      }
    }

    if (irq_ticks != 0)
    {
      CHECK_COUNT(irq_ticks);
      irq_ticks -= reg[EXECUTE_CYCLES];
    }
  }
  while (reg[CPU_HALT_STATE] != CPU_ACTIVE);

  return reg[EXECUTE_CYCLES];
}


static void vblank_interrupt_handler(u32 sub, u32 *parg)
{
  real_frame_count++;
  vblank_count++;
}

static void synchronize(void)
{
  static s32 fps = 60;
  static s32 frames_drawn = 60;

  u32 used_frameskip_type  = option_frameskip_type;
  u32 used_frameskip_value = option_frameskip_value;

  frames++;

  if (frames == 60)
  {
    fps = 3600 / vblank_count;
    frames_drawn = 60 - interval_skipped_frames;

    vblank_count = 0;
    frames = 0;
    interval_skipped_frames = 0;
  }

  if (!skip_next_frame)
  {
    // GBA - sleep mode
    if (reg[CPU_HALT_STATE] == CPU_STOP)
    {
      clear_screen(0);
	  if (option_language == 0)
      print_string(MSG[MSG_GBA_SLEEP_MODE], X_POS_CENTER, 130, COLOR15_WHITE, BG_NO_FILL);
	  else
      print_string_gbk(MSG[MSG_GBA_SLEEP_MODE], X_POS_CENTER, 130, COLOR15_WHITE, BG_NO_FILL);
    }

    // PSP controller - hold
    if (get_pad_input(PSP_CTRL_HOLD) != 0)
      print_string(FONT_KEY_ICON, 6, 258, COLOR15_YELLOW, BG_NO_FILL);

    // fps
    if (psp_fps_debug != 0)
    {
      char print_buffer[16];
      sprintf(print_buffer, "%02d(%02d)", fps, frames_drawn);
//    sprintf(print_buffer, "%02d(%02d)", fps % 100, frames_drawn);
      print_string(print_buffer, 0, 0, COLOR15_WHITE, COLOR15_BLACK);
    }
  }

  if (!synchronize_flag)
  {
    if (psp_fps_debug != 0)
	{
		if (option_language == 0)
			print_string(MSG[MSG_TURBO], 0, 12, COLOR15_WHITE, COLOR15_BLACK);
		else
			print_string_gbk(MSG[MSG_TURBO], 0, 12, COLOR15_WHITE, COLOR15_BLACK);
	}
	else
	{
		if (option_language == 0)
		print_string(MSG[MSG_TURBO], 0, 0, COLOR15_WHITE, COLOR15_BLACK);
		else
		print_string_gbk(MSG[MSG_TURBO], 0, 0, COLOR15_WHITE, COLOR15_BLACK);
	}
    used_frameskip_type = FRAMESKIP_MANUAL;
    used_frameskip_value = 4;
  }

  skip_next_frame = 0;
  virtual_frame_count++;

  if (real_frame_count >= virtual_frame_count)
  {
    if ((real_frame_count > virtual_frame_count) &&
        (used_frameskip_type == FRAMESKIP_AUTO) &&
        (num_skipped_frames < option_frameskip_value))
    {
      skip_next_frame = 1;
      num_skipped_frames++;
    }
    else
    {
      real_frame_count = 0;
      virtual_frame_count = 0;

      num_skipped_frames = 0;
    }
  }
  else
  {
    if (synchronize_flag != 0)
    {
      sceDisplayWaitVblankStart();
      real_frame_count = 0;
      virtual_frame_count = 0;
    }

    num_skipped_frames = 0;
  }

  if (used_frameskip_type == FRAMESKIP_MANUAL)
  {
    frameskip_counter = (frameskip_counter + 1) % (used_frameskip_value + 1);

    if (frameskip_counter != 0)
      skip_next_frame = 1;
  }

  interval_skipped_frames += skip_next_frame;
}


static void init_main(void)
{
  s32 i;

  FILE *debug_log = fopen("froglog.txt", "a");
  if (debug_log) {
    fprintf(debug_log, "INIT_MAIN: Starting GBA initialization\n");
    fclose(debug_log);
  }

  init_cpu();

  debug_log = fopen("froglog.txt", "a");
  if (debug_log) {
    fprintf(debug_log, "INIT_MAIN: CPU initialized, setting up system\n");
    fclose(debug_log);
  }

  skip_next_frame = 0;

  vblank_count = 0;
  frames = 0;
  interval_skipped_frames = 0;

  for (i = 0; i < 4; i++)
  {
    memset(&dma[i], 0, sizeof(DmaTransferType));

    dma[i].start_type = DMA_INACTIVE;
    dma[i].direct_sound_channel = DMA_NO_DIRECT_SOUND;

    memset(&timer[i], 0, sizeof(TimerType));

    timer[i].status = TIMER_INACTIVE;
    timer[i].reload = 0x10000;
    timer[i].direct_sound_channels = TIMER_DS_CHANNEL_NONE;
  }

  cpu_ticks = 0;

  video_count = option_boot_mode ? 960 : 272;
  reg[EXECUTE_CYCLES] = video_count;

  irq_ticks = 0;
  cpu_init_state = 0;

  if (!caches_inited)
  {
    flush_translation_cache(TRANSLATION_REGION_READONLY, FLUSH_REASON_INITIALIZING);
    flush_translation_cache(TRANSLATION_REGION_WRITABLE, FLUSH_REASON_INITIALIZING);
  }
  else
  {
    flush_translation_cache(TRANSLATION_REGION_READONLY, FLUSH_REASON_LOADING_ROM);
    clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_LOADING_ROM);
    clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_LOADING_ROM);
    clear_metadata_area(METADATA_AREA_VRAM,  CLEAR_REASON_LOADING_ROM);
  }

  caches_inited = 1;

  debug_log = fopen("froglog.txt", "a");
  if (debug_log) {
    fprintf(debug_log, "INIT_MAIN: GBA initialization completed\n");
    fclose(debug_log);
  }
}


static int load_ku_bridge_prx(int devkit_version)
{
  SceUID mod;
  char prx_path[MAX_PATH];

  sprintf(prx_path, "%sku_bridge.prx", main_path);

  if ((mod = kuKernelLoadModule(prx_path, 0, NULL)) >= 0)
  {
    int status;

    if (sceKernelStartModule(mod, 0, 0, &status, NULL) < 0)
    {
      sceKernelUnloadModule(mod);

      print_string("Could not start ku_bridge.prx", 6, 6, COLOR15_WHITE, COLOR15_BLACK);
      flip_screen(1);
      sceKernelDelayThread(5 * 1000 * 1000);
      quit();
    }

    init_ku_bridge(devkit_version);
    init_input_kernel();

    if (devkit_version >= 0x03050210)
      __draw_volume_status = draw_volume_status;

    return 0;
  }

  return -1;
}


static void load_setting_cfg(void)
{
  char filename[MAX_FILE];

  sprintf(filename, "%sdir.ini", main_path);

  if (load_dir_cfg(filename) < 0)
  {
    sprintf(filename, MSG[MSG_ERR_LOAD_DIR_INI], main_path);
    //error_msg(filename, CONFIRMATION_CONT);
  }
}

static void load_bios_file(void)
{
  char filename[MAX_FILE];

  sprintf(filename, "%sgba_bios.bin", main_path);

  if (load_bios(filename) < 0)
  {
    error_msg(MSG[MSG_ERR_BIOS_NONE], CONFIRMATION_QUIT);
    quit();
  }
}

static void setup_main(void)
{
  scePowerLock(0);

  initExceptionHandler();

  int devkit_version = sceKernelDevkitVersion();

  if (devkit_version < 0x05000010)
    __scePowerSetClockFrequency = scePowerSetClockFrequency;
  else
    __scePowerSetClockFrequency = scePowerSetClockFrequency371;

  __draw_volume_status = draw_volume_status_null;

  init_input();
  init_video(devkit_version);
  video_resolution_large();

  // Copy the directory path of the executable into main_path
  getcwd(main_path, MAX_PATH);
  strcat(main_path, "/");

  sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 0, vblank_interrupt_handler, NULL);
  sceKernelEnableSubIntr(PSP_VBLANK_INT, 0);

  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_DATE_FORMAT, &date_format);

  if ((load_ku_bridge_prx(devkit_version) < 0) && (devkit_version < 0x06000010))
    enable_home_menu = 0;
  else
    enable_home_menu = 1;

  load_config_file();
  init_input();

  setup_callbacks();
  sceImposeSetHomePopup(enable_home_menu ^ 1);

  load_setting_cfg();
  load_recent_games();
  init_overlays_at_boot();
  load_bios_file();

  init_gamepak_buffer();

  sound_pause = 1;
  set_sound_volume();
  init_sound();

  scePowerUnlock(0);
}

int user_main(int argc, char *argv[])
{
  char load_filename[MAX_FILE];
  //const char *file_ext[] = { ".zip", ".gba", ".bin", ".agb", ".gbz", NULL };

  setup_main();

  gamepak_filename[0] = 0;


  //strcpy(load_filename, "roms/game.gba");
  sprintf(load_filename, "%sgame.gba", dir_roms);

/*
  if (load_gamepak(load_filename) < 0)
  {
    clear_screen(COLOR32_BLACK);
    error_msg(MSG[MSG_ERR_LOAD_GAMEPACK], CONFIRMATION_CONT);
    menu();
  }
*/


  if (argc > 1)
  {
    if (load_gamepak((char *)argv[1]) < 0)
    {
      clear_screen(COLOR32_BLACK);
      error_msg(MSG[MSG_ERR_LOAD_GAMEPACK], CONFIRMATION_QUIT);
      quit();
    }
  }
  else
  {
    // Check if resume on boot is enabled
    if (option_resume_on_boot != 0)
    {
      // Try to load the last played game
      char last_game_path[MAX_PATH];
      if (load_last_played_game(last_game_path, MAX_PATH) == 0)
      {
        // Last played game path loaded, try to load the game
        if (load_gamepak(last_game_path) < 0)
        {
          // Failed to load last game, go to menu
          clear_screen(COLOR32_BLACK);
          menu();
        }
        else
        {
          // Game loaded successfully
          // Note: reset_gba() and auto-save loading will be handled after main initialization
          // to avoid interfering with backup ID detection during resume
        }
      }
      else
      {
        // No last played game found, go to menu
        clear_screen(COLOR32_BLACK);
        menu();
      }
    }
    else
    {
      // Resume disabled, go directly to load game menu
      clear_screen(COLOR32_BLACK);
      menu_load_game_direct();
    }
  }

  // Always reset first to ensure clean state
  reset_gba();
  
  // If we have a game loaded and auto-save state is enabled, set flag for delayed loading
  if (gamepak_filename[0] != 0 && option_auto_save_state != 0) {
    delayed_load_save_state = 1;
    
    FILE *debug_log = fopen("froglog.txt", "a");
    if (debug_log) {
      fprintf(debug_log, "MAIN: Set delayed save state loading flag for boot game: %s\n", gamepak_filename);
      fclose(debug_log);
    }
  }

  FILE *debug_log = fopen("froglog.txt", "a");
  if (debug_log) {
    fprintf(debug_log, "MAIN: About to set CPU clock\n");
    fclose(debug_log);
  }

  set_cpu_clock(option_clock_speed);

  debug_log = fopen("froglog.txt", "a");
  if (debug_log) {
    fprintf(debug_log, "MAIN: CPU clock set, about to initialize video\n");
    fclose(debug_log);
  }

  sceDisplayWaitVblankStart();
  video_resolution_small();

  debug_log = fopen("froglog.txt", "a");
  if (debug_log) {
    fprintf(debug_log, "MAIN: Video initialized, about to start sound and execute\n");
    fclose(debug_log);
  }

  sound_pause = 0;

  // Delayed save state loading is now handled inside update_gba() after game is running

  FILE *debug_log2 = fopen("froglog.txt", "a");
  if (debug_log2) {
    fprintf(debug_log2, "MAIN: About to call execute_arm_translate with cycles=%lu\n", (unsigned long)reg[EXECUTE_CYCLES]);
    fclose(debug_log2);
  }

  // Add debug info about initial CPU state
  FILE *debug_cpu = fopen("froglog.txt", "a");
  if (debug_cpu) {
    fprintf(debug_cpu, "MAIN: Initial CPU state - PC=0x%08lx, CPSR=0x%08lx, SP=0x%08lx\n", 
            (unsigned long)reg[REG_PC], (unsigned long)reg[REG_CPSR], (unsigned long)reg[REG_SP]);
    extern struct BiosData bios;
    extern u32 gamepak_size;
    fprintf(debug_cpu, "MAIN: BIOS loaded=1, ROM size=%lu bytes\n", 
            (unsigned long)gamepak_size);
    fclose(debug_cpu);
  }
  
  // We'll never actually return from here.
  execute_arm_translate(reg[EXECUTE_CYCLES]);
  
  // This should never be reached
  debug_log2 = fopen("froglog.txt", "a");
  if (debug_log2) {
    fprintf(debug_log2, "MAIN: ERROR - Unexpectedly returned from execute_arm_translate\n");
    fclose(debug_log2);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  user_main(argc, argv);

  return 0;
}


void quit(void)
{
  update_backup_immediately();
  save_config_file();

  sound_term();
  memory_term();
  video_term();
  cpu_term();

  set_cpu_clock(PSP_CLOCK_222);

  sceKernelDisableSubIntr(PSP_VBLANK_INT, 0);
  sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 0);

  scePowerUnregisterCallback(0);

  sceKernelExitGame();
}

void reset_gba(void)
{
  clear_texture(0x7FFF);
  init_memory();
  init_main();
  reset_sound();
}


static void psp_sleep_loop(void)
{
  if (FILE_CHECK_VALID(gamepak_file_large))
  {
    s32 i;

    FILE_CLOSE(gamepak_file_large);

    do
    {
      sceKernelDelayThread(500 * 1000);
    }
    while (sleep_flag != 0);

    for (i = 0; i < 5; i++)
    {
      FILE_OPEN(gamepak_file_large, gamepak_filename_raw, READ);

      if (FILE_CHECK_VALID(gamepak_file_large))
        return;

      sceKernelDelayThread(500 * 1000);
    }

    clear_screen(COLOR32_BLACK);
    error_msg(MSG[MSG_ERR_OPEN_GAMEPACK], CONFIRMATION_QUIT);
    quit();
  }
  else
  {
    do
    {
      sceKernelDelayThread(500 * 1000);
    }
    while (sleep_flag != 0);
  }
}


static int exit_callback(int arg1, int arg2, void *common)
{
  quit();

  return 0;
}

static int power_callback(int unknown, int powerInfo, void *arg)
{
  if ((powerInfo & PSP_POWER_CB_SUSPENDING) != 0)
  {
    sleep_flag = 1;
  }
  else

  if ((powerInfo & PSP_POWER_CB_RESUME_COMPLETE) != 0)
  {
    psp_sound_frequency(SOUND_SAMPLES, SOUND_FREQUENCY);

    sleep_flag = 0;
  }

  return 0;
}

static int callback_thread(SceSize args, void *argp)
{
  SceUID cbid;

  cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
  sceKernelRegisterExitCallback(cbid);

  cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
  scePowerRegisterCallback(0, cbid);

  sceKernelSleepThreadCB();

  return 0;
}

static void setup_callbacks(void)
{
  SceUID thid = -1;

  thid = sceKernelCreateThread("Update Thread",
                               callback_thread,
                               0x11,
                               0xFA0,
                               0,
                               NULL);
  if (thid < 0)
  {
    error_msg(MSG[MSG_ERR_START_CALLBACK_THREAD], CONFIRMATION_QUIT);
    quit();
  }

  sleep_flag = 0;
  sceKernelStartThread(thid, 0, 0);
}


u64 ticker(void)
{
  u64 current_ticks;

  sceRtcGetCurrentTick(&current_ticks);

  return current_ticks;
}

u32 file_length(char *filename)
{
  SceIoStat stats;

  sceIoGetstat(filename, &stats);

  return stats.st_size;
}

void change_ext(char *src, char *buffer, const char *extension)
{
  char *dot_position;

  strcpy(buffer, src);
  dot_position = strrchr(buffer, '.');

  if (dot_position)
  {
    strcpy(dot_position, extension);
  }
}

void error_msg(const char *text, u8 confirm)
{
  char text_buff[512];
  GUI_ACTION_TYPE gui_action = CURSOR_NONE;

  switch (confirm)
  {
    case CONFIRMATION_NONE:
      strcpy(text_buff, text);
      break;

    case CONFIRMATION_CONT:
      sprintf(text_buff, "%s\n\n%s", text, MSG[MSG_ERR_CONT]);
      break;

    case CONFIRMATION_QUIT:
      sprintf(text_buff, "%s\n\n%s", text, MSG[MSG_ERR_QUIT]);
      break;
  }
  if (option_language == 0)
  print_string(text_buff, 6, 6, COLOR15_WHITE, COLOR15_BLACK);
  else
  print_string_gbk(text_buff, 6, 6, COLOR15_WHITE, COLOR15_BLACK);
  flip_screen(1);

  while (gui_action == CURSOR_NONE)
  {
    gui_action = get_gui_input();
  }
}


int set_cpu_clock(u32 psp_clock)
{
  int ret = -1;

  if (__scePowerSetClockFrequency != NULL)
  {
    switch (psp_clock)
    {
      case PSP_CLOCK_333:
        ret = (*__scePowerSetClockFrequency)(333, 333, 166);
        break;

      case PSP_CLOCK_300:
        ret = (*__scePowerSetClockFrequency)(300, 300, 150);
        break;

      case PSP_CLOCK_266:
        ret = (*__scePowerSetClockFrequency)(266, 266, 133);
        break;

      default:
      case PSP_CLOCK_222:
        ret = (*__scePowerSetClockFrequency)(222, 222, 111);
        break;
    }
  }

  return ret;
}


SceUID psp_fopen(const char *filename, const char *mode)
{
  SceUID tag;

  if ((*mode != 'r') && (*mode != 'w'))
    return -1;

  tag = sceIoOpen(filename, PSP_O_RDONLY, 0777);

  if (*mode == 'w')
  {
    int flags;

    if (tag < 0)
    {
      flags = PSP_O_WRONLY | PSP_O_CREAT;
    }
    else
    {
      sceIoClose(tag);
      flags = PSP_O_WRONLY | PSP_O_TRUNC;
    }

    tag = sceIoOpen(filename, flags, 0777);
  }

  return tag;
}

void psp_fclose(SceUID filename_tag)
{
  if (filename_tag < 0)
    return;

  sceIoClose(filename_tag);
  filename_tag = -1;
}


void *safe_malloc(size_t size)
{
  void *p;

  // Conservative approach - disable dynamic volatile allocation to prevent BSOD
  // Keep only the translation cache optimizations which are working well
  if ((p = memalign(MEM_ALIGN, size)) == NULL)
  {
    clear_screen(COLOR32_BLACK);
    error_msg(MSG[MSG_ERR_MALLOC], CONFIRMATION_QUIT);
    quit();
  }

  return p;
}

void safe_free(void *ptr)
{
  if (!ptr) return;
  
  // Simple and safe - no volatile memory detection to avoid BSOD
  free(ptr);
}


// type = READ / WRITE_MEM
#define MAIN_SAVESTATE_BODY(type)                                             \
{                                                                             \
  FILE_##type##_VARIABLE(savestate_file, cpu_ticks);                          \
  FILE_##type##_VARIABLE(savestate_file, video_count);                        \
  FILE_##type##_VARIABLE(savestate_file, irq_ticks);                          \
  FILE_##type##_VARIABLE(savestate_file, cpu_init_state);                     \
  FILE_##type##_ARRAY(savestate_file, timer);                                 \
}                                                                             \

void main_read_savestate(SceUID savestate_file)
{
  MAIN_SAVESTATE_BODY(READ);
}

void main_write_mem_savestate(SceUID savestate_file)
{
  MAIN_SAVESTATE_BODY(WRITE_MEM);
}

