#include <iostream>
using namespace std;

#include "mraa.h"
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <csignal>
#include <stdint.h>
#include <pthread.h>


#define Frames 2
#define retardos 10
#define Norma 32767
unsigned int i;
snd_pcm_sframes_t err;
snd_pcm_sframes_t err_pb;
snd_pcm_sframes_t buffer_frames = Frames;

short buf[1];
short *buff=&buf[1];
unsigned int rate = 44100;

snd_pcm_t *capture_handle;
snd_pcm_t *playback_handle;

snd_pcm_hw_params_t *hw_params;
snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

float vol=0.5;
float low,mid,high=0.5;

char buffer_uart[3];
int data[2];

mraa_uart_context uart;
pthread_mutex_t audiomutex = PTHREAD_MUTEX_INITIALIZER;

void ini_caption_connect (char *Device);
void ini_playback_connect (char *Device);
void signalHandler (int a);

void *startaudio (void *a);
void getUartData ();

//****************************************************************
float low_p[]={0.0137,0.0297,0.0722,0.1261,0.1705,0.1875,0.1705,0.1261,0.0722,0.0297,0.0137};


float lowpass_coef[] = {0.0002,0.0002,0.0002,0.0003,0.0003,0.0004,0.0004,0.0004,0.0004,0.0004,0.0004,
	0.0003,0.0002,0.0001,-0.0001,-0.0003,-0.0005,-0.0009,-0.0012,-0.0017,-0.0021,-0.0026,-0.0032,-0.0037,
	-0.0043,-0.0049,-0.0054,-0.0059,-0.0063,-0.0066,-0.0068,-0.0068,-0.0067,-0.0064,-0.0059,-0.0052,-0.0043,
	-0.0031,-0.0017,-0.0001,0.0018,0.0039,0.0062,0.0087,0.0114,0.0142,0.0171,0.0201,0.0230,0.0260,0.0289,
	0.0317,0.0343,0.0367,0.0389,0.0408,0.0425,0.0438,0.0447,0.0453,0.0455,0.0453,0.0447,0.0438,0.0425,0.0408
	,0.0389,0.0367,0.0343,0.0317,0.0289,0.0260,0.0230,0.0201,0.0171,0.0142,0.0114,0.0087,0.0062,0.0039,0.0018,
	-0.0001,-0.0017,-0.0031,-0.0043,-0.0052,-0.0059,-0.0064,-0.0067,-0.0068,-0.0068,-0.0066,-0.0063,-0.0059,
	-0.0054,-0.0049,-0.0043,-0.0037,-0.0032,-0.0026,-0.0021,-0.0017,-0.0012,-0.0009,-0.0005,-0.0003,-0.0001,
	0.0001,0.0002,0.0003,0.0004,0.0004,0.0004,0.0004,0.0004,0.0004,0.0003,0.0003,0.0002,0.0002,0.0002};

float bandpass_coef[] = {0.000805653,2.17E-05,0.000586742,-0.000326508,-0.000242483,-0.000553366,-0.000626543,
	0.00026431,-0.000157609,0.001517224,-7.23E-05,0.00140686,-0.001325291,-0.000105624,-0.002097235,-0.000566397
	,-6.17E-18,0.000691512,0.00312539,0.000192035,0.002937157,-0.003795851,0.000236901,-0.006028693,0.000756237,
	-0.001523964,0.004313383,0.0045118,0.002318016,0.003615751,-0.007423164,-0.000308518,-0.01269083,0.004141514,
	-0.004883242,0.013392735,0.004061337,0.009130618,-3.22E-17,-0.010624267,-0.005501264,-0.02113749,0.00899265,
	-0.008915811,0.032017163,0.000914959,0.025976554,-0.014998855,-0.011461953,-0.02677317,-0.030969646,0.013373276,
	-0.008213429,0.082346489,-0.004155825,0.088003314,-0.093794579,-0.009015278,-0.242816154,-0.118621905,0.681395357,
	-0.118621905,-0.242816154,-0.009015278,-0.093794579,0.088003314,-0.004155825,0.082346489,-0.008213429,0.013373276,
	-0.030969646,-0.02677317,-0.011461953,-0.014998855,0.025976554,0.000914959,0.032017163,-0.008915811,0.00899265,
	0.02113749,-0.005501264,-0.010624267,-3.22E-17,0.009130618,0.004061337,0.013392735,-0.004883242,0.004141514,
	-0.01269083,-0.000308518,-0.007423164,0.003615751,0.002318016,0.0045118,0.004313383,-0.001523964,0.000756237,
	-0.006028693,0.000236901,-0.003795851,0.002937157,0.000192035,0.00312539,0.000691512,-6.17E-18,-0.000566397,
	-0.002097235,-0.000105624,-0.001325291,0.00140686,-7.23E-05,0.001517224,-0.000157609,0.00026431,-0.000626543,
	-0.000553366,-0.000242483,-0.000326508,0.000586742,2.17E-05,0.000805653};

