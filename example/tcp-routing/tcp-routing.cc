/** @file

  SSL Preaccept test plugin
  Implements blind tunneling based on the client IP address
  The client ip addresses are specified in the plugin's
  config file as an array of IP addresses or IP address ranges under the
  key "client-blind-tunnel"

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


#define PN "tcp-routing"
#define PCP "[" PN " Plugin] "

namespace
{

int
CB_Pre_Accept(TSCont, TSEvent event, void *edata)
{
  TSVConn ssl_vc = reinterpret_cast<TSVConn>(edata);
  IpAddr ip(TSNetVConnLocalAddrGet(ssl_vc));
  char buff[INET6_ADDRSTRLEN];
  IpAddr ip_client(TSNetVConnRemoteAddrGet(ssl_vc));
  char buff2[INET6_ADDRSTRLEN];

  TSDebug("skh", "tcp-routing callback %p - event is %s, target address %s, client address %s", ssl_vc,
          event == TS_EVENT_VCONN_PRE_ACCEPT ? "good" : "bad", ip.toString(buff, sizeof(buff)),
          ip_client.toString(buff2, sizeof(buff2)));

  TSDebug("skh", "Blind tunnel");
  // Push everything to blind tunnel
  TSVConnTunnel(ssl_vc);
  TSVConnReenable(ssl_vc);
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

  info.plugin_name   = const_cast<char *>("tcp routing test");
  info.vendor_name   = const_cast<char *>("we amp");
  info.support_email = const_cast<char *>("kspoelstra@we-amp.com");

  if (TS_SUCCESS != TSPluginRegister(&info)) {
    TSError(PCP "registration failed.");
  } else if (TSTrafficServerVersionGetMajor() < 2) {
    TSError(PCP "requires Traffic Server 2.0 or later.");
  } else if (nullptr == (cb_pa = TSContCreate(&CB_Pre_Accept, TSMutexCreate()))) {
    TSError(PCP "Failed to pre-accept callback.");
  } else {
    TSHttpHookAdd(TS_VCONN_PRE_ACCEPT_HOOK, cb_pa);
    success = true;
  }

  if (!success) {
    TSError(PCP "not initialized");
  }
  TSDebug(PN, "Plugin %s", success ? "online" : "offline");

  return;
}
