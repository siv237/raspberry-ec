#include <wiringPi.h>
#include <pcf8591.h>
#include <stdio.h>
#include <stdlib.h>
#define Address 0x48
#define BASE 64
#include <string.h>
#include <libconfig.h>
#include <math.h> 
#include <time.h>

double varVolt,varProcess,Pc,G,P,Xp,Zp,Xe;
struct tm *u;
char s1[40] = { 0 }, s2[40] = { 0 };
float polarity;
long fa,fb;
double fa0,fb0;
long t,r,rr;

///////////////////////////////////
//////        Функции        //////
///////////////////////////////////

// Получение температуры

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
float fMetering(int d1, int d2, int a, long maxr){

float f;
    pinMode (d1, OUTPUT) ;
    pinMode (d2, OUTPUT) ;

fa=0;
fb=0;
t = millis();

r=0;
while (r<=maxr){
r++;
   digitalWrite (d1, LOW) ;   digitalWrite (d2, HIGH);
   digitalWrite (d1, HIGH);   digitalWrite (d2, LOW);
   fa=fa+analogRead(64+a);
}

r=0;
while (r<=maxr){
r++;
   digitalWrite (d2, LOW); digitalWrite (d1, HIGH);
   digitalWrite (d2, HIGH); digitalWrite (d1, LOW) ;
   fb=fb+analogRead(64+a);
}

t=millis()-t;

    pinMode (d1, INPUT);
    pinMode (d2, INPUT);

fa0=255-((float)fa/maxr);
fb0=(float)fb/maxr;
float dac=(fa0+fb0)/2;
polarity=fa0-fb0;


return dac;
}



///////////////////////////////////////////////////////////////////
int main(){
double x1,x2,ec1,ec2,tk;
int d1,d2,a;
long cont,wait;
char *tdev,*log;

    wiringPiSetup();
    pcf8591Setup(BASE,Address);
// Получение переменных из конига

printf("Read config /etc/ecd.conf...\n");


while (1){

  config_t cfg;
  config_init(&cfg);

  if (config_read_file(&cfg, "/etc/ecd.conf"))
	{
	// Для калибровки
	config_lookup_float(&cfg, "x1", &x1);
	config_lookup_float(&cfg, "x2", &x2);
	config_lookup_float(&cfg, "ec1", &ec1);
	config_lookup_float(&cfg, "ec2", &ec2);
	config_lookup_float(&cfg, "tk", &tk);
	config_lookup_string(&cfg, "tdev", &tdev);
	config_lookup_string(&cfg, "log", &log);
	//Параметры опроса
	config_lookup_int(&cfg, "d1", &d1);
	config_lookup_int(&cfg, "d2", &d2);
	config_lookup_int(&cfg, "a", &a);
	config_lookup_int(&cfg, "cont", &cont);
	config_lookup_int(&cfg, "wait", &wait);
	// Для фильтрации
	if(!varVolt)config_lookup_float(&cfg, "varVolt", &varVolt);
	if(!varProcess)config_lookup_float(&cfg, "varProcess", &varProcess);
	if(!Pc)config_lookup_float(&cfg, "Pc", &Pc);
	if(!G)config_lookup_float(&cfg, "G", &G);
	if(!P)config_lookup_float(&cfg, "P", &P);
	if(!Xp)config_lookup_float(&cfg, "Xp", &Xp);
	if(!Zp)config_lookup_float(&cfg, "Zp", &Zp);
	if(!Xe)config_lookup_float(&cfg, "Xe", &Xe);
//	if(!Xe){delay(5000);Xe=fMetering(d1,d2,a,5000);printf("Xe:%3.3f\n",Xe);}

	}


float temper=ds18b20(tdev);
float dac=fMetering(d1,d2,a,cont);
float kdac=filter(dac);
if (fabs((kdac-dac)/dac)*100>20){Xe=dac;kdac=dac;}

float ec0=fCalibration(x1,ec1,x2,ec2,kdac);
float ec=ec0/(1+tk*(temper-25));
//printf("Temper:%3.3f, dac:%3.3f, kdac:%3.3f, ec0:%3.3f, ec:%3.3f\n",temper,dac,kdac,ec0,ec);

// Фиксация времени
const time_t timer = time(NULL);
u = localtime(&timer);
strftime(s1, 80, "%Y/%m/%d %H:%M:%S", u);


FILE *f1 = fopen("/run/shm/ecd", "wt");
fprintf(f1,"Time: %s\n",s1);
fprintf(f1,"Middle: %3.3f\n",dac);
fprintf(f1,"Kalman: %3.3f\n",kdac);
fprintf(f1,"EC0: %3.3f\n",ec0);
fprintf(f1,"EC: %3.3f\n",ec);
fprintf(f1,"Temperature: %3.3f\n",temper);
fprintf(f1,"t: %d\n",t);
fprintf(f1,"fa: %3.3f\n",fa0);
fprintf(f1,"fb: %3.3f\n",fb0);
fprintf(f1,"fa-fb: %3.3f\n",fa0-fb0);
fprintf(f1,"Freq: %3.3f\n",(float)cont*2/t);
fflush(f1);
fclose(f1); 

// Запись в лог
FILE *f2 = fopen(log, "at");
fprintf(f2,"%s;%3.3f;%3.3f;%3.3f;%3.3f;%3.3f;%3.3f;%3.3f;%3.3f;%3.3f\n",s1,dac,polarity,kdac,ec0,ec,temper,255-(float)fa/cont,(float)fb/cont,(float)cont*2/t);
fflush(f2);
fclose(f2); 


//printf("%3.3f\n",fCalibration(x1,ec1,x2,ec2,dac));

delay(wait*1000);
}
return 0;
}


