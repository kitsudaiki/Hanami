To build the protobuf-messages, run from the parent directory:

`protoc --python_out=. --proto_path ../../../libraries/hanami_messages/protobuffers  hanami_messages.proto3`

This build-process is also executed, when installing the sdk via `pip3 install .`
