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

use chrono::{DateTime, Utc};
use diesel::connection::SimpleConnection;
use diesel::dsl::count_star;
use diesel::prelude::*;
use std::error::Error;
use uuid::Uuid;

use crate::database::db_handle;

use ainari_api_structs::user_context::UserContext;
use ainari_common::enums;
use ainari_common::objects::*;

// Define the schema for floating_ips table
table! {
    floating_ips (uuid) {
        uuid -> Varchar,
        network_uuid -> Varchar,
        target_ip -> Varchar,
        floating_ip_address -> Varchar,
        owner_id -> Varchar,
        project_id -> Varchar,
        status -> Varchar,
        created_at -> Varchar,
        created_by -> Varchar,
        updated_at -> Varchar,
        updated_by -> Varchar,
        deleted_at -> Nullable<Varchar>,
        deleted_by -> Nullable<Varchar>,
    }
}

/// Represents an entry in the floating_ips table.
/// This struct contains all the fields required to create, query, and update meta floating_ip records.
#[derive(Insertable, Queryable, Selectable, Debug, PartialEq, Clone)]
#[diesel(table_name = floating_ips)]
pub struct FloatingIpEntry {
    #[diesel(serialize_as = DbUuid, deserialize_as = DbUuid)]
    pub uuid: Uuid,
    #[diesel(serialize_as = DbUuid, deserialize_as = DbUuid)]
    pub network_uuid: Uuid,
    pub target_ip: String,
    pub floating_ip_address: String,
    pub owner_id: String,
    pub project_id: String,
    pub status: String,
    #[diesel(serialize_as = DbDateTime, deserialize_as = DbDateTime)]
    pub created_at: DateTime<Utc>,
    pub created_by: String,
    #[diesel(serialize_as = DbDateTime, deserialize_as = DbDateTime)]
    pub updated_at: DateTime<Utc>,
    pub updated_by: String,
    #[diesel(serialize_as = DbOptDateTime, deserialize_as = DbOptDateTime)]
    pub deleted_at: Option<DateTime<Utc>>,
    pub deleted_by: Option<String>,
}

/// Initializes the floating_ips table in the database if it doesn't exist.
///
/// This function creates the table with the appropriate schema and constraints.
/// It's typically called during application startup to ensure the required tables exist.
pub fn init_floating_ip_table() -> Result<(), Box<dyn Error>> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    conn.batch_execute(
        "CREATE TABLE IF NOT EXISTS floating_ips (
        uuid VARCHAR(40) PRIMARY KEY,
        network_uuid VARCHAR(40),
        target_ip VARCHAR(40),
        floating_ip_address VARCHAR(40),
        owner_id VARCHAR(256),
        project_id VARCHAR(256),
        status VARCHAR(8),
        created_at VARCHAR(64),
        created_by VARCHAR(256),
        updated_at VARCHAR(64),
        updated_by VARCHAR(256),
        deleted_at VARCHAR(64),
        deleted_by VARCHAR(256)
    );",
    )?;

    Ok(())
}

/// Adds a new meta floating_ip to the database.
///
/// This function creates a new FloatingIpEntry with the provided parameters and inserts it into the database.
/// The status is set to "ACTIVE" and timestamps are set to the current time.
///
/// # Arguments
/// * `floating_ip_uuid` - The unique identifier for the meta floating_ip
/// * `floating_ip_name` - The name of the meta floating_ip
/// * `sakura_host_uuid` - The UUID of the Sakura host associated with this floating_ip
/// * `proxy_uuid` - The UUID of the proxy associated with this floating_ip
/// * `context` - The user context containing information about the user and project
///
/// # Returns
/// A QueryResult indicating the number of rows affected
pub fn add_new_floating_ip(
    floating_ip_uuid: &Uuid,
    network_uuid: &Uuid,
    target_ip: &String,
    floating_ip_address: &String,
    context: &UserContext,
) -> QueryResult<usize> {
    let floating_ip = FloatingIpEntry {
        uuid: *network_uuid,
        network_uuid: *floating_ip_uuid,
        target_ip: target_ip.clone(),
        floating_ip_address: floating_ip_address.clone(),
        owner_id: context.user_id.clone(),
        project_id: context.project_id.clone(),
        status: "ACTIVE".to_string(),
        created_at: Utc::now(),
        created_by: context.user_id.clone(),
        updated_at: Utc::now(),
        updated_by: context.user_id.clone(),
        deleted_at: None,
        deleted_by: None,
    };

    add_floating_ip(floating_ip)
}

