#ifndef __MD08A_H__
#define __MD08A_H__

enum MotorDir
{
	CW = 0,
	CCW
};

void MD08A(int pwma, int ina1, int ina2);
void initMotor(void);
void setSpeedDir(float duty, int dir);
void stopMotor(void);

void runMotor(void *pvParameters);

#endif