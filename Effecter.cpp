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
#define N 5000
#define size 13230
#define cut 2000

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
inline void ecualizer (float *Buffer_sample);

short delay_array[size];
short delay_sample;
short old_sample;
short out_sample;
short indice_delay = 0; 
inline void delay_effect (short Buffer_sample);

short temp, latest_input, oldest_input;
short reverberation_array[N]; 
short indice = 0; 
inline void reverb_effect (short Buffer_sample);

short out_overdrive;
inline void overdrive_effect (short Buffer_sample);
//****************************************************************

float lowpass_coef[] = {0.000158150493402378,0.000203146636521130,0.000246006148032745,0.000286959726055848,
						0.000325520710665255,0.000360398765753770,0.000389454638704695,0.000409700858627987,
						.000417351528673317,0.000407922475405973,0.000376381003651445,0.000317342439877520,
						0.000225308606022526,9.49414246715240e-05,-7.86369100299283e-05,-0.000299527283315116,
						-0.000570731834460887,-0.000893839109707925,-0.00126872879641086,-0.00169330865610199,
						-0.00216329583999030,-0.00267205388746727,-0.00321049539208671,-0.00376705860120827,
						-0.00432776414064062,-0.00487635568315487,-0.00539452578027412,-0.00586222533038315,
						-0.00625805235003194,-0.00655971294060902,-0.00674454469183117,-0.00679009032738016,
						-0.00667470726224942,-0.00637819698383026,-0.00588243685678069,-0.00517199613960419,
						-0.00423471772793671,-0.00306224742855418,-0.00165049342423875,3.42044920840970e-18,
						0.00188377846506661,0.00399031482680235,0.00630396659173004,0.00880405903124571,
						0.0114650962082618,0.0142570985879028,0.0171460623217359,0.0200945317809403,
						0.0230622735749248,0.0260070372313430,0.0288853850270702,0.0316535712329368,0.0342684493401167,
						0.0366883847297381,0.0388741497687333,0.0407897784845601,0.0424033587900180,0.0436877416779971,
						0.0446211488464573,0.0451876627898930,0.0453775864321749,0.0451876627898930,0.0446211488464573,
						0.0436877416779971,0.0424033587900180,0.0407897784845601,0.0388741497687333,0.0366883847297381,
						0.0342684493401167,0.0316535712329368,0.0288853850270702,0.0260070372313430,0.0230622735749248,
						0.0200945317809403,0.0171460623217359,0.0142570985879028,0.0114650962082618,0.00880405903124571,
						0.00630396659173004,0.00399031482680235,0.00188377846506661,3.42044920840970e-18,-0.00165049342423875,
						-0.00306224742855418,-0.00423471772793671,-0.00517199613960419,-0.00588243685678069,-0.00637819698383026,
						-0.00667470726224942,-0.00679009032738016,-0.00674454469183117,-0.00655971294060902,-0.00625805235003194,
						-0.00586222533038315,-0.00539452578027412,-0.00487635568315487,-0.00432776414064062,-0.00376705860120827,
						-0.00321049539208671,-0.00267205388746727,-0.00216329583999030,-0.00169330865610199,-0.00126872879641086,
						-0.000893839109707925,-0.000570731834460887,-0.000299527283315116,-7.86369100299283e-05,9.49414246715240e-05,
						0.000225308606022526,0.000317342439877520,0.000376381003651445,0.000407922475405973,0.000417351528673317,
						0.000409700858627987,0.000389454638704695,0.000360398765753770,0.000325520710665255,0.000286959726055848,
						0.000246006148032745,0.000203146636521130,0.000158150493402378};

