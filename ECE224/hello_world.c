/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <system.h>
#include <sys/alt_alarm.h>
#include <sys/alt_irq.h>
#include <io.h>

#include "fatfs.h"
#include "diskio.h"

#include "ff.h"
#include "monitor.h"
#include "uart.h"

#include "alt_types.h"

#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>
#include <altera_avalon_pio_regs.h>

#define DISK_NUM 0
#define DRIVE_NUM 0
#define FOPEN_MODE 1

#define PSTR(_a)  _a
#define MIN(a,b) ((a < b) ? a : b)

#define STOPPED -1
#define PAUSED 0
#define NORMAL 1
#define HALF 2
#define DOUBLE 3
#define MONO 4

typedef struct wav {
  char fname[20];
  unsigned long len;
  int entryNum;
} WAV;

static alt_alarm alarm;
static unsigned long Systick = 0;
static volatile unsigned short Timer;   /* 1000Hz increment timer */

uint32_t acc_size;                 /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[512];
#endif

char Line[256];                 /* Console input buffer */

FATFS Fatfs[_VOLUMES];          /* File system object for each logical drive */
FIL File1, File2;               /* File objects */
DIR Dir;                        /* Directory object */
uint8_t Buff[8192] __attribute__((aligned(4)));  /* Working buffer */

int numFiles = 0;
char fnames[20][20];
unsigned long lens[20];
int entryNums[20];
uint8_t stop, playing, pause, next, prev;
int speed = 1;
int track_num;
int inputs = 0;


static void buttonISR(void *context, alt_u32 id) {
  IOWR(BUTTON_PIO_BASE, 2, 0);
  //disable button interrupts

  int buttons = IORD(BUTTON_PIO_BASE, 0);
  if (buttons == 0b1110) {
    //next track
    inputs = 1; //received input
    next = 1;
  }
  if (buttons == 0b1101) {
    // play/pause button
    inputs = 2;
    if(pause) {
      //resume track
      display(track_num, speed);
      pause = 0;
      playing = 1;
    }
    else {
      display(track_num, PAUSED);
      pause = 1;
      playing = 0;
      //pause track
    }
  }
  if(buttons == 0b1011) {
    //stop track
    display(track_num, STOPPED);
    inputs = 3;
    stop = 1;
    printf("Stopped");
  }
  if(buttons == 0b0111) {
    inputs = 4;
    prev = 1;
    //play function takes care of displaying track
  }

  IOWR(TIMER_0_BASE, 1, 0b0111);

  IOWR(BUTTON_PIO_BASE, 3, 0);
}
static void timerISR(void* context, alt_u32 id) {
  //timer isr interrupt and clear

  //clear button interrupt reg
  IOWR(BUTTON_PIO_BASE, 3, 0x0);

  IOWR(BUTTON_PIO_BASE, 2, 0xF);

  //reset timer TO
  IOWR(TIMER_0_BASE, 0, 0);
  IOWR(TIMER_0_BASE, 1, 0b1011);
}

static alt_u32 TimerFunction (void *context)
{
   static unsigned short wTimer10ms = 0;

   (void)context;

   Systick++;
   wTimer10ms++;
   Timer++; /* Performance counter for this module */

   if (wTimer10ms == 10)
   {
      wTimer10ms = 0;
      ffs_DiskIOTimerproc();  /* Drive timer procedure of low level disk I/O module */
   }

   return(1);
} /* TimerFunction */

static void IoInit(void)
{
   uart0_init(115200);

   /* Init diskio interface */
   ffs_DiskIOInit();

   //SetHighSpeed();

   /* Init timer system */
   alt_alarm_start(&alarm, 1, &TimerFunction, NULL);

} /* IoInit */

static
FRESULT scan_files(char *path)
{
    DIR dirs;
    FRESULT res;
    uint8_t i;
    char *fn;


    if ((res = f_opendir(&dirs, path)) == FR_OK) {
        i = (uint8_t)strlen(path);
        while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
            if (_FS_RPATH && Finfo.fname[0] == '.')
                continue;
#if _USE_LFN
            fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
            fn = Finfo.fname;
#endif
            if (Finfo.fattrib & AM_DIR) {
                acc_dirs++;
                *(path + i) = '/';
                strcpy(path + i + 1, fn);
                res = scan_files(path);
                *(path + i) = '\0';
                if (res != FR_OK)
                    break;
            } else {
                //      xprintf("%s/%s\n", path, fn);
                acc_files++;
                acc_size += Finfo.fsize;
            }
        }
    }

    return res;
}

