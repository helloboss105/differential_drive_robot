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
from launch.actions import ExecuteProcess, DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory
import os
import xacro

def generate_launch_description():

    # Resolve package share directory
    pkg_share = get_package_share_directory('differential_drive_robot')
    urdf_path = os.path.join(pkg_share, 'urdf', 'my_robot.urdf')
    controllers_path = os.path.join(pkg_share, 'config', 'controllers.yaml')

    # Get turtlebot3_gazebo share directory path
    tb3_gazebo_share = get_package_share_directory('turtlebot3_gazebo')
    
    # Define the default world path
    default_world_path = os.path.join(tb3_gazebo_share, 'worlds', 'turtlebot3_house.world')

    # Append turtlebot3 model path to GAZEBO_MODEL_PATH
    tb3_model_path = os.path.join(tb3_gazebo_share, 'models')
    if 'GAZEBO_MODEL_PATH' in os.environ:
        os.environ['GAZEBO_MODEL_PATH'] += os.pathsep + tb3_model_path
    else:
        os.environ['GAZEBO_MODEL_PATH'] = tb3_model_path

    # Declared launch arguments
    world_arg = DeclareLaunchArgument(
        'world',
        default_value=default_world_path,
        description='Path to the Gazebo world file'
    )

    x_pose_arg = DeclareLaunchArgument(
        'x',
        default_value='-2.0',
        description='Spawn X position'
    )

    y_pose_arg = DeclareLaunchArgument(
        'y',
        default_value='-0.5',
        description='Spawn Y position'
    )

    z_pose_arg = DeclareLaunchArgument(
        'z',
        default_value='0.3',
        description='Spawn Z position'
    )

    world_path = LaunchConfiguration('world')
    x_pose = LaunchConfiguration('x')
    y_pose = LaunchConfiguration('y')
    z_pose = LaunchConfiguration('z')

    robot_desc = ParameterValue(
        Command(['xacro ', urdf_path]),
        value_type=str
    )

    return LaunchDescription([
        world_arg,
        x_pose_arg,
        y_pose_arg,
        z_pose_arg,

        ExecuteProcess(
            cmd=['gazebo', '--verbose', '-s', 'libgazebo_ros_init.so', '-s', 'libgazebo_ros_factory.so', world_path],
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
                '-entity', 'my_robot',
                '-x', x_pose,
                '-y', y_pose,
                '-z', z_pose
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
                controllers_path
            ]
        ),

    ])