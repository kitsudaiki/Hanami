// Copyright 2022-2026 Tobias Anker <tobias.anker@kitsunemimi.moe>

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use neko_wrapper::root_wrapper_client::RootWrapperClient;
use neko_wrapper::CommandRequest;

use ainari_common::errors::*;

pub mod wrapper {
    tonic::include_proto!("root_wrapper");
}

pub async fn run_client() -> Result<(), AinariError> {
    let mut client = RootWrapperClient::connect("http://127.0.0.1:54515")
        .await
        .map_err(|e| TestError::InternalError(e.to_string()))?;

    let req = tonic::Request::new(CommandRequest {
        command: "apt-get".into(),
        args: vec!["install".into(), "nginx".into()],
    });

    // 2. Execute (map tonic::Status to TestError)
    let response = client
        .execute(req)
        .await
        .map_err(|e| TestError::InternalError(e.to_string()))?;

    println!("Response: {:?}", response.into_inner());

    Ok(())
}