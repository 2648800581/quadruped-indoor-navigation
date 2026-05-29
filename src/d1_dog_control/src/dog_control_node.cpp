#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <mutex>
#include <cmath>
#include "highlevel.h"

// Global objects for signal handling
std::atomic<bool> g_shutdown_requested(false);
mc_sdk::HighLevel* g_highlevel_ptr = nullptr;

void signalHandler(int signum)
{
    if (signum == SIGINT && !g_shutdown_requested.load()) {
        g_shutdown_requested.store(true);

        printf("\n==========================================\n");
        printf("  Ctrl+C detected - Emergency shutdown\n");
        printf("==========================================\n\n");

        if (g_highlevel_ptr != nullptr) {
            printf("Making dog lie down (3 times)...\n");
            for (int i = 1; i <= 3; i++) {
                printf("  Lie down command %d/3\n", i);
                g_highlevel_ptr->lieDown();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            printf("Dog is now lying down\n\n");
        }

        // Exit immediately after lying down
        exit(0);
    }
}

class DogControlNode : public rclcpp::Node
{
public:
    DogControlNode() : Node("dog_control_node"),
                       current_vx_(0.0), current_vy_(0.0), current_yaw_rate_(0.0),
                       target_vx_(0.0), target_vy_(0.0), target_yaw_rate_(0.0),
                       odom_x_(0.0), odom_y_(0.0), odom_yaw_(0.0)
    {
        RCLCPP_INFO(this->get_logger(), "Dog control node starting...");

        // Declare parameters
        this->declare_parameter("max_linear_velocity", 1.0);
        this->declare_parameter("max_angular_velocity", 1.0);
        this->declare_parameter("cmd_vel_timeout", 0.5);
        this->declare_parameter("acceleration_limit", 2.0);
        this->declare_parameter("control_frequency", 500.0);

        // Get parameters
        max_linear_vel_ = this->get_parameter("max_linear_velocity").as_double();
        max_angular_vel_ = this->get_parameter("max_angular_velocity").as_double();
        cmd_vel_timeout_ = this->get_parameter("cmd_vel_timeout").as_double();
        accel_limit_ = this->get_parameter("acceleration_limit").as_double();
        control_freq_ = this->get_parameter("control_frequency").as_double();

        RCLCPP_INFO(this->get_logger(), "Parameters:");
        RCLCPP_INFO(this->get_logger(), "  max_linear_velocity: %.2f m/s", max_linear_vel_);
        RCLCPP_INFO(this->get_logger(), "  max_angular_velocity: %.2f rad/s", max_angular_vel_);
        RCLCPP_INFO(this->get_logger(), "  cmd_vel_timeout: %.2f s", cmd_vel_timeout_);
        RCLCPP_INFO(this->get_logger(), "  acceleration_limit: %.2f m/s²", accel_limit_);
        RCLCPP_INFO(this->get_logger(), "  control_frequency: %.1f Hz", control_freq_);

        // Initialize robot SDK
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        highlevel_.initRobot("127.0.0.1", 43988, "127.0.0.1");

        // Set global pointer for signal handler
        g_highlevel_ptr = &highlevel_;

        RCLCPP_INFO(this->get_logger(), "Robot SDK initialized");

        // Record initial offset from SDK (to align with FAST-LIO on each startup)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        offset_x_ = highlevel_.getPosWorldX();
        offset_y_ = highlevel_.getPosWorldY();
        offset_z_ = highlevel_.getPosWorldZ();
        offset_yaw_ = highlevel_.getYaw();

        RCLCPP_INFO(this->get_logger(), "SDK initial offset recorded:");
        RCLCPP_INFO(this->get_logger(), "  Position: (%.3f, %.3f, %.3f)", offset_x_, offset_y_, offset_z_);
        RCLCPP_INFO(this->get_logger(), "  Yaw: %.3f rad (%.1f deg)", offset_yaw_, offset_yaw_ * 180.0 / M_PI);

        // Subscribe to robot command topic
        cmd_sub_ = this->create_subscription<std_msgs::msg::String>(
            "/robot_cmd", 10,
            std::bind(&DogControlNode::cmdCallback, this, std::placeholders::_1));

        // Subscribe to cmd_vel topic
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 10,
            std::bind(&DogControlNode::cmdVelCallback, this, std::placeholders::_1));

        // Publisher for robot state
        state_pub_ = this->create_publisher<std_msgs::msg::String>("/robot_state", 10);

        // Publisher for odometry
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/odom", 10);

        // Timer for control loop (500Hz)
        auto control_period = std::chrono::microseconds(static_cast<int>(1000000.0 / control_freq_));
        control_timer_ = this->create_wall_timer(
            control_period,
            std::bind(&DogControlNode::controlLoop, this));

        // Timer for timeout check (10Hz)
        timeout_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&DogControlNode::checkTimeout, this));

        // Timer for state publishing (10Hz)
        state_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&DogControlNode::publishState, this));

        last_cmd_vel_time_ = this->now();

        RCLCPP_INFO(this->get_logger(), "Dog control node ready");
        RCLCPP_INFO(this->get_logger(), "Listening to /robot_cmd and /cmd_vel topics");
        RCLCPP_INFO(this->get_logger(), "Commands: 'stand' or 'sit'");
        RCLCPP_INFO(this->get_logger(), "Publishing /robot_state and /odom");
    }

    ~DogControlNode()
    {
        // Stop the robot
        std::lock_guard<std::mutex> lock(vel_mutex_);
        target_vx_ = 0.0;
        target_vy_ = 0.0;
        target_yaw_rate_ = 0.0;
        highlevel_.move(0.0, 0.0, 0.0);

        g_highlevel_ptr = nullptr;
        RCLCPP_INFO(this->get_logger(), "Dog control node shutting down");
    }

    void standUp()
    {
        RCLCPP_INFO(this->get_logger(), "Standing up...");
        highlevel_.standUp();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        RCLCPP_INFO(this->get_logger(), "Stand up complete");
    }

    void lieDown()
    {
        RCLCPP_INFO(this->get_logger(), "Lying down...");
        highlevel_.lieDown();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        RCLCPP_INFO(this->get_logger(), "Lie down complete");
    }