/// Adds a meta floating_ip to the database.
///
/// This is a helper function that performs the actual insertion of a FloatingIpEntry into the database.
///
/// # Arguments
/// * `floating_ip` - The FloatingIpEntry to be inserted
///
/// # Returns
/// A QueryResult indicating the number of rows affected
pub fn add_floating_ip(floating_ip: FloatingIpEntry) -> QueryResult<usize> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;
    diesel::insert_into(floating_ips)
        .values(floating_ip)
        .execute(&mut *conn)
}

/// Retrieves a meta floating_ip from the database.
///
/// This function queries the database for a meta floating_ip with the specified UUID and checks the user's permissions.
/// Only active floating_ips are returned, and the query is filtered based on the user's role and project membership.
///
/// # Arguments
/// * `floating_ip_uuid` - The UUID of the meta floating_ip to retrieve
/// * `context` - The user context containing information about the user and their permissions
///
/// # Returns
/// A Result containing the FloatingIpEntry if found, or a DbError if not found or an error occurs
pub fn get_floating_ip(
    floating_ip_uuid: &Uuid,
    context: &UserContext,
) -> Result<FloatingIpEntry, enums::DbError> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;

    let mut query = floating_ips
        .filter(
            uuid.eq(floating_ip_uuid.to_string())
                .and(status.eq("ACTIVE")),
        )
        .into_boxed();

    // Apply permission-based filtering
    if context.is_admin != true.to_string() {
        query = query.filter(project_id.eq(context.project_id.clone()));
        if context.is_project_admin != true.to_string() {
            query = query.filter(owner_id.eq(context.user_id.clone()));
        }
    }

    match query
        .select(FloatingIpEntry::as_select())
        .first::<FloatingIpEntry>(&mut *conn)
    {
        Ok(floating_ip) => Ok(floating_ip),
        Err(diesel::result::Error::NotFound) => Err(enums::DbError::NotFound),
        Err(e) => {
            log::error!("Database-error: {e:?}");
            Err(enums::DbError::InternalError)
        }
    }
}

/// Lists all meta floating_ips that the user has access to.
///
/// This function retrieves all active meta floating_ips and applies permission-based filtering.
/// The results are filtered based on the user's role and project membership.
///
/// # Arguments
/// * `context` - The user context containing information about the user and their permissions
///
/// # Returns
/// A QueryResult containing a vector of FloatingIpEntry objects
#[allow(dead_code)]
pub fn list_floating_ips(context: &UserContext) -> QueryResult<Vec<FloatingIpEntry>> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;

    let mut query = floating_ips.filter(status.eq("ACTIVE")).into_boxed();

    // Apply permission-based filtering
    if context.is_admin != true.to_string() {
        query = query.filter(project_id.eq(context.project_id.clone()));
        if context.is_project_admin != true.to_string() {
            query = query.filter(owner_id.eq(context.user_id.clone()));
        }
    }

    query.select(FloatingIpEntry::as_select()).load(&mut *conn)
}

/// Counts the number of meta floating_ips that the user has access to.
///
/// This function counts all active meta floating_ips and applies permission-based filtering.
/// The count is filtered based on the user's role and project membership.
///
/// # Arguments
/// * `context` - The user context containing information about the user and their permissions
///
/// # Returns
/// A QueryResult containing the count of meta floating_ips as an i64
pub fn count_floating_ips(context: &UserContext) -> QueryResult<i64> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;

    let mut query = floating_ips.filter(status.eq("ACTIVE")).into_boxed();

    // Apply permission-based filtering
    query = query.filter(project_id.eq(context.project_id.clone()));
    query = query.filter(owner_id.eq(context.user_id.clone()));

    query.select(count_star()).first::<i64>(&mut *conn)
}

