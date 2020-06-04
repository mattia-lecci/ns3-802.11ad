/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020, University of Padova, Department of Information
 * Engineering, SIGNET Lab.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Salman Mohebi <s.mohebi22@gmail.com>
 *
 */

#ifndef MIXTURE_RANDOM_VARIABLE_H
#define MIXTURE_RANDOM_VARIABLE_H

#include "ns3/random-variable-stream.h"

namespace ns3 {

/**
 * \ingroup randomvariable
 * \brief The mixture distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a mixture distribution.
 *
 * The mixture distribution is the probability distribution of a random variable
 * that is derived from a collection of other random variables as follows:
 * first, a random variable is selected by chance from the collection
 * according to given probabilities of selection, and then the value of
 * the selected random variable is realized.
 */
class MixtureRandomVariable : public RandomVariableStream
{
public:
  /**
   * \brief Get the type ID.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Creates MixtureRandomVariable object with the default Random variables
   */
  MixtureRandomVariable ();

  /**
   * \brief Creates MixtureRandomVariable object with the default Random variables
   *
   * \param rvs a vector of random variables which used to generate random numbers
   * \param probs a vector of probabilities for different random variables
   */
  MixtureRandomVariable (std::vector<Ptr<RandomVariableStream> > rvs, std::vector <double> probs);
  virtual ~MixtureRandomVariable () override;

  /**
   * \brief Returns a random double from defined distributions based on their probabilities
   * \return A floating point random value.
   */
  virtual double GetValue (void) override;

  /**
   * \brief Returns a random unsigned integer from defined distributions based on their probabilities
   * \return A random unsigned integer value.
   */
  virtual uint32_t GetInteger (void) override;

  /**
   * \brief Set the random variables with their own probabilities
   *
   * \param rvs a vector of random variables which used to generate random numbers
   * \param probs a vector of probabilities for different random variables
   */
  void SetRandomVariables (std::vector<Ptr<RandomVariableStream> > rvs, std::vector<double> probs);

private:
  /**
   *  \brief Compute the CDF for the given probability vector
   *
   * \param probs a vector of probabilities
   * \return cumulative distribution function of the given probability vector
   */
  std::vector<double> GetProbCdf (std::vector<double> probs);

  Ptr<UniformRandomVariable> m_uniform;  //!< Uniform random number generator
  std::vector<Ptr<RandomVariableStream> > m_rvs;    //!< Vector of RandomVariables
  std::vector<double> m_probsCdf;  //!< Vector of cumulative probabilities for RandomVariables

};  // class MixtureRandomVariable

} // namespace ns3

#endif /* MIXTURE_RANDOM_VARIABLE_H */
