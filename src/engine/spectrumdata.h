#pragma once

namespace engine
{
	class SpectrumData
	{
	public:
		SpectrumData() : rate(0) {}

		SpectrumData(int rate, const std::vector<float> &data)
			:
			rate(rate),
			data(data)
		{
		}

		inline float getValue(float time)
		{
			int pos = int(time * rate);
			if (pos >= int(data.size())) pos = int(data.size()) - 1;

			float v1 = data[pos];
			if (pos == data.size() - 1) return v1;
			float v2 = data[pos + 1];

			float t = ((time * rate) - pos);

			return v1 + (v2 - v1) * t;
		}

	private:
		std::vector<float> data;
		int rate;
	};

	SpectrumData loadSpectrumData(const char *filename);
}