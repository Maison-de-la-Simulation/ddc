#pragma once

#include <vector>

#include "ddc/chunk.hpp"
#include "ddc/chunk_span.hpp"

class LocalDistribution;

template <
        class ElementType,
        class DomainType,
        class LayoutStridedPolicy = std::experimental::layout_right,
        class Allocator = std::allocator<ElementType>,
        class Distribution = LocalDistribution>
class DistributedField;

template <
        class ElementType,
        class... DDims,
        class LayoutStridedPolicy,
        class Allocator,
        class Distribution>
class DistributedField<
        ElementType,
        DiscreteDomain<DDims...>,
        LayoutStridedPolicy,
        Allocator,
        Distribution>
{
public:
    using element_type = ElementType;

    using domain_type = DiscreteDomain<DDims...>;

    using chunk_type = Chunk<ElementType, DiscreteDomain<DDims...>, Allocator>;

    using coord_type = typename domain_type::element_type;

    using allocator = Allocator;

    using local_chunk_span_type
            = ChunkSpan<ElementType, DiscreteDomain<DDims...>, LayoutStridedPolicy>;

    using local_chunk_view_type
            = ChunkSpan<const ElementType, DiscreteDomain<DDims...>, LayoutStridedPolicy>;

private:
    domain_type m_global_domain;

public:
    local_chunk_view_type local() const;

    local_chunk_span_type local();
};
