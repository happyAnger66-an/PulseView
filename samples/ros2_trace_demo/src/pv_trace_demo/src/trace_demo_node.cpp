#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class TraceDemoNode : public rclcpp::Node {
public:
  TraceDemoNode() : Node("pv_trace_demo") {
    const auto duration_sec = this->declare_parameter<int>("duration_sec", 5);
    const auto timer_hz = this->declare_parameter<double>("timer_hz", 20.0);
    const auto topic = this->declare_parameter<std::string>("topic", "chatter");
    const int max_callbacks = static_cast<int>(duration_sec * std::max(timer_hz, 1.0));

    publisher_ = this->create_publisher<std_msgs::msg::String>(topic, 10);
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      topic, 10,
      [this](const std_msgs::msg::String::SharedPtr msg) {
        volatile int sum = 0;
        for (char c : msg->data) {
          sum += static_cast<unsigned char>(c);
        }
        (void)sum;
      });

    const auto period = std::chrono::duration<double>(1.0 / std::max(timer_hz, 1.0));
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      [this, max_callbacks]() {
        if (count_ >= max_callbacks) {
          RCLCPP_INFO(this->get_logger(), "done after %d timer callbacks", count_);
          rclcpp::shutdown();
          return;
        }
        std_msgs::msg::String msg;
        msg.data = "hello-" + std::to_string(count_);
        publisher_->publish(msg);
        ++count_;
      });

    RCLCPP_INFO(
      this->get_logger(), "trace demo running for %ds at %.0f Hz on topic '%s'",
      duration_sec, timer_hz, topic.c_str());
  }

private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr timer_;
  int count_{0};
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TraceDemoNode>());
  rclcpp::shutdown();
  return 0;
}
