// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <type_traits>
#include <vector>

#include <Kokkos_Core.hpp>

#include "ddc/coordinate.hpp"
#include "ddc/detail/macros.hpp"
#include "ddc/discrete_coordinate.hpp"
#include "ddc/discrete_vector.hpp"
#include "ddc/discretization.hpp"

/// `NonUniformDiscretization` models a non-uniform discretization of the `CDim` segment \f$[a, b]\f$.
template <class CDim>
class NonUniformDiscretization
{
public:
    using continuous_dimension_type = CDim;

    using continuous_element_type = Coordinate<CDim>;


    using discrete_dimension_type = NonUniformDiscretization;

    using discrete_domain_type = DiscreteDomain<NonUniformDiscretization>;

    using discrete_element_type = DiscreteElement<NonUniformDiscretization>;

    using discrete_vector_type = DiscreteVector<NonUniformDiscretization>;

public:
    static constexpr std::size_t rank()
    {
        return 1;
    }

    template <class MemorySpace>
    class Impl
    {
        template <class OMemorySpace>
        friend class Impl;

        Kokkos::View<continuous_element_type*, MemorySpace> m_points;

    public:
        using discrete_dimension_type = NonUniformDiscretization<CDim>;

        Impl() = default;

        /// @brief Construct a `NonUniformDiscretization` using a brace-list, i.e. `NonUniformDiscretization mesh({0., 1.})`
        explicit Impl(std::initializer_list<continuous_element_type> points)
        {
            std::vector<continuous_element_type> host_points(points.begin(), points.end());
            Kokkos::View<continuous_element_type*, Kokkos::HostSpace>
                    host(host_points.data(), host_points.size());
            Kokkos::resize(m_points, host.extent(0));
            Kokkos::deep_copy(m_points, host);
        }

        /// @brief Construct a `NonUniformDiscretization` using a C++20 "common range".
        template <class InputRange>
        explicit inline constexpr Impl(InputRange const& points)
        {
            if constexpr (Kokkos::is_view<InputRange>::value) {
                Kokkos::deep_copy(m_points, points);
            } else {
                std::vector<continuous_element_type> host_points(points.begin(), points.end());
                Kokkos::View<continuous_element_type*, Kokkos::HostSpace>
                        host(host_points.data(), host_points.size());
                Kokkos::resize(m_points, host.extent(0));
                Kokkos::deep_copy(m_points, host);
            }
        }

        /// @brief Construct a `NonUniformDiscretization` using a pair of iterators.
        template <class InputIt>
        inline constexpr Impl(InputIt points_begin, InputIt points_end)
        {
            std::vector<continuous_element_type> host_points(points_begin, points_end);
            Kokkos::View<continuous_element_type*, Kokkos::HostSpace>
                    host(host_points.data(), host_points.size());
            Kokkos::resize(m_points, host.extent(0));
            Kokkos::deep_copy(m_points, host);
        }

        template <class OriginMemorySpace>
        explicit Impl(Impl<OriginMemorySpace> const& impl)
            : m_points(Kokkos::create_mirror_view_and_copy(MemorySpace(), impl.m_points))
        {
        }

        Impl(Impl const& x) = delete;

        Impl(Impl&& x) = default;

        ~Impl() = default;

        constexpr std::size_t size() const
        {
            return m_points.size();
        }

        /// @brief Convert a mesh index into a position in `CDim`
        constexpr continuous_element_type to_real(
                discrete_element_type const& icoord) const noexcept
        {
            return m_points(icoord.uid());
        }
    };
};

template <class>
struct is_non_uniform_discretization : public std::false_type
{
};

template <class CDim>
struct is_non_uniform_discretization<NonUniformDiscretization<CDim>> : public std::true_type
{
};

template <class DDim>
constexpr bool is_non_uniform_discretization_v = is_non_uniform_discretization<DDim>::value;

template <
        class DDimImpl,
        std::enable_if_t<
                is_non_uniform_discretization_v<typename DDimImpl::discrete_dimension_type>,
                int> = 0>
std::ostream& operator<<(std::ostream& out, DDimImpl const& mesh)
{
    return out << "NonUniformDiscretization(" << mesh.size() << ")";
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> to_real(
        DiscreteElement<NonUniformDiscretization<CDim>> const& c)
{
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
    return discrete_space_device<NonUniformDiscretization<CDim>>().to_real(c);
#else
    return discrete_space_host<NonUniformDiscretization<CDim>>().to_real(c);
#endif
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> distance_at_left(
        DiscreteElement<NonUniformDiscretization<CDim>> i)
{
    return to_real(i) - to_real(i - 1);
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> distance_at_right(
        DiscreteElement<NonUniformDiscretization<CDim>> i)
{
    return to_real(i + 1) - to_real(i);
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> rmin(DiscreteDomain<NonUniformDiscretization<CDim>> const& d)
{
    return to_real(d.front());
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> rmax(DiscreteDomain<NonUniformDiscretization<CDim>> const& d)
{
    return to_real(d.back());
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> rlength(
        DiscreteDomain<NonUniformDiscretization<CDim>> const& d)
{
    return rmax(d) - rmin(d);
}
