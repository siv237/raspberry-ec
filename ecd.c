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
float varVolt = 5,varProcess = 0.0001,Pc = 0.0,G = 1.0,P = 0.0,Xp = 0.0,Zp = 0.0,Xe = 0.0;

// Калибровка ЕС
float ec1= 0.01;    // Фактическое значение ЕС при нижнем пределе
float x1=16;        // Показания АЦП при нижнем значении ЕС
float ec2= 4;     // Фактическое значение ЕС при верхнем пределе
float x2=75;        // Показания АЦП при верхнем значении ЕС
float Ft=0;  // Минимальная частота опроса для снятия показаний кГц
void clrscr(void);


void clrscr(void)
{
char a[80];
printf("\033[2J"); /* Clear the entire screen. */
printf("\033[0;0f"); /* Move cursor to the top left hand corner
*/
}

float filter(float val) {  //функция фильтрации
  
  Pc = P + varProcess;
  G = Pc/(Pc + varVolt);
  P = (1-G)*Pc;
  Xp = Xe;
  Zp = Xp;
  Xe = G*(val-Zp)+Xp; // "фильтрованное" значение
 
  return(Xe);

}


// Функция калибровки
float fCalibration(float x1, float ec1, float x2, float ec2, float x){
  float b=(-log(ec1/ec2))/log(x2/x1);
  float a=((pow(x1,-b))*ec1); 
  float ec0 = a * pow(x,b);
  return ec0;
}




int main(int argc, char * argv[])
{

         int i;
         for( i = 0 ; i < argc; i++) {
         }
         if(argc == 1) {
                 printf("Default settings\n");
         }
         else
{
x1 = atof(argv[1]);
ec1 = atof(argv[2]);
x2 = atof(argv[3]);
ec2 = atof(argv[4]);
Ft = atof(argv[5]);
//ec1=argv[2];
//x2=argv[3];
//ec2=argv[4];

}



//                 printf("x1: %s\n", argv[1]);


    wiringPiSetup();
    pcf8591Setup(BASE,Address);
    float vlp,vlm,vl,fvl=0,n,nn,ec0;

while (1){
long t = millis();

    pinMode (12, OUTPUT) ;
    pinMode (13, OUTPUT) ;


n=0;
nn=0;
vlm=0;
vlp=0;
long j=0;
long ga=1200,gg=60;
    while(n<ga)

       {
        nn=0;

       
        digitalWrite (12, HIGH) ;
        vlp = vlp+analogRead(A3);
        digitalWrite (12, LOW) ;

        digitalWrite (13, HIGH) ;
        vlm = vlm+analogRead(A3);
        digitalWrite (13, LOW) ;
        n++;

        while (nn<gg)
            {

             digitalWrite (12, HIGH) ;
             digitalWrite (12, LOW) ;
             digitalWrite (13, HIGH) ;
             digitalWrite (13, LOW) ;
             nn++;

            }

       }
t=(millis()-t);
long tt=((ga*gg)+ga)*2;
    pinMode (12, INPUT) ;
    pinMode (13, INPUT) ;

float F=tt/(float)t;
if(F>Ft){

vlp=vlp/n;
vlm=255-(vlm/n);
vl=(vlp+vlm)/2;
 if(fvl==0){Xe=vl;}
fvl=filter(vl);

ec0=fCalibration(x1,ec1,x2,ec2,fvl);
clrscr();
printf("Middle: %3.3f\n",vl);
printf("Polarity: %3.3f\n",(vlp-vlm));
printf("Kalman: %3.3f\n",fvl);
printf("EC: %3.3f\n",ec0);
printf("Time: %dms\n",t);
printf("Count: %d\n",tt);
printf("Frequency: %3.3fkHz\n",F);
printf("Calibration x1=%3.2f, ec1=%3.2f, x2=%3.2f, ec2=%3.2f, Ft=%3.2f\n",x1,ec1,x2,ec2);

FILE *f = fopen("/run/shm/ecd", "wt");
fprintf(f,"Middle: %3.3f\n",vl);
fprintf(f,"Polarity: %3.3f\n",(vlp-vlm));
fprintf(f,"Kalman: %3.3f\n",fvl);
fprintf(f,"EC: %3.3f\n",ec0);
fprintf(f,"Time: %dms\n",t);
fprintf(f,"Count: %d\n",tt);
fprintf(f,"Frequency: %3.3fkHz\n",tt/(float)t);
fprintf(f,"Calibration: x1=%3.2f, ec1=%3.2f, x2=%3.2f, ec2=%3.2f, Ft=%3.2f\n",x1,ec1,x2,ec2,Ft);

fflush(f);

fclose(f); 
}
else
{
printf("Frequency: %3.3fkHz < %3.3fkHz\n",tt/(float)t,Ft);

}
}
}

