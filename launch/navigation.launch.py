

import os
from launch import LaunchDescription
from launch_ros.actions import Node, LifecycleNode
from launch.actions import DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_dir = get_package_share_directory('differential_drive_robot')
    nav2_params = os.path.join(pkg_dir, 'config', 'nav2_params.yaml')

    use_sim_time = True

    return LaunchDescription([
        LogInfo(msg="Starting Nav2 Navigation Stack for Autonomous Robot"),

        # 1. LIFECYCLE NODE MANAGER
        # Manages startup/shutdown of Nav2 components
        # LifecycleNode(
        #     package='nav2_core',
        #     executable='nav2_core',
        #     namespace='',
        #     name='lifecycle_manager_navigation',
        #     output='screen',
        #     parameters=[
        #         {'use_sim_time': use_sim_time},
        #         {'autostart': True},
        #         {'node_names': ['controller_server', 'planner_server', 
        #                        'recoveries_server', 'bt_navigator']}
        #     ]
        # ),

        # 2. CONTROLLER SERVER
        # Controls the robot's movement (sends velocity commands)
        LifecycleNode(
            package='nav2_controller',
            executable='controller_server',
            namespace='',
            name='controller_server',
            output='screen',
            parameters=[nav2_params],
            remappings=[
                ('/cmd_vel', '/cmd_vel'),
                ('/controller_server/local_costmap/costmap_raw', '/local_costmap/costmap_raw'),
            ]
        ),

        # 3. PLANNER SERVER
        # Plans global path from current position to goal
        LifecycleNode(
            package='nav2_planner',
            executable='planner_server',
            namespace='',
            name='planner_server',
            output='screen',
            parameters=[nav2_params]
        ),

        # 4. RECOVERY SERVER
        # Handles stuck situations (spin, backup, wait)
        # LifecycleNode(
        #     package='nav2_recoveries',
        #     executable='recoveries_server',
        #     namespace='',
        #     name='recoveries_server',
        #     output='screen',
        #     parameters=[nav2_params]
        # ),

        # 5. BEHAVIOR TREE NAVIGATOR
        # Orchestrates overall navigation behavior
        LifecycleNode(
            package='nav2_bt_navigator',
            executable='bt_navigator',
            namespace='',
            name='bt_navigator',
            output='screen',
            parameters=[nav2_params]
        ),

        # 6. COSTMAP FILTER
        # For advanced obstacle avoidance
        # Node(
        #     package='nav2_costmap_2d',
        #     executable='costmap_filter_info_server',
        #     name='costmap_filter_info_server',
        #     output='screen',
        #     parameters=[nav2_params]
        # ),

        # 7. MAP SERVER
        # Serves saved map to navigation system
        Node(
            package='nav2_map_server',
            executable='map_server',
            name='map_server',
            output='screen',
            parameters=[{'yaml_filename': os.path.join(pkg_dir, 'maps', 'map.yaml')}]
        ),

        # 8. AMCL (LOCALIZATION)
        # Probabilistic localization - matches robot position to map
        Node(
            package='nav2_amcl',
            executable='amcl',
            name='amcl',
            output='screen',
            parameters=[nav2_params]
        ),
    ])