#pragma once

template<typename T, typename I = std::string>
class ResourceProxy {
public:
	virtual ~ResourceProxy() {
		std::map<T*, int>::iterator iter;
		for (iter = ref_count.begin(); iter != ref_count.end(); ++iter) {
			T *obj    = iter->first;
			int count = iter->second;
			for (int i = 0; i < count; ++i) release(obj);
		}
	}

	T* get(I filename) {
		T* obj = filename_map_static[filename];

		if (obj && ref_count_static[obj] > 0) add_ref(obj);
		else {
			try {
				obj = load(filename);
			} catch (FatalException &e) {
				throw FatalException(std::string("Failed to load ") + filename + std::string("\n") + std::string(e.what()));
			}

			if (obj) {
				filename_map_static[filename] = obj;
				add_ref(obj);
			}
		}
		return obj;
	}

	void release(T* obj) {
		assert(ref_count[obj] > 0);
		assert(ref_count_static[obj] > 0);
		--ref_count[obj];
		if (0 == --ref_count_static[obj]) delete obj;
	}

	void add_ref(T* obj) {
		ref_count[obj]++;
		ref_count_static[obj]++;
	}

private:
	virtual T* load(I) = 0;
	std::map<T*, int> ref_count;
	static std::map<T*, int> ref_count_static;
	static std::map<I, T*> filename_map_static;
};