static
void put_rc(FRESULT rc)
{
    const char *str =
        "OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
        "INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
        "INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
        "LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
    FRESULT i;

    for (i = 0; i != rc && *str; i++) {
        while (*str++);
    }
    xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}

// Function to check if a file is a WAV file
int isWavFile(const char *filename) { //written by chatgpt needs changing to properly read files
    char *dot = strrchr(filename, '.');
    return (dot && !strcmp(dot, ".WAV"));
}

int loadWav() { //add files that will be looped through
  uint8_t res;
  long p1;
  uint32_t s1, s2 = sizeof(Buff);

  res = f_opendir(&Dir, "");
  if (res) // if res in non-zero there is an error; print the error.
  {
      return -1;
  }
  p1 = s1 = s2 = 0; // otherwise initialize the pointers and proceed.
  int i = 0;
  for (;;)
  {
      res = f_readdir(&Dir, &Finfo);
      if ((res != FR_OK) || !Finfo.fname[0]) {
        return -1;
      }
      if (Finfo.fattrib & AM_DIR)
      {
          s2++;
      }
      else
      {
          s1++;
          p1 += Finfo.fsize;
      }

      i++;

      if(isWavFile(&Finfo.fname[0])) {
        strcpy(&fnames[numFiles][0], &Finfo.fname[0]);
        lens[numFiles] = Finfo.fsize;
        entryNums[numFiles] = i;
        numFiles++;
      }
  }
}

int display(int i, int mode) {
  FILE* lcd;
  lcd = fopen("/dev/lcd_display", "w");
  char* first = &fnames[i][0];

  if(lcd != NULL) {
    fprintf(lcd, "%c%s", 27, "[2J"); //comes from embedded peripheral datasheet number 27.3.2
    fprintf(lcd, "%d: %s\n", entryNums[i], first);
    switch (mode) {
      case STOPPED:
        fprintf(lcd, "STOPPED\n");
        break;
      case PAUSED:
        fprintf(lcd, "PAUSED\n");
        break;
      case NORMAL:
        fprintf(lcd, "PBACK-NORM SPD\n");
        break;
      case HALF:
        fprintf(lcd, "PBACK-HALF SPD\n");
        break;
      case DOUBLE:
        fprintf(lcd, "PBACK-DBL SPD\n");
        break;
      case MONO:
        fprintf(lcd, "PBACK-MONO\n");
        break;
    }
  }
  fclose(lcd);
  return 0;
}

