#!python3

# Copyright 2022 Tobias Anker
#
# Licensed under the Apache License, Version 2.0 (the "License")
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

from hanami_sdk import hanami_token
from hanami_sdk import checkpoint
from hanami_sdk import cluster
from hanami_sdk import dataset
from hanami_sdk import direct_io
from hanami_sdk import hosts
from hanami_sdk import project
from hanami_sdk import request_result
from hanami_sdk import task
from hanami_sdk import user
from hanami_sdk import hanami_exceptions
import test_values
import json
import time
import configparser
import urllib3
import asyncio


# the test use insecure connections, which is totally ok for the tests
# and neaded for testings endpoints with self-signed certificastes,
# but the warnings are anoying and have to be disabled by this line
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

config = configparser.ConfigParser()
config.read('/etc/hanami/hanami_testing.conf')

address = config["connection"]["address"]
test_user_id = config["connection"]["test_user"]
test_user_pw = config["connection"]["test_pw"]

train_inputs = config["test_data"]["train_inputs"]
train_labels = config["test_data"]["train_labels"]
request_inputs = config["test_data"]["request_inputs"]
request_labels = config["test_data"]["request_labels"]

cluster_template = \
    "version: 1\n" \
    "settings:\n" \
    "   neuron_cooldown: 10000000.0\n" \
    "   refractory_time: 1\n" \
    "   max_connection_distance: 1\n" \
    "   enable_reduction: false\n" \
    "bricks:\n" \
    "    1,1,1\n" \
    "        input: test_input\n" \
    "        number_of_neurons: 784\n" \
    "    2,1,1\n" \
    "        number_of_neurons: 400\n" \
    "    3,1,1\n" \
    "        output: test_output\n" \
    "        number_of_neurons: 10"

user_id = "tsugumi"
user_name = "Tsugumi"
password = "new password"
is_admin = True
role = "tester"
projet_id = "test_project"
project_name = "Test Project"

cluster_name = "test_cluster"
checkpoint_name = "test_checkpoint"
generic_task_name = "test_task"
template_name = "dynamic"
request_dataset_name = "request_test_dataset"
train_dataset_name = "train_test_dataset"


def delete_all_cluster():
    result = cluster.list_clusters(token, address, False)
    body = json.loads(result)["body"]

    for entry in body:
        cluster.delete_cluster(token, address, entry[0], False)


def delete_all_projects():
    result = project.list_projects(token, address, False)
    body = json.loads(result)["body"]

    for entry in body:
        project.delete_project(token, address, entry[0], False)


def delete_all_user():
    result = user.list_users(token, address, False)
    body = json.loads(result)["body"]

    for entry in body:
        try:
            user.delete_user(token, address, entry[0], False)
        except hanami_exceptions.ConflictException:
            pass


def delete_all_datasets():
    result = dataset.list_datasets(token, address, False)
    body = json.loads(result)["body"]

    for entry in body:
        dataset.delete_dataset(token, address, entry[0], False)


def delete_all_checkpoints():
    result = checkpoint.list_checkpoints(token, address, False)
    body = json.loads(result)["body"]

    for entry in body:
        checkpoint.delete_checkpoint(token, address, entry[0], False)


def delete_all_results():
    result = request_result.list_request_results(token, address, False)
    print(result)
    body = json.loads(result)["body"]

    for entry in body:
        request_result.delete_request_result(token, address, entry[0], False)


def test_project():
    print("test project")

    project.create_project(token, address, projet_id, project_name, False)
    try:
        project.create_project(token, address, projet_id, project_name, False)
    except hanami_exceptions.ConflictException:
        pass
    project.list_projects(token, address, False)
    project.get_project(token, address, projet_id, False)
    try:
        project.get_project(token, address, "fail_project", False)
    except hanami_exceptions.NotFoundException:
        pass
    project.delete_project(token, address, projet_id, False)
    try:
        project.delete_project(token, address, projet_id, False)
    except hanami_exceptions.NotFoundException:
        pass


def test_user():
    print("test user")

    user.create_user(token, address, user_id, user_name, password, is_admin, False)
    try:
        user.create_user(token, address, user_id, user_name, password, is_admin, False)
    except hanami_exceptions.ConflictException:
        pass
    user.list_users(token, address, False)
    user.get_user(token, address, user_id, False)
    try:
        user.get_user(token, address, "fail_user", False)
    except hanami_exceptions.NotFoundException:
        pass
    user.delete_user(token, address, user_id, False)
    try:
        user.delete_user(token, address, user_id, False)
    except hanami_exceptions.NotFoundException:
        pass


def test_dataset():
    print("test dataset")

    result = dataset.upload_mnist_files(
        token, address, train_dataset_name, train_inputs, train_labels, False)
    dataset_uuid = result

    result = dataset.list_datasets(token, address, False)
    result = dataset.get_dataset(token, address, dataset_uuid, False)

    try:
        result = dataset.get_dataset(token, address, "fail_dataset", False)
    except hanami_exceptions.BadRequestException:
        pass
    result = dataset.delete_dataset(token, address, dataset_uuid, False)
    try:
        result = dataset.delete_dataset(token, address, dataset_uuid, False)
    except hanami_exceptions.NotFoundException:
        pass


