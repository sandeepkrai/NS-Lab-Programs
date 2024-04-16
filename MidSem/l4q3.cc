/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Network topology
//
//                     Lan1                       Lan2
//                 ===========                   ==========      
//                 |    |    |                   |   |   |
//       n0   n1   n2   n3   n4-----------------n5  n6  n7
//       |    |    |
//       ===========
//           Lan0
//
// - Multicast source is at node n0;
// - Multicast forwarded by node n2 onto LAN1;
// - Nodes n0, n1, n2, n3, and n4 receive the multicast frame.
// - Node n4 listens for the data 

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CsmaMulticastExample");

int 
main (int argc, char *argv[])
{
  //
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //
  LogComponentEnable ("CsmaMulticastExample", LOG_LEVEL_INFO);
LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  //
  // Set up default values for the simulation.
  //
  // Select DIX/Ethernet II-style encapsulation (no LLC/Snap header)
  Config::SetDefault ("ns3::CsmaNetDevice::EncapsulationMode", StringValue ("Dix"));

  int nCsma = 10;
  CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.Parse(argc, argv);

  // Allow the user to override any of the defaults at
  // run-time, via command-line arguments
  //CommandLine cmd (__FILE__);
  //cmd.Parse (argc, argv);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (nCsma);
  // We will later want two subcontainers of these nodes, for the two LANs
  NodeContainer c0 = NodeContainer (c.Get (0), c.Get (1), c.Get (2));
  NodeContainer c1 = NodeContainer (c.Get (2), c.Get (3), c.Get (4),c.Get (5));
  NodeContainer c2 = NodeContainer (c.Get (5), c.Get (6), c.Get (7));
  NodeContainer p1 = NodeContainer (c.Get (4), c.Get (5));
  NodeContainer c3 = NodeContainer (c.Get (7), c.Get (8),c.Get(9));
  NodeContainer p2 = NodeContainer (c.Get (8), c.Get (10));

  NS_LOG_INFO ("Build Topology.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  // We will use these NetDevice containers later, for IP addressing
  NetDeviceContainer nd0 = csma.Install (c0);  // First LAN
  NetDeviceContainer nd1 = csma.Install (c1);  // Second LAN
  NetDeviceContainer nd2 = csma.Install (c2);
  NetDeviceContainer nd4 = csma.Install (c3);
  NetDeviceContainer nd3 = p2p.Install (p1);
  NetDeviceContainer nd5 = p2p.Install (p2);
  NS_LOG_INFO ("Add IP Stack.");
  InternetStackHelper internet;
  internet.Install (c);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4Addr;
  ipv4Addr.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4Addr.Assign (nd0);
  ipv4Addr.SetBase ("10.1.2.0", "255.255.255.0");
  ipv4Addr.Assign (nd1);
  ipv4Addr.SetBase ("10.1.3.0", "255.255.255.0");
  ipv4Addr.Assign (nd2);
  ipv4Addr.SetBase ("10.1.4.0", "255.255.255.0");
  ipv4Addr.Assign (nd3);
  ipv4Addr.SetBase ("10.1.5.0", "255.255.255.0");
  ipv4Addr.Assign (nd4);
  ipv4Addr.SetBase ("10.1.6.0", "255.255.255.0");
  ipv4Addr.Assign (nd5);


  NS_LOG_INFO ("Configure multicasting.");
  //
  // Now we can configure multicasting.  As described above, the multicast 
  // source is at node zero, which we assigned the IP address of 10.1.1.1 
  // earlier.  We need to define a multicast group to send packets to.  This
  // can be any multicast address from 224.0.0.0 through 239.255.255.255
  // (avoiding the reserved routing protocol addresses).
  //

  Ipv4Address multicastSource ("10.1.1.1");
  Ipv4Address multicastGroup ("225.1.2.4");

  // Now, we will set up multicast routing.  We need to do three things:
  // 1) Configure a (static) multicast route on node n2
  // 2) Set up a default multicast route on the sender n0 
  // 3) Have node n4 join the multicast group
  // We have a helper that can help us with static multicast
  Ipv4StaticRoutingHelper multicast;

  // 1) Configure a (static) multicast route on node n2 (multicastRouter)
  Ptr<Node> multicastRouter = c.Get (2);  // The node in question
  Ptr<NetDevice> inputIf = nd0.Get (2);  // The input NetDevice
  NetDeviceContainer outputDevices;  // A container of output NetDevices
  outputDevices.Add (nd1.Get (0));  // (we only need one NetDevice here)
  // outputDevices.Add (nd3.Get(0)); 

  multicast.AddMulticastRoute (multicastRouter, multicastSource, 
                               multicastGroup, inputIf, outputDevices);

  Ptr<Node> multicastRouter1 = c.Get (4);  // The node in question
  Ptr<NetDevice> inputIf1 = nd1.Get (2);  // The input NetDevice
  NetDeviceContainer outputDevices1;  // A container of output NetDevices
  outputDevices1.Add (nd3.Get (0));  // (we only need one NetDevice here)
  // outputDevices.Add (nd3.Get(0)); 

  multicast.AddMulticastRoute (multicastRouter1, multicastSource, 
                               multicastGroup, inputIf1, outputDevices1);


  Ptr<Node> multicastRouter2 = c.Get (5);  // The node in question
  Ptr<NetDevice> inputIf2 = nd3.Get (1);  // The input NetDevice
  NetDeviceContainer outputDevices2;  // A container of output NetDevices
  outputDevices2.Add (nd2.Get (0));  // (we only need one NetDevice here)
  // outputDevices.Add (nd3.Get(0)); 

  multicast.AddMulticastRoute (multicastRouter2, multicastSource, 
                               multicastGroup, inputIf2, outputDevices2);
    
  Ptr<Node> multicastRouter3 = c.Get (7);  // The node in question
  Ptr<NetDevice> inputIf3 = nd2.Get (2);  // The input NetDevice
  NetDeviceContainer outputDevices3;  // A container of output NetDevices
  outputDevices3.Add (nd4.Get (0));  // (we only need one NetDevice here)
  // outputDevices.Add (nd3.Get(0)); 

  multicast.AddMulticastRoute (multicastRouter3, multicastSource, 
                               multicastGroup, inputIf3, outputDevices3);
  
  Ptr<Node> multicastRouter4 = c.Get (8);  // The node in question
  Ptr<NetDevice> inputIf4 = nd4.Get (1);  // The input NetDevice
  NetDeviceContainer outputDevices4;  // A container of output NetDevices
  outputDevices4.Add (nd5.Get (0));  // (we only need one NetDevice here)
  // outputDevices.Add (nd3.Get(0)); 

  multicast.AddMulticastRoute (multicastRouter4, multicastSource, 
                               multicastGroup, inputIf4, outputDevices4);
  


  // 2) Set up a default multicast route on the sender n0 
  Ptr<Node> sender = c.Get (0);
  Ptr<NetDevice> senderIf = nd0.Get (0);
  multicast.SetDefaultMulticastRoute (sender, senderIf);

  //
  // Create an OnOff application to send UDP datagrams from node zero to the
  // multicast group (node four will be listening).
  //
  NS_LOG_INFO ("Create Applications.");

  uint16_t multicastPort = 9;   // Discard port (RFC 863)

  // Configure a multicast packet generator that generates a packet
  // every few seconds
  OnOffHelper onoff ("ns3::UdpSocketFactory", 
                     Address (InetSocketAddress (multicastGroup, multicastPort)));
  onoff.SetConstantRate (DataRate ("255b/s"));
  onoff.SetAttribute ("PacketSize", UintegerValue (128));

  ApplicationContainer srcC = onoff.Install (c0.Get (0));

  //
  // Tell the application when to start and stop.
  //
  srcC.Start (Seconds (1.));
  srcC.Stop (Seconds (10.));

  // Create an optional packet sink to receive these packets
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), multicastPort));

  ApplicationContainer sinkC = sink.Install (p1.Get (1)); // Node n5
  // Start the sink
  sinkC.Start (Seconds (1.0));
  sinkC.Stop (Seconds (10.0));

  NS_LOG_INFO ("Configure Tracing.");
  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Ascii trace output will be sent to the file "csma-multicast.tr"
  //
  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("l4q3.tr"));
  AnimationInterface anim("l4q3.xml");

  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     csma-multicast-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  // csma.EnablePcapAll ("csma-multicast", false);

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
    // AnimationInterface anim ("csmam.xml");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}