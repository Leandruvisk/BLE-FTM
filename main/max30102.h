#ifndef MAX30102_H
#define MAX30102_H

// Define the I2C address for MAX30102 sensor
#define I2C_ADDR_MAX30102 0x57


void max30102_init(void);
void max30102_start();
void read_max30102 () ;
void max30102_task (void *pvParameters);

#endif // MAX30102_H