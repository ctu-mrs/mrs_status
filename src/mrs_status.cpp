/* INCLUDES //{ */

// some ros includes
#include <ros/ros.h>
#include <ros/package.h>
// some std includes
#include <stdlib.h>
#include <stdio.h>
// mutexes are good to keep your shared variables our of trouble
#include <mutex>

#include <ncurses.h>

#include <topic_tools/shape_shifter.h>  // for generic topic subscribers

#include <mrs_msgs/TrackerPoint.h>
#include <mrs_msgs/UavState.h>
#include <std_srvs/Trigger.h>
#include <mrs_lib/transformer.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Pose.h>

using namespace std;
using topic_tools::ShapeShifter;

//}

#define NORMAL 100
#define GREEN 101
#define RED 102
#define YELLOW 103
#define BLUE 104

#define BACKGROUND_DEFAULT -1
#define BACKGROUND_TRUE_BLACK 16

#define COLOR_NICE_RED 196
#define COLOR_NICE_GREEN 82
#define COLOR_NICE_BLUE 33
#define COLOR_NICE_YELLOW 220

/* STATUS WINDOW CLASS //{ */

/* ------------------- STATUS WINDOW CLASS ------------------- */

/* class StatusWindow //{ */

class StatusWindow {

public:
  StatusWindow(int lines, int cols, int begin_y, int begin_x, double rate_filter_coef, double desired_rate);
  void Redraw(int counter);

private:
  WINDOW* win_;

  double    rate_;
  double    rate_filter_coef_;
  double    desired_rate_;
  ros::Time last_time_ = ros::Time::now();
};

//}

/* StatusWindow() //{ */

StatusWindow::StatusWindow(int lines, int cols, int begin_y, int begin_x, double rate_filter_coef, double desired_rate, &MrsStatus::UavStateCallback) {

  win_              = newwin(lines, cols, begin_y, begin_x);
  rate_filter_coef_ = rate_filter_coef;
  desired_rate_     = desired_rate;
}

//}

/* Redraw() //{ */

void StatusWindow::Redraw(int counter) {

  rate_ -= rate_ / rate_filter_coef_;
  rate_ += (counter / ((ros::Time::now() - last_time_).toSec())) / rate_filter_coef_;

  if (!isfinite(rate_)) {
    rate_ = 0;
  }

  last_time_ = ros::Time::now();
  /* counter   = 0; TODO - fix this, needs reference outside of this class! */

  wattron(win_, A_BOLD);

  box(win_, '|', '-');

  int tmp_color = RED;
  if (rate_ > 0.95 * desired_rate_) {
    tmp_color = GREEN;
  } else if (rate_ > 0.5 * desired_rate_) {
    tmp_color = YELLOW;
  }

  wattron(win_, COLOR_PAIR(tmp_color));

  wattroff(win_, COLOR_PAIR(tmp_color));
  wattroff(win_, A_BOLD);
  wrefresh(win_);
}

//}

/* ----------------- //STATUS WINDOW CLASS ------------------- */

//}

/* MENU CLASS //{ */

/* ------------------- MENU CLASS ------------------- */

#define KEY_ENT 10
#define KEY_ESC 27

/* class Menu //{ */

class Menu {

public:
  Menu(std::shared_ptr<WINDOW> win);
  Menu(std::shared_ptr<WINDOW> win, std::vector<string> menu_items);
  void Activate();

private:
  std::shared_ptr<WINDOW> win;
  std::vector<string>     menu_items;
};

//}

/* Menu() //{ */

Menu::Menu(std::shared_ptr<WINDOW> win) {

  this->win = win;  // set the pointer to the window that this menu is contained in
}

Menu::Menu(std::shared_ptr<WINDOW> win, std::vector<string> menu_items) {

  this->win        = win;         // set the pointer to the window that this menu is contained in
  this->menu_items = menu_items;  // set the pointer to the window that this menu is contained in
}