def test_cluster():
    print("test cluster")

    result = cluster.create_cluster(token, address, cluster_name, cluster_template, False)
    cluster_uuid = json.loads(result)["uuid"]
    result = cluster.list_clusters(token, address, False)
    result = cluster.get_cluster(token, address, cluster_uuid, False)
    try:
        result = cluster.get_cluster(token, address, "fail_cluster", False)
    except hanami_exceptions.BadRequestException:
        pass
    result = cluster.delete_cluster(token, address, cluster_uuid, False)
    try:
        result = cluster.delete_cluster(token, address, cluster_uuid, False)
    except hanami_exceptions.NotFoundException:
        pass


async def test_direct_io(token, address, cluster_uuid):
    # check direct-mode
    ws = await cluster.switch_to_direct_mode(token, address, cluster_uuid, False)
    for i in range(0, 100):
        await direct_io.send_train_input(ws,
                                         "test_input",
                                         test_values.get_direct_io_test_intput(),
                                         False,
                                         False)
        await direct_io.send_train_input(ws,
                                         "test_output",
                                         test_values.get_direct_io_test_output(),
                                         True,
                                         False)
    output_values = await direct_io.send_request_input(ws,
                                                       "test_input",
                                                       test_values.get_direct_io_test_intput(),
                                                       True,
                                                       False)
    # print(output_values)
    await ws.close()

    cluster.switch_to_task_mode(token, address, cluster_uuid, False)

    assert list(output_values).index(max(output_values)) == 5


def test_workflow():
    print("test workflow")

    # init
    result = cluster.create_cluster(token, address, cluster_name, cluster_template, False)
    cluster_uuid = json.loads(result)["uuid"]
    train_dataset_uuid = dataset.upload_mnist_files(
        token, address, train_dataset_name, train_inputs, train_labels, False)
    request_dataset_uuid = dataset.upload_mnist_files(
        token, address, request_dataset_name, request_inputs, request_labels, False)

    result = hosts.list_hosts(token, address, False)
    hosts_json = json.loads(result)["body"]
    if len(hosts_json) > 1:
        print("test move cluster to gpu")
        target_host_uuid = hosts_json[1][0]
        cluster.switch_host(token, address, cluster_uuid, target_host_uuid, False)

    # run training
    for i in range(0, 5):
        result = task.create_task(
            token, address, generic_task_name, "train", cluster_uuid, train_dataset_uuid, False)
        task_uuid = json.loads(result)["uuid"]

        finished = False
        while not finished:
            result = task.get_task(token, address, task_uuid, cluster_uuid, False)
            finished = json.loads(result)["state"] == "finished"
            print("wait for finish train-task")
            time.sleep(1)

        result = task.delete_task(token, address, task_uuid, cluster_uuid, False)

    # save and reload checkpoint
    result = cluster.save_cluster(token, address, checkpoint_name, cluster_uuid, False)
    checkpoint_uuid = json.loads(result)["uuid"]
    result = checkpoint.list_checkpoints(token, address, False)
    # print(json.dumps(json.loads(result), indent=4))

    cluster.delete_cluster(token, address, cluster_uuid, False)
    result = cluster.create_cluster(token, address, cluster_name, cluster_template, False)
    cluster_uuid = json.loads(result)["uuid"]

    result = cluster.restore_cluster(token, address, checkpoint_uuid, cluster_uuid, False)
    result = checkpoint.delete_checkpoint(token, address, checkpoint_uuid, False)
    try:
        result = checkpoint.delete_checkpoint(token, address, checkpoint_uuid, False)
    except hanami_exceptions.NotFoundException:
        pass

    # run testing
    result = task.create_task(
        token, address, generic_task_name, "request", cluster_uuid, request_dataset_uuid, False)
    task_uuid = json.loads(result)["uuid"]

    finished = False
    while not finished:
        result = task.get_task(token, address, task_uuid, cluster_uuid, False)
        finished = json.loads(result)["state"] == "finished"
        print("wait for finish request-task")
        time.sleep(1)

    result = task.list_tasks(token, address, cluster_uuid, False)
    result = task.delete_task(token, address, task_uuid, cluster_uuid, False)

    # check request-result
    result = request_result.get_request_result(token, address, task_uuid, False)
    result = request_result.list_request_results(token, address, False)
    result = request_result.check_against_dataset(
        token, address, task_uuid, request_dataset_uuid, False)
    accuracy = json.loads(result)["accuracy"]
    print("=======================================")
    print("test-result: " + str(accuracy))
    print("=======================================")
    assert accuracy > 90.0
    result = request_result.delete_request_result(token, address, task_uuid, False)

    asyncio.run(test_direct_io(token, address, cluster_uuid))

    # cleanup
    dataset.delete_dataset(token, address, train_dataset_uuid, False)
    dataset.delete_dataset(token, address, request_dataset_uuid, False)
    cluster.delete_cluster(token, address, cluster_uuid, False)


token = hanami_token.request_token(address, test_user_id, test_user_pw, False)

delete_all_results()
delete_all_datasets()
delete_all_checkpoints()
delete_all_cluster()
delete_all_projects()
delete_all_user()

test_project()
test_user()
test_dataset()
test_cluster()
test_workflow()