float highpass_coef[] = {0.000152045,0.000197971,0.000241812,0.000283832,0.00032358,0.000359804,0.000390394,0.000412385,0.000422003,0.000414759,0.000385601,0.000329105,0.000239717,0.000112026,-5.89E-05,-0.000277376,-0.000546409,-0.000867745,-0.001241385,-0.001665357,-0.002135488,-0.002645238,-0.003185596,-0.003745054,-0.004309657,-0.004863142,-0.005387161,-0.00586159,-0.006264917,-0.006574706,-0.00676812,-0.006822504,-0.006715992,-0.00642815,-0.005940612,-0.005237699,-0.004307019,-0.003139991,-0.001732326,-8.44E-05,0.001798478,0.003905859,0.00622215,0.00872667,0.011393869,0.014193659,0.017091881,0.020050876,0.02303017,0.025987234,0.028878326,0.03165938,0.034286917,0.036718971,0.038915995,0.040841721,0.042463966,0.043755346,0.044693893,0.045263548,0.045454528,0.045263548,0.044693893,0.043755346,0.042463966,0.040841721,0.038915995,0.036718971,0.034286917,0.03165938,0.028878326,0.025987234,0.02303017,0.020050876,0.017091881,0.014193659,0.011393869,0.00872667,0.00622215,0.003905859,0.001798478,-8.44E-05,-0.001732326,-0.003139991,-0.004307019,-0.005237699,-0.005940612,-0.00642815,-0.006715992,-0.006822504,-0.00676812,-0.006574706,-0.006264917,-0.00586159,-0.005387161,-0.004863142,-0.004309657,-0.003745054,-0.003185596,-0.002645238,-0.002135488,-0.001665357,-0.001241385,-0.000867745,-0.000546409,-0.000277376,-5.89E-05,0.000112026,0.000239717,0.000329105,0.000385601,0.000414759,0.000422003,0.000412385,0.000390394,0.000359804,0.00032358,0.000283832,0.000241812,0.000197971,0.000152045};
	


	float low_pass;
	float band_pass;
	float high_pass;


	float Buffer_low[121]; //Buffer para filtro pasa bajas
	float Buffer_band[121]; //Buffer para filtro pasa banda
	float Buffer_high[121]; //Buffer para filtro pasa altas
	unsigned int order=120;
	bool run=0;
	
int main(int argc, char *argv[])

{
//	cout<<lowpass_coef[0]<<"\n";
	for (int a=0;a<121;a++) {
		Buffer_low[a]=0.0;
		Buffer_band[a]=0.0;
		Buffer_high[a]=0.0;
		//Normalizacion de coeficientes a 2^32
		lowpass_coef[a]=lowpass_coef[a]*22.0373;
		bandpass_coef[a]=bandpass_coef[a];	
		highpass_coef[a]=highpass_coef[a];
	}

	//cout<<lowpass_coef[0]<<"\n";
	pthread_t audiothread;

//	short *buffer=buf; 
	signal(SIGINT, signalHandler); //Catura de interrupcion

		// Open de connection with audio card argv[1]
	ini_caption_connect(argv[1]);
	ini_playback_connect(argv[1]);
	
        uart = mraa_uart_init(0);
	
	pthread_create(&audiothread, NULL, startaudio, NULL);	

	while(1){
		getUartData();
	}

	pthread_join(audiothread, NULL);	
	
	mraa_uart_stop(uart);
	mraa_deinit();
	snd_pcm_close (playback_handle);
	snd_pcm_close (capture_handle);

	return 0;

}

