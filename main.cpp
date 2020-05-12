#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#define signalLength (240)
#define bufferLength (32)
#define song_size (40)

DA7212 audio;
int16_t waveform[kAudioTxBufferSize];

DigitalOut led1(LED1); //if in mode1 led1 is light
DigitalOut led2(LED2); //if in mode2 led2 is light

InterruptIn sw2(SW2); //stop current song and go into mode1
InterruptIn sw3(SW3); //confirm the song selected
Serial pc(USBTX, USBRX);
uLCD_4DGL uLCD(D1, D0, D2);

Thread threadDNN(osPriorityNormal, 120*1024);

void loadSignal(void);
void uLCDprint(void);
void mode_selection(void);
void confirm_selection(void);
int PredictGesture(float*);
void playNote(int freq);
void DNN(void);


int cur_song = 0; //song0 --> Twinkle Twinkle Little Star
                  //song1 --> Two Tiger Run Fast
                  //song2 --> Little Bee

int mode = 0; //mode0 stands for playing song's mode
              //mode1 stands for mode selection 
                                //gesture0 --> forward
                                //gesture1 --> Backward
                                //gesture2 --> song selection
              //mode2 stands for song selection mode

bool uLCD_cls = false; //To clear uLCD's screen

int song[120];
int noteLength[120];

void uLCDprint(void){
    if(uLCD_cls) {
      uLCD.cls();
      uLCD_cls = false;
    }

    if(mode == 2) {
      uLCD.locate(1, 1);
      uLCD.printf("\nSong Selection\n");
      if(cur_song == 0){
        uLCD.printf("\nTwinkle Twinkle \nLittle Star\n");
      }else if(cur_song == 1){
        uLCD.printf("\nTwo-Tiger Run Fast                       \n");
      }else if(cur_song == 2){ 
        uLCD.printf("\nLittle Bee                        \n");
      }else{
      }
    }else if(mode == 1){
      if(cur_song == 0){
        uLCD.locate(1, 1);
        uLCD.printf("\nTwinkle Twinkle \nLittle Star\n");
      }else if(cur_song == 1){
        uLCD.locate(1, 1);
        uLCD.printf("\nTwo-Tiger Run Fast                     \n");
      }else if(cur_song == 2){
        uLCD.locate(1, 1);
        uLCD.printf("\nLittle Bee                        \n");
      }else{
      }
    }else if(mode == 0){
      if(cur_song == 0) {
        uLCD.locate(1, 1);
        uLCD.printf("\nTwinkle Twinkle \nLittle Star\n");
      }else if(cur_song == 1) {
        uLCD.locate(1, 1);
        uLCD.printf("\nTwo-Tiger Run Fast                     \n");
      }else if(cur_song == 2) {
        uLCD.locate(1, 1);
        uLCD.printf("\nLittle Bee                           \n");
      }
    } 
}

void mode_selection(void){
  mode = 1;
  led1 = 0;
}

void confirm_selection(void){
  mode = 0;
  uLCD_cls = true;
  led1 = 1;
  led2 = 1;
}

int PredictGesture(float* output) {
  static int continuous_count = 0;
  static int last_predict = -1;
  int this_predict = -1;
  for (int i = 0; i < label_num; i++) {
    if (output[i] > 0.8) this_predict = i;
  }
  if (this_predict == -1) {
    continuous_count = 0;
    last_predict = label_num;
    return label_num;
  }
  if (last_predict == this_predict) {
    continuous_count += 1;
  } else {
    continuous_count = 0;
  }
  last_predict = this_predict;
  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
    return label_num;
  }
  continuous_count = 0;
  last_predict = -1;
  return this_predict;
}

