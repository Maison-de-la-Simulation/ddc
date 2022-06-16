// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <type_traits>

#include <Kokkos_Core.hpp>

#include "ddc/coordinate.hpp"
#include "ddc/detail/macros.hpp"
#include "ddc/discrete_coordinate.hpp"
#include "ddc/discrete_domain.hpp"
#include "ddc/discrete_vector.hpp"
#include "ddc/discretization.hpp"

/** UniformDiscretization models a uniform discretization of the provided continuous dimension
 */
template <class CDim>
class UniformDiscretization
{
public:
    using continuous_dimension_type = CDim;

    using continuous_element_type = Coordinate<CDim>;


    using discrete_dimension_type = UniformDiscretization;

    using discrete_element_type = DiscreteElement<UniformDiscretization>;

    using discrete_domain_type = DiscreteDomain<UniformDiscretization>;

    using discrete_vector_type = DiscreteVector<UniformDiscretization>;

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

    private:
        continuous_element_type m_origin {0.};

        continuous_element_type m_step {1.};

    public:
        using discrete_dimension_type = UniformDiscretization;

        Impl() = default;

        Impl(Impl const&) = delete;

        template <class OriginMemorySpace>
        explicit Impl(Impl<OriginMemorySpace> const& impl)
        {
            m_origin = impl.m_origin;
            m_step = impl.m_step;
        }

        Impl(Impl&&) = default;

        /** @brief Construct a `Impl` from a point and a spacing step.
     *
     * @param origin the real coordinate of mesh coordinate 0
     * @param step   the real distance between two points of mesh distance 1
     */
        constexpr Impl(continuous_element_type origin, continuous_element_type step)
            : m_origin(origin)
            , m_step(step)
        {
            assert(step > 0);
        }

        /** @brief Construct a `Impl` from a segment \f$[a, b] \subset [a, +\infty[\f$ and a number of points `n`.
     *
     * @param a the coordinate of a first real point (will have mesh coordinate 0)
     * @param b the coordinate of the second real point (will have mesh coordinate `n-1`)
     * @param n the number of points to map the segment \f$[a, b]\f$ including a & b
     * 
     * @deprecated use the version accepting a vector for n instead
     */
        [[deprecated(
                "Use the version accepting a vector for n "
                "instead.")]] constexpr Impl(continuous_element_type a, continuous_element_type b, std::size_t n)
            : m_origin(a)
            , m_step((b - a) / (n - 1))
        {
            assert(a < b);
            assert(n > 1);
        }

        /** @brief Construct a `Impl` from a segment \f$[a, b] \subset [a, +\infty[\f$ and a number of points `n`.
     * 
     * @param a the coordinate of a first real point (will have mesh coordinate 0)
     * @param b the coordinate of the second real point (will have mesh coordinate `n-1`)
     * @param n the number of points to map the segment \f$[a, b]\f$ including a & b
     */
        constexpr Impl(continuous_element_type a, continuous_element_type b, discrete_vector_type n)
            : m_origin(a)
            , m_step((b - a) / (n - 1))
        {
            assert(a < b);
            assert(n > 1);
        }

        ~Impl() = default;

        /// @brief Lower bound index of the mesh
        constexpr continuous_element_type origin() const noexcept
        {
            return m_origin;
        }

        /// @brief Lower bound index of the mesh
        constexpr discrete_element_type front() const noexcept
        {
            return discrete_element_type {0};
        }

        /// @brief Spacing step of the mesh
        constexpr continuous_element_type step() const
        {
            return m_step;
        }

        /// @brief Convert a mesh index into a position in `CDim`
        constexpr continuous_element_type to_real(
                discrete_element_type const& icoord) const noexcept
        {
            return m_origin + continuous_element_type(icoord.uid()) * m_step;
        }
    };

    /** Construct a Impl<Kokkos::HostSpace> and associated discrete_domain_type from a segment
     *  \f$[a, b] \subset [a, +\infty[\f$ and a number of points `n`.
     *
     * @param a coordinate of the first point of the domain
     * @param b coordinate of the last point of the domain
     * @param n number of points to map on the segment \f$[a, b]\f$ including a & b
     */
    static std::tuple<Impl<Kokkos::HostSpace>, discrete_domain_type> init(
            continuous_element_type a,
            continuous_element_type b,
            discrete_vector_type n)
    {
        assert(a < b);
        assert(n > 1);
        Impl<Kokkos::HostSpace> disc(a, continuous_element_type {(b - a) / (n - 1)});
        discrete_domain_type domain {disc.front(), n};
        return std::make_tuple(std::move(disc), std::move(domain));
    }

    /** Construct a uniform `DiscreteDomain` from a segment \f$[a, b] \subset [a, +\infty[\f$ and a
     *  number of points `n`.
     *
     * @param a coordinate of the first point of the domain
     * @param b coordinate of the last point of the domain
     * @param n the number of points to map the segment \f$[a, b]\f$ including a & b
     * @param n_ghosts_before number of additional "ghost" points before the segment
     * @param n_ghosts_after number of additional "ghost" points after the segment
     */
    static std::tuple<
            Impl<Kokkos::HostSpace>,
            discrete_domain_type,
            discrete_domain_type,
            discrete_domain_type,
            discrete_domain_type>
    init_ghosted(
            continuous_element_type a,
            continuous_element_type b,
            discrete_vector_type n,
            discrete_vector_type n_ghosts_before,
            discrete_vector_type n_ghosts_after)
    {
        using continuous_element_type = continuous_element_type;
        using discrete_domain_type = discrete_domain_type;
        assert(a < b);
        assert(n > 1);
        continuous_element_type discretization_step {(b - a) / (n - 1)};
        Impl<Kokkos::HostSpace>
                disc(a - n_ghosts_before.value() * discretization_step, discretization_step);
        discrete_domain_type ghosted_domain
                = discrete_domain_type(disc.front(), n + n_ghosts_before + n_ghosts_after);
        discrete_domain_type pre_ghost
                = discrete_domain_type(ghosted_domain.front(), n_ghosts_before);
        discrete_domain_type main_domain
                = discrete_domain_type(ghosted_domain.front() + n_ghosts_before, n);
        discrete_domain_type post_ghost
                = discrete_domain_type(main_domain.back() + 1, n_ghosts_after);
        return std::make_tuple(
                std::move(disc),
                std::move(main_domain),
                std::move(ghosted_domain),
                std::move(pre_ghost),
                std::move(post_ghost));
    }

    /** Construct a uniform `DiscreteDomain` from a segment \f$[a, b] \subset [a, +\infty[\f$ and a
     *  number of points `n`.
     *
     * @param a coordinate of the first point of the domain
     * @param b coordinate of the last point of the domain
     * @param n the number of points to map the segment \f$[a, b]\f$ including a & b
     * @param n_ghosts number of additional "ghost" points before and after the segment
     */
    static std::tuple<
            Impl<Kokkos::HostSpace>,
            discrete_domain_type,
            discrete_domain_type,
            discrete_domain_type,
            discrete_domain_type>
    init_ghosted(
            continuous_element_type a,
            continuous_element_type b,
            discrete_vector_type n,
            discrete_vector_type n_ghosts)
    {
        return init_ghosted(a, b, n, n_ghosts, n_ghosts);
    }
};