float bandpass_coef[] = {-0.000728165718434670,-0.000774957077315955,-0.000624085946278469,-0.000350336900400363,
						-9.23834873672577e-05,-2.10817682568121e-06,-0.000176326157667355,-0.000596549914583500,
						-0.00110962509490518,-0.00147299050760968,-0.00146042044321951,-0.000987556943752453,
						-0.000191789712954353,0.000591933362136340,0.000973257558765052,0.000714965907307002,
						-9.57293142915057e-05,-0.00102053311882357,-0.00143682977088641,-0.000846036441357672,
						0.000820938587391788,0.00307451339277655,0.00502218952739291,0.00577831492000430,
						0.00494795455151091,0.00292961427143155,0.000835755357763749,6.19067418154946e-06,
						0.00131011156207179,0.00458246432396391,0.00852508217588044,0.0112041362052027,
						0.0109729275345594,0.00739154049767492,0.00164893447999646,-0.00381693369908622,
						-0.00642353034517108,-0.00479769790021648,0.000299997801880775,0.00603473662564978,
						0.00866037342881648,0.00534567693112356,-0.00418708298018614,-0.0170910118217897,
						-0.0283511520358989,-0.0330151879232001,-0.0287596265415015,-0.0175442817126588,
						-0.00537775320788094,-4.08263786083719e-06,-0.00730466934663645,-0.0279383003053281,
						-0.0558395095980638,-0.0795476548285770,-0.0861717689112073,-0.0665962872577906,
						-0.0198413238983640,0.0453421180712153,0.112550447239066,0.162922920031642,0.181589681798144,
						0.162922920031642,0.112550447239066,0.0453421180712153,-0.0198413238983640,
						-0.0665962872577906,-0.0861717689112073,-0.0795476548285770,-0.0558395095980638,
						-0.0279383003053281,-0.00730466934663645,-4.08263786083719e-06,-0.00537775320788094,
						-0.0175442817126588,-0.0287596265415015,-0.0330151879232001,-0.0283511520358989,
						-0.0170910118217897,-0.00418708298018614,0.00534567693112356,0.00866037342881648,
						0.00603473662564978,0.000299997801880775,-0.00479769790021648,-0.00642353034517108,
						-0.00381693369908622,0.00164893447999646,0.00739154049767492,0.0109729275345594,
						0.0112041362052027,0.00852508217588044,0.00458246432396391,0.00131011156207179,
						6.19067418154946e-06,0.000835755357763749,0.00292961427143155,0.00494795455151091,
						0.00577831492000430,0.00502218952739291,0.00307451339277655,0.000820938587391788,
						-0.000846036441357672,-0.00143682977088641,-0.00102053311882357,-9.57293142915057e-05,
						0.000714965907307002,0.000973257558765052,0.000591933362136340,-0.000191789712954353,
						-0.000987556943752453,-0.00146042044321951,-0.00147299050760968,-0.00110962509490518,
						-0.000596549914583500,-0.000176326157667355,-2.10817682568121e-06,-9.23834873672577e-05,
						-0.000350336900400363,-0.000624085946278469,-0.000774957077315955,-0.000728165718434670};

float highpass_coef[] = {0.000812464009532233,-3.07263619456037e-05,0.000634820204731149,-0.000497511976500387,
							-0.000106466283396724,-0.000744106855948070,-0.000381420562142629,0.000112157738666025,
							8.24654857420347e-05,0.00138411015025249,-1.80551272464375e-05,0.00132831004255386,
							-0.00150000300836449,4.61414213976803e-05,-0.00238460512297172,-7.98050809607684e-05,
							-0.000352415539307788,0.00129488148297140,0.00263823604040302,0.000552742839866871,
							0.00244534833799160,-0.00376762200213912,0.000180844892116978,-0.00620267133029125,
							0.00139171017329729,-0.00198068357760335,0.00530122508822443,0.00354831559524886,
							0.00314200458734532,0.00240615305486079,-0.00684776479090993,-0.000962640838186077,
							-0.0122784283906065,0.00450049589933648,-0.00501234254375964,0.0143722476727868,
							0.00288592681335469,0.0102092271112083,-0.00186720446223361,-0.00940585823203265,
							-0.00693730280680261,-0.0197719140949866,0.00861406270906649,-0.00819365366986063,
							0.0323507817307754,0.000123568569671456,0.0267165770496624,-0.0169665667052176,
							-0.00998508145940009,-0.0286734775737952,-0.0287778931913621,0.0121814267783416,
							-0.00646527750371524,0.0816107544790844,-0.00404178971789214,0.0877939985889639,
							-0.0951027663947543,-0.00796444970953170,-0.244454347023407,-0.116254617952015,
							0.679824183133601,-0.116254617952015,-0.244454347023407,-0.00796444970953170,
							-0.0951027663947543,0.0877939985889639,-0.00404178971789214,0.0816107544790844,
							-0.00646527750371524,0.0121814267783416,-0.0287778931913621,-0.0286734775737952,
							-0.00998508145940009,-0.0169665667052176,0.0267165770496624,0.000123568569671456,
							0.0323507817307754,-0.00819365366986063,0.00861406270906649,-0.0197719140949866,
							-0.00693730280680261,-0.00940585823203265,-0.00186720446223361,0.0102092271112083,
							0.00288592681335469,0.0143722476727868,-0.00501234254375964,0.00450049589933648,
							-0.0122784283906065,-0.000962640838186077,-0.00684776479090993,0.00240615305486079,
							0.00314200458734532,0.00354831559524886,0.00530122508822443,-0.00198068357760335,
							0.00139171017329729,-0.00620267133029125,0.000180844892116978,-0.00376762200213912,
							0.00244534833799160,0.000552742839866871,0.00263823604040302,0.00129488148297140,
							-0.000352415539307788,-7.98050809607684e-05,-0.00238460512297172,4.61414213976803e-05,
							-0.00150000300836449,0.00132831004255386,-1.80551272464375e-05,0.00138411015025249,
							8.24654857420347e-05,0.000112157738666025,-0.000381420562142629,-0.000744106855948070,
							-0.000106466283396724,-0.000497511976500387,0.000634820204731149,-3.07263619456037e-05,
							0.000812464009532233};
	


	float low_pass=0.0;
	float band_pass=0.0;
	float high_pass=0.0;

	float low_p=0.0;
	float band_p=0.0;
	float high_p=0.0;

	float Buffer_low[121]; //Buffer para filtro pasa bajas
	float Buffer_band[121]; //Buffer para filtro pasa banda
	float Buffer_high[121]; //Buffer para filtro pasa altas
	unsigned int order=120;
	bool run=0;
	
	float scale_factor=22.0373;
	short normalize=32767;
	short gain_low=50;
	short gain_mid=25;
	short gain_high=25;
	
