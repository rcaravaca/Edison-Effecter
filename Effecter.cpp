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
void ecualizer (float Buffer);
//****************************************************************

float lowpass_coef[] = {0.0002, 0.0002, 0.0002, 0.0003, 0.0003, 0.0004, 0.0004, 0.0004, 0.0004, 0.0004, 0.0004, 0.0003
						, 0.0002, 0.0001,-0.0001,-0.0003,-0.0006,-0.0009,-0.0013,-0.0017,-0.0022,-0.0027,-0.0032,-0.0038
						,-0.0043,-0.0049,-0.0054,-0.0059,-0.0063,-0.0066,-0.0067,-0.0068,-0.0067,-0.0064,-0.0059,-0.0052
						,-0.0042,-0.0031,-0.0017, 0.0000, 0.0019, 0.0040, 0.0063, 0.0088, 0.0115, 0.0143, 0.0171, 0.0201
						, 0.0231, 0.0260, 0.0289, 0.0317, 0.0343, 0.0367, 0.0389, 0.0408, 0.0424, 0.0437, 0.0446, 0.0452
						, 0.0454, 0.0452, 0.0446, 0.0437, 0.0424, 0.0408, 0.0389, 0.0367, 0.0343, 0.0317, 0.0289, 0.0260
						, 0.0231, 0.0201, 0.0171, 0.0143, 0.0115, 0.0088, 0.0063, 0.0040, 0.0019, 0.0000,-0.0017,-0.0031
						,-0.0042,-0.0052,-0.0059,-0.0064,-0.0067,-0.0068,-0.0067,-0.0066,-0.0063,-0.0059,-0.0054,-0.0049
						,-0.0043,-0.0038,-0.0032,-0.0027,-0.0022,-0.0017,-0.0013,-0.0009,-0.0006,-0.0003,-0.0001, 0.0001
						, 0.0002, 0.0003, 0.0004, 0.0004, 0.0004, 0.0004, 0.0004, 0.0004, 0.0003, 0.0003, 0.0002, 0.0002
						, 0.0002};

float bandpass_coef[] = {-0.0007,-0.0008,-0.0006,-0.0004,-0.0001,-0.0000,-0.0002,-0.0006,-0.0011,-0.0015,-0.0015,-0.0010
						,-0.0002, 0.0006, 0.0010, 0.0007,-0.0001,-0.0010,-0.0014,-0.0008, 0.0008, 0.0031, 0.0050, 0.0058
						, 0.0049, 0.0029, 0.0008, 0.0000, 0.0013, 0.0046, 0.0085, 0.0112, 0.0110, 0.0074, 0.0016,-0.0038
						,-0.0064,-0.0048, 0.0003, 0.0060, 0.0087, 0.0053,-0.0042,-0.0171,-0.0284,-0.0330,-0.0288,-0.0175
						,-0.0054,-0.0000,-0.0073,-0.0279,-0.0558,-0.0795,-0.0862,-0.0666,-0.0198, 0.0453, 0.1126, 0.1629
						, 0.1816, 0.1629, 0.1126, 0.0453,-0.0198,-0.0666,-0.0862,-0.0795,-0.0558,-0.0279,-0.0073,-0.0000
						,-0.0054,-0.0175,-0.0288,-0.0330,-0.0284,-0.0171,-0.0042, 0.0053, 0.0087, 0.0060, 0.0003,-0.0048
						,-0.0064,-0.0038, 0.0016, 0.0074, 0.0110, 0.0112, 0.0085, 0.0046, 0.0013, 0.0000, 0.0008, 0.0029
						, 0.0049, 0.0058, 0.0050, 0.0031, 0.0008,-0.0008,-0.0014,-0.0010,-0.0001, 0.0007, 0.0010, 0.0006
						,-0.0002,-0.0010,-0.0015,-0.0015,-0.0011,-0.0006,-0.0002,-0.0000,-0.0001,-0.0004,-0.0006,-0.0008
						,-0.0007};

