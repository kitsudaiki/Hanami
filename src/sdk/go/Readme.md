# go-sdk

## prepare protobuf-message

```
cd hanami_sdk
protoc --go_out=. --proto_path ../../../libraries/hanami_messages/protobuffers hanami_messages.proto3
sed -i 's/hanami_messages/hanami_sdk/g' hanami_messages.proto3.pb.go
```
