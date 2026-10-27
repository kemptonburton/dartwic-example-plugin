# Stress device

`stress_device` is a deterministic local module for DARTWIC's internal operator stress workspace. It performs no network or hardware I/O. It has five numeric input registers and five numeric output registers, with no feedback channels.

Configure `maximum` (default 1,000) and `threshold` (default 800) in the module's `parameters`. The maximum must be at least three; the threshold must be greater than one and less than the maximum. Each read advances one integer through a triangle wave between one and the maximum. All five input registers receive that value. The waveform follows completed reads, so missed releases increase its elapsed period.

Create one `example_device_plugin.stress_read` task and one `example_device_plugin.stress_write` task for each instance. Both accept `module_instance_name`, `record_mode` (`on_value_change`, default, or `every_value`), and five rows in `read_mappings` or `write_mappings`:

```json
{"register": 0, "channel": "stress_0000_in_1"}
```

Map registers zero through four exactly once to distinct, nonempty channel names. Configuration validates the table, creates module-owned fixed RAPID channels, and retains their handles. The write task declares its fixed inputs. Repeated task bodies use those handles, and the standard interface task editor exposes the mappings through its registered Config Map, including module selection and recording mode. The module overview uses the configured channel names rather than reconstructing implicit bindings.

Read publication is synchronous, including attached calculations and event reactions. The write task reads the current commands and stores them in simulated output registers during its callback. Neither successful callback completion nor these simulated registers establish physical I/O performance.

The verifier expects the operator stress workspace's control behavior: latch zero at the minimum and one at the maximum; override output one to one above the threshold and to two at the minimum; release the override when clear. It compares all five commands on each write callback. It counts mismatches and retains at most sixteen examples; exceptional example capture may allocate. Module state uses a module-local mutex, including cold status queries. Plugin costs remain part of caller timing.

The read-only plugin operation `example_device_plugin.stress_status` accepts `{"modules":["stress_0000"]}` and returns an object keyed by instance name. Each entry contains read/write/mismatch counters, current input, output registers, bounded mismatch examples, and `steady_ns`, a monotonic snapshot time for counter-rate comparisons. Unknown module instances fail the request. The operation completes synchronously after copying current state; it does not wait for recordings or confirm device response. Query in bounded batches outside the periodic task body. Status queries briefly contend with the corresponding module callbacks.

Control, clamping calculations, and the declared warning belong to the workspace's Native C++ DCode templates. This module does not reproduce those engine mechanisms internally. Its expected-value calculation is an independent check of commands reaching the simulated write task; incomplete recording still prevents a claim about every transient accepted channel value.
