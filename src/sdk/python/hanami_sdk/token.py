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

import requests
import json
from hanami_sdk import exceptions


def request_token(address: str, 
                  user_id: str, 
                  pw: str) -> str:
    url = f'{address}/control/v1/token'
    json_body = {
        "id": user_id,
        "password": pw,
    }

    response = requests.post(url, json=json_body) 
    if response.status_code == 200:
        return response.json()["token"]
    if response.status_code == 400:
        raise exceptions.BadRequestException(response.content)
    if response.status_code == 401:
        raise exceptions.UnauthorizedException(response.content)
    if response.status_code == 404:
        raise exceptions.NotFoundException(response.content)
    if response.status_code == 409:
        raise exceptions.ConflictException(response.content)
    if response.status_code == 500:
        raise exceptions.InternalServerErrorException()

# check_token