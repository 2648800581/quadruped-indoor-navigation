#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/string.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <csignal>
#include <atomic>
#include <iostream>

// Global flag for Ctrl+C detection
std::atomic<bool> g_exit_requested(false);

// Signal handler
void signalHandler(int signal)
{
    if (signal == SIGINT) {
        g_exit_requested.store(true);
        std::cout << "\n\nCtrl+C detected - exiting...\n" << std::endl;
    }
}

// Set terminal to non-blocking mode
void setTerminalMode()
{
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &new_termios);
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// Restore terminal mode
void restoreTerminalMode()
{
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &new_termios);
    new_termios.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// Check if key is pressed
int kbhit()
{
    struct termios oldt, newt;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    int ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

class KeyboardTeleopNode : public rclcpp::Node
{
public:
    KeyboardTeleopNode() : Node("keyboard_teleop_node"),
                           current_vx_(0.0), current_vy_(0.0), current_yaw_rate_(0.0),
                           key_pressed_(false)
    {
        // Declare parameters
        this->declare_parameter("linear_speed", 0.5);
        this->declare_parameter("angular_speed", 0.5);
        this->declare_parameter("max_linear_speed", 1.0);
        this->declare_parameter("max_angular_speed", 1.0);

        // Get parameters
        linear_speed_ = this->get_parameter("linear_speed").as_double();
        angular_speed_ = this->get_parameter("angular_speed").as_double();
        max_linear_speed_ = this->get_parameter("max_linear_speed").as_double();
        max_angular_speed_ = this->get_parameter("max_angular_speed").as_double();

        // Publishers
        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
        robot_cmd_pub_ = this->create_publisher<std_msgs::msg::String>("/robot_cmd", 10);

        // Timer for keyboard checking (50Hz)
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),
            std::bind(&KeyboardTeleopNode::timerCallback, this));

        printInstructions();
    }

    ~KeyboardTeleopNode()
    {
        // Send stop command
        publishCmdVel(0.0, 0.0, 0.0);
        restoreTerminalMode();
    }

private:
    void printInstructions()
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Keyboard Teleop Control" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nMovement Controls:" << std::endl;
        std::cout << "  w - Forward" << std::endl;
        std::cout << "  s - Backward" << std::endl;
        std::cout << "  a - Left" << std::endl;
        std::cout << "  d - Right" << std::endl;
        std::cout << "  q - Turn Left" << std::endl;
        std::cout << "  e - Turn Right" << std::endl;
        std::cout << "  c - Stop" << std::endl;
        std::cout << "\nRobot Commands:" << std::endl;
        std::cout << "  0 - Emergency Stop (passive)" << std::endl;
        std::cout << "  1 - Lie Down" << std::endl;
        std::cout << "  2 - Stand Up" << std::endl;
        std::cout << "\nSpeed Adjustment:" << std::endl;
        std::cout << "  + - Increase speed" << std::endl;
        std::cout << "  - - Decrease speed" << std::endl;
        std::cout << "\nCurrent Settings:" << std::endl;
        std::cout << "  Linear speed: " << linear_speed_ << " m/s" << std::endl;
        std::cout << "  Angular speed: " << angular_speed_ << " rad/s" << std::endl;
        std::cout << "\nPress Ctrl+C to exit" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    void timerCallback()
    {
        if (g_exit_requested.load()) {
            rclcpp::shutdown();
            return;
        }

        bool key_is_pressed = false;
        double vx = 0.0, vy = 0.0, yaw_rate = 0.0;

        if (kbhit()) {
            char ch = getchar();
            key_is_pressed = true;

            switch (ch) {
                case 'w':
                    vx = linear_speed_;
                    std::cout << "\rForward: " << vx << " m/s     " << std::flush;
                    break;
                case 's':
                    vx = -linear_speed_;
                    std::cout << "\rBackward: " << vx << " m/s     " << std::flush;
                    break;
                case 'a':
                    vy = linear_speed_;
                    std::cout << "\rLeft: " << vy << " m/s     " << std::flush;
                    break;
                case 'd':
                    vy = -linear_speed_;
                    std::cout << "\rRight: " << vy << " m/s     " << std::flush;
                    break;
                case 'q':
                    yaw_rate = angular_speed_;
                    std::cout << "\rTurn Left: " << yaw_rate << " rad/s     " << std::flush;
                    break;
                case 'e':
                    yaw_rate = -angular_speed_;
                    std::cout << "\rTurn Right: " << yaw_rate << " rad/s     " << std::flush;
                    break;
                case 'c':
                    vx = 0.0;
                    vy = 0.0;
                    yaw_rate = 0.0;
                    std::cout << "\rStop                    " << std::flush;
                    break;
                case '0':
                    publishRobotCmd("passive");
                    std::cout << "\rEmergency Stop!         " << std::endl;
                    key_is_pressed = false;
                    break;
                case '1':
                    publishRobotCmd("sit");
                    std::cout << "\rLie Down                " << std::endl;
                    key_is_pressed = false;
                    break;
                case '2':
                    publishRobotCmd("stand");
                    std::cout << "\rStand Up                " << std::endl;
                    key_is_pressed = false;
                    break;
                case '+':
                case '=':
                    linear_speed_ = std::min(linear_speed_ + 0.1, max_linear_speed_);
                    angular_speed_ = std::min(angular_speed_ + 0.1, max_angular_speed_);
                    std::cout << "\rSpeed increased: linear=" << linear_speed_
                              << " m/s, angular=" << angular_speed_ << " rad/s     " << std::endl;
                    key_is_pressed = false;
                    break;
                case '-':
                case '_':
                    linear_speed_ = std::max(linear_speed_ - 0.1, 0.1);
                    angular_speed_ = std::max(angular_speed_ - 0.1, 0.1);
                    std::cout << "\rSpeed decreased: linear=" << linear_speed_
                              << " m/s, angular=" << angular_speed_ << " rad/s     " << std::endl;
                    key_is_pressed = false;
                    break;
                default:
                    key_is_pressed = false;
                    break;
            }

            if (key_is_pressed) {
                current_vx_ = vx;
                current_vy_ = vy;
                current_yaw_rate_ = yaw_rate;
                key_pressed_ = true;
            }
        } else {
            // No key pressed - stop if previously moving
            if (key_pressed_) {
                current_vx_ = 0.0;
                current_vy_ = 0.0;
                current_yaw_rate_ = 0.0;
                key_pressed_ = false;
                std::cout << "\rStopped (key released)  " << std::flush;
            }
        }

        // Always publish current velocity
        publishCmdVel(current_vx_, current_vy_, current_yaw_rate_);
    }

    void publishCmdVel(double vx, double vy, double yaw_rate)
    {
        auto msg = geometry_msgs::msg::Twist();
        msg.linear.x = vx;
        msg.linear.y = vy;
        msg.angular.z = yaw_rate;
        cmd_vel_pub_->publish(msg);
    }

    void publishRobotCmd(const std::string& cmd)
    {
        auto msg = std_msgs::msg::String();
        msg.data = cmd;
        robot_cmd_pub_->publish(msg);
    }

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr robot_cmd_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    double linear_speed_;
    double angular_speed_;
    double max_linear_speed_;
    double max_angular_speed_;

    double current_vx_;
    double current_vy_;
    double current_yaw_rate_;
    bool key_pressed_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    // Install signal handler
    std::signal(SIGINT, signalHandler);

    // Set terminal mode
    setTerminalMode();

    auto node = std::make_shared<KeyboardTeleopNode>();

    rclcpp::spin(node);

    restoreTerminalMode();
    rclcpp::shutdown();
    return 0;
}
