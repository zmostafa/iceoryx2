// Copyright (c) 2023 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache Software License 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0, or the MIT license
// which is available at https://opensource.org/licenses/MIT.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

//! [`SharedMemory`] based implementation of a [`DynamicStorage`].
//!
//! # Example
//!
//! ```
//! use iceoryx2_bb_system_types::file_name::FileName;
//! use iceoryx2_bb_container::semantic_string::SemanticString;
//! use iceoryx2_cal::dynamic_storage::posix_shared_memory::*;
//! use iceoryx2_cal::named_concept::*;
//! use std::sync::atomic::{AtomicI64, Ordering};
//!
//! let additional_size: usize = 1024;
//! let storage_name = FileName::new(b"myStorageName").unwrap();
//! let owner = Builder::new(&storage_name)
//!                 .supplementary_size(additional_size)
//!                 // we always have to use a thread-safe object since multiple processes can
//!                 // access this concurrently
//!                 .create(AtomicI64::new(0)).unwrap();
//! owner.get().store(123, Ordering::Relaxed);
//!
//! // usually a different process
//! let storage = Builder::<AtomicI64>::new(&storage_name)
//!                 .open().unwrap();
//!
//! println!("Initial value: {}", storage.get().load(Ordering::Relaxed));
//! // returns a reference to the underlying atomic
//! storage.get().store(456, Ordering::Relaxed);
//!
//! ```

use iceoryx2_bb_log::fail;
use iceoryx2_bb_posix::directory::*;
use iceoryx2_bb_posix::file_descriptor::FileDescriptorManagement;
use iceoryx2_bb_posix::shared_memory::*;
use std::fmt::Debug;
use std::fmt::Display;
use std::marker::PhantomData;
use std::ptr::NonNull;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;

pub use crate::dynamic_storage::*;
use crate::static_storage::file::NamedConceptConfiguration;
use crate::static_storage::file::NamedConceptRemoveError;
use iceoryx2_bb_system_types::path::Path;
pub use std::ops::Deref;

const FINAL_PERMISSIONS: Permission = Permission::OWNER_ALL;

#[derive(PartialEq, Eq, PartialOrd, Ord)]
struct PackageVersion(u64);

impl PackageVersion {
    fn from_u64(value: u64) -> Self {
        Self(value)
    }

    fn from_version(major: u16, minor: u16, patch: u16) -> Self {
        Self(((major as u64) << 32) | ((minor as u64) << 16) | patch as u64)
    }

    fn major(&self) -> u16 {
        ((self.0 >> 32) & (u16::MAX as u64)) as u16
    }

    fn minor(&self) -> u16 {
        ((self.0 >> 16) & (u16::MAX as u64)) as u16
    }

    fn patch(&self) -> u16 {
        ((self.0) & (u16::MAX as u64)) as u16
    }
}

impl Display for PackageVersion {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}.{}.{}", self.major(), self.minor(), self.patch())
    }
}

fn get_package_version() -> PackageVersion {
    static PACKAGE_VERSION: AtomicU64 = AtomicU64::new(0);

    if PACKAGE_VERSION.load(Ordering::Relaxed) == 0 {
        let major = option_env!("CARGO_PKG_VERSION_MAJOR")
            .unwrap_or("65535")
            .parse::<u16>()
            .unwrap_or(65535);
        let minor = option_env!("CARGO_PKG_VERSION_MINOR")
            .unwrap_or("65535")
            .parse::<u16>()
            .unwrap_or(65535);
        let patch = option_env!("CARGO_PKG_VERSION_PATCH")
            .unwrap_or("65535")
            .parse::<u16>()
            .unwrap_or(65535);

        PACKAGE_VERSION.store(
            PackageVersion::from_version(major, minor, patch).0,
            Ordering::Relaxed,
        );
    }

    PackageVersion::from_u64(PACKAGE_VERSION.load(Ordering::Relaxed))
}

/// The builder of [`Storage`].
#[derive(Debug)]
pub struct Builder<T: Send + Sync + Debug> {
    storage_name: FileName,
    supplementary_size: usize,
    has_ownership: bool,
    config: Configuration,
    _phantom_data: PhantomData<T>,
}

#[derive(Debug, Clone)]
pub struct Configuration {
    suffix: FileName,
    prefix: FileName,
    path: Path,
}

#[repr(C)]
struct Data<T: Send + Sync + Debug> {
    version: AtomicU64,
    data: T,
}

impl Default for Configuration {
    fn default() -> Self {
        Self {
            path: Storage::<()>::default_path_hint(),
            suffix: Storage::<()>::default_suffix(),
            prefix: Storage::<()>::default_prefix(),
        }
    }
}

