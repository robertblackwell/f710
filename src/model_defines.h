#ifndef H_f710_model_defines_H
#define H_f710_model_defines_H
#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>
#include <functional>
#include <dirent.h>
#include <fcntl.h>
#include <climits>
#include <vector>
#include <optional>
#include <assert.h>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <cmath>
#include <sys/stat.h>
#include <unistd.h>
// #include "f710_time.h"
// #include "f710_exceptions.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////
/// The following constants are applicable to Direct Mode - when the toggle on the front of the
/// controller points at the 'D'.
/// In this mode there are 3 AXIS input providers and 12 buttons.
///
/// #define values for event types are in <linux/joystick.h>
///
/// Events are identified by the pair (event_type, event_number)
///
//////////////////////////////////////////////////////////////////////////////////////////////////////
enum class DModeAxisEventNumber {
    RIGHT_STICK_LEFT_RIGHT = 2,
    RIGHT_STICK_FWD_BKWD = 3,
    LEFT_STICK_LEFT_RIGHT = 0,
    LEFT_STICK_FWD_BKWD = 1,
    CROSS_LEFT_RIGHT = 4,
    CROSS_FWD_BKWD = 5,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// The following constants are applicable to Direct Mode - when the toggle on the front of the
/// controller points at the 'D'.
/// In this mode there are 3 AXIS input providers and 12 buttons.
//////////////////////////////////////////////////////////////////////////////////////////////////////
// axis events
#define D_AXIS_CROSS_LEFT_RIGHT_NUMBER 4
#define D_AXIS_CROSS_FWD_BKWD_NUMBER 5

#define D_AXIS_RIGHT_STICK_LEFT_RIGHT_NUMBER 2
#define D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER 3
#define D_AXIS_LEFT_STICK_LEFT_RIGHT_NUMBER 0
#define D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER 1
enum class DModeButtonEventNumber {
    /// Button events only have 0 or 1 for values and the value reverts to 0
    /// as soon as the button is released. If you want to use a button as an on-off
    /// awitch you have to remember when it goes to 1
    ///
    ///
    /// Buttons in the circle on the left front of the coroller
    ///
    X = 0,
    A = 1,
    B = 2,
    Y = 3,
    ///
    /// Buttons on the RIGHT front/leading face of the controller.
    /// They are labelled LB and LT. Notice the labels are upside down
    /// and the one labelled LB is actually on the top while LT is on
    /// the bottom when holding the controller in the "useing" position.
    ///
    LB = 4,
    LT = 6,
    ///
    /// Buttons on the RIGHT front/leading face of the controller.
    /// They are labelled RB and RT. Notice the labels are upside down
    /// and the one labelled RB is actually on the top while RT is on
    /// the bottom when holding the controller in the "useing" position
    ///
    RB = 5,
    RT = 7,

    BACK = 8,
    START = 9,
    ///
    /// The left and right sticks have a push button function as given below
    ///
    LEFT_STICK_PUSH = 10,
    RIGHT_STICK_PUSH = 11,
};
// button events
/// Button events only have 0 or 1 for values and the value reverts to 0
/// as soon as the button is released. If you want to use a button as an on-off
/// awitch you have to remember when it goes to 1
///
///
/// Buttons in the circle on the left front of the coroller
///
#define D_BUTTON_X  0
#define D_BUTTON_A  1
#define D_BUTTON_B  2
#define D_BUTTON_Y  3
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled LB and LT. Notice the labels are upside down
/// and the one labelled LB is actually on the top while LT is on
/// the bottom when holding the controller in the "useing" position.
///
#define D_BUTTON_LB 4
#define D_BUTTON_LT 6
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled RB and RT. Notice the labels are upside down
/// and the one labelled RB is actually on the top while RT is on
/// the bottom when holding the controller in the "useing" position
///
#define D_BUTTON_RB 5
#define D_BUTTON_RT 7

#define D_BUTTON_BACK 8
#define D_BUTTON__START 9
///
/// The left and right sticks have a push button function as given below
///
#define D_BUTTON_LEFT_STICK_PUSH 10
#define D_BUTTON_RIGHT_STICK_PUSH_11
////////////////////////////////////////////////////////////////////////////////////////////////////////
/// When the button on the front of the conttroller is in the X position the controller and driver
/// will operate in X mode.
/// In this mode there are 8 axis controls -
/// -   left stick FWD-BWD, left stick LEFT-RIGHT, right stick FWD-BWD, right stick LEFT-RIGHT
/// -   cross LEFT-RIGHT, cros FWD-BCKWD
/// -   LT hold down (negative values only) RT hold down (negative values only)
/// and 10 buttons, they are:
/// -   A,B,X,Y, START, BACK, LB, RB, left stick PUSH, right stick PUSH
//////////////////////////////////////////////////////////////////////////////////////////////////////
// axis events

#define X_AXIS_LEFT_STICK_LEFT_RIGHT_NUMBER 0
#define X_AXIS_LEFT_STICK_FWD_BKWD_NUMBER 1

#define X_AXIS_RIGHT_STICK_LEFT_RIGHT_NUMBER 3
#define X_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER 4

#define X_AXIS_CROSS_LEFT_RIGHT_NUMBER 6
#define X_AXIS_CROSS_FWD_BKWD_NUMBER 7

#define X_AXIS_LT 4
#define X_AXIS_RT 5


// button events
/// Button events only have 0 or 1 for values and the value reverts to 0
/// as soon as the button is released. If you want to use a button as an on-off
/// awitch you have to remember when it goes to 1
///
///
/// Buttons in the circle on the left front of the coroller
///
#define X_BUTTON_X  2
#define X_BUTTON_A  0
#define X_BUTTON_B  1
#define X_BUTTON_Y  3
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled LB and LT. Notice the labels are upside down
/// and the one labelled LB is actually on the top while LT is on
/// the bottom when holding the controller in the "useing" position.
///
#define X_BUTTON_LB 4
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled RB and RT. Notice the labels are upside down
/// and the one labelled RB is actually on the top while RT is on
/// the bottom when holding the controller in the "useing" position
///
#define X_BUTTON_RB 5

#define X_BUTTON_BACK 6
#define X_BUTTON_START 7
///
/// The left and right sticks have a push button function as given below
///
#define X_BUTTON_LEFT_STICK_PUSH  9
#define X_BUTTON_RIGHT_STICK_PUSH 10



#define CONST_SELECT_TIMEOUT_INTERVAL_MS 500
#define CONST_SELECT_TIMEOUT_EPSILON_MS 5


#endif