int main(int argc, char *argv[])

{
//	cout<<lowpass_coef[0]<<"\n";
	for (int a=0;a<121;a++) {
		Buffer_low[a]=0.0;
		Buffer_band[a]=0.0;
		Buffer_high[a]=0.0;
	}
	
	for (int a=0;a<N;a++) {
		reverberation_array[a]=0;
	}
	
	for (int a=0;a<size;a++) {
		delay_array[a]=0;
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

float Buffer,Buffer_from_filter;
short Bff;

void *punt_buffer=&Bff;

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
		else if (data[0]==1)
			gain_low = (100*data[1])/9.0;
		else if (data[0]==2)
			gain_mid = (100*data[1])/9.0;
		else if (data[0]==3)
			gain_high = (100*data[1])/9.0;
			
			//printf("data 0: %i, data 1: %i\n", data[0],data[1]);

		
		if ((err = snd_pcm_readi (capture_handle, buf, buffer_frames)) != Frames) {	
		} else {


			Buffer=buf[0];
			Buffer=Buffer/normalize;
			//Bff=Buffer*normalize;

			ecualizer (&Buffer);
	
			low_p=low_pass;
			band_p=band_pass;
			high_p=high_pass;

			Buffer_from_filter=(gain_low*low_p+gain_mid*band_p+gain_high*high_p);
			//Buffer_from_filter=Buffer;
			
			Bff=Buffer_from_filter*normalize;
			//printf("Muestra: %i Buffer: %f, Buffer_from_filter %f, Bff: %i\n",
				//buf[0],Buffer, Buffer_from_filter,Bff);

			// ****** EFECTO DELAY *************
			if (delay==true) {
				delay_effect(Bff);
				Bff+=delay_sample;
			
			// ****** EFECTO REVERB *************
			} 
			
			if (reverb==true) {
				reverb_effect(Bff);
				Bff=temp;
			}
			
			// ****** EFECTO OVERDRIVE *************
			if (overdrive==true) {
				overdrive_effect(Bff);
				Bff=out_overdrive;
			} 
			
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
	
}

inline void ecualizer (float *Buffer_sample) {
	
		for(i=(order-1);i>0;--i) {
				
				Buffer_low[i-1]=Buffer_low[i];
				Buffer_band[i-1]=Buffer_band[i];
				Buffer_high[i-1]=Buffer_high[i];
				low_pass=low_pass+(Buffer_low[i]*lowpass_coef[i]);
				band_pass=band_pass+(Buffer_band[i]*bandpass_coef[i]);
				high_pass=high_pass+(Buffer_high[i]*highpass_coef[i]);
		
			}
			
			Buffer_high[order]=*Buffer_sample;
			Buffer_band[order]=*Buffer_sample;
			Buffer_low[order]=*Buffer_sample;

			low_pass=low_pass+(Buffer_low[order]*lowpass_coef[order]);
			band_pass=band_pass+(Buffer_band[order]*bandpass_coef[order]);
			high_pass=high_pass+(Buffer_high[order]*highpass_coef[order]);
	
	}


inline void delay_effect (short Buffer_sample) {
	
	
	delay_sample = delay_array[indice_delay]; 
	delay_array[indice_delay] = Buffer_sample;

	if ( indice_delay < (size - 1) )
		indice_delay++;	
	else	
		indice_delay = 0;	 

}



inline void reverb_effect (short Buffer_sample) {

	latest_input=Buffer_sample;
	oldest_input=reverberation_array[indice]*0.7; 
	temp = latest_input + oldest_input;

	reverberation_array[indice] = temp; 

	if ( indice < (N-1)) 
		indice++;
	else
		indice = 0; 
}



inline void overdrive_effect (short Buffer_sample) {

	if (Buffer_sample>=cut) {	
		Buffer_sample=cut;
		Buffer_sample*=1.5;	
	} else if (Buffer_sample<=-cut)	{					
		Buffer_sample=-cut;
		Buffer_sample*=1.5;
	}
	
	out_overdrive=Buffer_sample;

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