void getUartData (){
	
	if (uart == NULL) {
                printf("UART failed to setup\n");
	}
	
	signal(SIGINT, signalHandler); //Catura de interrupcion

        mraa_uart_read(uart, buffer_uart, sizeof(buffer_uart));
	
	data[0]=buffer_uart[1]-48;
	data[1]=buffer_uart[2]-48;
}

bool delay=false;
bool reverb=false;
bool overdrive=false;


void *startaudio (void *a){

float Buffer;
short Bff[1];
void *punt_buffer=&Bff;
//******** Parametro para efecto Delay
//int numSamples=retardos;
//Arreglo de muestras del tamano numSample
/*short channelData[retardos];
short delayData[retardos];	//Buffer de delay circular demuestras
int delayBufLength=retardos;	//logitud del buffer curcular
int dpr=0;
int dpw=0; //punteros de escritura y lectura para el buffer de delay
float dryMix_=1; // nivel se senal seca sin retardo
float wetMix_=0.3; // nive del retardo
float feedback_=0;//Nivelderealimentacion (0 si no hay retroalimentacion)
short in,out;
delayBufLength=retardos;*/
//int j=0;

	//for(unsigned int z=0;z<sizeof(delayData);z++)
	//	delayData[z]=0;

	while(1){
		if(data[0]==4 && data[1]==1)
			delay=true;			
		else if (data[0]==4 && data[1]==2)
			delay=false;

		if(data[0]==5 && data[1]==1)
			reverb=true;			
		else if (data[0]==5 && data[1]==2)
			reverb=false;

		if(data[0]==6 && data[1]==1)
			overdrive=true;			
		else if (data[0]==6 && data[1]==2)
			overdrive=false;
	

	//************** Control de ganancia para cada filtro **************//	
	/*	if (data[0]==1){
			low=data[1]/9.0;
			for(unsigned int g=0;g<120;g++){
				lowpass_coef[g]=lowpass_coef[g]*low;
			}
		}

		if (data[0]==2){
			mid=data[1]/9.0;
			for(unsigned int g=0;g<120;g++){
				bandpass_coef[g]=bandpass_coef[g]*mid;
			}
		}
		
		if (data[0]==3){
			high=data[1]/9.0;
			for(unsigned int g=0;g<120;g++){
				highpass_coef[g]=highpass_coef[g]*high;
			}
		}

*/
			

		
		if ((err = snd_pcm_readi (capture_handle, buf, buffer_frames)) != Frames) {	
		} else {


			Buffer=buf[0];
			Buffer=Buffer/32767;
			//printf("Muestra %i,Buffer Norm %f\n",buf[0],Buffer[0]);
			//////////////Buffer[0]=buf[0];
			//******* Pasa bajas ************************
			for(i=0;i<(order-1);i++) {
				
				Buffer_low[i]=Buffer_low[i+1];
				//Buffer_band[i]=Buffer_band[i+1];
				//Buffer_high[i]=Buffer_high[i+1];
				low_pass=low_pass+(Buffer_low[i]*lowpass_coef[i]);
			//	band_pass+=Buffer_band[i]*bandpass_coef[i];
			//	high_pass+=Buffer_high[i]*highpass_coef[i];
		
			}
			
			//Buffer_high[order]=Buffer;
		//	Buffer_band[order]=*Buffer;
			Buffer_low[order]=Buffer;

			low_pass=low_pass+(Buffer_low[order]*lowpass_coef[order]);
			//band_pass+=Buffer_band[order]*bandpass_coef[order];
			//high_pass+=Buffer_high[order]*highpass_coef[order];
			

//			cout<<" "<<Buffer[0]<<" "<<3*low_pass<<"\n";
			
			Bff[0]=low_pass*32767;
//			printf("Muestra: %i, lowpass, %f, Bff: %i\n",buf[0],low_pass,Bff[0]);
			//Buffer[0]=high_pass+band_pass+low_pass;
			// ****** EFECTO DELAY *************
		/*	channelData[j]=Buffer[0]; // Se llena el buffer con las muestras
			if (++j>=numSamples)
				j=0;

			if (0){
				
				for (int x = 0 ; x < numSamples ; x++ )
				{
					in = channelData[x];
					out = 0;

					out = dryMix_ * in + wetMix_ * delayData[dpr];

					delayData[dpw] = in + (delayData[dpr] * feedback_);

					if (++dpr >= delayBufLength)
						dpr = 0;
					if (++dpw >= delayBufLength)
						dpw = 0;

					channelData[x] = out;

					Buffer[0]=out;
				}
			}*/


//			printf("Data: %i Vol: %f\n",data[0],vol);
			
			

			/*if (overdrive==true && Buffer[0]<1000) {	
				Buffer[0]=1000;
				Buffer[0]*=1.3;	
			} else if (overdrive==true && Buffer[0]<-1000)	{					Buffer[0]=-1000;
				Buffer[0]*=1.3;
			}*/
			
		
			// ****** CONTROL DE VOLUMEN ******* 
			if (data[0]==0)
				vol = data[1]/9.0;

			Bff[0]=Bff[0]*vol;
			
			err_pb = snd_pcm_writei (playback_handle, &punt_buffer, buffer_frames);

			if (err_pb<0)				
				err_pb = snd_pcm_recover(playback_handle,err_pb,0);
			if (err_pb<0)
				printf("snd_pcm_writei failed: %s\n", snd_strerror(err_pb));
			if (err_pb > 0 && err_pb < (long)sizeof(buf))
			printf("Short write (expected %li, wrote %li)\n", (long)sizeof(buf), err_pb);
			
		}
		
	}
//return 0;	
}

