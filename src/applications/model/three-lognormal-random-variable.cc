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

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/simulator.h"
#include "three-lognormal-random-variable.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ThreeLogNormalRandomVariable");

NS_OBJECT_ENSURE_REGISTERED(ThreeLogNormalRandomVariable);

TypeId
ThreeLogNormalRandomVariable::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeLogNormalRandomVariable")
      .SetParent<LogNormalRandomVariable> ()
      .SetGroupName ("Core")
      .AddConstructor<ThreeLogNormalRandomVariable> ()
      .AddAttribute("Threshold", "The threshold value for the three-parameters log-normal "
                                 "distribution returned by this RNG stream.",
                    DoubleValue (0.0),
                    MakeDoubleAccessor (&ThreeLogNormalRandomVariable::m_threshold),
                    MakeDoubleChecker<double> ())
  ;
  return tid;
}

ThreeLogNormalRandomVariable::ThreeLogNormalRandomVariable ()
{
  NS_LOG_FUNCTION (this);
}

ThreeLogNormalRandomVariable::~ThreeLogNormalRandomVariable ()
{
  NS_LOG_FUNCTION (this);
}

double
ThreeLogNormalRandomVariable::GetThreshold (void) const
{
  NS_LOG_FUNCTION (this);
  return m_threshold;
}

double
ThreeLogNormalRandomVariable::GetValue (double  mu, double sigma, double threshold)
{
  NS_LOG_FUNCTION (this << mu << sigma << threshold);
  double x = LogNormalRandomVariable::GetValue (mu, sigma);
  double y = x + threshold;
  return y;
}

uint32_t
ThreeLogNormalRandomVariable::GetInteger (uint32_t  mu, uint32_t sigma, uint32_t threshold)
{
  NS_LOG_FUNCTION (this << mu << sigma << threshold);
  return static_cast<uint32_t> (GetValue (mu, sigma, threshold));
}

double
ThreeLogNormalRandomVariable::GetValue (void)
{
  NS_LOG_FUNCTION (this);
  return GetValue (GetMu(), GetSigma(), m_threshold);
}

uint32_t
ThreeLogNormalRandomVariable::GetInteger (void)
{
  NS_LOG_FUNCTION (this);
  return static_cast<uint32_t> (GetValue (GetMu(), GetSigma(), m_threshold));
}

} // Namespace ns3
