/*
  @file

  TXN start test plugin
  Implements blind tunneling based on the client IP address
  takes an hash of the client ip and offsets the target port
  with 1+hash%num_upstream

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include <cstdio>
#include <memory.h>
#include <cinttypes>
#include <ts/ts.h>
#include <ts/ink_inet.h>
#include <getopt.h>


#define PN "tcp-routing-txn"
#define PCP "[" PN " Plugin] "

namespace
{
const int num_upstream=5;

int CB_txn_start(TSCont contp, TSEvent event, void *edata)
{
  TSHttpTxn txnp = reinterpret_cast<TSHttpTxn>(edata);
  auto target = TSHttpTxnIncomingAddrGet(txnp);
  IpAddr ip_target(target);
  IpEndpoint ep_target;
  ep_target.assign(target);
  char buff[INET6_ADDRSTRLEN];
  auto *client=TSHttpTxnClientAddrGet(txnp);
  IpAddr ip_client(client);
  char buff2[INET6_ADDRSTRLEN];

  TSDebug(PN, "tcp-routing callback %p, target address %s, client address %s", txnp,
          ip_target.toString(buff, sizeof(buff)),
          ip_client.toString(buff2, sizeof(buff2))
);
  // calculate some offset based on IP address
  // ats_ip_hash is just copying the ipv4 in case of ipv4, 
  // so we need to bring it back from network level, or else
  // all our offsets will be the same with a num_upstream=2
  // a better hash is needed
  auto ip_select = ntohl(ats_ip_hash(client)) % num_upstream;
  TSDebug(PN, "tcp-routing client hash %x",ip_select); 

  // simple algorithm to load-balance, offset the target port by 1+hash % upstream nodes
  // it's also possible to set next hop 
  auto port = htons(ep_target.host_order_port()+1+ip_select);
  ep_target.assign(ip_target,port);
  TSDebug(PN, "tcp-routing upstream select %d",ip_select); 
  // set target address
  TSHttpTxnServerAddrSet(txnp,ep_target);
  // setup transaction for tunnel
  TSTxnTunnel(txnp);
  TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
  return TS_SUCCESS;
}

} // Anon namespace

// Called by ATS as our initialization point
void
TSPluginInit(int argc, const char *argv[])
{
  bool success = false;
  TSPluginRegistrationInfo info;
  TSCont cb_pa                         = nullptr; // pre-accept callback continuation

  info.plugin_name   = const_cast<char *>("tcp routing txn test");
  info.vendor_name   = const_cast<char *>("we amp");
  info.support_email = const_cast<char *>("kspoelstra@we-amp.com");

  if (TS_SUCCESS != TSPluginRegister(&info)) {
    TSError(PCP "registration failed.");
  } else if (TSTrafficServerVersionGetMajor() < 2) {
    TSError(PCP "requires Traffic Server 2.0 or later.");
  } else if (nullptr == (cb_pa = TSContCreate(&CB_txn_start, nullptr ))) {
    TSError(PCP "Failed to pre-accept callback.");
  } else {
    TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, cb_pa);
    success = true;
  }

  if (!success) {
    TSError(PCP "not initialized");
  }
  TSDebug(PN, "Plugin %s", success ? "online" : "offline");

  return;
}
