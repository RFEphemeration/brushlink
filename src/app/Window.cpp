#include "Window.h"

namespace Brushlink
{

/*
void ScaleNotQuiteNearest()
{
	float scale = 1.8f;
	constexpr float blend_cap = 0.9f;
	constexpr float blend_threshold = 0.1f;

	int dy = 0;
	float y_weight_remaining = 0.0f;
	for (int sy = 0, sy < sh; sy++)
	{
		float y_blend_weight = 1.0f - y_weight_remaining;
		if (y_blend_weight > blend_cap)
		{
			y_blend_weight = 1.0f;
		}
		if (y_blend_weight < blend_threshold)
		{
			y_blend_weight = 0.0f;
		}
		y_weight_remaining += scale;

		while (y_weight_remaining > 1.0f)
		{
			int dx = 0;
			float x_weight_remaining = 0.0f;

			for(int sx = 0; sx < sw; sx++)
			{
				float x_blend_weight = 1.0f - x_weight_remaining;
				if (x_blend_weight > blend_cap)
				{
					x_blend_weight = 1.0f;
				}
				if (x_blend_weight < blend_threshold)
				{
					x_blend_weight = 0.0f;
				}
				x_weight_remaining += scale;

				while(x_weight_remaining > 1.0f)
				{
					float x_prev = 1.0f - x_blend_weight;
					float y_prev = 1.0f - y_blend_weight;
					float x_curr = x_blend_weight;
					float y_curr = y_blend_weight;

					dest[dx][dy] = source[sx][sy] * (x_curr * y_curr);
					if (x_blend_weight < 1.0f)
					{
						dest[dx][dy] += source[sx-1][sy] * (x_prev * y_curr);
					}	
					if (y_blend_weight < 1.0f)
					{
						dest[dx][dy] += source[sx][sy-1] * (x_curr * y_prev);
					}
					if (x_blend_weight < 1.0f
						&& y_blend_weight < 1.0f)
					{
						dest[dx][dy] += source[sx-1][sy-1] * (x_prev * y_prev);
					}

					x_blend_weight = 1.0f;
					dx++;
					x_weight_remaining -= 1.0f;
				}
			}

			y_blend_weight = 1.0f;
			dy++;
			y_weight_remaining -= 1.0f;
		}
	}
}

void ScaleNotQuiteNearest()
{
	float scale = 1.8f;
	int sx = 0;
	float x_acc = 0.0f;
	for (int dx = 0; dx < dw; dx++)
	{
		float x_weight = 0.0f;
		if (dx > x_acc)
		{
			x_weight = x_acc - dx + 1.0f;
			x_acc += scale;
			sx++;
		}
		dest[dx][dy] = source[sx-1][sy] * x_weight
			+ source[sx][sy] * (1.0f - x_weight);
	}

	float scale = 1.8f;
	int dx = 10;
	int sx_left = dx / scale // this might be off by 1
	int sx_right = (dx + 1) / scale
	float left_weight = 0.0f;
	if (sx_left != sx_right)
	{
		left_weight = remainder(sx_left * scale, 1.0f);
	}
	float right_weight = 1.0f - left_weight;
	dest[dx][dy] = source[sx_left][sy] * left_weight
		+ source[sx_right][sy] * right_weight;
}
*/

void Window::PresentAndUpdate()
{
	// integer scale screen_buffer to screen
	int screen_scale = screen->w / screen_buffer->w;
	for (int sx = 0; sx < screen_buffer->w; sx++)
	{
		for (int sy = 0; sy < screen_buffer->h; sy++)
		{
			int dxmax = (sx + 1) * screen_scale;
			int dymax = (sy + 1) * screen_scale;
			for (int dx = sx * screen_scale; dx < dxmax; dx++)
			{
				for (int dy = sy * screen_scale; dy < dymax; dy++)
				{
					screen->pix[dx][dy] = screen_buffer[sx][sy];
				}
			}
		}
	}

	TSize window = tigrUpdate(screen.get());
	// ensure screen is bigger than window so we're only ever downscaling
	// there will be a 1 frame lag where we might upscale
	int w = screen->w;
	int h = screen->h;
	while (w < window->w || h < window->h)
	{
		w += settings.width;
		h += settings.height;
	}
	if (w != screen->w)
	{
		tigrResize(screen.get(), w, h);
	}
}

Dimensions Window::GetWorldPortion()
{
	int scale = screen->w / settings.width;
	return Dimensions{
		settings.world_portion.x * scale,
		settings.world_portion.y * scale,
		settings.world_portion.width * scale,
		settings.world_portion.height * scale,
	};
}

} // namespace Brushlink