private:
    void cmdCallback(const std_msgs::msg::String::SharedPtr msg)
    {
        std::string cmd = msg->data;
        RCLCPP_INFO(this->get_logger(), "Received command: '%s'", cmd.c_str());

        if (cmd == "stand") {
            standUp();
        } else if (cmd == "sit") {
            lieDown();
        } else {
            RCLCPP_WARN(this->get_logger(), "Unknown command: '%s'", cmd.c_str());
        }
    }

    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(vel_mutex_);

        // Update last command time
        last_cmd_vel_time_ = this->now();

        // Apply velocity limits
        target_vx_ = std::clamp(msg->linear.x, -max_linear_vel_, max_linear_vel_);
        target_vy_ = std::clamp(msg->linear.y, -max_linear_vel_, max_linear_vel_);
        target_yaw_rate_ = std::clamp(msg->angular.z, -max_angular_vel_, max_angular_vel_);
    }

    void controlLoop()
    {
        std::lock_guard<std::mutex> lock(vel_mutex_);

        // Calculate time step
        double dt = 1.0 / control_freq_;

        // Apply acceleration limits (smooth acceleration/deceleration)
        double max_delta_v = accel_limit_ * dt;

        // Smooth vx
        double delta_vx = target_vx_ - current_vx_;
        if (std::abs(delta_vx) > max_delta_v) {
            current_vx_ += (delta_vx > 0 ? max_delta_v : -max_delta_v);
        } else {
            current_vx_ = target_vx_;
        }

        // Smooth vy
        double delta_vy = target_vy_ - current_vy_;
        if (std::abs(delta_vy) > max_delta_v) {
            current_vy_ += (delta_vy > 0 ? max_delta_v : -max_delta_v);
        } else {
            current_vy_ = target_vy_;
        }

        // Smooth yaw_rate
        double delta_yaw = target_yaw_rate_ - current_yaw_rate_;
        if (std::abs(delta_yaw) > max_delta_v) {
            current_yaw_rate_ += (delta_yaw > 0 ? max_delta_v : -max_delta_v);
        } else {
            current_yaw_rate_ = target_yaw_rate_;
        }

        // Send command to robot
        highlevel_.move(current_vx_, current_vy_, current_yaw_rate_);
    }

    void checkTimeout()
    {
        std::lock_guard<std::mutex> lock(vel_mutex_);

        auto now = this->now();
        auto elapsed = (now - last_cmd_vel_time_).seconds();

        // If timeout, set target velocities to zero
        if (elapsed > cmd_vel_timeout_) {
            if (target_vx_ != 0.0 || target_vy_ != 0.0 || target_yaw_rate_ != 0.0) {
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                    "cmd_vel timeout (%.2f s) - stopping robot", elapsed);
                target_vx_ = 0.0;
                target_vy_ = 0.0;
                target_yaw_rate_ = 0.0;
            }
        }
    }

    void publishState()
    {
        // Publish robot state
        auto state_msg = std_msgs::msg::String();
        state_msg.data = "active"; // Simplified state
        state_pub_->publish(state_msg);

        auto current_time = this->now();

        // ========== Method 1: Velocity Integration (Dead Reckoning) ==========
        // This method integrates commanded velocity to estimate position.
        // Pros: Simple, uses commanded velocity
        // Cons: Accumulates drift over time, less accurate
        /*
        static auto last_time = this->now();
        double dt = (current_time - last_time).seconds();
        last_time = current_time;

        std::lock_guard<std::mutex> lock(vel_mutex_);

        // Integrate velocity to get position (dead reckoning)
        // Transform velocity from body frame to odom frame
        double vx_odom = current_vx_ * cos(odom_yaw_) - current_vy_ * sin(odom_yaw_);
        double vy_odom = current_vx_ * sin(odom_yaw_) + current_vy_ * cos(odom_yaw_);

        odom_x_ += vx_odom * dt;
        odom_y_ += vy_odom * dt;
        odom_yaw_ += current_yaw_rate_ * dt;

        // Normalize yaw to [-pi, pi]
        while (odom_yaw_ > M_PI) odom_yaw_ -= 2.0 * M_PI;
        while (odom_yaw_ < -M_PI) odom_yaw_ += 2.0 * M_PI;
        */

        // ========== Method 2: SDK Built-in Odometry (IMU + Leg Encoders) ==========
        // This method uses the robot SDK's built-in odometry from IMU + leg encoders.
        // Pros: More accurate, uses sensor fusion
        // Cons: Position is relative to boot origin, resets on robot restart
        // Solution: Record initial offset on node startup, subtract it to align with FAST-LIO

        // Get position from SDK (world frame, relative to boot origin)
        double sdk_x = highlevel_.getPosWorldX();
        double sdk_y = highlevel_.getPosWorldY();
        double sdk_z = highlevel_.getPosWorldZ();

        // Get orientation from SDK (roll, pitch, yaw)
        double roll = highlevel_.getRoll();
        double pitch = highlevel_.getPitch();
        double sdk_yaw = highlevel_.getYaw();

        // Get velocity from SDK (body frame)
        double body_vx = highlevel_.getBodyVelX();
        double body_vy = highlevel_.getBodyVelY();
        double body_vz = highlevel_.getBodyVelZ();

        // Subtract initial offset to align with FAST-LIO coordinate frame
        odom_x_ = sdk_x - offset_x_;
        odom_y_ = sdk_y - offset_y_;
        double odom_z = sdk_z - offset_z_;
        odom_yaw_ = sdk_yaw - offset_yaw_;

        // Normalize yaw to [-pi, pi]
        while (odom_yaw_ > M_PI) odom_yaw_ -= 2.0 * M_PI;
        while (odom_yaw_ < -M_PI) odom_yaw_ += 2.0 * M_PI;

        // Debug: Print SDK data every 1 second
        static auto last_debug_time = this->now();
        if ((current_time - last_debug_time).seconds() > 1.0) {
            RCLCPP_INFO(this->get_logger(),
                "SDK Raw: (%.3f, %.3f, Y:%.3f) -> Odom: (%.3f, %.3f, Y:%.3f)",
                sdk_x, sdk_y, sdk_yaw, odom_x_, odom_y_, odom_yaw_);
            last_debug_time = current_time;
        }

        // Publish odometry
        auto odom_msg = nav_msgs::msg::Odometry();
        odom_msg.header.stamp = current_time;
        odom_msg.header.frame_id = "odom";
        odom_msg.child_frame_id = "body";

        // Position (from SDK)
        odom_msg.pose.pose.position.x = odom_x_;
        odom_msg.pose.pose.position.y = odom_y_;
        odom_msg.pose.pose.position.z = odom_z;

        // Orientation (from SDK, convert Euler to quaternion)
        // Using ZYX convention (yaw-pitch-roll)
        double cy = cos(odom_yaw_ * 0.5);
        double sy = sin(odom_yaw_ * 0.5);
        double cp = cos(pitch * 0.5);
        double sp = sin(pitch * 0.5);
        double cr = cos(roll * 0.5);
        double sr = sin(roll * 0.5);

        odom_msg.pose.pose.orientation.w = cr * cp * cy + sr * sp * sy;
        odom_msg.pose.pose.orientation.x = sr * cp * cy - cr * sp * sy;
        odom_msg.pose.pose.orientation.y = cr * sp * cy + sr * cp * sy;
        odom_msg.pose.pose.orientation.z = cr * cp * sy - sr * sp * cy;

        // Velocity (from SDK, body frame)
        odom_msg.twist.twist.linear.x = body_vx;
        odom_msg.twist.twist.linear.y = body_vy;
        odom_msg.twist.twist.linear.z = body_vz;
        odom_msg.twist.twist.angular.z = 0.0; // SDK doesn't provide angular velocity directly

        odom_pub_->publish(odom_msg);
    }

    // Robot SDK
    mc_sdk::HighLevel highlevel_;

    // Subscribers
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr cmd_sub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;

    // Publishers
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_pub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;

    // Timers
    rclcpp::TimerBase::SharedPtr control_timer_;
    rclcpp::TimerBase::SharedPtr timeout_timer_;
    rclcpp::TimerBase::SharedPtr state_timer_;

    // Velocity control
    std::mutex vel_mutex_;
    double current_vx_, current_vy_, current_yaw_rate_;
    double target_vx_, target_vy_, target_yaw_rate_;
    rclcpp::Time last_cmd_vel_time_;

    // Odometry (dead reckoning from velocity integration)
    double odom_x_, odom_y_, odom_yaw_;

    // SDK initial offset (to align with FAST-LIO on each startup)
    double offset_x_, offset_y_, offset_z_, offset_yaw_;

    // Parameters
    double max_linear_vel_;
    double max_angular_vel_;
    double cmd_vel_timeout_;
    double accel_limit_;
    double control_freq_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    // Install custom signal handler
    std::signal(SIGINT, signalHandler);

    auto node = std::make_shared<DogControlNode>();

    // Spin the node
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
