
#include <acl/allocators/coalescing_allocator.hpp>

namespace acl
{
#define ACL_BINARY_SEARCH_STEP2                                                                                        \
	do                                                                                                                   \
	{                                                                                                                    \
		const auto* const middle = it + (size >> 1);                                                                       \
		size										 = (size + 1) >> 1;                                                                        \
		it											 = *middle < key ? middle : it;                                                            \
	}                                                                                                                    \
	while (0)

static inline auto mini2(auto const* it, size_t size, auto key) noexcept
{
	do
	{
		ACL_BINARY_SEARCH_STEP2;
		ACL_BINARY_SEARCH_STEP2;
	}
	while (size > 2);
	it += size > 1 && (*it < key);
	it += size > 0 && (*it < key);
	return it;
}

#define ACL_BINARY_SEARCH_STEP_INDIRECT                                                                                \
	do                                                                                                                   \
	{                                                                                                                    \
		const auto* const middle = it + (size >> 1);                                                                       \
		size										 = (size + 1) >> 1;                                                                        \
		it											 = data[*middle] < key ? middle : it;                                                      \
	}                                                                                                                    \
	while (0)

static inline auto mini2(auto const* it, auto const* data, size_t size, auto key) noexcept
{
	do
	{
		ACL_BINARY_SEARCH_STEP_INDIRECT;
		ACL_BINARY_SEARCH_STEP_INDIRECT;
	}
	while (size > 2);
	it += size > 1 && (data[*it] < key);
	it += size > 0 && (data[*it] < key);
	return it;
}

coalescing_allocator::size_type coalescing_allocator::allocate(size_type size)
{
	// first fit
	for (uint32_t i = 0, end = static_cast<uint32_t>(sizes_.size()); i < end; ++i)
	{
		if (size <= sizes_[i])
		{
			auto ret = offsets_[i];
			sizes_[i] -= size;
			offsets_[i] += size;
			if (!sizes_[i])
			{
				offsets_.erase(i + offsets_.begin());
				sizes_.erase(i + sizes_.begin());
			}
			return ret;
		}
	}

	return std::numeric_limits<uint32_t>::max();
}

void coalescing_allocator::deallocate(size_type offset, size_type size)
{
	assert(!offsets_.empty());

	auto it = mini2(offsets_.data(), offsets_.size(), offset);

	if (it == offsets_.data())
	{
		if (offset + size == offsets_.front())
		{
			offsets_.front() = offset;
			sizes_.front() += size;
		}
		else
		{
			offsets_.insert(offsets_.begin(), offset);
			sizes_.insert(sizes_.begin(), size);
		}
	}
	else if (it == offsets_.data() + offsets_.size())
	{
		if (offsets_.back() + sizes_.back() == offset)
		{
			sizes_.back() += size;
		}
		// no merge, push at end
		else
		{
			offsets_.emplace_back(offset);
			sizes_.emplace_back(size);
		}
	}
	else
	{
		auto idx = std::distance((size_type const*)offsets_.data(), it);
		if (offsets_[idx - 1] + sizes_[idx - 1] == offset)
		{
			sizes_[idx - 1] += size;

			if (offsets_[idx] == offsets_[idx - 1] + sizes_[idx - 1])
			{
				sizes_[idx - 1] += sizes_[idx];
				offsets_.erase(offsets_.begin() + idx);
				sizes_.erase(sizes_.begin() + idx);
			}
		}
		else if (offsets_[idx] == offset + size)
		{
			offsets_[idx] -= size;
			sizes_[idx] += size;
		}
		else
		{
			offsets_.insert(offsets_.begin() + idx, offset);
			sizes_.insert(sizes_.begin() + idx, size);
		}
	}
}

} // namespace acl
