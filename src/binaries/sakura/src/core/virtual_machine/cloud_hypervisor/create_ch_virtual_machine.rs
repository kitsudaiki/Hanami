use cloud_hypervisor_client::apis::DefaultApi;
use cloud_hypervisor_client::models::{
    ConsoleMode, CpusConfig, DiskConfig, MemoryConfig, NetConfig, PayloadConfig, SerialConfig,
    VmConfig,
};
use cloud_hypervisor_client::socket_based_api_client;
use std::path::Path;
use std::process::Command;
use std::time::Duration;
use uuid::Uuid;

use ainari_common::config as ainari_config;
use ainari_common::error::AinariError;

pub async fn create_ch_virtual_machine(
    uuid: &Uuid,
    number_of_cores: i32,
    memory_size: i64,
    root_disk_path: Option<String>,
    seed_path: &String,
    tap_name: &String,
    mac_address: &String,
) -> Result<(), AinariError> {
    log::info!("Start creation of VM {uuid}");

    let socket_path = format!("/tmp/cloud-hypervisor-{}.sock", uuid);
    let _ = std::fs::remove_file(&socket_path);

    let vmm_process = Command::new("/tmp/cloud-hypervisor")
        .arg("--api-socket")
        .arg(&socket_path)
        .spawn();

    let mut child = match vmm_process {
        Ok(c) => c,
        Err(e) => {
            return Err(AinariError::InternalError(format!(
                "Failed to spawn VMM: {}",
                e
            )));
        }
    };

    tokio::time::sleep(Duration::from_millis(500)).await;

    let client = socket_based_api_client(&socket_path);

    let payload = PayloadConfig {
        firmware: Some(String::from("/usr/local/share/CLOUDHV.fd")),
        ..Default::default()
    };

    let (disk_path, has_root_disk) = if let Some(disk_path) = root_disk_path {
        (disk_path, true)
    } else {
        ("".to_string(), false)
    };

    let vm_config = VmConfig {
        payload,
        cpus: Some(CpusConfig {
            boot_vcpus: number_of_cores,
            max_vcpus: number_of_cores,
            ..Default::default()
        }),

        serial: Some(SerialConfig {
            mode: ConsoleMode::File,
            file: Some(format!("/tmp/{}-serial.log", uuid)),
            ..Default::default()
        }),

        net: Some(vec![NetConfig {
            tap: Some(tap_name.clone()),
            mac: Some(mac_address.clone()),
            ..Default::default()
        }]),
        memory: Some(MemoryConfig {
            size: memory_size,
            ..Default::default()
        }),
        disks: Some(vec![
            DiskConfig {
                path: Some(disk_path.clone()),
                readonly: Some(false),
                ..Default::default()
            },
            DiskConfig {
                path: Some(seed_path.clone()),
                readonly: Some(true),
                ..Default::default()
            },
        ]),
        ..Default::default()
    };

    log::info!("Creating VM {uuid} attached to {tap_name}");
    if let Err(e) = client.create_vm(vm_config).await {
        return Err(AinariError::InternalError(format!(
            "Create VM {} failed: {:?}",
            uuid, e
        )));
    }

    log::info!("Booting VM {uuid} attached to {tap_name}");
    if let Err(e) = client.boot_vm().await {
        return Err(AinariError::InternalError(format!(
            "Boot VM {} failed: {:?}",
            uuid, e
        )));
    }

    tokio::spawn(async move {
        let _ = child
            .wait()
            .map_err(|e| AinariError::InternalError(format!("Failed to spawn VM: {e}")))?;

        Ok::<(), AinariError>(())
    });

    log::info!("New VM {uuid} started");

    Ok(())
}