void signalHandler(int a)
{
	
	run=true;		
	if (!snd_pcm_close(capture_handle)) {
		
		cout<<"\n -- Finishing processing --\n";
		//exit(0);
	}
	if (!snd_pcm_close(playback_handle)) {
		cout<<"\n -- Finishing processing --\n";
	//	exit(0);
	}

	exit(0);

}


	   
void ini_playback_connect (char *Device){

	if ((err_pb = snd_pcm_open(&playback_handle, Device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        	printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
		
	}
	
	if ((err_pb = snd_pcm_set_params(playback_handle,
					SND_PCM_FORMAT_S16_LE,
					SND_PCM_ACCESS_RW_INTERLEAVED,
					2,
					44100,
					1,
					0)) < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
		
	}

}


void ini_caption_connect (char *Device){

	if ((err = snd_pcm_open (&capture_handle, Device , SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		printf ("cannot open audio device %s (%s)\n", Device,snd_strerror (err));
		exit (1);

	}
	// **************************************************************


	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		printf ("cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
		printf ("cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		printf ("cannot set access type (%s)\n", snd_strerror (err));
		exit (1);
	}
				
	if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		printf ("cannot set sample format (%s)\n", snd_strerror (err));
		exit (1);
	}
	

	if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
		printf ("cannot set sample rate (%s)\n", snd_strerror (err));
		exit (1);
	}
		
	if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
		printf ("cannot set channel count (%s)\n", snd_strerror (err));
		exit (1);
	}


	if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
		printf ("cannot set parameters (%s)\n", snd_strerror (err));
		exit (1);
	}

	snd_pcm_hw_params_free (hw_params);
		
	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		printf ("cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		exit (1);
	}


}