float highpass_coef[] = {0.0008,-0.0000, 0.0006,-0.0005,-0.0001,-0.0007,-0.0004, 0.0001, 0.0001, 0.0014,-0.0000, 0.0013
						,-0.0015, 0.0000,-0.0024,-0.0001,-0.0004, 0.0013, 0.0026, 0.0006, 0.0024,-0.0038, 0.0002,-0.0062
						, 0.0014,-0.0020, 0.0053, 0.0035, 0.0031, 0.0024,-0.0068,-0.0010,-0.0123, 0.0045,-0.0050, 0.0144
						, 0.0029, 0.0102,-0.0019,-0.0094,-0.0069,-0.0198, 0.0086,-0.0082, 0.0324, 0.0001, 0.0267,-0.0170
						,-0.0100,-0.0287,-0.0288, 0.0122,-0.0065, 0.0816,-0.0040, 0.0878,-0.0951,-0.0080,-0.2445,-0.1163
						, 0.6798,-0.1163,-0.2445,-0.0080,-0.0951, 0.0878,-0.0040, 0.0816,-0.0065, 0.0122,-0.0288,-0.0287
						,-0.0100,-0.0170, 0.0267, 0.0001, 0.0324,-0.0082, 0.0086,-0.0198,-0.0069,-0.0094,-0.0019, 0.0102
						, 0.0029, 0.0144,-0.0050, 0.0045,-0.0123,-0.0010,-0.0068, 0.0024, 0.0031, 0.0035, 0.0053,-0.0020
						, 0.0014,-0.0062, 0.0002,-0.0038, 0.0024, 0.0006, 0.0026, 0.0013,-0.0004,-0.0001,-0.0024, 0.0000
						,-0.0015, 0.0013,-0.0000, 0.0014, 0.0001, 0.0001,-0.0004,-0.0007,-0.0001,-0.0005, 0.0006,-0.0000
						, 0.0008};
	


	float low_pass=0.0;
	float band_pass=0.0;
	float high_pass=0.0;


	float Buffer_low[121]; //Buffer para filtro pasa bajas
	float Buffer_band[121]; //Buffer para filtro pasa banda
	float Buffer_high[121]; //Buffer para filtro pasa altas
	unsigned int order=120;
	bool run=0;
	
	float scale_factor=22.0373;
	short normalize=32767;
	short gain=1;
	
int main(int argc, char *argv[])

{
//	cout<<lowpass_coef[0]<<"\n";
	for (int a=0;a<121;a++) {
		Buffer_low[a]=0.0;
		Buffer_band[a]=0.0;
		Buffer_high[a]=0.0;
		//Normalizacion de coeficientes a 2^32
		//lowpass_coef[a]=lowpass_coef[a];
		//bandpass_coef[a]=bandpass_coef[a];	
		//highpass_coef[a]=highpass_coef[a];
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
short Bff;

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
		
		if (data[0]==0)
			vol = data[1]/9.0;	

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
			Buffer=Buffer/normalize;
			//Bff=Buffer*normalize;

			ecualizer (Buffer);

			Bff=(low_pass)*normalize*100;

			//printf("Muestra: %i Buffer: %f, low_pass: %f, Bff: %i\n",buf[0],Buffer, low_pass,Bff);

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
			

			Bff=Bff*vol;

			//printf("Muestra: %i, Bff: %i  -->> ",buf[0],Bff);
			
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
//int a,b,c=0;
void ecualizer (float Buffer_sample) {
	
		for(i=0;i<(order-1);i++) {
				
				Buffer_low[i]=Buffer_low[i+1];
				/*Buffer_band[i]=Buffer_band[i+1];
				Buffer_high[i]=Buffer_high[i+1];*/
				low_pass=low_pass+(Buffer_low[i]*lowpass_coef[i]);
				/*band_pass=band_pass+(Buffer_band[i]*bandpass_coef[i]);
				high_pass=high_pass+(Buffer_high[i]*highpass_coef[i]);*/
		
			}
			
/*			Buffer_high[order]=Buffer;
			Buffer_band[order]=Buffer;*/
			Buffer_low[order]=Buffer_sample;

			low_pass=low_pass+(Buffer_low[order]*lowpass_coef[order]);
	/*		band_pass=band_pass+(Buffer_band[order]*bandpass_coef[order]);
			high_pass=high_pass+(Buffer_high[order]*highpass_coef[order]);*/
	
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