impl NamedConceptConfiguration for Configuration {
    fn prefix(mut self, value: FileName) -> Self {
        self.prefix = value;
        self
    }

    fn get_prefix(&self) -> &FileName {
        &self.prefix
    }

    fn suffix(mut self, value: FileName) -> Self {
        self.suffix = value;
        self
    }

    fn path_hint(mut self, value: Path) -> Self {
        self.path = value;
        self
    }

    fn get_suffix(&self) -> &FileName {
        &self.suffix
    }

    fn get_path_hint(&self) -> &Path {
        &self.path
    }
}

impl<T: Send + Sync + Debug> NamedConceptBuilder<Storage<T>> for Builder<T> {
    fn new(storage_name: &FileName) -> Self {
        Self {
            has_ownership: true,
            storage_name: *storage_name,
            supplementary_size: 0,
            config: Configuration::default(),
            _phantom_data: PhantomData,
        }
    }

    fn config(mut self, config: &Configuration) -> Self {
        self.config = config.clone();
        self
    }
}

impl<T: Send + Sync + Debug> DynamicStorageBuilder<T, Storage<T>> for Builder<T> {
    fn has_ownership(mut self, value: bool) -> Self {
        self.has_ownership = value;
        self
    }

    fn supplementary_size(mut self, value: usize) -> Self {
        self.supplementary_size = value;
        self
    }

    fn create_and_initialize<F: FnOnce(&mut T, &mut BumpAllocator) -> bool>(
        self,
        initial_value: T,
        initializer: F,
    ) -> Result<Storage<T>, DynamicStorageCreateError> {
        let msg = "Failed to create dynamic_storage::PosixSharedMemory";

        let full_name = self.config.path_for(&self.storage_name).file_name();
        let mut shm = match SharedMemoryBuilder::new(&full_name)
            .creation_mode(CreationMode::CreateExclusive)
            // posix shared memory is always aligned to the greatest possible value (PAGE_SIZE)
            // therefore we do not have to add additional alignment space for T
            .size(std::mem::size_of::<Data<T>>() + self.supplementary_size)
            .permission(Permission::OWNER_WRITE)
            .zero_memory(false)
            .has_ownership(self.has_ownership)
            .create()
        {
            Ok(v) => v,
            Err(SharedMemoryCreationError::AlreadyExist) => {
                fail!(from self, with DynamicStorageCreateError::AlreadyExists,
                    "{} since a shared memory with the name already exists.", msg);
            }
            Err(SharedMemoryCreationError::InsufficientPermissions) => {
                fail!(from self, with DynamicStorageCreateError::InsufficientPermissions,
                    "{} due to insufficient permissions.", msg);
            }
            Err(_) => {
                fail!(from self, with DynamicStorageCreateError::InternalError,
                    "{} since the underlying shared memory could not be created.", msg);
            }
        };

        let value = shm.base_address().as_ptr() as *mut Data<T>;
        unsafe { core::ptr::addr_of_mut!((*value).data).write(initial_value) };

        let supplementary_start =
            (shm.base_address().as_ptr() as usize + std::mem::size_of::<Data<T>>()) as *mut u8;
        let supplementary_len = shm.size() - std::mem::size_of::<Data<T>>();

        let mut allocator = BumpAllocator::new(
            unsafe { NonNull::new_unchecked(supplementary_start) },
            supplementary_len,
        );

        if !initializer(unsafe { &mut (*value).data }, &mut allocator) {
            fail!(from self, with DynamicStorageCreateError::InitializationFailed,
                "{} since the initialization of the underlying construct failed.", msg);
        }

        unsafe {
            core::ptr::addr_of_mut!((*value).version).write(AtomicU64::new(get_package_version().0))
        };

        if let Err(e) = shm.set_permission(FINAL_PERMISSIONS) {
            fail!(from self, with DynamicStorageCreateError::InternalError,
                "{} since the final permissions could not be applied to the underlying shared memory ({:?}).",
                msg, e);
        }

        Ok(Storage {
            shm,
            name: self.storage_name,
            _phantom_data: PhantomData,
        })
    }

    fn open(self) -> Result<Storage<T>, DynamicStorageOpenError> {
        let msg = "Failed to open ";
        let origin = format!("{:?}", self);
        match self.try_open() {
            Err(DynamicStorageOpenError::DoesNotExist) => {
                fail!(from origin, with DynamicStorageOpenError::DoesNotExist, "{} since a shared memory with that name does not exists.", msg);
            }
            Err(DynamicStorageOpenError::InitializationNotYetFinalized) => {
                fail!(from origin, with DynamicStorageOpenError::InitializationNotYetFinalized, "{} since it is not yet readable - most likely since it is not finalized.", msg);
            }
            Err(e) => Err(e),
            Ok(s) => Ok(s),
        }
    }

