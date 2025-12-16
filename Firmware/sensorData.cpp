#include <Arduino.h>
#include <arduinoFFT.h>

int SAMPLES = 128;
int SAMPLING_FREQUENCY = 200;

double t = 0;


float vReal[SAMPLES];
float vImage[SAMPLES];

ArduinoFFT<float> FFT(vReal, vImage, SAMPLES, SAMPLING_FREQUENCY);

//example loop

void loop(){
    for(int i = 0; i<SAMPLES; i++){
        vReal[i] = noisySine();
        vImag[i] = 0.0;

        delay(100/SAMPLING_FREQUENCY);
    }
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
    float peakFreq = 0, peakMag = 0;
    FFT.majorPeak(&peakFreq, &peakMag);

}