/// Force deletes a meta floating_ip from the database.
///
/// This function marks a meta floating_ip as deleted without checking permissions.
/// It's intended for system-level operations where permission checks are not required.
///
/// # Arguments
/// * `floating_ip_uuid` - The UUID of the meta floating_ip to delete
///
/// # Returns
/// A Result indicating success or an error
#[allow(dead_code)]
pub fn force_delete_floating_ip(floating_ip_uuid: &Uuid) -> Result<(), enums::DbError> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;
    match diesel::update(floating_ips.filter(uuid.eq(floating_ip_uuid.to_string())))
        .set((
            status.eq("DELETED"),
            deleted_at.eq(Utc::now().to_rfc3339()),
            deleted_by.eq("HOST_INIT"),
        ))
        .execute(&mut *conn)
    {
        Ok(_) => Ok(()),
        Err(diesel::result::Error::NotFound) => Err(enums::DbError::NotFound),
        Err(e) => {
            log::error!("Database-error: {e:?}");
            Err(enums::DbError::InternalError)
        }
    }
}

/// Deletes a meta floating_ip from the database.
///
/// This function marks a meta floating_ip as deleted after verifying that the user has permission to delete it.
/// It first checks if the floating_ip exists and if the user has the necessary permissions.
///
/// # Arguments
/// * `floating_ip_uuid` - The UUID of the meta floating_ip to delete
/// * `context` - The user context containing information about the user and their permissions
///
/// # Returns
/// A Result indicating success or an error
pub fn delete_floating_ip(
    floating_ip_uuid: &Uuid,
    context: &UserContext,
) -> Result<(), enums::DbError> {
    // Verify the meta floating_ip exists and the user has permission to delete it
    get_floating_ip(floating_ip_uuid, context)?;

    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;
    match diesel::update(floating_ips.filter(uuid.eq(floating_ip_uuid.to_string())))
        .set((
            status.eq("DELETED"),
            deleted_at.eq(Utc::now().to_rfc3339()),
            deleted_by.eq(context.user_id.clone()),
        ))
        .execute(&mut *conn)
    {
        Ok(_) => Ok(()),
        Err(diesel::result::Error::NotFound) => Err(enums::DbError::NotFound),
        Err(e) => {
            log::error!("Database-error: {e:?}");
            Err(enums::DbError::InternalError)
        }
    }
}

