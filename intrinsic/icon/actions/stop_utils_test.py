# Copyright 2023 Intrinsic Innovation LLC
# Intrinsic Proprietary and Confidential
# Provided subject to written agreement between the parties.

from absl.testing import absltest
from google.protobuf import any_pb2
from intrinsic.icon.actions import stop_utils


class StopUtilsTest(absltest.TestCase):

  def test_create_action(self):
    action = stop_utils.CreateStopAction(
        action_id=17, joint_position_part_name="my_part"
    )

    self.assertEqual(action.proto.action_instance_id, 17)
    self.assertEqual(action.proto.part_name, "my_part")
    self.assertEmpty(action.reactions)
    self.assertEqual(action.proto.action_type_name, "xfa.stop")
    self.assertEqual(action.proto.fixed_parameters, any_pb2.Any())


if __name__ == "__main__":
  absltest.main()