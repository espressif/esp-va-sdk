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
from transport import *

conn = None

class Transport_Softap(Transport):
    def __init__(self, url):
        self.conn = httplib.HTTPConnection(url, timeout=30)
        self.headers = {"Content-type": "application/x-www-form-urlencoded","Accept": "text/plain"}

    def _send_post_request(self, path, data):
        try:
            self.conn.request("POST", path, data, self.headers)
        except Exception as err:
            print "error:", err
            return None
        response = self.conn.getresponse()
        response_data = response.read().encode('hex')
        return response_data

    def send_session_data(self, data):
        return self._send_post_request('/prov-session', data)

    def send_config_data(self, data):
        return self._send_post_request('/prov-config', data)

    def send_avsconfig_data(self, data):
        return self._send_post_request('/avsconfig', data)
