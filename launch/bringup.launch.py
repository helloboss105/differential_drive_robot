import os
from pathlib import Path
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess, DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    # Get package directory
    pkg_dir = get_package_share_directory('differential_drive_robot')
    urdf_file = os.path.join(pkg_dir, 'urdf', 'my_robot.urdf')
    
    # Read URDF content
    with open(urdf_file, 'r') as f:
        robot_desc_content = f.read()
    
    # Create robot_description parameter
    robot_description = ParameterValue(robot_desc_content, value_type=str)

    return LaunchDescription([
        # 1. ROBOT STATE PUBLISHER - Publishes static transform tree
        #    WHY: Connects all robot links together in TF
        #    PUBLISHES: /tf (transform messages)
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{'robot_description': robot_description}],
            remappings=[('/tf', 'tf'), ('/tf_static', 'tf_static')]
        ),

        # 2. JOINT STATE PUBLISHER - Reads joint positions from controllers
        #    WHY: Updates joint angles so TF tree stays current
        #    PUBLISHES: /joint_states (used by robot_state_publisher)
        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            name='joint_state_publisher',
            output='screen'
        ),

        # 3. DIFFERENTIAL DRIVE ODOMETRY NODE
        #    WHY: Estimates position from wheel velocities
        #    PUBLISHES: /tf (odom → base_link), /odom (odometry messages)
        #    LISTENS: /cmd_vel (velocity commands)
        Node(
            package='differential_drive_robot',
            executable='odometry_node',
            name='odometry_node',
            output='screen',
            parameters=[
                {'wheel_separation': 0.46},
                {'wheel_radius': 0.1},
                {'publish_tf': True},
                {'odom_frame': 'odom'},
                {'base_frame': 'base_link'},
            ]
        ),
    ])