void playNote(int freq) {
  for(int i = 0; i < kAudioTxBufferSize; i++){
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  audio.spk.play(waveform, kAudioTxBufferSize);
}

void DNN(void) {
  constexpr int kTensorArenaSize = 60 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];
  bool should_clear_buffer = false;
  bool got_data = false;
  int gesture_index;

  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;
  const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  static tflite::MicroOpResolver<6> micro_op_resolver;
  micro_op_resolver.AddBuiltin(
      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                               tflite::ops::micro::Register_MAX_POOL_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                               tflite::ops::micro::Register_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                               tflite::ops::micro::Register_FULLY_CONNECTED());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                               tflite::ops::micro::Register_SOFTMAX());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(), 1);
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  tflite::MicroInterpreter* interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  TfLiteTensor* model_input = interpreter->input(0);
  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] != config.seq_length) ||
      (model_input->dims->data[2] != kChannelNumber) ||
      (model_input->type != kTfLiteFloat32)) {
    error_reporter->Report("Bad input tensor parameters in model");
    return;
  }
  int input_length = model_input->bytes / sizeof(float);
  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
  if (setup_status != kTfLiteOk) {
    error_reporter->Report("Set up failed\n");
    return;
  }
  error_reporter->Report("Set up successful...\n");
  while (true) {
    got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                 input_length, should_clear_buffer);
    if (!got_data) {
      should_clear_buffer = false;
      continue;
    }
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      error_reporter->Report("Invoke failed on index: %d\n", begin_index);
      continue;
    }
    gesture_index = PredictGesture(interpreter->output(0)->data.f);
    should_clear_buffer = gesture_index < label_num;

    if (gesture_index < label_num) {
      error_reporter->Report(config.output_message[gesture_index]);
      
      //gesture0 --> right --> Forward
      //gesture1 --> left --> Backward
      //gesture2 --> up --> Go Into Song Selection Mode
      if(mode == 1) {
        if(gesture_index == 0) {
          if(cur_song == 2) cur_song = 0;  
          else  cur_song++;
        }else if(gesture_index == 1) {
          if(cur_song == 0) cur_song = 2;
          else  cur_song--;
        }else if(gesture_index == 2) {
          led1 = 1;
          led2 = 0;
          mode = 2;
          uLCD_cls = true;
        }else {
        }
      }else if(mode == 2) {
        led1 = 1;
        led2 = 0;
        if(gesture_index < 3)
          cur_song = gesture_index;
      }
    }
  }
}

void loadSignal(void) {
  int i = 0;
  audio.spk.pause();
  char serialInBuffer[bufferLength];
  int serialCount = 0;
  while(i < signalLength) {
    if(pc.readable()) {
      if(i < signalLength/2) {
        serialInBuffer[serialCount++] = pc.getc();
        if(serialCount == 5) {
          serialInBuffer[serialCount] = '\0';
          song[i++] = (int) 1000*(float) atof(serialInBuffer);
          serialCount = 0;
        }
      }else {
        serialInBuffer[serialCount++] = pc.getc();
        if(serialCount == 5) {
          serialInBuffer[serialCount] = '\0';
          noteLength[i-signalLength/2] = (int) 1000*((float) atof(serialInBuffer));
          serialCount = 0;
          i++;
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
  led1 = 1;
  led2 = 1;
  threadDNN.start(DNN);

  sw2.rise(mode_selection);
  sw3.rise(confirm_selection);

  uLCD.printf("Loading Songs...\n\n");
  loadSignal();
  uLCD.printf("Loading Finished!\n\n");
  wait(1);
  uLCD.cls();

  while(1){
    uLCDprint();
    for(int i = 0; i < song_size; i++){
      if(mode == 0){
        int length = noteLength[song_size * cur_song + i];
        while(length > 0){
        // the loop below will play the note for the duration of 1s
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j){
            playNote(song[song_size * cur_song + i]);
          }
          if(length == 1) {
            audio.spk.pause();
            wait(0.2);
            length--;
          }else {
            wait(0.1);
            audio.spk.pause();
            wait(0.3);
            length -= 2;
          }
        }
      }else{
        audio.spk.pause();
      }
    }
  }
}