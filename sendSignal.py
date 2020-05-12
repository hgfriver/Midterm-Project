import numpy as np
import serial
import time

waitTime = 0.1

# generate the song
Signals = np.array([
            ##Songs' Freq##

            #Twinkle Twinkle Little Star
            261, 261, 392, 392, 440, 440, 392, 349, 349, 330, 330, 294, 294, 261, 392, 392, 349, 349, 330, 330, 294, 392, 392, 349, 349, 330, 330, 294, 261, 261, 392, 392, 440, 440, 392, 349, 349, 330, 330, 294,           
            #Two-Tiger Run Fast
            524, 588, 660, 524, 524, 588, 660, 524, 660, 698, 784, 660, 698, 784, 784, 880, 784, 698, 660, 524, 784, 880, 784, 698, 660, 524, 588, 392, 524, 588, 392, 524, 524, 588, 660, 524, 524, 588, 660, 524,                
            #Little Bee
            392, 330, 330, 349, 294, 294, 262, 294, 300, 349, 392, 392, 392, 392, 330, 330, 349, 294, 294, 262, 330, 392, 392, 330, 392, 330, 330, 349, 294, 294, 262, 294, 300, 349, 392, 392, 392, 392, 330, 330,                   
            
            ##Songs' voice Length##

            #Twinkle Twinkle Little Star
            1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,                     
            #Two-Tiger Run Fast
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,                        
            #Little Bee
            1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2                          
          ])

# output formatter
formatter = lambda x: "%.3f" % x

# send the song to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
print("Sending songs ...")
print("It may take about %d seconds ..." % (int(np.size(Signals) * waitTime)))

for data in Signals:
  s.write(bytes(formatter(data/1000), 'UTF-8'))
  time.sleep(waitTime)
s.close()
print("Songs sended")