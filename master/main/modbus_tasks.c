#include "modbus.h"

// User operation function to read slave values and check alarm
void master_read(void *arg)
{
	const char *TAG = "master read";

	esp_err_t err = ESP_OK;

	float value = 0;
	float threshold = 750;
	float duty = 20;

	const mb_parameter_descriptor_t *param_descriptor = NULL;

	ESP_LOGI(TAG, "Start modbus client...");

	while (1)
	{
		// Read all found characteristics from slave(s)
		for (uint16_t cid = 0; (err != ESP_ERR_NOT_FOUND) && cid < MASTER_MAX_CIDS; cid++)
		{
			// Get data from parameters description table
			// and use this information to fill the characteristics description table
			// and having all required fields in just one table
			err = mbc_master_get_cid_info(cid, &param_descriptor);
			if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL))
			{
				void *temp_data_ptr = master_get_param_data(param_descriptor);
				assert(temp_data_ptr);
				uint8_t type = 0;

				if (param_descriptor->mb_param_type == MB_PARAM_INPUT)
				{
					err = mbc_master_get_parameter(cid, (char *)param_descriptor->param_key,
												   (uint8_t *)&value, &type);

					if (err == ESP_OK)
					{
						*(float *)temp_data_ptr = value;

						ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %f (0x%x) read successful.",
								param_descriptor->cid,
								(char *)param_descriptor->param_key,
								(char *)param_descriptor->param_units,
								value,
								*(uint32_t *)temp_data_ptr);
					}
					else
					{
						ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = %d (%s).",
								param_descriptor->cid,
								(char *)param_descriptor->param_key,
								(int)err,
								(char *)esp_err_to_name(err));
					}
				}
				else if (param_descriptor->mb_param_type == MB_PARAM_HOLDING)
				{
					if (param_descriptor->cid == CID_POWER_THRESHOLD)
					{
						*(float *)temp_data_ptr = threshold;
						err = mbc_master_set_parameter(cid, (char *)param_descriptor->param_key,
												   	   (uint8_t *)&threshold, &type);

						if (err == ESP_OK)
						{
							ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %f (0x%x) write successful.",
									param_descriptor->cid,
									(char *)param_descriptor->param_key,
									(char *)param_descriptor->param_units,
									threshold,
									*(uint32_t *)temp_data_ptr);
						}
						else
						{
							ESP_LOGE(TAG, "Characteristic #%d (%s) write fail, err = %d (%s).",
									param_descriptor->cid,
									(char *)param_descriptor->param_key,
									(int)err,
									(char *)esp_err_to_name(err));
						}
					}

					if (param_descriptor->cid == CID_DUTY_CYCLE)
					{
						duty += 2;

						if (duty > 40)
						{
							duty = 20;
						}

						*(float *)temp_data_ptr = duty;
						err = mbc_master_set_parameter(cid, (char *)param_descriptor->param_key,
												   	   (uint8_t *)&duty, &type);

						if (err == ESP_OK)
						{
							ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %f (0x%x) write successful.",
									param_descriptor->cid,
									(char *)param_descriptor->param_key,
									(char *)param_descriptor->param_units,
									duty,
									*(uint32_t *)temp_data_ptr);
						}
						else
						{
							ESP_LOGE(TAG, "Characteristic #%d (%s) write fail, err = %d (%s).",
									param_descriptor->cid,
									(char *)param_descriptor->param_key,
									(int)err,
									(char *)esp_err_to_name(err));
						}
					}
				}
				else if (param_descriptor->mb_param_type == MB_PARAM_COIL)
				{
					err = mbc_master_get_parameter(cid, (char *)param_descriptor->param_key,
												   (uint8_t *)&value, &type);

					if (err == ESP_OK)
					{
						*(float *)temp_data_ptr = value;
						uint16_t state = *(uint16_t *)temp_data_ptr;
						const char *rw_str = (state & param_descriptor->param_opts.opt1) ? "ON" : "OFF";
						ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %s (0x%x) read successful.",
								 param_descriptor->cid,
								 (char *)param_descriptor->param_key,
								 (char *)param_descriptor->param_units,
								 (const char *)rw_str,
								 *(uint16_t *)temp_data_ptr);
						// if (state & param_descriptor->param_opts.opt1)
						// {
						// 	alarm_state = true;
						// 	break;
						// }
					}
				}




				// err = mbc_master_get_parameter(cid, (char *)param_descriptor->param_key,
				// 							   (uint8_t *)&value, &type);
				// if (err == ESP_OK)
				// {
				// 	*(float *)temp_data_ptr = value;
				// 	if ((param_descriptor->mb_param_type == MB_PARAM_HOLDING) ||
				// 		(param_descriptor->mb_param_type == MB_PARAM_INPUT))
				// 	{
				// 		ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %f (0x%x) read successful.",
				// 				 param_descriptor->cid,
				// 				 (char *)param_descriptor->param_key,
				// 				 (char *)param_descriptor->param_units,
				// 				 value,
				// 				 *(uint32_t *)temp_data_ptr);
				// 		if (((value > param_descriptor->param_opts.max) ||
				// 			 (value < param_descriptor->param_opts.min)))
				// 		{
				// 			alarm_state = true;
				// 			break;
				// 		}
				// 	}
				// 	else
				// 	{
				// 		uint16_t state = *(uint16_t *)temp_data_ptr;
				// 		const char *rw_str = (state & param_descriptor->param_opts.opt1) ? "ON" : "OFF";
				// 		ESP_LOGI(TAG, "Characteristic #%d %s (%s) value = %s (0x%x) read successful.",
				// 				 param_descriptor->cid,
				// 				 (char *)param_descriptor->param_key,
				// 				 (char *)param_descriptor->param_units,
				// 				 (const char *)rw_str,
				// 				 *(uint16_t *)temp_data_ptr);
				// 		if (state & param_descriptor->param_opts.opt1)
				// 		{
				// 			alarm_state = true;
				// 			break;
				// 		}
				// 	}
				// }
				// else
				// {
				// 	ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = %d (%s).",
				// 			 param_descriptor->cid,
				// 			 (char *)param_descriptor->param_key,
				// 			 (int)err,
				// 			 (char *)esp_err_to_name(err));
				// }
				vTaskDelay(POLL_TIMEOUT_TICS); // timeout between polls
			}
		}
		vTaskDelay(UPDATE_CIDS_TIMEOUT_TICS);
	}
}