#pragma once

#include <mpi.h>

#include "ddc/discrete_domain.hpp"

template <class... DDims>
class CartesianMpiDistribution
{
    MPI_Comm m_comm;

    DiscreteVector<DDims...> m_nb_procs;

public:
    CartesianMpiDistribution(MPI_Comm comm, DiscreteVector<DDims...> nb_procs);

    CartesianMpiDistribution(
            MPI_Comm comm,
            DiscreteVector<DDims...> nb_procs,
            DiscreteVector<DDims...> overlap);

    CartesianMpiDistribution with_overlap(DiscreteVector<DDims...> overlap) const;
};
