#!/usr/bin/env python
# Copyright 2018 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import httplib
import time
import argparse
import sys

sys.path.insert(0, "../../protocomm/python/protocomm")

import security
import transport
import proto

parser = argparse.ArgumentParser(description="Generate ESP prov payload")
parser.add_argument("--ssid", dest='ssid', type= str,
    help= "SSID of wifi newtork", required= True, default= '')
parser.add_argument("--passphrase", dest = 'passphrase', type = str,
    required = True, help = "Passphrase of wifi network", default = '')
parser.add_argument("--sec_ver", dest = 'secver', type = int,
    help = "Security scheme version", default = 1)
parser.add_argument("--proto_ver", dest = 'protover', type= int,
    help= "Protocol version", default = 1)
parser.add_argument("--pop", dest = 'pop', type = str,
    help = "Proof of possession", default = '')
parser.add_argument("--softap_endpoint", dest = 'softap_endpoint', type = str,
    help = "<softap_ip:port>, http(s):// shouldn't be included", default = '192.168.4.1:80')
parser.add_argument("--ble_devname", dest = 'ble_devname', type = str,
    help = "BLE Device Name", default = 'PROV_')
parser.add_argument("--prov_mode", dest = 'provmode', type = str,
    help = "provisioning mode i.e cli or softap or ble", default = 'softap')
parser.add_argument("-v","--verbose", help="increase output verbosity",
                    action="store_true")
args = parser.parse_args()

print "==== Esp_Prov Version: 0.1 ===="

if args.secver == 1:
    security = security.Security1(args.pop, args.verbose)
elif args.secver == 0:
    security = security.Security0(args.verbose)
else:
    print "---- Unexpected state ----"
    exit(1)

if (args.provmode == 'softap'):
    transport = transport.Transport_Softap(args.softap_endpoint)

elif (args.provmode == 'ble'):
    try:
        transport = transport.Transport_BLE(args.ble_devname, verbose = args.verbose)
    except RuntimeError, e:
        print "---- Error occured in BLE provisioning mode ----"
        print e
        exit()

elif (args.provmode == 'cli'):
    transport = transport.Transport_CLI()

else:
    print "---- Invalid provisioning mode ----"
    exit()

print "==== Starting Session ===="
response = None
while True:
    request = security.security_session(response)
    if request == None:
        break
    response = transport.send_session_data(request)
    if (response == None):
        exit()
print "==== Session Established ===="

print "\n==== Sending Wifi credential to esp32 ===="
message = proto.config_set_config_request(security, args.ssid, args.passphrase)
response = transport.send_config_data(message)
proto.config_set_config_response(security, response)
print "==== Wifi Credentials sent successfully ===="

print "\n==== Applying config to esp32 ===="
message = proto.config_apply_config_request(security)
response = transport.send_config_data(message)
proto.config_set_config_response(security, response)
print "==== Apply config sent successfully ===="

while True:
    time.sleep(5)
    print "\n==== Wifi connection state  ===="
    message = proto.config_get_status_request(security)
    response = transport.send_config_data(message)
    ret = proto.config_get_status_response(security, response)
    if (ret == 1):
       continue
    else:
       if (ret == 0):
           print "==== Provisioning was successful ===="
       else:
           print "---- Provisioning failed ----"
       break
