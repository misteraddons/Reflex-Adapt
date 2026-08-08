from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / "firmware"


def compact_routes(source_count: int, connected_mask: int) -> tuple[list[int], list[int]]:
    source_to_target = [-1] * 4
    target_to_source = [-1] * 4
    next_target = 0
    for source in range(min(source_count, 4)):
        if not (connected_mask & (1 << source)):
            continue
        source_to_target[source] = next_target
        target_to_source[next_target] = source
        next_target += 1
    return source_to_target, target_to_source


class XInputDynamicSlotTests(unittest.TestCase):
    def test_all_four_source_combinations_compact_without_duplicates(self) -> None:
        for mask in range(16):
            source_to_target, target_to_source = compact_routes(4, mask)
            live_sources = [source for source in range(4) if mask & (1 << source)]
            self.assertEqual(
                [source_to_target[source] for source in live_sources],
                list(range(len(live_sources))),
            )
            self.assertEqual(target_to_source[: len(live_sources)], live_sources)
            self.assertEqual(
                target_to_source[len(live_sources) :],
                [-1] * (4 - len(live_sources)),
            )

    def test_classic_two_port_transition_matrix(self) -> None:
        expected = {
            0b00: ([-1, -1, -1, -1], [-1, -1, -1, -1]),
            0b01: ([0, -1, -1, -1], [0, -1, -1, -1]),
            0b10: ([-1, 0, -1, -1], [1, -1, -1, -1]),
            0b11: ([0, 1, -1, -1], [0, 1, -1, -1]),
        }
        for mask, routes in expected.items():
            self.assertEqual(compact_routes(2, mask), routes)

    def test_source_and_feedback_use_inverse_routes(self) -> None:
        sender = (FIRMWARE / "output/usb/output_usb_send_runtime.h").read_text()
        driver = (
            FIRMWARE / "output/xinputw/output_xinputw_driver_runtime.cpp"
        ).read_text()
        self.assertIn("xinputw_target_slot_for_source(port)", sender)
        self.assertIn("xinputw_source_port_for_target(target)", sender)
        self.assertIn("xinputw_source_port_for_target(i)", driver)
        self.assertIn("rumble_callback(sourcePort, rumble.left, rumble.right)", driver)

    def test_receiver_shape_remains_fixed_while_presence_is_dynamic(self) -> None:
        slots = (
            FIRMWARE / "output/usb/output_usb_xinput2p_slots_runtime.h"
        ).read_text()
        configure = (
            FIRMWARE / "output/usb/output_usb_configure_runtime.h"
        ).read_text()
        self.assertIn("min((uint8_t)XINPUT_MULTI_CONTROLLERS, (uint8_t)MAX_USB_OUT)", slots)
        self.assertIn("xinputw_slot_routing_update(sourceCount, connectedMask, true)", slots)
        self.assertIn("get_effective_output_mode() != OUTPUT_XINPUTW", slots)
        self.assertIn("service_xinputw_slot_routing();", configure)
        self.assertIn("_xinputw->set_connected(target, targetConnected);", configure)


if __name__ == "__main__":
    unittest.main()
