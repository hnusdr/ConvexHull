#include "file_work.h"
#include <vector>
#include <iostream>
#include <ctime>
#include <tuple>
#include "windows.h"
#include "assert.h"
#include "stb_image.h"
#include "point_loader.h"
#include "convex_hull.h"
#include "point_iterator.h"
#include "renderer.h"
#include "color.h"

namespace {
#ifdef PRINT_TIMING
	void PrintTimer(std::string what, clock_t begin)
	{
		clock_t end = clock();
		float diff = static_cast<float>(end) - static_cast<float>(begin);
		cout << what << " = " << diff << endl;
	}
#else
	void PrintTimer(std::string what, clock_t begin)
	{

	}
#endif

	template <typename PointIt>
	void RunFindHull(PointIt begin, PointIt end, std::vector<Point> &convex_points)
	{
		std::back_insert_iterator<std::vector<Point> > back_inserter(convex_points);
		//FindHull(PointIterator<PointLoader>(point_loader), PointIterator<PointLoader>(), back_inserter);
		FindHull(begin, end, back_inserter);
	}

	struct FileParameters
	{
		unsigned char *buffer;
		int width;
		int height; 
		int pixel_length; 
		int stride;
		RgbaColor rgba;
		double stroke_width;
	};

	template<typename Color>
	Color ConvertColor(RgbaColor color)
	{
		return Color(color.r, color.g, color.b, color.a);
	}

	template<typename Color>
	Color ConvertColor(GreyColor color)
	{
		return Color(color.r, color.g, color.b, color.a);
	}


	template <typename Color, typename PixelFormat>
	void DrawHullInFile(const FileParameters &fp)
	{
		typedef PointLoader<Color, Renderer<PixelFormat, Color> > loader_t;
		typedef PointIterator<loader_t, Point> point_iterator_t;

		//auto loader = LoaderFactory::Create(data_, x_pixels_, y_pixels_, pixel_length_, x_pixels_ * pixel_length_, rgba);

		Renderer<PixelFormat, Color> renderer(fp.buffer, fp.width, fp.height, fp.pixel_length, fp.stride);
		loader_t loader(fp.buffer, fp.width, fp.height, fp.pixel_length, &renderer);

		clock_t begin_clock = clock();
		clock_t total_begin = begin_clock;

		PrintTimer("Loading the file", begin_clock);

		std::vector<Point> convex_points;

		//begin_clock = clock();
		//PointIterator<PointLoader>(point_loader), PointIterator<PointLoader>()
		RunFindHull(point_iterator_t(loader), point_iterator_t(), convex_points);

		//Line line;
		if(convex_points.size() >= 2)
		{
			auto it = convex_points.begin();
			loader.MoveTo(*it);
			++ it;
			for(; it != convex_points.end(); ++ it)
			{
				loader.LineTo(*it);
			}
		}

		loader.Render(fp.stroke_width, ConvertColor<Color>(fp.rgba));

		PrintTimer("Total ticks", total_begin);
		std::cout << "Points = " << loader.get_point_count() << std::endl;
		size_t convex_size = convex_points.size();
		if(convex_size > 0)
			-- convex_size;
		std::cout << "Convex Points = " << convex_size << std::endl;
	}

}

void DrawHullInFile(const std::string &in_file, const std::string &out_file, double stroke_width, RgbaColor rgba)
{
	clock_t total_begin = clock();
	clock_t begin_clock;

	begin_clock = clock();

	// 8 bytes each pixel. First three bytes are for RGB. Example: 00 00 00 are for a black dot
	int x_pixels, y_pixels, pixel_length;
	unsigned char *data = stbi_load(in_file.c_str(), &x_pixels, &y_pixels, &pixel_length, 0);
	if(data == NULL)
	{
		const char *reason = stbi_failure_reason();
		OutputDebugString(reason);
		throw ChException(std::string("stbi error on loading the image:") + reason);
	}

	if(x_pixels == 0 || y_pixels == 0)
	{
		throw ChException("Image file has no pixels");
	}

	FileParameters fp = {data, x_pixels, y_pixels, pixel_length, x_pixels * pixel_length, rgba, stroke_width};

	switch(pixel_length)
	{
		case 1:
			DrawHullInFile<GreyColor, agg::pixfmt_gray8>(fp);
			break;
		case 2:
			DrawHullInFile<GreyColor, agg::pixfmt_gray16>(fp);
			break;
		case 3:						   
			DrawHullInFile<RgbaColor, agg::pixfmt_rgb24>(fp);
			break;
		case 4:						   
			DrawHullInFile<RgbaColor, agg::pixfmt_rgba32>(fp);
			break;
		default:
			throw ChException("Unknown pixel length");
	};

	int result = stbi_write_png(
		out_file.c_str(), 
		x_pixels, y_pixels, pixel_length, 
		data, x_pixels * pixel_length);

	if(!result)
		throw ChException("PointLoader::Save: Cannot save image.");

	stbi_image_free(data);

}