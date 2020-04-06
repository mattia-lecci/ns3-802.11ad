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

#ifndef GAMING_STREAMING_SERVER_H
#define GAMING_STREAMING_SERVER_H

#include "ns3/application.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Socket;
/**
 * \ingroup applications
 *
 * This traffic generator Generates different traffic streams
 * based on different random variables
 *
 * Each random stream defined by two random variables:
 *      packetSize: A random variable which determines the size of generated packets
 *      interArrivalTime: A random variable which determines the packets inter-arrival time
*/
class GamingStreamingServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief create a GamingStreamingServer object with default parameters
   */
  GamingStreamingServer ();

  /**
   * \brief Create a GamingStreamingServer object
   *
   * \param ip Remote peer address
   * \param port Remote peer port
   */
  GamingStreamingServer (Address ip, uint16_t port);

  virtual ~GamingStreamingServer () override;

  /**
   * \brief set the remote address and port
   *
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);

  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);

  /**
   * \brief return the number of total sent packets
   * \return total number of sent packets
   */
  uint32_t GetTotSentPackets (void);

  /**
   * \brief return the number of total failed packets
   * \return total number of failed packets
   */
  uint32_t GetTotFailedPackets (void);

  /**
   * \brief return the total bytes sent
   * \return the total bytes sent
   */
  uint32_t GetTotSentBytes (void);

  /**
   * \brief Erase the statistics of sent packets
   */
  void EraseStatistics (void);

  /**
   * \brief Add parameters of new stream
   *
   * \param packetSize A random number generator for packet size
   * \param interArrivalTime A random number generator for inter-arrival time
   */
  void AddNewTrafficStream (Ptr<RandomVariableStream> packetSize, Ptr<RandomVariableStream> interArrivalTime);

  virtual void StartApplication (void) override;
  virtual void StopApplication (void) override;

protected:
  virtual void DoDispose (void)  override;

  /**
   *  Initialize the parameters of different streams
   */
  virtual void InitializeStreams () = 0;

private:
  struct TrafficStream;

  /**
   * \brief Generate and send packets based on the   random
   * distribution for packet sizes and inter-arrival times
   *
   * \param traffic A pointer to traffic stream defined by TrafficStream struct
   */
  void Send (Ptr<TrafficStream> traffic);

  /**
   * \brief Schedule the next packet transmission
   *
   * \param traffic A pointer to traffic stream defined by TrafficStream struct
   */
  void ScheduleNextTx (Ptr<TrafficStream> traffic);

  uint32_t     m_seq;  //!< Sequence number for packets
  uint32_t     m_totSentPackets; //!< Counter for sent packets
  uint32_t     m_totFailedPackets; //!< Counter for failed packets
  uint32_t     m_totSentBytes; //!< Total bytes sent so far
  Ptr<Socket>  m_socket; //!< Socket
  Address      m_peerAddress; //!< Remote peer address
  uint16_t     m_peerPort; //!< Remote peer port

  struct TrafficStream : public SimpleRefCount<TrafficStream>
    {
      EventId sendEvent;  //!< Send event for next packet
      Ptr<RandomVariableStream> packetSizeVariable;  //!< Random number generator for packet size
      Ptr<RandomVariableStream> interArrivalTimesVariable; //!< Random number generator for packets inter-arrival time

      ~TrafficStream ();
    };

  std::vector<Ptr<TrafficStream> > m_trafficStreams; //!< A list of traffic streams

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;

};

} // namespace ns3

#endif /* GAMING_STREAMING_SERVER_H */
