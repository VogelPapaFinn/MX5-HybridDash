#pragma once

// C++ includes
#include <algorithm>
#include <array>
#include <cstddef>

template <class NumericalValue, size_t Slots>
class MedianFilter
{
public:
	MedianFilter() = default;

	MedianFilter(const NumericalValue& defaultValue) { ringBuffer_.fill(defaultValue); }

	void fill(const NumericalValue& defaultValue) { ringBuffer_.fill(defaultValue); }

	void add(const NumericalValue& value)
	{
		ringBuffer_[bufferIndex_] = value;
		bufferIndex_++;

		if (bufferIndex_ >= ringBuffer_.size()) {
			bufferIndex_ = 0;
		}
	}

	NumericalValue get() const
	{
		auto ringBufferCopy = ringBuffer_;

		std::nth_element(ringBufferCopy.begin(), ringBufferCopy.begin() + (Slots / 2), ringBufferCopy.end());

		return ringBufferCopy[Slots / 2];
	}

private:
	std::array<NumericalValue, Slots> ringBuffer_{};

	static_assert(Slots % 2 == 1, "The MedianFilter<> only supports odd sizes");

	size_t bufferIndex_ = 0;
};
