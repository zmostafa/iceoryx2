use core::time::Duration;
use iceoryx2::prelude::*;

const CYCLE_TIME: Duration = Duration::from_secs(1);

#[no_mangle]
pub extern "C" fn run_publisher(seconds: u32) -> i32 {
    let service_name = ServiceName::new("Hello/from/C");

    if service_name.is_err() {
        return -1;
    }

    let service_name = service_name.unwrap();

    let service = zero_copy::Service::new(&service_name)
        .publish_subscribe::<u64>()
        .open_or_create();

    if service.is_err() {
        return -1;
    }

    let service = service.unwrap();

    let publisher = service.publisher().create();

    if publisher.is_err() {
        return -1;
    }

    let publisher = publisher.unwrap();

    let mut counter: u64 = 0;

    let mut remaining_seconds = seconds;

    while let Iox2Event::Tick = Iox2::wait(CYCLE_TIME) {
        counter += 1;
        let sample = publisher.loan_uninit();

        if sample.is_err() {
            return -1;
        }

        let sample = sample.unwrap();

        let sample = sample.write_payload(counter);

        if sample.send().is_err() {
            return -1;
        }

        println!("Send sample {} ...", counter);

        remaining_seconds = remaining_seconds.saturating_sub(1);
        if remaining_seconds == 0 {
            break;
        }
    }

    println!("exit");

    return 0;
}