int play(int index, char* fname, unsigned long p1, alt_up_audio_dev *audio_dev) {
  int fifospace;
  char *ptr, *ptr2;
  long p2, p3;
  uint8_t res = 0;
  uint32_t s1, s2, cnt, blen = sizeof(Buff);
  uint32_t ofs = 0;
  unsigned long temp1 = p1;
  FATFS *fs;

  unsigned int l_buf;
  unsigned int r_buf;

  // open file
  f_open(&File1, fname, (uint8_t)FOPEN_MODE);

  ofs = File1.fptr;
  unsigned int bytesRead = 0;
  res = f_read(&File1, Buff, 44, &s2);
  bytesRead += s2;
  uint32_t remaining;
  int pos;

  // int switch0 = IORD(SWITCH_PIO_BASE, 0) & 0x1; // speed switch
  // int switch1 = IORD(SWITCH_PIO_BASE, 0) & 0x2; // mono switch

  // if(switch0 && switch1) {
  //   //printf("Normal Speed\n");
  //   display(index, NORMAL);
  //   speed = 1;
  // }
  // if (!switch0 && switch1){
  //   //printf("Half Speed\n");
  //   display(index, HALF);
  //   speed = 2;
  // }

  // if(!switch1 && switch0) {
  //   //printf("Double Speed\n");
  //   display(index, DOUBLE);
  //   speed = 3;
  // }
  // if(!switch1 && !switch0){
  //   //printf("MONO - Left Audio\n");
  //   display(index, MONO);
  //   speed = 4;
  // }
  uint8_t playback_buf[64] __attribute__((aligned(4)));
  int switches = IORD(SWITCH_PIO_BASE, 0x0) & 3;
  switch (switches)
  {
  case 0:
      display(index, NORMAL);
      speed = NORMAL;
      break;
  case 1:
      display(index, HALF);
      speed = HALF;
      break;
  case 2:
      display(index, DOUBLE);
      speed = DOUBLE;
      break;
  case 3:
      display(index, MONO);
      speed = MONO;
      break;
  default:
      break;
  }

  while(bytesRead < p1) {
    switch(switches) {
      case 0: //normal speed
    	  if (p1 - bytesRead > 64) {
			res = f_read(&File1, playback_buf, 64, &s2);
			if (res != FR_OK) {
			  put_rc(res);
			  break;
			}
		  }
		  else { //put remaining bytes into buffer
			res = f_read(&File1, playback_buf, p1 - bytesRead, &s2);
			if(res != FR_OK) {
			  put_rc(res);
			  break;
			}
		  }
		  bytesRead += s2;
		  pos = 0;
        while(pos < s2) {
          while (pause) {
            if (stop || next || prev) {
              stop = 0;
              playing = 0;
              pause = 1; //cancel out other commands
              return 0;
            }
          }
          if (stop) {
            stop = 0;
            playing = 0;
            pause = 1;
            return 0; //immediately stop playback, exit function
          }
          if(next || prev) {
            playing = 1;
            return 0;
            //continue playing in next/previous song
            //exit out of function to go to next/previous song
          }

          int fifospace = alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT);
          if(fifospace > 0) {
            l_buf = playback_buf[pos] | (playback_buf[pos + 1] << 8);
            r_buf = playback_buf[pos + 2] | (playback_buf[pos + 3] << 8);

            alt_up_audio_write_fifo (audio_dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
            alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);

            pos += 4;
          }
        }
        break;
      case 1: //half speed
		  if ((uint32_t) temp1 >= 1024)
			{
			  	res = f_read(&File1, Buff, 1024, &s2);
			}
			else
			{
				res = f_read(&File1, Buff, temp1, &s2);
			}
      remaining = s2;

		  bytesRead += s2;
		  pos = 0;
        while(remaining > 0) {
          while (pause) {
            if (stop || next || prev) {
              stop = 0;
              playing = 0;
              pause = 1; //cancel out other commands
              return 0;
            }
          }
          if (stop) {
            stop = 0;
            playing = 0;
            pause = 1;
            return 0; //immediately stop playback, exit function
          }
          if(next || prev) {
            playing = 1;
            return 0;
            //continue playing in next/previous song
            //exit out of function to go to next/previous song
          }

          	uint32_t min;
			uint32_t right_space = (alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT)) * 2;
			uint32_t left_space = (alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_LEFT)) * 2;

			// min space of buffers
			if (right_space < left_space){
				min = right_space;
			}

			else{
				min = left_space;
			}

			if (min > remaining){
				min = remaining;
			}
			// calculates the index in Buff we stopped transferring at last cycle of the loop
			for (int i = (s2 - remaining); i < (s2 - remaining + min); i += 2)
			{
				l_buf = (uint16_t)Buff[i] |((uint16_t) Buff[i+1] << 8);
				r_buf = (uint16_t)Buff[i+2] | ((uint16_t) Buff[i+3] << 8);

				alt_up_audio_write_fifo (audio_dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
				// write audio buffer
				alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);

				temp1 -= 2;
			}

			remaining -= min; //decrease bytes left to read
        }
		break;
      case 2: //double speed
    	  if ((uint32_t) p1 >= 1024)
			{
res = f_read(&File1, Buff, 1024, &s2);				
			}
			else
			{
res = f_read(&File1, Buff, temp1, &s2);				
			}
      remaining = s2;

		  bytesRead += s2;
		  pos = 0;
        while(remaining > 0) {
          while (pause) {
            if (stop || next || prev) {
              stop = 0;
              playing = 0;
              pause = 1; //cancel out other commands
              return 0;
            }
          }
          if (stop) {
            stop = 0;
            playing = 0;
            pause = 1;
            return 0; //immediately stop playback, exit function
          }
          if(next || prev) {
            playing = 1;
            return 0;
            //continue playing in next/previous song
            //exit out of function to go to next/previous song
          }

          uint32_t min;
			uint32_t right_space = (alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT)) * 8;
			uint32_t left_space = (alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_LEFT)) * 8;

			// min space of buffers
			if (right_space < left_space){
				min = right_space;
			}

			else{
				min = left_space;
			}

			if (min > remaining){
				min = remaining;
			}
			// s2 - remaining calculates the index in Buff we stopped transferring at last cycle of the loop
			for (int i = (s2 - remaining); i < (s2 - remaining + min); i += 8)
			{
				l_buf = (uint16_t)Buff[i] |((uint16_t) Buff[i+1] << 8);
				r_buf = (uint16_t)Buff[i+2] | ((uint16_t) Buff[i+3] << 8);
        // write audio buffer
				alt_up_audio_write_fifo (audio_dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
				alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);

				temp1 -= 8;
			}
			// calculate the number of bytes still to be pushed into the output buffers
			remaining -= min;

        }
        break;
      case 3: //mono
    	  if (p1 - bytesRead > 64) {
			res = f_read(&File1, playback_buf, 64, &s2);
			if (res != FR_OK) {
			  put_rc(res);
			  break;
			}
		  }
		  else { //put remaining bytes into buffer
			res = f_read(&File1, playback_buf, p1 - bytesRead, &s2);
			if(res != FR_OK) {
			  put_rc(res);
			  break;
			}
		  }
		  bytesRead += s2;
		  pos = 0;
        while(pos < s2) {
          while (pause) {
            if (stop || next || prev) {
              stop = 0;
              playing = 0;
              pause = 1; //cancel out other commands
              return 0;
            }
          }
          if (stop) {
            stop = 0;
            playing = 0;
            pause = 1;
            return 0; //immediately stop playback, exit function
          }
          if(next || prev) {
            playing = 1;
            return 0;
            //continue playing in next/previous song
            //exit out of function to go to next/previous song
          }

          int fifospace = alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT);
          if(fifospace > 0) {
            l_buf = playback_buf[pos] | (playback_buf[pos + 1] << 8);
            //only write to left speaker

            alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_RIGHT);
            alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);

            pos += 4;
          }
        }
        break;
        playing = 0;
        pause = 1;
        stop = 1;
        display(track_num, STOPPED);
        return 0;
    }
  }
}


