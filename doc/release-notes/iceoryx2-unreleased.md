# iceoryx2 v?.?.?

## [vx.x.x](https://github.com/eclipse-iceoryx/iceoryx2/tree/vx.x.x)

[Full Changelog](https://github.com/eclipse-iceoryx/iceoryx2/compare/vx.x.x...vx.x.x)

### Features

<!--
    NOTE: Add new entries sorted by issue number to minimize the possibility of
    conflicts when merging.
-->

* Read LogLevel from environment variable
    [#396](https://github.com/eclipse-iceoryx/iceoryx2/issues/396)
* Lookup config file in default locations
    [#442](https://github.com/eclipse-iceoryx/iceoryx2/issues/442)
* Introduce `socket_pair` abstraction in POSIX wrapper
    [#508](https://github.com/eclipse-iceoryx/iceoryx2/issues/508)
* Introduce `socket_pair` event concept
    [#508](https://github.com/eclipse-iceoryx/iceoryx2/issues/508)
* Deadline property for event services
    [#573](https://github.com/eclipse-iceoryx/iceoryx2/issues/573)
* Use 'std_instead_of_core' clippy warning
    [#579](https://github.com/eclipse-iceoryx/iceoryx2/issues/579)
* Use 'std_instead_of_alloc' and 'alloc_instead_of_core' clippy warning
    [#581](https://github.com/eclipse-iceoryx/iceoryx2/issues/581)
* Introduce platform abstraction based on the 'libc' crate
    [#604](https://github.com/eclipse-iceoryx/iceoryx2/issues/604)
* Extend benchmarks to test setups with multiple sending/receiving ports
    [#610](https://github.com/eclipse-iceoryx/iceoryx2/issues/610)
* Reduce iceoryx2 dependencies
    [#640](https://github.com/eclipse-iceoryx/iceoryx2/issues/640)
* Allow customizable payload and user header type name in C++ binding
    [#643](https://github.com/eclipse-iceoryx/iceoryx2/issues/643)

### Bugfixes

<!--
    NOTE: Add new entries sorted by issue number to minimize the possibility of
    conflicts when merging.
-->

* Corrupted services are removed when they are part of a dead node
    [#458](https://github.com/eclipse-iceoryx/iceoryx2/issues/458)
* Remove stale shm state files in Windows
    [#458](https://github.com/eclipse-iceoryx/iceoryx2/issues/458)
* Make process local services truly process local by using socket pairs
    for events
    [#508](https://github.com/eclipse-iceoryx/iceoryx2/issues/508)
* Completion queue capacity exceeded when history > buffer
    [#571](https://github.com/eclipse-iceoryx/iceoryx2/issues/571)
* Increase max supported shared memory size in Windows that restricts
    the maximum supported payload size
    [#575](https://github.com/eclipse-iceoryx/iceoryx2/issues/575)
* Undefined behavior due to ZeroCopyConnection removal when stale resources
    are cleaned up
    [#596](https://github.com/eclipse-iceoryx/iceoryx2/issues/596)
* Remove `SIGPOLL` that lead to compile issues on older glibc versions.
    Fix issue where fatal signals are generated with non-fatal values.
    [#605](https://github.com/eclipse-iceoryx/iceoryx2/issues/605)
* LogLevel is considered for custom loggers.
    [#608](https://github.com/eclipse-iceoryx/iceoryx2/issues/608)
* Allow missing legal characters in system type for user- and group-name
    [#677](https://github.com/eclipse-iceoryx/iceoryx2/issues/677)

### Refactoring

<!--
    NOTE: Add new entries sorted by issue number to minimize the possibility of
    conflicts when merging.
-->

* Remove the `print_system_configuration()` function in
`iceoryx2-bb/posix/src/system_configuration.rs` file and move it into the CLI `iox2-config`
    [#432](https://github.com/eclipse-iceoryx/iceoryx2/issues/432)

* Remove obsolete POSIX wrapper
    [#594](https://github.com/eclipse-iceoryx/iceoryx2/issues/594)
* Updated all dependencies and increased MSRV to 1.81
    [#638](https://github.com/eclipse-iceoryx/iceoryx2/issues/638)
* Reduce indentation in `main.rs` of CLI binaries
    [#646](https://github.com/eclipse-iceoryx/iceoryx2/issues/646)

### Workflow

<!--
    NOTE: Add new entries sorted by issue number to minimize the possibility of
    conflicts when merging.
-->

### New API features

<!--
    NOTE: Add new entries sorted by issue number to minimize the possibility of
    conflicts when merging.
-->

* Add CLI to display complete system configuration
    [#432](https://github.com/eclipse-iceoryx/iceoryx2/issues/432)

* Add simplified attribute value accessors
    [#590](https://github.com/eclipse-iceoryx/iceoryx2/issues/590)

### API Breaking Changes

1. Renamed `PublisherLoanError` into `LoanError`

   ```rust
   // old
   let sample = match publisher.loan() {
     Ok(sample) => sample,
     Err(PublisherLoanError::OutOfMemory) => handle_error(),
     // ...
   };

   // new
   let sample = match publisher.loan() {
     Ok(sample) => sample,
     Err(LoanError::OutOfMemory) => handle_error(),
     // ...
   };
   ```

2. Renamed `PublisherSendError` into `SendError`

   ```rust
   // old
   match sample.send() {
     Ok(n) => println!("send data to {n} subscribers"),
     Err(PublisherSendError::ConnectionCorrupted) => handle_error(),
     // ...
   };

   // new
   match sample.send() {
     Ok(n) => println!("send data to {n} subscribers"),
     Err(SendError::ConnectionCorrupted) => handle_error(),
     // ...
   };
   ```

3. Renamed `SubscriberReceiveError` into `ReceiveError`

   ```rust
   // old
   match subscriber.receive() {
     Ok(sample) => println!("received: {:?}", *sample),
     Err(SubscriberReceiveError::ExceedsMaxBorrowedSamples) => handle_error(),
     // ...
   }

   // new
   match subscriber.receive() {
     Ok(sample) => println!("received: {:?}", *sample),
     Err(ReceiveError::ExceedsMaxBorrowedSamples) => handle_error(),
     // ...
   }
   ```

4. Renamed `PublisherSendError::ConnectionBrokenSincePublisherNoLongerExists`
   into `SendError::ConnectionBrokenSinceSenderNoLongerExists`

5. Renamed `ConnectionFailure::UnableToMapPublishersDataSegment`
   into `ConnectionFailure::UnableToMapSendersDataSegment`