    fn try_open(self) -> Result<Storage<T>, DynamicStorageOpenError> {
        let msg = "Failed to open ";

        let full_name = self.config.path_for(&self.storage_name).file_name();
        let shm = match SharedMemoryBuilder::new(&full_name).open_existing(AccessMode::ReadWrite) {
            Ok(v) => v,
            Err(SharedMemoryCreationError::DoesNotExist) => {
                return Err(DynamicStorageOpenError::DoesNotExist);
            }
            Err(SharedMemoryCreationError::InsufficientPermissions) => {
                return Err(DynamicStorageOpenError::InitializationNotYetFinalized);
            }
            Err(_) => {
                fail!(from self, with DynamicStorageOpenError::InternalError, "{} since the underlying shared memory could not be opened.", msg);
            }
        };

        let required_size = std::mem::size_of::<Data<T>>() + self.supplementary_size;
        if shm.size() < required_size {
            fail!(from self, with DynamicStorageOpenError::InternalError,
                "{} since the actual size {} does not match the required size of {}.", msg, shm.size(), required_size);
        }

        let init_state = shm.base_address().as_ptr() as *const Data<T>;
        let package_version = unsafe { &(*init_state) }
            .version
            .load(std::sync::atomic::Ordering::Relaxed);
        let package_version = PackageVersion::from_u64(package_version);
        if package_version.0 == 0 {
            return Err(DynamicStorageOpenError::InitializationNotYetFinalized);
        } else if package_version != get_package_version() {
            fail!(from self, with DynamicStorageOpenError::VersionMismatch,
                "{} since the dynamic storage was created with version {} but this process requires version {}.",
                msg, package_version, get_package_version());
        }

        Ok(Storage {
            shm,
            name: self.storage_name,
            _phantom_data: PhantomData,
        })
    }
}

/// Implements [`DynamicStorage`] for POSIX shared memory. It is built by
/// [`Builder`].
#[derive(Debug)]
pub struct Storage<T> {
    shm: SharedMemory,
    name: FileName,
    _phantom_data: PhantomData<T>,
}

impl<T: Send + Sync + Debug> NamedConcept for Storage<T> {
    fn name(&self) -> &FileName {
        &self.name
    }
}

impl<T: Send + Sync + Debug> NamedConceptMgmt for Storage<T> {
    type Configuration = Configuration;

    fn does_exist_cfg(
        name: &FileName,
        cfg: &Self::Configuration,
    ) -> Result<bool, crate::static_storage::file::NamedConceptDoesExistError> {
        let full_name = cfg.path_for(name).file_name();

        Ok(iceoryx2_bb_posix::shared_memory::SharedMemory::does_exist(
            &full_name,
        ))
    }

    fn list_cfg(
        config: &Self::Configuration,
    ) -> Result<Vec<FileName>, crate::static_storage::file::NamedConceptListError> {
        let entries = SharedMemory::list();

        let mut result = vec![];
        for entry in &entries {
            if let Some(entry_name) = config.extract_name_from_file(entry) {
                result.push(entry_name);
            }
        }

        Ok(result)
    }

    unsafe fn remove_cfg(
        name: &FileName,
        cfg: &Self::Configuration,
    ) -> Result<bool, crate::static_storage::file::NamedConceptRemoveError> {
        let full_name = cfg.path_for(name).file_name();
        let msg = "Unable to remove dynamic_storage::posix_shared_memory";
        let origin = "dynamic_storage::posix_shared_memory::Storage::remove_cfg()";

        match iceoryx2_bb_posix::shared_memory::SharedMemory::remove(&full_name) {
            Ok(v) => Ok(v),
            Err(
                iceoryx2_bb_posix::shared_memory::SharedMemoryRemoveError::InsufficientPermissions,
            ) => {
                fail!(from origin, with NamedConceptRemoveError::InsufficientPermissions,
                             "{} \"{}\" due to insufficient permissions.", msg, name);
            }
            Err(v) => {
                fail!(from origin, with NamedConceptRemoveError::InternalError,
                            "{} \"{}\" due to an internal failure ({:?}).", msg, name, v);
            }
        }
    }
}

impl<T: Send + Sync + Debug> DynamicStorage<T> for Storage<T> {
    type Builder = Builder<T>;

    fn does_support_persistency() -> bool {
        SharedMemory::does_support_persistency()
    }

    fn acquire_ownership(&self) {
        self.shm.acquire_ownership()
    }

    fn get(&self) -> &T {
        unsafe { &(*(self.shm.base_address().as_ptr() as *const Data<T>)).data }
    }

    fn has_ownership(&self) -> bool {
        self.shm.has_ownership()
    }

    fn release_ownership(&self) {
        self.shm.release_ownership()
    }
}