int main()
{
  int fifospace;
  char *ptr, *ptr2;
  long p1, p2, p3;
  uint8_t res, b1, drv = 0;
  uint16_t w1;
  uint32_t s1, s2, cnt, blen = sizeof(Buff);
  static const uint8_t ft[] = { 0, 12, 16, 32 };
  uint32_t ofs = 0, sect = 0, blk[2];
  FATFS *fs;                  /* Pointer to file system object */

  //register button irq
  alt_irq_register(BUTTON_PIO_IRQ, (void*) 0, buttonISR);

  //clear button register
  IOWR(BUTTON_PIO_BASE, 3, 0);

  //hook up button irq
  IOWR(BUTTON_PIO_BASE, 2, 0xF);
  //write timer periods for 64-bit timer
  IOWR(TIMER_0_BASE, 2, 0xFFFF);
  IOWR(TIMER_0_BASE, 3, 0x00FF);
  IOWR(TIMER_0_BASE, 4, 0x0000);
  IOWR(TIMER_0_BASE, 5, 0x0000);

  IOWR(TIMER_0_BASE, 1, 0x3); //intialize timer control to 1, enable IRQ to be run through ITO bit

  alt_irq_register(TIMER_0_IRQ, (void* ) 0, timerISR);

  alt_up_audio_dev * audio_dev;
  /* used for audio record/playback */
  unsigned int l_buf;
  unsigned int r_buf;
  // open the Audio port
  audio_dev = alt_up_audio_open_dev ("/dev/Audio");

  if (audio_dev == NULL)
  alt_printf ("Error: could not open audio device \n");
//  else
//  alt_printf ("Opened audio device \n");
  //printf("Hello from Nios II!\n");
  IOWR(SEVEN_SEG_PIO_BASE,1,0x0000);

  IoInit();
  //replaces CLI command di 0 and fi 0
  disk_initialize((uint8_t) DISK_NUM);
  f_mount((uint8_t) DRIVE_NUM, &Fatfs[DRIVE_NUM]);
  loadWav();
  printf("%d\n",numFiles);

  track_num = 0; //track number
  display(track_num, STOPPED); //start player stopped
  int i;
  pause = 1;

  while(1) {
    display(track_num, STOPPED);
    while(pause) {
      if(next) {
        next = 0;
        track_num++;
        if(track_num >= numFiles) {
          track_num -= numFiles;
        }
        display(track_num, STOPPED);
      }
      if(prev) {
        prev = 0;
        track_num--;
        if(track_num < 0) {
          track_num += numFiles;
        }
        display(track_num, STOPPED);
      }
    }
    play(track_num, &fnames[track_num][0], lens[track_num], audio_dev);
    display(track_num, STOPPED);
    while(playing) {
      stop = 0;
      pause = 0;
      if(next) {
        next = 0;
        track_num++;
        if(track_num >= numFiles) {
          track_num -= numFiles;
        }
        play(track_num, &fnames[track_num][0], lens[track_num], audio_dev);
      }
      if(prev) {
        prev = 0;
        track_num--;
        if(track_num < 0) {
          track_num += numFiles;
        }
        play(track_num, &fnames[track_num][0], lens[track_num], audio_dev);
      }
    }
    pause = 1;
    display(track_num, STOPPED);
  }
  return 0;
}
