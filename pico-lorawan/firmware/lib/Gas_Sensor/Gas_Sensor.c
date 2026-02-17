#include "Gas_Sensor.h"

#include <math.h>





#define Conversion_factor 3.3/(4096)  

#define ratioInCleanair 9.83 	// ratio mq2 in clean air

#define Volt_resolution 3.3	// voltage supplied

#define RL 10  // resistence in KiloOhms





float getR0(float ad_value, float *Gas_pres)

{



	float sensor_voltage = ad_value * Conversion_factor;

	

	*Gas_pres= sensor_voltage;



	float RS_air; // variable for sensor resistence

	float R0;

	RS_air = ((Volt_resolution * RL)/sensor_voltage)- RL ; // calculate RS in fresh air

	

	if (RS_air < 0) {RS_air = 0;} // no negative value accepted

	

	R0 = RS_air/ratioInCleanair;  

	if (R0 < 0) {R0 = 0;} 

	

	DEV_Delay_ms(1000);

	
	return R0;

}


float Gas ( float a, float b, float ad_value, float R0)


{

	float sensor_voltage = ad_value * Conversion_factor;

	float RS_calc = ((Volt_resolution* RL)/sensor_voltage)- RL ; 


	if (RS_calc < 0) {RS_calc = 0;}

	

	float ratio = R0/RS_calc;


	if (ratio < 0) { ratio = 0;} 


	float PPM = a * pow(ratio, b);

		
	return PPM; 
}



void  Gas_Sensor(float *CO, float* Gas_pres)

{

 
    uint16_t value = adc_read();        

    DEV_Digital_Read(AOUT_PIN);		 

    float R0 = getR0 (value, Gas_pres);


    DEV_Delay_ms(1000);  

    *CO = Gas (36974, -3.109,value, R0);

	return;

}

