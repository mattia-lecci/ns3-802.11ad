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
#include "mixture-random-variable.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MixtureRandomVariable");

NS_OBJECT_ENSURE_REGISTERED (MixtureRandomVariable);

TypeId
MixtureRandomVariable::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MixtureRandomVariable")
    .SetParent<RandomVariableStream> ()
    .SetGroupName ("Core")
    .AddConstructor<MixtureRandomVariable> ()
  ;
  return tid;
}

MixtureRandomVariable::MixtureRandomVariable ()
{
  NS_LOG_FUNCTION (this);
  m_uniform = CreateObjectWithAttributes<UniformRandomVariable> ();
}

MixtureRandomVariable::MixtureRandomVariable (std::vector<Ptr<RandomVariableStream> > rvs, std::vector<double> probs)
  : MixtureRandomVariable ()
{
  NS_LOG_FUNCTION (this);
  SetRandomVariables (rvs, probs);
}

MixtureRandomVariable::~MixtureRandomVariable ()
{
  NS_LOG_FUNCTION (this);
}

double
MixtureRandomVariable::GetValue ()
{
  NS_LOG_FUNCTION (this);

  double thresh = m_uniform->GetValue ();
  for (size_t index = 0; index < m_probsCdf.size (); ++index)
    {
      if (thresh <= m_probsCdf[index])
        {
          return m_rvs[index]->GetValue ();
        }
    }
  NS_FATAL_ERROR ("thresh should be less than 1");
}

uint32_t
MixtureRandomVariable::GetInteger (void)
{
  NS_LOG_FUNCTION (this);
  return static_cast<uint32_t> (GetValue ());
}

void
MixtureRandomVariable::SetRandomVariables (std::vector<Ptr<RandomVariableStream> > rvs, std::vector<double> probs)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (rvs.size () == probs.size (), "The vectors rvs and probs should be the same size");
  m_rvs = rvs;
  m_probsCdf = GetProbCdf (probs);
}

std::vector<double>
MixtureRandomVariable::GetProbCdf (std::vector<double> probs)
{
  double cumProb = 0;
  std::vector<double> probsCdf;
  for (const auto &prob : probs)
    {
      cumProb += prob;
      probsCdf.push_back (cumProb);
    }
  NS_ASSERT_MSG (fabs (cumProb - 1) < 1e-9, "Probability vector should sum to 1.0 +/- 1e-9");
  probsCdf.back () = 1.0;
  return probsCdf;
}

} // Namespace ns3
