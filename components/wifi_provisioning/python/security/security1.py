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
from security import *

import proto_python
import curve25519
import Crypto.Cipher.AES
import Crypto.Util.Counter
import Crypto.Hash.SHA256

import session_pb2

class security_state:
    REQUEST1 = 0
    RESPONSE1_REQUEST2 = 1
    RESPONSE2 = 2
    FINISHED = 3

class Security1(Security):
    def __init__(self, pop, verbose):
        self.session_state = security_state.REQUEST1
        self.pop = pop
        self.verbose = verbose
        Security.__init__(self, self.security1_session)

    def security1_session(self, response_data):
        if (self.session_state == security_state.REQUEST1):
            self.session_state = security_state.RESPONSE1_REQUEST2
            return self.setup0_request()
        if (self.session_state == security_state.RESPONSE1_REQUEST2):
            self.session_state = security_state.RESPONSE2
            self.setup0_response(response_data)
            return self.setup1_request()
        if (self.session_state == security_state.RESPONSE2):
            self.session_state = security_state.FINISHED
            self.setup1_response(response_data)
            return None
        else:
            print "Unexpected state"
            return None

    def __generate_key(self):
        self.client_private_key = curve25519.genkey()
        self.client_public_key = curve25519.public(self.client_private_key)

    def _xor_two_str(self, a, b):
        ret = ''
        for i in range(max(len(a), len(b))):
            num = hex(ord(a[i%len(a)]) ^ ord(b[i%(len(b))]))[2:]
            if len(num) == 0:
                num = '00'
            if len(num) == 1:
                num = '0'+ num
            ret = ret + num
        return ret.decode('hex')

    def _print_verbose(self, data):
        if (self.verbose):
            print "++++ " + data + " ++++"

    def setup0_request(self):
        setup_req = session_pb2.SessionData()
        setup_req.sec_ver = session_pb2.SecScheme1
        self.__generate_key()
        setup_req.sec1.sc0.client_pubkey = self.client_public_key
        self._print_verbose("client_public_key:\t" +  setup_req.sec1.sc0.client_pubkey.encode('hex'))
        return setup_req.SerializeToString()

    def setup0_response(self, response_data):
        setup_resp = proto_python.session_pb2.SessionData()
        setup_resp.ParseFromString(response_data.decode('hex'))
        self._print_verbose("Security version:\t" + str(setup_resp.sec_ver))
        if setup_resp.sec_ver != session_pb2.SecScheme1:
            print "Incorrect sec scheme"
            exit(1)
        self._print_verbose("device_pubkey:\t"+setup_resp.sec1.sr0.device_pubkey.encode('hex'))
        self._print_verbose("device_random:\t"+setup_resp.sec1.sr0.device_random.encode('hex'))
        sharedK = curve25519.shared(self.client_private_key, setup_resp.sec1.sr0.device_pubkey)
        self._print_verbose("Shared Key:\t" + sharedK.encode('hex'))
        if len(self.pop) > 0:
            h = Crypto.Hash.SHA256.new()
            h.update(self.pop)
            digest = h.digest()
            sharedK = self._xor_two_str(sharedK, digest)
            self._print_verbose("New Shared Key xored with pop:\t" + sharedK.encode('hex'))
        ctr = Crypto.Util.Counter.new(128, initial_value=long(setup_resp.sec1.sr0.device_random.encode('hex'), 16))
        self._print_verbose("IV " +  hex(long(setup_resp.sec1.sr0.device_random.encode('hex'), 16)))
        self.cipher = Crypto.Cipher.AES.new(sharedK, Crypto.Cipher.AES.MODE_CTR, counter=ctr)
        self.client_verify = self.cipher.encrypt(setup_resp.sec1.sr0.device_pubkey)
        self._print_verbose("Client Verify:\t" + self.client_verify.encode('hex'))

    def setup1_request(self):
        setup_req = proto_python.session_pb2.SessionData()
        setup_req.sec_ver = session_pb2.SecScheme1
        setup_req.sec1.msg = proto_python.sec1_pb2.Session_Command1
        setup_req.sec1.sc1.client_verify_data = self.client_verify
        return setup_req.SerializeToString()

    def setup1_response(self, response_data):
        setup_resp = proto_python.session_pb2.SessionData()
        setup_resp.ParseFromString(response_data.decode('hex'))
        if setup_resp.sec_ver == session_pb2.SecScheme1:
            self._print_verbose("Device verify:\t"+setup_resp.sec1.sr1.device_verify_data.encode('hex'))
            enc_client_pubkey = self.cipher.encrypt(setup_resp.sec1.sr1.device_verify_data)
            self._print_verbose("Enc client pubkey:\t "+enc_client_pubkey.encode('hex'))
        else:
            print ("Unsupported security protocol")
            return -1

    def encrypt_data(self, data):
        return self.cipher.encrypt(data)

    def decrypt_data(self, data):
        return self.cipher.decrypt(data)