void Menu::Activate() {

  box(win.get(), '|', '-');

  for (unsigned long i = 0; i < menu_items.size(); i++) {

    if (i == 0)

      wattron(win.get(), A_STANDOUT);  // highlights the first item.

    else

      wattroff(win.get(), A_STANDOUT);

    mvwaddstr(win.get(), i + 1, 2, menu_items[i].c_str());
  }

  wrefresh(win.get());  // update the terminal screen

  keypad(win.get(), TRUE);  // enable keyboard input for the window.
  curs_set(0);              // hide the default screen cursor.

  int  ch  = 0;
  int  i   = 0;
  bool run = true;

  while (run) {
    ch = wgetch(win.get());
    // right pad with spaces to make the items appear with even width.

    mvwaddstr(win.get(), i + 1, 2, menu_items[i].c_str());
    // use a variable to increment or decrement the value based on the input.
    if (ch == KEY_UP || ch == 'k') {
      i--;
      i = (i < 0) ? menu_items.size() - 1 : i;
    } else if (ch == KEY_DOWN || ch == 'j') {
      i++;
      i = (i > int(menu_items.size() - 1)) ? 0 : i;
    } else if (ch == KEY_ENT) {
      menu_items[i] = "e";
    } else if (ch == 'q' || ch == KEY_ESC) {
      run = false;
    }

    // now highlight the next item in the list.
    //
    wattron(win.get(), A_STANDOUT);
    mvwaddstr(win.get(), i + 1, 2, menu_items[i].c_str());
    wattroff(win.get(), A_STANDOUT);
  }
}

//}

/* ----------------- //MENU CLASS ------------------- */

//}

/* MRS_STATUS CLASS //{ */

/* class MrsStatus //{ */

class MrsStatus {

public:
  MrsStatus();

private:
  ros::NodeHandle nh_;

  ros::Timer status_timer_;

  void UavStateCallback(const mrs_msgs::UavStateConstPtr& msg);
  void GenericCallback(const ShapeShifter::ConstPtr& msg, const std::string& topic_name);

  void statusTimer(const ros::TimerEvent& event);

  void PrintLimitedDouble(WINDOW* win, int y, int x, string str_in, double num, double limit);
  void PrintLimitedString(WINDOW* win, int y, int x, string str_in, unsigned long limit);

  void UavStateHandler(WINDOW* win);

  ros::Subscriber state_subscriber_;
  ros::Subscriber mpc_diag_subscriber_;

  ros::Subscriber generic_subscriber;

  std::string _uav_name_;
  int         counter = 0;

  bool initialized_ = false;


  mrs_msgs::UavState uav_state_;

  /* int       uav_state_counter_   = 0; */
  /* double    uav_state_rate_      = 0.0; */
  /* ros::Time uav_state_last_time_ = ros::Time::now(); */

  StatusWindow uav_state_window{20, 20, 1, 1, 2.0, 100.0};
};

//}

/* MrsStatus() //{ */

MrsStatus::MrsStatus() {

  // initialize node and create no handle
  nh_ = ros::NodeHandle("~");

  // PARAMS
  /* mrs_lib::ParamLoader param_loader(nh_, "MrsStatus"); */

  /* if (!param_loader.loaded_successfully()) { */
  /*   ROS_ERROR("[MrsStatus]: Could not load all parameters!"); */
  /*   ros::shutdown(); */
  /* } else { */
  /*   ROS_INFO("[MrsStatus]: All params loaded!"); */
  /* } */

  // TIMERS
  status_timer_ = nh_.createTimer(ros::Rate(10), &MrsStatus::statusTimer, this);

  // SUBSCRIBERS
  state_subscriber_ = nh_.subscribe("state_in", 1, &MrsStatus::UavStateCallback, this, ros::TransportHints().tcpNoDelay());

  string                                                            topic_name = "/uav1/odometry/odom_main";
  boost::function<void(const topic_tools::ShapeShifter::ConstPtr&)> callback;
  callback           = [this, topic_name](const topic_tools::ShapeShifter::ConstPtr& msg) -> void { GenericCallback(msg, topic_name); };
  generic_subscriber = nh_.subscribe(topic_name, 10, callback);

  initialized_ = true;
  ROS_INFO("[Mrs Status]: Node initialized!");

  refresh();
}

//}

/* statusTimer //{ */

void MrsStatus::statusTimer([[maybe_unused]] const ros::TimerEvent& event) {

  wattron(stdscr, COLOR_PAIR(NORMAL));
  uav_state_window.Redraw(10);

  refresh();
}

//}

/* UavStateHandler() //{ */

