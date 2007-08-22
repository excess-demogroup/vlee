#include "stdafx.h"
#include "spectrumdata.h"
#include "../core/fatalexception.h"

engine::SpectrumData engine::loadSpectrumData(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (NULL == fp) throw core::FatalException("failed to load spectrum data");

	int rate;
	fread(&rate, 4, 1, fp);
	
	std::vector<float> d;
	while (FALSE == feof(fp))
	{
		float val;
		fread(&val, 4, 1, fp);
//		printf("%f\n", val);
		d.push_back(val);
	}
	fclose(fp);

	return engine::SpectrumData(rate, d);
}
