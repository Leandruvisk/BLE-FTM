#ifndef MAX30102_H
#define MAX30102_H

// Define the I2C address for MAX30102 sensor
#define I2C_ADDR_MAX30102 0x57

extern float heartrate;
extern float pctspo2;
extern int irpower, rpower, lirpower, lrpower;
extern float meastime;

void max30102_init(void);
void read_max30102 () ;
void max30102_task (void *pvParameters);


#endif // MAX30102_H