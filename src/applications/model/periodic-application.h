/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef PERIODIC_APPLICATION_H
#define PERIODIC_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

/**
 * \ingroup applications 
 *
 */
class PeriodicApplication : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  PeriodicApplication ();

  virtual ~PeriodicApplication();

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

  /**
   * \return the total packets transmitted
   */
  uint64_t GetTotalTxPackets (void) const;
  /**
   * \return the total bytes transmitted
   */
  uint64_t GetTotalTxBytes (void) const;

 /**
  * \brief Assign a fixed random variable stream number to the random variables
  * used by this model.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  // inherited from Application base class.
  virtual void StopApplication (void);     // Called at time specified by Stop
  /**
   * Stop the application without closing the socket.
   * This allows to restart the application later.
   */
  virtual void SuspendApplication (void);     // Called at time specified by Stop

protected:
  virtual void DoDispose (void);
private:
  //helpers
  /**
   * \brief Cancel all pending events.
   */
  void CancelEvents ();

  // Event handlers
  /**
   * \brief Start an On period
   */
  void StartSending ();
  /**
   * \brief Send a packet
   * \param pktSize the packet size in Bytes
   */
  void SendPacket (uint32_t pktSize);
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  bool            m_connected;    //!< True if connected
  Ptr<RandomVariableStream>  m_periodRv;       //!< rng for application period
  Ptr<RandomVariableStream>  m_burstSizeRv;      //!< rng for burst size [B]
  uint32_t        m_pktSize;      //!< Size of packets
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  uint64_t        m_txPackets;     //!< Total packets sent so far
  EventId         m_nextBurstEvent;     //!< Event id for next start or stop event
  TypeId          m_socketTid;          //!< Type of the socket used

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;
};

} // namespace ns3

#endif /* PERIODIC_APPLICATION_H */
