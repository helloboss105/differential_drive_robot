import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import yaml


def generate_launch_description():
    pkg_dir = get_package_share_directory('differential_drive_robot')
    slam_config_file = os.path.join(pkg_dir, 'config', 'slam_toolbox.yaml')

    return LaunchDescription([
        # SLAM Node - Creates map from LIDAR data
        #
        # KEY CONCEPT: scan_matcher_queue_size controls how many LIDAR scans
        # are kept in memory for matching. Larger = more accurate but slower.
        #
        # WHY: Reads LIDAR scans and odometry, outputs:
        #   - /map (the created map)
        #   - /tf: map → odom (map frame to odometry frame correction)
        #
        Node(
            package='slam_toolbox',
            executable='async_slam_toolbox_node',
            name='slam_toolbox',
            output='screen',
            parameters=[
                slam_config_file,
                {'use_sim_time': True}
            ],
            remappings=[
                ('/scan', '/scan'),
                ('/odom', '/odom'),
            ]
        )    
    ])