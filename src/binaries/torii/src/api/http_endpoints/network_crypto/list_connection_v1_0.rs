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

use actix_web::web::Json;
use apistos::api_operation;

use crate::core::routing_interface::ROUTE_HANDLER;

use ainari_api::errors::ErrorResponse;
use ainari_api_structs::route_structs::*;
use ainari_api_structs::user_context::UserContext;

#[api_operation(
    tag = "network_crypto",
    summary = "List connections",
    description = r###"List the VM-to-VM connections this gateway knows about.

Shows for every connection whether its encryption is currently switched on and
which outbound key is in use, which is the quickest way to tell a protected
connection from a deliberately unprotected one."###,
    error_code = 401,
    error_code = 500
)]
pub async fn list_connection(
    _context: UserContext,
) -> Result<Json<ConnectionListResponse>, ErrorResponse> {
    let st = ROUTE_HANDLER.lock().await;

    let mut connections: Vec<Connection> = st.connections.values().cloned().collect();
    connections.sort_by(|a, b| (&a.local_ip, &a.remote_ip).cmp(&(&b.local_ip, &b.remote_ip)));

    Ok(Json(ConnectionListResponse { connections }))
}
