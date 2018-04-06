#include "imagebright.h"
#include <math.h>
namespace nim_comp
{
	void brightprocess(uint8_t *i420, uint32_t w, uint32_t h)
	{
		uint32_t data_size = w * h * 3 / 2;
		//处理数据
		uint8_t *buffer = i420;
		for (int i = 0; i < h; ++i)
		{
			for (int j = 0; j < w; ++j)
			{
				uint8_t y_temp = *buffer;
				if (y_temp >= 178)
				{
					uint32_t temp = 255 - y_temp;
					y_temp = 255 - temp / 2;
				}
				else
				if (y_temp >= 100)
				{
					uint32_t temp = 255 - y_temp;
					y_temp = 255 - temp * temp / 156;
				}
				else
				{
					y_temp = (uint32_t)(y_temp * y_temp / 100);
				}

				*buffer = y_temp;
				++buffer;
			}
		}
	}

	void brightprocess2(uint8_t*i420, uint32_t w, uint32_t h, uint32_t belt)
	{
		//？
		uint32_t data_size = w*h * 3 / 2;
		uint8_t *buffer = i420;
		for (int i = 0; i < data_size; i++)
		{
			*(buffer + i) = log(*(buffer + i)*(belt - 1) + 1) / log(4);
		}
	}
}