template <class>
struct is_uniform_discretization : public std::false_type
{
};

template <class CDim>
struct is_uniform_discretization<UniformDiscretization<CDim>> : public std::true_type
{
};

template <class DDim>
constexpr bool is_uniform_discretization_v = is_uniform_discretization<DDim>::value;


template <
        class DDimImpl,
        std::enable_if_t<
                is_uniform_discretization_v<typename DDimImpl::discrete_dimension_type>,
                int> = 0>
std::ostream& operator<<(std::ostream& out, DDimImpl const& mesh)
{
    return out << "UniformDiscretization( origin=" << mesh.origin() << ", step=" << mesh.step()
               << " )";
}

/// @brief Lower bound index of the mesh
template <class DDim>
DDC_INLINE_FUNCTION std::
        enable_if_t<is_uniform_discretization_v<DDim>, typename DDim::continuous_element_type>
        origin() noexcept
{
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
    return discrete_space_device<DDim>().origin();
#else
    return discrete_space_host<DDim>().origin();
#endif
}

/// @brief Lower bound index of the mesh
template <class DDim>
DDC_INLINE_FUNCTION std::
        enable_if_t<is_uniform_discretization_v<DDim>, typename DDim::discrete_element_type>
        front() noexcept
{
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
    return discrete_space_device<DDim>().front();
#else
    return discrete_space_host<DDim>().front();
#endif
}

/// @brief Spacing step of the mesh
template <class DDim>
DDC_INLINE_FUNCTION std::
        enable_if_t<is_uniform_discretization_v<DDim>, typename DDim::continuous_element_type>
        step() noexcept
{
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
    return discrete_space_device<DDim>().step();
#else
    return discrete_space_host<DDim>().step();
#endif
}

template <class CDim>
DDC_INLINE_FUNCTION constexpr Coordinate<CDim> to_real(
        DiscreteElement<UniformDiscretization<CDim>> const& c)
{
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
    return discrete_space_device<UniformDiscretization<CDim>>().to_real(c);
#else
    return discrete_space_host<UniformDiscretization<CDim>>().to_real(c);
#endif
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> distance_at_left(
        DiscreteElement<UniformDiscretization<CDim>> i)
{
    return step<UniformDiscretization<CDim>>();
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> distance_at_right(
        DiscreteElement<UniformDiscretization<CDim>> i)
{
    return step<UniformDiscretization<CDim>>();
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> rmin(DiscreteDomain<UniformDiscretization<CDim>> const& d)
{
    return to_real(d.front());
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> rmax(DiscreteDomain<UniformDiscretization<CDim>> const& d)
{
    return to_real(d.back());
}

template <class CDim>
DDC_INLINE_FUNCTION Coordinate<CDim> rlength(DiscreteDomain<UniformDiscretization<CDim>> const& d)
{
    return rmax(d) - rmin(d);
}

template <class T>
struct is_uniform_domain : std::false_type
{
};

template <class... DDims>
struct is_uniform_domain<DiscreteDomain<DDims...>>
    : std::conditional_t<
              (is_uniform_discretization_v<DDims> && ...),
              std::true_type,
              std::false_type>
{
};

template <class T>
constexpr bool is_uniform_domain_v = is_uniform_domain<T>::value;


// /** Construct a uniform `DiscreteDomain` from a segment \f$[a, b] \subset [a, +\infty[\f$ and a
//  *  number of points `n`.
//  *
//  * @param a the coordinate of a first real point (will have mesh coordinate 0)
//  * @param b the coordinate of the second real point (will have mesh coordinate `n-1`)
//  * @param n the number of points to map the segment \f$[a, b]\f$ including a & b
//  */
// template <class D, class = std::enable_if_t<is_uniform_discretization_v<D>>>
// constexpr DiscreteDomain<D> init_global_domain(
//         typename D::continuous_element_type a,
//         typename D::continuous_element_type b,
//         typename D::discrete_vector_type n)
// {
//     assert(a < b);
//     assert(n > 1);
//     init_discretization<D>(a, typename D::continuous_element_type {(b - a) / (n - 1)});
//     return DiscreteDomain<D>(n);
// }
