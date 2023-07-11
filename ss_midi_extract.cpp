// SS+MIDI 

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#define pi 3.1415926535897932384626433832795
#define Frame_Size	800		/////fre.bin number of each frame
#define DFT_point 800
#define Sampling Rate 8000
#define MaxFrequency 4000

double match(double *midi);

short * Read_File(int * length)
{
	short * Source;
	FILE* fr;

	if((fr = fopen("C:\\Users\\inhye\\Desktop\\sssnr5.snd", "rb")) == NULL){
		printf("file not found\n");
		return 0;
	}

	fseek(fr,0,2);
	*length = ftell(fr)/sizeof(short);
	fseek(fr,0,0);
	Source = (short * )malloc(sizeof(short)**length);
	fread((short*)(Source), sizeof(short),*length,fr);

	fclose(fr);

	return Source;
}

double * Genarate_HW()
{
	int i;
	double temp;
	double * window = (double *) malloc (sizeof(double)*Frame_Size);
	////////////////// Create Hamming Window //////////
	for(i=0;i<Frame_Size;i++)
	{
		temp = 0.54 - 0.46*cos(2*pi*i/Frame_Size);
		window[i] = temp/1.08;
	}
	return window;
}

int main()
{
	int i,j,k;
	int length;
	double index;
	double max;
	double f0;
	double midinote;
	double matching_rate;
	
	short * Source;  
	double * Hamming_window;
	double * win_sig; 
	double * nwin_sig;
	
	double * Y_real_value;
	double * Y_image_value;
	double * Y_phase;
	double * Y_power;
	
	double * D_real_value;
	double * D_image_value;
	double * D_power;
	
	double * Noise; //for noise

	double * frame1;
	double * frame2;
	double * frame3;
	double * frame4;
	double * frame5;
	
	double * X; // noisy - noise
	
	Source = Read_File(&length);
	Hamming_window = Genarate_HW();
	
	int midi_num = length/Frame_Size; //midi number
	double midi[160];
	
	win_sig = (double *)malloc(sizeof(double)*Frame_Size);
	nwin_sig = (double *)malloc(sizeof(double)*Frame_Size);
	X = (double *)malloc(sizeof(double)*Frame_Size);
	
	Y_real_value = (double *)malloc(sizeof(double)*Frame_Size);
	Y_image_value = (double *)malloc(sizeof(double)*Frame_Size);
	Y_phase = (double *)malloc(sizeof(double)*Frame_Size);
	Y_power = (double *)malloc(sizeof(double)*Frame_Size);	
	
	D_real_value = (double *)malloc(sizeof(double)*Frame_Size); 
	D_image_value = (double *)malloc(sizeof(double)*Frame_Size); 
	D_power = (double *)malloc(sizeof(double)*Frame_Size);
	Noise = (double *)malloc(sizeof(double)*(Frame_Size));

	//noise = 초기 5 frame//
	frame1 = (double *)malloc(sizeof(double)*(Frame_Size));
	frame2 = (double *)malloc(sizeof(double)*(Frame_Size));
	frame3 = (double *)malloc(sizeof(double)*(Frame_Size));
	frame4 = (double *)malloc(sizeof(double)*(Frame_Size));
	frame5 = (double *)malloc(sizeof(double)*(Frame_Size));
 
	//noise extract
	for(i=0;i<Frame_Size;i++){
		frame1[i]=Source[i];
		frame2[i]=Source[i+Frame_Size];
		frame3[i]=Source[i+Frame_Size*2];
		frame4[i]=Source[i+Frame_Size*3];
		frame5[i]=Source[i+Frame_Size*4];
	}
	//Noise = 5 frames avg
	for (i=0;i<Frame_Size;i++){
		Noise[i]=(frame1[i]+frame2[i]+frame3[i]+frame4[i]+frame5[i])/5;  
	}
		
	//noise DFT	
	for(j=0;j<DFT_point;j++){
		D_real_value[j] = D_image_value[j] = 0;
	}
		
	for(j=0;j<DFT_point;j++)
	{
		for(k=0;k<DFT_point;k++)
		{
			D_real_value[j] += Noise[k]*cos(2*pi*j*k/DFT_point);
			D_image_value[j]-= Noise[k]*sin(2*pi*j*k/DFT_point);
		}
	D_power[j] =sqrt(D_real_value[j]*D_real_value[j] + D_image_value[j]*D_image_value[j]); // abs(D)
	}			
	
	
	//Frame by frame
	for(i=40000;i<length;i+=Frame_Size)
	{
		// windowing //
		for(j=0;j<Frame_Size;j++){
			if (i+j<length){
				win_sig[j] = Source[i+j]*Hamming_window[j];
			}
			else{	// 마지막 Frame에서 source의 length를 초과할 경우에
				win_sig[j] = 0;
			}
		}

		// DFT //
		for(j=0;j<DFT_point;j++)
			Y_real_value[j] = Y_image_value[j] = Y_power[j] = 0;
		
		for(j=0;j<DFT_point;j++)
		{
			for(k=0;k<DFT_point;k++)
			{
				Y_real_value[j] += win_sig[k]*cos(2*pi*j*k/DFT_point);
				Y_image_value[j]-= win_sig[k]*sin(2*pi*j*k/DFT_point);
			}
			// abs(Source)
			Y_power[j] =sqrt(Y_real_value[j]*Y_real_value[j] + Y_image_value[j]*Y_image_value[j]);	
			//Y_phase[j] =atan2(Y_image_value[j],Y_real_value[j]);	
		}		
		
		//초기화 	
		for(j=0;j<Frame_Size;j++){
			X[j]=0;
		}
	
		for(j=0;j<Frame_Size;j++){
			X[j]=fabs(Y_power[j]-D_power[j]);
		}
		
		//peak picking
		max = 0;
		index = 0;
		midinote = 0;
		
		for(k=0;k<Frame_Size/2;k++)
		{ 
			if(X[k] > max){
				max = X[k];
				index = k;			
			}
		}
		
		f0 = ((index * MaxFrequency)/ Frame_Size);  
		
		midinote = round(69+12*log2(f0/440));
		
		int num = (i-40000)/Frame_Size;  //Frame number
	    midi[num] = midinote;
	}
	
	//error check
	int num2 = (length-40000)/Frame_Size;  
	for(i=1;i<(num2)-1;i++)
	{
		double bdiff = fabs(midi[i]-midi[i-1]);
		double ndiff = fabs(midi[i+1]-midi[i]);
		if((bdiff>5) && (ndiff>5)){    //미와 도 사이가 4지만 두 음의 각 오차값 1정도를 생각해서 6 
			midi[i] = 0;
			if(midi[i-1]==midi[i+1]){
				midi[i] = midi[i-1];
			}
		}
		else if((bdiff>5) && (ndiff<=5)){
			midi[i] = midi[i+1];
		}
		else if((bdiff<=5) && (ndiff>5)){
			midi[i] = midi[i-1];
		}
	
	}
	
	//humming pedding
	for(i=num2;i<160;i++){
		midi[i] = 0;   	//묵음 
	}
	for(i=0;i<160;i++){
		printf("%lf\n",midi[i]);
	}
	
	//matching_rate
	matching_rate = match(midi);
	printf("MATCHING RATE : %lf",matching_rate);
	
	free(Source);
	free(Hamming_window);
	free(win_sig);
	free(Y_real_value);
	free(Y_image_value);	
	free(Y_power);
	    	
	return 0;
}