void MrsStatus::UavStateHandler(WINDOW* win) {

  /* uav_state_rate_ -= uav_state_rate_ / 2; */
  /* uav_state_rate_ += (uav_state_counter_ / ((ros::Time::now() - uav_state_last_time_).toSec())) / 2; */

  /* if (!isfinite(uav_state_rate_)) { */
  /*   uav_state_rate_ = 0; */
  /* } */

  /* uav_state_last_time_ = ros::Time::now(); */
  /* uav_state_counter_   = 0; */

  /* wattron(win, A_BOLD); */

  double         roll, pitch, yaw;
  tf::Quaternion quaternion_odometry;
  quaternionMsgToTF(uav_state_.pose.orientation, quaternion_odometry);
  tf::Matrix3x3 m(quaternion_odometry);
  m.getRPY(roll, pitch, yaw);

  /* box(win, '|', '-'); */

  /* int tmp_color = RED; */
  /* if (uav_state_rate_ > 95) { */
  /*   tmp_color = GREEN; */
  /* } else if (uav_state_rate_ > 40) { */
  /*   tmp_color = YELLOW; */
  /* } */

  /* wattron(win, COLOR_PAIR(tmp_color)); */

  PrintLimitedDouble(win, 0, 2, "Odom %5.1f Hz", uav_state_rate_, 1000);

  PrintLimitedDouble(win, 1, 2, "X %7.2f", uav_state_.pose.position.x, 1000);
  PrintLimitedDouble(win, 2, 2, "Y %7.2f", uav_state_.pose.position.y, 1000);
  PrintLimitedDouble(win, 3, 2, "Z %7.2f", uav_state_.pose.position.z, 1000);
  PrintLimitedDouble(win, 4, 2, "Yaw %5.2f", yaw, 1000);

  PrintLimitedString(win, 2, 14, "Hori: " + uav_state_.estimator_horizontal.name, 14);
  PrintLimitedString(win, 3, 14, "Vert: " + uav_state_.estimator_vertical.name, 14);
  PrintLimitedString(win, 4, 14, "Head: " + uav_state_.estimator_heading.name, 14);

  /* wattroff(win, COLOR_PAIR(tmp_color)); */

  /* wrefresh(win); */
}

//}

/* PrintLimitedDouble() //{ */

void MrsStatus::PrintLimitedDouble(WINDOW* win, int y, int x, string str_in, double num, double limit) {

  if (fabs(num) > limit) {

    // if the number is larger than limit, replace it with scientific notation - 1e+01 to fit the screen
    for (unsigned long i = 0; i < str_in.length() - 2; i++) {
      if (str_in[i] == '.' && str_in[i + 2] == 'f') {
        str_in[i + 1] = '0';
        str_in[i + 2] = 'e';
        break;
      }
    }
  }

  const char* format = str_in.c_str();

  mvwprintw(win, y, x, format, num);
}

//}

/* PrintLimitedString() //{ */

void MrsStatus::PrintLimitedString(WINDOW* win, int y, int x, string str_in, unsigned long limit) {

  if (str_in.length() > limit) {
    str_in.resize(limit);
  }

  const char* format = str_in.c_str();

  mvwprintw(win, y, x, format);
}

//}

/* UavStateCallback() //{ */

void MrsStatus::UavStateCallback(const mrs_msgs::UavStateConstPtr& msg) {
  /* uav_state_counter_++; */
  uav_state_ = *msg;
}

//}

/* GenericCallback() //{ */

void MrsStatus::GenericCallback(const ShapeShifter::ConstPtr& msg, const std::string& topic_name) {
  counter++;
}

//}

//}

/* main() //{ */

int main(int argc, char** argv) {

  ros::init(argc, argv, "mrs_status");

  /* std::vector<string> items; */
  /* items.push_back("1"); */
  /* items.push_back("2"); */
  /* items.push_back("3"); */
  /* items.push_back("4"); */

  initscr();
  start_color();
  cbreak();
  noecho();
  clear();
  use_default_colors();

  init_pair(NORMAL, COLOR_WHITE, BACKGROUND_DEFAULT);
  init_pair(RED, COLOR_NICE_RED, BACKGROUND_DEFAULT);
  init_pair(YELLOW, COLOR_NICE_YELLOW, BACKGROUND_DEFAULT);
  init_pair(GREEN, COLOR_NICE_GREEN, BACKGROUND_DEFAULT);
  init_pair(BLUE, COLOR_NICE_BLUE, BACKGROUND_DEFAULT);

  attron(A_BOLD);

  /* DEBUG COLOR RAINBOW //{ */

  /* for (int j = 0; j < 256; j++) { */
  /*   init_pair(j, j, BACKGROUND_DEFAULT); */
  /* } */

  /* int k = 0; */
  /* for (int j = 0; j < 256; j++) { */
  /*   attron(COLOR_PAIR(j)); */
  /*   mvwprintw(stdscr, k, (3 * j + 1) - k * 60 * 3, "%i", j); */
  /*   k = int(j / 60); */
  /*   attroff(COLOR_PAIR(j)); */
  /* } */

  //}

  MrsStatus status;

  /* std::shared_ptr<WINDOW> menu_win(newwin(10, 12, 1, 1)); */
  /* Menu menu(menu_win, items); */
  /* menu.Activate(); */
  /* endwin(); */

  while (ros::ok()) {

    refresh();
    ros::spin();
    return 0;
  }
}

//
