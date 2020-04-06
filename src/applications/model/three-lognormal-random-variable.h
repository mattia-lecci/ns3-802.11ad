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

#ifndef THREE_LOGNORMAL_RANDOM_VARIABLE_H
#define THREE_LOGNORMAL_RANDOM_VARIABLE_H

#include "ns3/random-variable-stream.h"

namespace ns3 {

/**
 * \ingroup randomvariable
 * \brief The three-parameters log-normal distribution Random Number Generator
 * (RNG) that allows stream numbers to be set deterministically.
 *
 * The three-parameter log-normal distribution is simply the usual two-parameter
 * log-normal distribution with a location shift.
 * Let X be a random variable with a three-parameter log-normal distribution
 * with parameters mu, sigma, and threshold then the random variable Y=X−threshold
 * has a log-normal distribution with parameters mu and sigma
 */
class ThreeLogNormalRandomVariable : public LogNormalRandomVariable
{
public:
  /**
   * \brief Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Creates a log-normal distribution RNG with the default
   * values for mu, sigma and threshold.
   */
  ThreeLogNormalRandomVariable ();
  virtual ~ThreeLogNormalRandomVariable () override;

  /**
   * \brief Returns the threshold value for the three-parameters log-normal distribution returned by this RNG stream.
   * \return The threshold value for the three-parameters log-normal distribution returned by this RNG stream.
   */
  double GetThreshold (void) const;

  /**
   * \brief Returns a random double from a log-normal distribution with the specified mu and sigma.
   * \param [in] mu The mu value for the three-parameters log-normal distribution.
   * \param [in] sigma The sigma value for the three-parameters log-normal distribution.
   * \param [in] threshold The threshold value for the three-parameters log-normal distribution.
   * \return A floating point random value.
   */
  double GetValue (double  mu, double sigma, double threshold);

  /**
   * \brief Returns a random unsigned integer from a three-parameters log-normal distribution with the specified mu and sigma.
   * \param [in] mu The mu value for the three-parameters log-normal distribution.
   * \param [in] sigma The sigma value for the three-parameters log-normal distribution.
   * \param [in] threshold The threshold value for the three-parameters log-normal distribution.
   * \return A floating point random value.
   */
  uint32_t GetInteger (uint32_t  mu, uint32_t sigma, uint32_t threshold);

  /**
   * \brief Returns a random double from a three-parameters log-normal distribution with the current mu and sigma.
   * \return A floating point random value.
   */
  virtual double GetValue (void) override;

  /**
   * \brief Returns a random unsigned integer from a three-parameters log-normal
   * distribution with the current mu, sigma and threshold
   * \return A random unsigned integer value.
   */
  virtual uint32_t GetInteger (void) override;

private:
  double m_threshold;  //!< The threshold value for the three-parameter log-normal distribution

};  // class ThreeLogNormalRandomVariable

} // namespace ns3

#endif /* THREE_LOGNORMAL_RANDOM_VARIABLE_H */
