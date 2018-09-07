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
from transport import *
import pexpect
import time

def verbose_print(msg, verbose):
    if verbose:
        print "BTCTL>>", msg

def bytestream_2_bytearray(msg):
    res = ""
    for i in range(len(msg)/2):
        byte = "0x"+msg[2*i:2*i+2]+" "
        #print int(bt, 16),
        res += byte
    res = "\"" + res[:-1] + "\""
    return res

def blob_2_bytestream(msg_blob):
    msg_blob = msg_blob.splitlines()
    msg_blob = msg_blob[2:]
    msg_blob = msg_blob[:len(msg_blob)/2]
    msg_bs = ""
    for msg_line in msg_blob:
        msg_line = msg_line[2:]
        msg_bs += msg_line[:msg_line.find("  ")] + " "
    msg = "".join(msg_bs.split())
    return msg

class Transport_BLE(Transport):
    def __init__(self, devname, timeout=2, verbose=False):
        self.devname = devname
        self.timeout = timeout
        self.verbose = verbose
        self.mac     = None
        self.btctl   = None
        try:
            self.btctl = pexpect.spawn("bluetoothctl", echo = False)
        except pexpect.exceptions.ExceptionPexpect:
            err_msg = "Please install bluetoothctl to run BLE prov"
            raise RuntimeError(err_msg)
        self._powered   = False
        self._connected = False
        self._gatt_menu = False
        self._connect()

    def __del__(self):
        self.close()

    def _connect(self):
        self._exec_n_expect("power on", "Changing power on succeeded")
        self._powered = True
        self._exec_n_expect("scan on", "Discovery started")
        self._exec_n_expect("devices", self.devname)
        self.mac = self.btctl.before[-18:-1]
        self._exec_n_expect("connect " + self.mac, "Connection successful")
        self._connected = True
        self._exec_n_expect("scan off", "Discovery stopped")
        self._exec_n_expect("menu gatt", "Menu gatt:")
        self._gatt_menu = True

    def _disconnect(self):
        if (self._gatt_menu):
            self._gatt_menu = False
            self._exec_n_expect("back", "Menu main:")
        if (self._connected):
            self._connected = False
            self._exec_n_expect("disconnect", "Successful disconnected")
        if (self._powered):
            self._powered = False
            self._exec_n_expect("power off", "Changing power off succeeded")

    def _exec_n_expect(self, cmd, expt):
        verbose_print(cmd, self.verbose)
        self.btctl.send(cmd + "\n")
        if expt == None:
            return
        try:
            if self.btctl.expect(expt) == 0:
                return
        except pexpect.exceptions.TIMEOUT:
            pass
        err_msg = "Failed in executing command : " + cmd
        raise RuntimeError(err_msg)

    def _exec_n_not_expect(self, cmd, not_expt):
        verbose_print(cmd, self.verbose)
        self.btctl.send(cmd + "\n")
        if not_expt == None:
            return
        try:
            if self.btctl.expect(not_expt, timeout = self.timeout) != 0:
                return
            err_msg = "Failed in executing command : " + cmd
            raise RuntimeError(err_msg)
        except pexpect.exceptions.TIMEOUT:
            pass

    def _read(self, maxlen):
        verbose_print("read", self.verbose)
        self.btctl.send("read\n")
        time.sleep(self.timeout)
        msg = blob_2_bytestream(self.btctl.read_nonblocking(maxlen))
        verbose_print("Read Value : " + msg, self.verbose)
        return msg

    def send_data(self, path, data):
        self._exec_n_expect("list-attributes", "\t" + path + "\r\n")
        attr_str = self.btctl.before.splitlines()[-1][1:]
        self._exec_n_expect("select-attribute " + attr_str, None)
        self._exec_n_expect("attribute-info", path)
        msg = bytestream_2_bytearray(data.encode('hex'))
        self._exec_n_not_expect("write " + msg, "Connected: no")
        return self._read(2048)

    def send_session_data(self, data):
        return self.send_data("0000ff51-0000-1000-8000-00805f9b34fb", data)

    def send_config_data(self, data):
        return self.send_data("0000ff52-0000-1000-8000-00805f9b34fb", data)

    def send_avsconfig_data(self, data):
        return None

    def close(self):
        if self.btctl:
            self._disconnect()
            self.btctl.kill(1)
            verbose_print ("is alive : " + str(self.btctl.isalive()), self.verbose)
        verbose_print ("closed", self.verbose)
