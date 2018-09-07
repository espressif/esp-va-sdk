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
import proto_python
from security import *

class Security0(Security):
    def __init__(self, verbose):
        self.session_state = 0
        self.verbose = verbose
        Security.__init__(self, self.security0_session)

    def security0_session(self, response_data):
        if (self.session_state == 0):
            self.session_state = 1
            return self.setup0_request()
        if (self.session_state == 1):
            self.setup0_response(response_data)
            return None

    def setup0_request(self):
        setup_req = proto_python.session_pb2.SessionData()
        setup_req.sec_ver = 0
        session_cmd = proto_python.sec0_pb2.S0SessionCmd()
        setup_req.sec0.sc.MergeFrom(session_cmd)
        return setup_req.SerializeToString()

    def setup0_response(self, response_data):
        setup_resp = proto_python.session_pb2.SessionData()
        setup_resp.ParseFromString(response_data.decode('hex'))
        if setup_resp.sec_ver != proto_python.session_pb2.SecScheme0:
            print "Incorrect sec scheme"

    def encrypt_data(self, data):
        return data

    def decrypt_data(self, data):
        return data
