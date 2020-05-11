#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"

DA7212 audio;
int16_t waveform[kAudioTxBufferSize];

DigitalOut gLED(LED2);
DigitalOut rLED(LED1);
InterruptIn button2(SW2);
InterruptIn button3(SW3);
uLCD_4DGL uLCD(D1, D0, D2);

EventQueue queue(32 * EVENTS_EVENT_SIZE);
EventQueue queue2(32 * EVENTS_EVENT_SIZE);

Thread t;
Thread t2;
Thread t_song;

void play_song(void);
void play_song2(void);
void stop_play_songs(void);

int audio_stop = 0;
int audio_next = 0;

int song[42] = {
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261 };

int noteLength[42] = {
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2 };

int song2[32] = {
  524, 588, 660, 524,
  524, 588, 660, 524,
  660, 698, 784,
  660, 698, 784,
  784, 880, 784, 698, 660, 524,
  784, 880, 784, 698, 660, 524,
  588, 392, 524, 
  588, 392, 524  }; //Two-Tiger Run Fast

int noteLength2[32] = {
  1, 1, 1, 1,
  1, 1, 1, 1,
  1, 1, 2, 
  1, 1, 2, 
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 2,
  1, 1, 2  };

void playNote(int freq) {
  for(int i = 0; i < kAudioTxBufferSize; i++){
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  audio.spk.play(waveform, kAudioTxBufferSize);
}

void play_song(void) {
  uLCD.printf("\nTwinkle Twinkle\nLittle Star\n");
  for(int i = 0; i < 42; i++){
    if(audio_next) {
      audio_next = 0;
      uLCD.printf("\nNext Song:\n");
      play_song2();
      break;
    }else {
      if(!audio_stop) {
        int length = noteLength[i];
        while(length--) {
          // the loop below will play the note for the duration of 1s
            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j){
              queue.call(playNote, song[i]);
            }
            if(length < 1) wait(1.0);
            //uLCD.printf("yellow\n");
        }
      }else {
        uLCD.cls();
        while(audio_stop) {
          uLCD.printf("Stop\n");
        }
        uLCD.cls();
        if(!audio_next)
          uLCD.printf("Resume!\n");
      } 
    }
  }
}

void play_song2(void) {
  uLCD.printf("\nTwo-Tiger Run Fast\n"); 
  for(int i = 0; i < 32; i++){
    if(audio_next) {
      audio_next = 0;
      uLCD.printf("\nNext Song:\n");
      play_song();
      break;
    }else {
      if(!audio_stop) {
        int length = noteLength2[i];
        while(length--) {
            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j){
              queue2.call(playNote, song2[i]);
            }
            if(length < 1) wait(1.0);
            //uLCD.printf("yellow\n");
        }
      }else {
        uLCD.cls();
        while(audio_stop) {
          uLCD.printf("Stop!\n");
        }
        uLCD.cls();
        if(!audio_next)
          uLCD.printf("Resume!\n");
      }
    } 
  }
}

// void stop_play_songs(void) {
//   audio_stop = 1;
//   audio_next = 0;
//   gLED = 0;
//   wait(0.5);
//   audio.spk.pause();
//   gLED = 1;
//   wait(0.5);
// }

// void resume_song(void) {
//   audio_stop = 0;
//   audio_next = 0;
//   rLED = 0;
//   wait(0.5);
//   rLED = 1;
//   wait(0.5);
// }

void stop_resume_songs(void) {
  audio_next = 0;
  audio_stop = !audio_stop;
  if(audio_stop)
    audio.spk.pause();
  rLED = 0;
  wait(0.5);
  rLED = 1;
  wait(0.5);
}

void next_song(void) {
  audio_stop = 0;
  audio_next = 1;
  gLED = 0;
  wait(0.5);
  gLED = 1;
  wait(0.5);
}

int main(void) {
  gLED = 1;
  rLED = 1;
  t.start(callback(&queue, &EventQueue::dispatch_forever));
  t2.start(callback(&queue2, &EventQueue::dispatch_forever));
  t_song.start(play_song);

  button2.rise(queue.event(stop_resume_songs));
  button3.rise(queue.event(next_song));
}