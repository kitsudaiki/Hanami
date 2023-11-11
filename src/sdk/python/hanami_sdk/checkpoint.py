# Copyright 2022 Tobias Anker
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from hanami_sdk import hanami_request
import json


def list_checkpoints(token: str, address: str) -> tuple[bool,str]:
    path = "/control/v1/checkpoint/all"
    return hanami_request.send_get_request(token, address, path, "")


def delete_checkpoint(token: str,
                      address: str, 
                      checkpoint_uuid: str) -> tuple[bool,str]:
    path = "/control/v1/checkpoint";
    values = f'uuid={checkpoint_uuid}'
    return hanami_request.send_delete_request(token, address, path, values)