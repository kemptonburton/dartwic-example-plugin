# Mock flight peer

Start the mock rocket workspace and its engine first. This application models a flight computer; it does not model flight dynamics.

For the supplied custom connection:

```powershell
./rocket-flight-peer.exe --transport custom
```

Flight receives on `tcp://127.0.0.1:17601` and sends to `tcp://127.0.0.1:17600`. Ground uses the reverse endpoints. Set `--receive-endpoint` and `--send-endpoint` to match a changed ground connection. This loopback example transport has no authentication or encryption.

For native TEMPEST, remove the custom connection and stop its flight process first:

```powershell
$env:DARTWIC_PASSWORD = Read-Host 'Ground admin password' -MaskInput
./rocket-flight-peer.exe --transport native --host 127.0.0.1 --port 7400
```

Expect `FLIGHT_COMPUTER:sample_counter` to advance, a flight-ready ARGUS event, and a **Flight computer** log stream. Ground can command `telemetry_rate_hz` from 1 to 20 and call `flight/set-mode` with `standby` or `test`. Flight reads ground ambient temperature and sends `rocket_sim_flight_check=1` after registration. Measurement channels reject writes.

Stop with Ctrl+C. Commands while disconnected fail; after restarting, confirm new snapshots before commanding. The workspace includes the complete native and custom walkthroughs, recording instructions, and failure exercises.

To build from the public plugin source, set `VCPKG_ROOT`, run `cmake --preset windows-clang-release`, then `cmake --build --preset build-windows-clang-release --target rocket-flight-peer`. CMake uses the bundled, locked TEMPEST and Engine Protocol source snapshots. The downloadable executable needs the adjacent ZeroMQ DLL and the Windows C++ runtime, but no compiler or private checkout.
