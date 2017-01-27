#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>  // Twist msg for Jeton JoyStick to publish command
#include <sensor_msgs/Joy.h>      // Joy msg for Jetson JoyStick to subscribe signals from joystick.
#include <car_info_msg/CarInfo.h> // Tesing simpler message
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
/*
 * Jetson Joy Stick Object:
 *
 * Translate LogiTech F710 Joystick signals into commands for controlling Jetson Car.
 *
 * Tutorials:
 *      - How to validate joystick working on Linux (http://wiki.ros.org/joy/Tutorials/ConfiguringALinuxJoystick)
 *      - How to create simple Joystick TeleOp on ROS (http://wiki.ros.org/joy/Tutorials/WritingTeleopNode)
 *
 * Dependencies:
 *        roscpp, joy, std_msgs
 *
 * Parameters:
 *   Linear_:  axes  of joy stick to control the ESC speed
 *   Angular_: axes of joy stick to control the steering servo
 */
class JetsonJoyStick{
    private:
        ros::NodeHandle node_handle_;
  //    ros::Publisher  vel_pub_;
	ros::Publisher  car_pub_;
        ros::Subscriber joy_sub_;

        int             linear_;
        int             angular_;
        bool            emergency_brake;
        double          l_scale_;
        double          a_scale_;

	pid_t		record_process;
        float           previous[2];
        void joyCallback(const sensor_msgs::Joy::ConstPtr& joy);
    public:
        // Default Constructor
        JetsonJoyStick();
};

JetsonJoyStick::JetsonJoyStick():
        linear_(3) ,
        angular_(0)
{

    a_scale_ = 0.9;
    l_scale_ = 0.3;
    emergency_brake = 1;

    //Set up parameters
    node_handle_.param("axis_linear", linear_, linear_);
    node_handle_.param("axis_angular",angular_, angular_);
    node_handle_.param("scale_angular", a_scale_, a_scale_);
    node_handle_.param("scale_linear", l_scale_, l_scale_);

    // Setup Ros Publisher and Subscriber
//  vel_pub_ = node_handle_.advertise<geometry_msgs::Twist>("cmd_vel", 1, true);
    car_pub_ = node_handle_.advertise<car_info_msg::CarInfo>("car_info", 1, true);
    joy_sub_ = node_handle_.subscribe<sensor_msgs::Joy>("joy", 10, &JetsonJoyStick::joyCallback, this);
}

void JetsonJoyStick::joyCallback(const sensor_msgs::Joy::ConstPtr &joy) 
{
//   geometry_msgs::Twist twist; // create a Twist message to publish
    car_info_msg::CarInfo car; 
    // Turn on Record data
    if (joy->buttons[1] == 1){
	record_process = fork();
	if(record_process = 0){
	   execl("/home/ubuntu/", "roslaunch jetson_joystick record_data.launch",0);
	}
    }
    if (joy->buttons[0] ==1) {
	if (record_process != 0)
	   kill(record_process, 15); // kill the record process
    } 

    //If emergency brake is pressed
    if (emergency_brake == joy->buttons[4] || emergency_brake == joy->buttons[5])
    {
	if(previous[1] > 0)   car.throttle = l_scale_*-1.0;
	if(previous[1] < 0)   car.throttle = l_scale_*1.0;
	if(previous[1] == 0 ) car.throttle = l_scale_*0.0;
f    }
    else
    {
        car.steer = a_scale_*joy->axes[angular_];
        car.throttle = l_scale_*joy->axes[linear_];
    }
    car.header.stamp = ros::Time::now();
    car_pub_.publish(car);
    previous[0] =  car.steer;
    previous[1] =  car.throttle;
}

int main(int argc, char** argv) 
{
    ros::init(argc, argv, "jetson_teleop");
    JetsonJoyStick jetson_teleop;
    std::cout << "Jetson Joystick Node activated" << std::endl;

    ros::spin();
}
