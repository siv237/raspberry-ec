#include <wiringPi.h>
#include <pcf8591.h>
#include <stdio.h>
#include <stdlib.h>
#define Address 0x48
#define BASE 64
#define A0 BASE+0
#define A1 BASE+1
#define A2 BASE+2
#define A3 BASE+3
#include <string.h>
#include <libconfig.h>
#include <math.h> 
double varVolt,varProcess,Pc,G,P,Xp,Zp,Xe;


///////////////////////////////////
//////        Функции        //////
///////////////////////////////////

//Получение температуры

float ds18b20(char* device)
{
int nString=0;
FILE *file;
char arr[80];
char *chksum;
char path[80];
strcpy(path,"/sys/bus/w1/devices/28-");
strcat(path, device);
strcat(path, "/w1_slave");

file = fopen(path, "rt");
while (fgets (arr, 80, file) != NULL)
	{
	nString++;
	if(nString == 1){

			if(strstr(arr,"YES") != NULL)
				{
				chksum = "TRUE";
				}
				else
				{
				chksum = "FALSE";
				}
			}

	if(nString == 2){
		if(chksum = "TRUE")
				{
				fclose(file);
  				strtok (arr,"=");
				return atof(strtok (NULL, "="))/1000;
				}
			}
	}
}



// Функция калибровки
float fCalibration(float x1, float ec1, float x2, float ec2, float x){
  float b=(-log(ec1/ec2))/log(x2/x1);
  float a=((pow(x1,-b))*ec1); 
  float ec0 = a * pow(x,b);
  return ec0;
}


// Функция фильтрации
float filter(float val) { 
  Pc = P + varProcess;
  G = Pc/(Pc + varVolt);
  P = (1-G)*Pc;
  Xp = Xe;
  Zp = Xp;
  Xe = G*(val-Zp)+Xp; // "фильтрованное" значение
  return(Xe);

}

// Функция замера
long fa,fb,fc,fd;
float fMetering(int d1, int d2, int a, long maxr){
float f;

    pinMode (d1, OUTPUT) ;
    pinMode (d2, OUTPUT) ;

long r=0;
fa=0;
fb=0;
while (r<maxr){
r++;
   digitalWrite (d1, HIGH) ;
   fa=fa+analogRead(64+a);
   digitalWrite (d1, LOW) ;

   digitalWrite (d2, HIGH) ;
   fb=fb+analogRead(64+a);
   digitalWrite (d2, LOW) ;
}


    pinMode (d1, INPUT) ;
    pinMode (d2, INPUT) ;

float dac=(255-((float)fa/r))+((float)fb/r)/2;
float polarity=(255-((float)fa/r))-((float)fb/r);
//float kalman=filter(dac);
return dac;
}


void clrscr(void)
{
char a[80];
printf("\033[2J"); /* Clear the entire screen. */
printf("\033[0;0f"); /* Move cursor to the top left hand corner
*/
}


///////////////////////////////////////////////////////////////////
int main(){
double x1,x2,ec1,ec2,tk;
int d1,d2,a;
long cont;
char *tdev;

    wiringPiSetup();
    pcf8591Setup(BASE,Address);
// Получение переменных из конига

printf("Read config /etc/ecd.conf...\n");


while (1){

  config_t cfg;
  config_init(&cfg);

  if (config_read_file(&cfg, "/etc/ecd.conf"))
	{
	// Для колибровки
	config_lookup_float(&cfg, "x1", &x1);
	config_lookup_float(&cfg, "x2", &x2);
	config_lookup_float(&cfg, "ec1", &ec1);
	config_lookup_float(&cfg, "ec2", &ec2);
	config_lookup_float(&cfg, "tk", &tk);
	config_lookup_string(&cfg, "tdev", &tdev);
	//Параметры опроса
	config_lookup_int(&cfg, "d1", &d1);
	config_lookup_int(&cfg, "d2", &d2);
	config_lookup_int(&cfg, "a", &a);
	config_lookup_int(&cfg, "cont", &cont);

	// Для фильтрации
	if(!varVolt)config_lookup_float(&cfg, "varVolt", &varVolt);
	if(!varProcess)config_lookup_float(&cfg, "varProcess", &varProcess);
	if(!Pc)config_lookup_float(&cfg, "Pc", &Pc);
	if(!G)config_lookup_float(&cfg, "G", &G);
	if(!P)config_lookup_float(&cfg, "P", &P);
	if(!Xp)config_lookup_float(&cfg, "Xp", &Xp);
	if(!Zp)config_lookup_float(&cfg, "Zp", &Zp);
	//if(!Xe)config_lookup_float(&cfg, "Xe", &Xe);
	if(!Xe){Xe=fMetering(d1,d2,a,cont);printf("Xe:%3.3f\n",Xe);}


	}


float temper=ds18b20(tdev);
float dac=fMetering(d1,d2,a,cont);
float kdac=filter(dac);
float ec0=fCalibration(x1,ec1,x2,ec2,kdac);
float ec=ec0/(1-tk*(temper-25));
printf("Temper:%3.3f, dac:%3.3f, kdac:%3.3f, ec0:%3.3f, ec:%3.3f\n",temper,dac,kdac,ec0,ec);

FILE *f = fopen("/run/shm/ecd", "wt");
fprintf(f,"Middle: %3.3f\n",dac);
fprintf(f,"Kalman: %3.3f\n",kdac);
fprintf(f,"EC0: %3.3f\n",ec0);
fprintf(f,"EC: %3.3f\n",ec);
fprintf(f,"Temperature: %3.3f\n",temper);

fflush(f);

fclose(f); 


//printf("%3.3f\n",fCalibration(x1,ec1,x2,ec2,dac));
}
return 0;
}


