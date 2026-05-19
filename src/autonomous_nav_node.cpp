#include "rclcpp/rclcpp.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "std_srvs/srv/trigger.hpp"
#include <cmath>


class AutonomousNavigationNode : public rclcpp::Node {
private:
    using NavigateToPose = nav2_msgs::action::NavigateToPose;
    using GoalHandleNavigateToPose = rclcpp_action::ClientGoalHandle<NavigateToPose>;

    rclcpp_action::Client<NavigateToPose>::SharedPtr nav_client_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr start_autonomy_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_service_;
    rclcpp::TimerBase::SharedPtr mission_timer_;

    bool autonomy_active_ = false;
    std::vector<geometry_msgs::msg::Pose> waypoints_;
    size_t current_waypoint_idx_ = 0;

public:
    AutonomousNavigationNode() : Node("autonomous_navigation_node") {
        // Navigation action client
        nav_client_ = rclcpp_action::create_client<NavigateToPose>(
            this, "navigate_to_pose");

        // Service to start autonomous mission
        start_autonomy_service_ = this->create_service<std_srvs::srv::Trigger>(
            "start_autonomy",
            std::bind(&AutonomousNavigationNode::start_autonomy_callback, 
                     this, std::placeholders::_1, std::placeholders::_2));

        // Service to stop autonomous mission
        stop_service_ = this->create_service<std_srvs::srv::Trigger>(
            "stop_autonomy",
            std::bind(&AutonomousNavigationNode::stop_autonomy_callback, 
                     this, std::placeholders::_1, std::placeholders::_2));

        // Initialize waypoints (example mission)
        initialize_waypoints();

        RCLCPP_INFO(this->get_logger(), "Autonomous Navigation Node initialized");
    }

    void initialize_waypoints() {
        // Define mission waypoints in map frame
        // Waypoint 1: Move 2m forward (x=2.0)
        geometry_msgs::msg::Pose wp1;
        wp1.position.x = 2.0;
        wp1.position.y = 0.0;
        wp1.position.z = 0.0;
        wp1.orientation.w = 1.0;
        waypoints_.push_back(wp1);

        // Waypoint 2: Turn right, move 2m (y=-2.0)
        geometry_msgs::msg::Pose wp2;
        wp2.position.x = 2.0;
        wp2.position.y = -2.0;
        wp2.position.z = 0.0;
        // 90 degree turn (quaternion for -90° in yaw)
        double yaw = -M_PI / 2.0;
        wp2.orientation.z = std::sin(yaw / 2.0);
        wp2.orientation.w = std::cos(yaw / 2.0);
        waypoints_.push_back(wp2);

        // Waypoint 3: Move forward 2m
        geometry_msgs::msg::Pose wp3;
        wp3.position.x = 4.0;
        wp3.position.y = -2.0;
        wp3.position.z = 0.0;
        wp3.orientation.w = 1.0;
        waypoints_.push_back(wp3);

        // Waypoint 4: Return to origin
        geometry_msgs::msg::Pose wp4;
        wp4.position.x = 0.0;
        wp4.position.y = 0.0;
        wp4.position.z = 0.0;
        // 180 degree turn
        wp4.orientation.z = 1.0;
        wp4.orientation.w = 0.0;
        waypoints_.push_back(wp4);
    }

    void start_autonomy_callback(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
        
        autonomy_active_ = true;
        current_waypoint_idx_ = 0;
        response->success = true;
        response->message = "Autonomous mission started";
        RCLCPP_INFO(this->get_logger(), "🤖 Autonomy activated! Starting mission with %ld waypoints", 
                   waypoints_.size());
        
        // Send first goal
        send_goal_to_waypoint(0);
    }

    void stop_autonomy_callback(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
        
        autonomy_active_ = false;
        response->success = true;
        response->message = "Autonomous mission stopped";
        RCLCPP_INFO(this->get_logger(), "⏹️  Autonomy stopped");
    }

    void send_goal_to_waypoint(size_t idx) {
        if (idx >= waypoints_.size() || !autonomy_active_) {
            RCLCPP_INFO(this->get_logger(), "✅ Mission complete!");
            autonomy_active_ = false;
            return;
        }

        if (!nav_client_->action_server_is_ready()) {
            RCLCPP_WARN(this->get_logger(), "Navigation action server not ready, waiting...");
            rclcpp::sleep_for(std::chrono::seconds(1));
            send_goal_to_waypoint(idx);
            return;
        }

        auto goal_msg = NavigateToPose::Goal();
        goal_msg.pose.header.frame_id = "map";
        goal_msg.pose.header.stamp = this->now();
        goal_msg.pose.pose = waypoints_[idx];

        RCLCPP_INFO(this->get_logger(), "📍 Sending goal %ld: (%.2f, %.2f)",
                   idx + 1, 
                   waypoints_[idx].position.x,
                   waypoints_[idx].position.y);

        auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
        send_goal_options.goal_response_callback =
            std::bind(&AutonomousNavigationNode::goal_response_callback, this, std::placeholders::_1);
        send_goal_options.feedback_callback =
            std::bind(&AutonomousNavigationNode::feedback_callback, this, 
                     std::placeholders::_1, std::placeholders::_2);
        send_goal_options.result_callback =
            std::bind(&AutonomousNavigationNode::result_callback, this, 
                     std::placeholders::_1, idx);

        nav_client_->async_send_goal(goal_msg, send_goal_options);
    }

private:
    void goal_response_callback(const GoalHandleNavigateToPose::SharedPtr & goal_handle) {
        if (!goal_handle) {
            RCLCPP_ERROR(this->get_logger(), "Goal was rejected by server");
        } else {
            RCLCPP_INFO(this->get_logger(), "Goal accepted by server, waiting for result");
        }
    }

    void feedback_callback(
        GoalHandleNavigateToPose::SharedPtr,
        const std::shared_ptr<const NavigateToPose::Feedback> feedback) {
        RCLCPP_DEBUG(this->get_logger(), "Distance remaining: %.2f meters",
                    feedback->distance_remaining);
    }

    void result_callback(
        const GoalHandleNavigateToPose::WrappedResult & result,
        size_t waypoint_idx) {
        switch (result.code) {
            case rclcpp_action::ResultCode::SUCCEEDED:
                RCLCPP_INFO(this->get_logger(), "✅ Waypoint %ld reached!", waypoint_idx + 1);
                current_waypoint_idx_++;
                // Send next waypoint
                if (autonomy_active_) {
                    send_goal_to_waypoint(current_waypoint_idx_);
                }
                break;
            case rclcpp_action::ResultCode::ABORTED:
                RCLCPP_ERROR(this->get_logger(), "❌ Goal was aborted");
                autonomy_active_ = false;
                break;
            case rclcpp_action::ResultCode::CANCELED:
                RCLCPP_INFO(this->get_logger(), "Goal was canceled");
                break;
            default:
                RCLCPP_ERROR(this->get_logger(), "Unknown result code");
        }
    }
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<AutonomousNavigationNode>());
    rclcpp::shutdown();
    return 0;
}