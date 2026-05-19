#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <cmath>


class OdometryNode : public rclcpp::Node {
private:
    // Parameters
    double wheel_separation_;
    double wheel_radius_;
    bool publish_tf_;
    std::string odom_frame_;
    std::string base_frame_;

    // State variables
    double x_ = 0.0;
    double y_ = 0.0;
    double theta_ = 0.0;
    double left_wheel_pos_ = 0.0;
    double right_wheel_pos_ = 0.0;
    rclcpp::Time last_time_;

    // Publishers and subscribers
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

public:
    OdometryNode() : Node("odometry_node") {
        // Declare and get parameters
        this->declare_parameter("wheel_separation", 0.46);
        this->declare_parameter("wheel_radius", 0.1);
        this->declare_parameter("publish_tf", true);
        this->declare_parameter("odom_frame", "odom");
        this->declare_parameter("base_frame", "base_link");

        wheel_separation_ = this->get_parameter("wheel_separation").as_double();
        wheel_radius_ = this->get_parameter("wheel_radius").as_double();
        publish_tf_ = this->get_parameter("publish_tf").as_bool();
        odom_frame_ = this->get_parameter("odom_frame").as_string();
        base_frame_ = this->get_parameter("base_frame").as_string();

        // Create publishers
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10, std::bind(&OdometryNode::cmd_vel_callback, this, std::placeholders::_1));

        // TF broadcaster
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(this);

        last_time_ = this->now();

        RCLCPP_INFO(this->get_logger(), "Odometry node started");
        RCLCPP_INFO(this->get_logger(), "Wheel separation: %.3f m", wheel_separation_);
        RCLCPP_INFO(this->get_logger(), "Wheel radius: %.3f m", wheel_radius_);
    }

    void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        // Convert linear and angular velocity to wheel velocities
        double vx = msg->linear.x;
        double wz = msg->angular.z;

        // Differential drive kinematics
        // v_left = (vx - wz * wheel_sep/2) / wheel_radius
        // v_right = (vx + wz * wheel_sep/2) / wheel_radius
        double left_vel = (vx - wz * wheel_separation_ / 2.0) / wheel_radius_;
        double right_vel = (vx + wz * wheel_separation_ / 2.0) / wheel_radius_;

        // Update wheel positions (simple integration)
        rclcpp::Time current_time = this->now();
        double dt = (current_time - last_time_).seconds();
        last_time_ = current_time;

        left_wheel_pos_ += left_vel * dt;
        right_wheel_pos_ += right_vel * dt;

        // Calculate wheel distances traveled
        double left_distance = left_wheel_pos_ * wheel_radius_;
        double right_distance = right_wheel_pos_ * wheel_radius_;

        // Odometry calculation (inverse of differential drive kinematics)
        double distance = (left_distance + right_distance) / 2.0;
        double delta_theta = (right_distance - left_distance) / wheel_separation_;

        // Update position
        x_ += distance * std::cos(theta_ + delta_theta / 2.0);
        y_ += distance * std::sin(theta_ + delta_theta / 2.0);
        theta_ += delta_theta;

        // Normalize angle to [-π, π]
        while (theta_ > M_PI) theta_ -= 2 * M_PI;
        while (theta_ < -M_PI) theta_ += 2 * M_PI;

        // Publish odometry message
        publish_odometry(msg->linear.x, msg->angular.z);

        // Publish TF
        if (publish_tf_) {
            publish_transform();
        }
    }

    void publish_odometry(double vx, double wz) {
        nav_msgs::msg::Odometry odom;
        odom.header.stamp = this->now();
        odom.header.frame_id = odom_frame_;
        odom.child_frame_id = base_frame_;

        // Set position
        odom.pose.pose.position.x = x_;
        odom.pose.pose.position.y = y_;
        odom.pose.pose.position.z = 0.0;

        // Set orientation (convert theta to quaternion)
        double half_theta = theta_ / 2.0;
        odom.pose.pose.orientation.x = 0.0;
        odom.pose.pose.orientation.y = 0.0;
        odom.pose.pose.orientation.z = std::sin(half_theta);
        odom.pose.pose.orientation.w = std::cos(half_theta);

        // Set velocity
        odom.twist.twist.linear.x = vx;
        odom.twist.twist.linear.y = 0.0;
        odom.twist.twist.angular.z = wz;

        odom_pub_->publish(odom);
    }

    void publish_transform() {
        geometry_msgs::msg::TransformStamped t;
        t.header.stamp = this->now();
        t.header.frame_id = odom_frame_;
        t.child_frame_id = base_frame_;

        // Translation
        t.transform.translation.x = x_;
        t.transform.translation.y = y_;
        t.transform.translation.z = 0.0;

        // Rotation (theta to quaternion)
        double half_theta = theta_ / 2.0;
        t.transform.rotation.x = 0.0;
        t.transform.rotation.y = 0.0;
        t.transform.rotation.z = std::sin(half_theta);
        t.transform.rotation.w = std::cos(half_theta);

        tf_broadcaster_->sendTransform(t);
    }
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdometryNode>());
    rclcpp::shutdown();
    return 0;
}