double match(double *midi)
{
	int i;
	int song[160];   //32개의 음,timestep = 5 
	double error;
	double diff;
	double matching_rate;

	for(i=0;i<10;i++){
		song[i] = 46;   //솔솔 
	}
	for(i=10;i<20;i++){
		song[i] = 48;   //라라 
	}
	for(i=20;i<30;i++){
		song[i] = 46;   //솔솔 
	}
	for(i=30;i<40;i++){
		song[i] = 43;    //미미 
	}
	for(i=40;i<50;i++){
		song[i] = 46;    //솔솔 
	}
	for(i=50;i<60;i++){
		song[i] = 43;    //미미 
	}
	for(i=60;i<75;i++){
		song[i] = 42;    //레레레 
	}
	for(i=75;i<80;i++){
		song[i] = 0;     //묵음 
	}
	for(i=80;i<90;i++){
		song[i] = 46;    //솔솔 
	}
	for(i=90;i<100;i++){
		song[i] = 48;    //라라 
	}
	for(i=100;i<110;i++){
		song[i] = 46;    //솔솔 
	}
	for(i=110;i<120;i++){
		song[i] = 43;    //미미 
	}
	for(i=120;i<125;i++){
		song[i] = 46;		//솔 
	}
	for(i=125;i<130;i++){
		song[i] = 43;		//미 
	}
	for(i=130;i<135;i++){
		song[i] = 42;		//레 
	}
	for(i=135;i<140;i++){
		song[i] = 43;		//미 
	}
	for(i=140;i<150;i++){
		song[i] = 39;		//도도 
	}
	for(i=150;i<160;i++){
		song[i] = 0;   	//묵음 
	}

	error = 0;
	diff = 0;
	for(i=0;i<75;i++){
		diff = fabs(midi[i]-song[i]);
		if(diff>2){   
			error++;
		}
		diff = 0;
	}
	for(i=75;i<80;i++){                     //묵음 부분 따로 채점  
		if((midi[i]>39) && (midi[i]<49)){   
			error++;
		}
		diff = 0;		
	}
	for(i=80;i<160;i++){
		diff = fabs(midi[i]-song[i]);
		if(diff>2){  
			error++;
		}
		diff = 0;
	}
	
	printf("%lf\n",error);
	matching_rate = round(((160 - error)/160)*100);   

	return matching_rate;
} 

