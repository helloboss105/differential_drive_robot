#  launch/sim.launch.py
# import os
# from ament_index_python.packages import get_package_share_directory
# from launch import LaunchDescription
# from launch_ros.actions import Node
# from launch.actions import ExecuteProcess

# def generate_launch_description():
#     # Get package directory
#     pkg_dir = get_package_share_directory('my_robot')
#     urdf_file = os.path.join(pkg_dir, 'urdf', 'old_model.urdf')

#     return LaunchDescription([
#         # Start Gazebo with ROS factory plugin
#         ExecuteProcess(
#             cmd=['gazebo', '--verbose', '-s', 'libgazebo_ros_factory.so'],
#             output='screen'
#         ),

#         # Spawn the robot
#         Node(
#             package='gazebo_ros',
#             executable='spawn_entity.py',
#             arguments=[
#                 '-entity', 'my_robot',
#                 '-file', urdf_file
#             ],
#             output='screen'
#         ),
#     ])



from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue
import os

def generate_launch_description():

    # robot_desc = ParameterValue(
    #     Command(['cat ', '/home/hello-boss/practice_ros_ws/src/my_robot/urdf/old_model.urdf']),
    #     value_type=str
    # )

    #Above code is not working because cat command is not available in windows, so we will use xacro command instead of cat command to read the urdf file.
    # 
    # Note: If you are using windows, you need to install xacro package in order to use xacro command. You can install xacro package using pip command: pip install xacro
    # 
    # 
    # 
    robot_desc = ParameterValue(
        Command(['cat ', '/home/hello-boss/practice_ros_ws/src/my_robot/urdf/my_robot.urdf']),
        value_type=str
    )

    return LaunchDescription([

        ExecuteProcess(
            cmd=['gazebo', '--verbose', '-s', 'libgazebo_ros_factory.so'],
            output='screen'
        ),

        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_desc}],
            output='screen'
        ),

        Node(
            package='gazebo_ros',
            executable='spawn_entity.py',
            arguments=[
                '-topic', 'robot_description',
                '-entity', 'my_robot'
            ],
            output='screen'
        ),

        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['joint_state_broadcaster']
        ),

        Node(
            package='controller_manager',
            executable='spawner',
            arguments=[
                'diff_cont',
                '--param-file',
                '/home/hello-boss/practice_ros_ws/src/my_robot/config/controllers.yaml'
            ]
        ),

    ])