/* -*- Mode:C++; c-file-style:''gnu''; indent-tabs-mode:nil; -*- */

/* 
 * Copyright 2012, Old Dominion University
 * Copyright 2012, University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * M.C. Weigle, P. Adurthi, F. Hernandez-Campos, K. Jeffay, and F.D. Smith, 
 * Tmix: A Tool for Generating Realistic Application Workloads in ns-2, 
 * ACM Computer Communication Review, July 2006, Vol 36, No 3, pp. 67-76.
 *
 * Contact: Michele Weigle (mweigle@cs.odu.edu)
 * http://www.cs.odu.edu/inets/Tmix
 */

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/global-route-manager.h"

#include <cerrno>

#include "ns3/tmix.h"
#include "ns3/tmix-helper.h"
#include "ns3/delaybox-net-device.h"
#include "ns3/tmix-topology.h"
#include "ns3/tmix-ns2-style-trace-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TmixTest");

void SetTmixPairOptions(TmixTopology::TmixNodePair &pair)  //pair of initiator-acceptor
{
  pair.helper->SetLossless(true);
}

void ReadAndAddCvecs(Ptr<TmixTopology> tmix, TmixTopology::InitiatorSide side,
    std::istream& cvecfile, double chance)
{
  const int cvecsPerPair = 10000;  //cloud of TCP connections/connection vectors between each initiator-acceptor

  TmixTopology::TmixNodePair pair = tmix->NewPair(side);
  SetTmixPairOptions(pair);
  int nPairs = 1;  //1 pair 

  Tmix::ConnectionVector cvec;
  int totncvec =0;
  int nCvecs = 0;  //initially, there are 0 cvec's
  while (Tmix::ParseConnectionVector(cvecfile, cvec))  //taking cvec as input one by one, and parsing 
    {
        totncvec++;      
if (UniformVariable().GetValue() < chance)
        {
          pair.helper->AddConnectionVector(cvec);  //adding the cvec
         if (++nCvecs >= cvecsPerPair) //if no.of cvec's exceed the number for 1 pair, then create another pair. 
            {
              pair = tmix->NewPair(side);
              SetTmixPairOptions(pair);
              nCvecs = 0;
              nPairs++;
            }
        }
    }  //total no.of cvec's taken as input
  NS_LOG_UNCOND("Total cvecs read "<<totncvec<<".");
  NS_LOG_UNCOND("Read a total of " << ((nPairs-1)*cvecsPerPair + nCvecs) << " cvecs, distributing them to " << nPairs << " node pairs.");
        
}

int main(int argc, char** argv)
{

  LogComponentEnableAll(LOG_PREFIX_FUNC);  //to display all log messages 
  LogComponentEnableAll(LOG_PREFIX_NODE);
  LogComponentEnableAll(LOG_PREFIX_TIME);

  LogComponentEnable("TmixTest", LOG_LEVEL_WARN);
  LogComponentEnable("Tmix", LOG_LEVEL_WARN);
  LogComponentEnable("TmixHelper", LOG_LEVEL_WARN);
  LogComponentEnable("TmixTopology", LOG_LEVEL_WARN);

  std::ifstream cvecfileA;
  std::ifstream cvecfileB; //2 cvec files, inbound and outbound. 
  //argv[0]=tmix file name, argv[1]=inbound file, argv[2]=outbound file. 
  double cvec_portion = 1.0;
    {
      if (argc < 2)  //only file name given, then error
        {
          std::cerr
              << "Usage: tmix cvec-file-a [cvec-file-b] [portion of cvecs to use, 0.0-1.0]\n";
          exit(1);
        }
      cvecfileA.open(argv[1]); //only inbound file given, then accept that in A(1st cvec file)
      if (!cvecfileA)
        {
          std::cerr << "Error opening " << argv[1] << ": " << strerror(errno)
              << std::endl;
          exit(1);
        }
      if (argc >= 3) //accept outbound file in B(2nd cvec file). 
        {
          cvecfileB.open(argv[2]);
          if (!cvecfileB)
            {
              std::cerr << "Error opening " << argv[2] << ": " << strerror(
                  errno) << std::endl;
              exit(1);
            }
        }
      if (argc >= 4)
        {
          sscanf(argv[3], "%lf", &cvec_portion); //if extra, then if they are of double type, accept it in cvec_portion
        }
    }

  CommandLine cmd;
  cmd.Parse (argc, argv); //parse(resolve) command line arguments 

  InternetStackHelper internet; //aggregates all the 7 layers on every node 
  Ptr<TmixTopology> tmix = Create<TmixTopology> (internet);

  NS_LOG_INFO("Adding connection vectors to LEFT side.");  //initiator on left side - inbound file(cvecfileA)
  ReadAndAddCvecs(tmix, TmixTopology::LEFT, cvecfileA, cvec_portion);
  if (cvecfileB.is_open())
    {
      NS_LOG_INFO("Adding connection vectors to RIGHT side.");  //initiator on right side - outbound file(cvecfileB)
      ReadAndAddCvecs(tmix, TmixTopology::RIGHT, cvecfileB, cvec_portion);
    }

  std::ostream& ns2StyleTraceFile = std::cout;
  Tmix::Ns2StyleTraceHelper ost(tmix, ns2StyleTraceFile);
  ost.Install(); //object ost calling Install method

  GlobalRouteManager::BuildGlobalRoutingDatabase(); //collect info from all routers about the link state advertisement(routers' local route topology).
  GlobalRouteManager::InitializeRoutes();  //uses Dijkstra's algo to compute the routes(forwarding tables). 

  NS_LOG_INFO("Starting simulation");
  Simulator::Stop(Seconds(10));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
