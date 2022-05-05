#include "MD08A.h"

#include "driver/mcpwm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "modbus_params.h" // for modbus parameters structures

static int PWM;
static int IN1;
static int IN2;

void MD08A(int pwma, int ina1, int ina2)
{
	PWM = pwma;
	IN1 = ina1;
	IN2 = ina2;
}

void initMotor(void)
{
	gpio_reset_pin(IN1);
	gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
	gpio_set_level(IN1, 0);
	
	gpio_reset_pin(IN2);
	gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
	gpio_set_level(IN2, 0);

	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PWM);

	mcpwm_config_t pwm_config = {
		.frequency = 1000,
		.cmpr_a = 0,
		.duty_mode = MCPWM_DUTY_MODE_0,
		.counter_mode = MCPWM_UP_COUNTER
	};
	
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

	xTaskCreate(runMotor, "setMotor", 2048, NULL, 1, NULL);
}

void setSpeedDir(float duty, int dir)
{
	mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);

	if (dir == CW)
	{
		gpio_set_level(IN1, 1);
		gpio_set_level(IN2, 0);
	}
	else
	{
		gpio_set_level(IN1, 0);
		gpio_set_level(IN2, 1);
	}
}

void stopMotor(void)
{
	gpio_set_level(IN1, 0);
	gpio_set_level(IN2, 0);
	mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0f);
}

void runMotor(void *pvParameters)
{
	float duty = 0.0f;

	while (1)
	{
		if (coil_reg_params.coils_port0)
		{
			duty = holding_reg_params.holding_data1;

			if (duty > 100.0f)
			{
				duty = 100.0f;
			}

			if (duty < 0.0f)
			{
				duty = 0.0f;
			}

			setSpeedDir(duty, CW);
		}
		else
		{
			stopMotor();
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}