/// Deletes all meta floating_ips from the database.
///
/// This function marks all active meta floating_ips as deleted without checking permissions.
/// It's intended for system-level operations where permission checks are not required.
///
/// # Returns
/// A Result indicating success or an error
#[allow(dead_code)]
pub fn delete_all_floating_ip() -> Result<(), enums::DbError> {
    let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
    use self::floating_ips::dsl::*;
    match diesel::update(floating_ips.filter(status.eq("ACTIVE")))
        .set((
            status.eq("DELETED"),
            deleted_at.eq(Utc::now().to_rfc3339()),
            deleted_by.eq("AINARI_START"),
        ))
        .execute(&mut *conn)
    {
        Ok(_) => Ok(()),
        Err(diesel::result::Error::NotFound) => Err(enums::DbError::NotFound),
        Err(e) => {
            log::error!("Database-error: {e:?}");
            Err(enums::DbError::InternalError)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serial_test::serial;

    fn hard_delete_floating_ip(floating_ip_uuid: &Uuid) {
        use self::floating_ips::dsl::*;
        let mut conn = db_handle::DB_CONN.lock().expect("mutex poisoned");
        let _ = diesel::delete(floating_ips.filter(uuid.eq(floating_ip_uuid.to_string())))
            .execute(&mut *conn);
    }

    #[test]
    #[serial]
    fn test_add_get_floating_ip() {
        let _ = init_floating_ip_table();
        let uuid1 = Uuid::new_v4();
        let network_uuid = Uuid::new_v4();
        let target_ip = "127.0.0.1".to_owned();
        let floating_ip_address = "192.168.0.1".to_owned();

        let project_id = "test-project".to_string();
        let owner_id = "test-user".to_string();
        let context = UserContext {
            token: "".to_string(),
            user_id: owner_id.clone(),
            project_id: project_id.clone(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };

        let floating_ip = FloatingIpEntry {
            uuid: uuid1,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        hard_delete_floating_ip(&uuid1);

        add_floating_ip(floating_ip.clone()).unwrap();
        match get_floating_ip(&uuid1, &context) {
            Ok(retrieved_floating_ip) => {
                assert_eq!(retrieved_floating_ip.uuid, floating_ip.uuid);
                assert_eq!(retrieved_floating_ip.network_uuid, floating_ip.network_uuid);
                assert_eq!(retrieved_floating_ip.target_ip, floating_ip.target_ip);
                assert_eq!(
                    retrieved_floating_ip.floating_ip_address,
                    floating_ip.floating_ip_address
                );
                assert_eq!(retrieved_floating_ip.owner_id, floating_ip.owner_id);
                assert_eq!(retrieved_floating_ip.project_id, floating_ip.project_id);
                assert_eq!(retrieved_floating_ip.status, floating_ip.status);
                assert_eq!(retrieved_floating_ip.created_by, floating_ip.created_by);
                assert_eq!(retrieved_floating_ip.updated_by, floating_ip.updated_by);
                assert_eq!(retrieved_floating_ip.deleted_at, floating_ip.deleted_at);
                assert_eq!(retrieved_floating_ip.deleted_by, floating_ip.deleted_by);
            }
            Err(_) => {
                assert_eq!(true, false);
            }
        };

        hard_delete_floating_ip(&uuid1);
    }

    #[test]
    #[serial]
    fn test_list_floating_ips() {
        let _ = init_floating_ip_table();
        let uuid1 = Uuid::new_v4();
        let uuid2 = Uuid::new_v4();
        let network_uuid = Uuid::new_v4();
        let target_ip = "127.0.0.1".to_owned();
        let floating_ip_address = "192.168.0.1".to_owned();

        let project_id = "test-project".to_string();
        let owner_id = "test-user".to_string();
        let context = UserContext {
            token: "".to_string(),
            user_id: owner_id.clone(),
            project_id: project_id.clone(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };

        let floating_ip1 = FloatingIpEntry {
            uuid: uuid1,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        let floating_ip2 = FloatingIpEntry {
            uuid: uuid2,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "DELETED".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        hard_delete_floating_ip(&uuid1);
        hard_delete_floating_ip(&uuid2);

        add_floating_ip(floating_ip1).unwrap();
        add_floating_ip(floating_ip2).unwrap();
        let floating_ips = list_floating_ips(&context).unwrap();
        assert_eq!(floating_ips.len(), 1);
        hard_delete_floating_ip(&uuid1);
        hard_delete_floating_ip(&uuid2);
    }

    #[test]
    #[serial]
    fn test_delete_floating_ip() {
        let _ = init_floating_ip_table();
        let uuid1 = Uuid::new_v4();
        let network_uuid = Uuid::new_v4();
        let target_ip = "127.0.0.1".to_owned();
        let floating_ip_address = "192.168.0.1".to_owned();

        let project_id = "test-project".to_string();
        let owner_id = "test-user".to_string();
        let context = UserContext {
            token: "".to_string(),
            user_id: owner_id.clone(),
            project_id: project_id.clone(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };

        let floating_ip = FloatingIpEntry {
            uuid: uuid1,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        hard_delete_floating_ip(&uuid1);

        add_floating_ip(floating_ip.clone()).unwrap();
        let _ = delete_floating_ip(&uuid1, &context);
        let result = get_floating_ip(&uuid1, &context);
        assert!(result.is_err());
    }

    #[test]
    #[serial]
    fn test_count_floating_ips() {
        let _ = init_floating_ip_table();
        let uuid1 = Uuid::new_v4();
        let uuid2 = Uuid::new_v4();
        let uuid3 = Uuid::new_v4();
        let network_uuid = Uuid::new_v4();
        let target_ip = "127.0.0.1".to_owned();
        let floating_ip_address = "192.168.0.1".to_owned();

        let project_id = "test-project".to_string();
        let owner_id = "test-user".to_string();
        let context = UserContext {
            token: "".to_string(),
            user_id: owner_id.clone(),
            project_id: project_id.clone(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };

        let floating_ip1 = FloatingIpEntry {
            uuid: uuid1,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        let floating_ip2 = FloatingIpEntry {
            uuid: uuid2,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        let floating_ip3 = FloatingIpEntry {
            uuid: uuid3,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: owner_id.clone(),
            project_id: project_id.clone(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        hard_delete_floating_ip(&uuid1);
        hard_delete_floating_ip(&uuid2);
        hard_delete_floating_ip(&uuid3);

        add_floating_ip(floating_ip1).unwrap();
        add_floating_ip(floating_ip2).unwrap();
        add_floating_ip(floating_ip3).unwrap();

        let number = count_floating_ips(&context).unwrap();
        assert_eq!(number, 3);

        hard_delete_floating_ip(&uuid1);
        hard_delete_floating_ip(&uuid2);
        hard_delete_floating_ip(&uuid3);
    }

    #[test]
    #[serial]
    fn test_floating_ips_permissions() {
        let _ = init_floating_ip_table();
        let uuid1 = Uuid::new_v4();
        let uuid2 = Uuid::new_v4();
        let uuid3 = Uuid::new_v4();
        let network_uuid = Uuid::new_v4();
        let target_ip = "127.0.0.1".to_owned();
        let floating_ip_address = "192.168.0.1".to_owned();

        let floating_ip1 = FloatingIpEntry {
            uuid: uuid1,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        let floating_ip2 = FloatingIpEntry {
            uuid: uuid2,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: "test-user-43".to_string(),
            project_id: "test_permissions_1".to_string(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        let floating_ip3 = FloatingIpEntry {
            uuid: uuid3,
            network_uuid: network_uuid,
            target_ip: target_ip.clone(),
            floating_ip_address: floating_ip_address.clone(),
            owner_id: "test-user-44".to_string(),
            project_id: "test_permissions_2".to_string(),
            status: "ACTIVE".to_string(),
            created_at: Utc::now(),
            created_by: "admin".to_string(),
            updated_at: Utc::now(),
            updated_by: "admin".to_string(),
            deleted_at: None,
            deleted_by: None,
        };

        hard_delete_floating_ip(&uuid1);
        hard_delete_floating_ip(&uuid2);
        hard_delete_floating_ip(&uuid3);

        add_floating_ip(floating_ip1).unwrap();
        add_floating_ip(floating_ip2).unwrap();
        add_floating_ip(floating_ip3).unwrap();

        // list-test normal user
        let context = UserContext {
            token: "".to_string(),
            user_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };
        let floating_ips = list_floating_ips(&context).unwrap();
        assert_eq!(floating_ips.len(), 1);

        // list-test project-admin
        let context = UserContext {
            token: "".to_string(),
            user_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            is_admin: false.to_string(),
            is_project_admin: true.to_string(),
        };
        let floating_ips = list_floating_ips(&context).unwrap();
        assert_eq!(floating_ips.len(), 2);

        // list-test admin
        let context = UserContext {
            token: "".to_string(),
            user_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            is_admin: true.to_string(),
            is_project_admin: false.to_string(),
        };
        let floating_ips = list_floating_ips(&context).unwrap();
        assert_eq!(floating_ips.len(), 3);

        // get-test normal user
        let context = UserContext {
            token: "".to_string(),
            user_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };
        match get_floating_ip(&uuid1, &context) {
            Ok(retrieved_floating_ip) => {
                assert_eq!(retrieved_floating_ip.uuid, uuid1);
            }
            Err(_) => {
                assert_eq!(true, false);
            }
        };

        // get-test normal user false uuid
        let context = UserContext {
            token: "".to_string(),
            user_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };
        if get_floating_ip(&uuid3, &context).is_ok() {
            assert_eq!(true, false);
        };

        // delete-test normal user false uuid
        let context = UserContext {
            token: "".to_string(),
            user_id: "test-user-42".to_string(),
            project_id: "test_permissions_1".to_string(),
            is_admin: false.to_string(),
            is_project_admin: false.to_string(),
        };
        if delete_floating_ip(&uuid3, &context).is_ok() {
            assert_eq!(true, false);
        };

        hard_delete_floating_ip(&uuid1);
        hard_delete_floating_ip(&uuid2);
        hard_delete_floating_ip(&uuid3);
    }
}
