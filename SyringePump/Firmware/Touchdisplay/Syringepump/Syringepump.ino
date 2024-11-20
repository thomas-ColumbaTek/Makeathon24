
#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"

#include "ui.h"


#include <ESP_FlexyStepper.h>



// create the stepper motor object
ESP_FlexyStepper stepper;

int previousDirection = 1;

//variables for software debouncing of the limit switches
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100; //the minimum delay in milliseconds to check for bouncing of the switch. Increase this slighlty if you switches tend to bounce a lot
bool buttonStateChangeDetected = false;
byte limitSwitchState = 0;
byte oldConfirmedLimitSwitchState = 0;

  const int MOTOR_STEP_PIN      = 5;    //  1
  const int MOTOR_DIRECTION_PIN = 6;    //  2
  const int MOTOR_EN_PIN        = 7;    //  3
  const int LIMIT_SWITCH_PIN    = 15;   //  4 // using internal Pullup and in series with NC (Normally Closed) mode


// Speed settings
const int DISTANCE_TO_TRAVEL_IN_STEPS = 2000;
const int SPEED_IN_STEPS_PER_SECOND = 300;
const int ACCELERATION_IN_STEPS_PER_SECOND = 800;
const int DECELERATION_IN_STEPS_PER_SECOND = 800;



static int variable = 0; // Variable to be controlled by slider and buttons
static lv_obj_t * label_variable; // Label to display variable value


// Event handler for slider
static void slider_event_handler(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    variable = lv_slider_get_value(slider);
    // Place your code here to handle the variable change by slider
    printf("Slider value changed: %d\n", variable);
    lv_label_set_text_fmt(label_variable, "Value: %d", variable); // Update label with new value

}

// Event handler for buttons
static void button_event_handler(lv_event_t * e)
{
    lv_obj_t * button = lv_event_get_target(e);
    const char * label = lv_label_get_text(lv_obj_get_child(button, 0));
    
    if (strcmp(label, "STEP+") == 0) {
        variable++;
    } else if (strcmp(label, "STEP-") == 0) {
        variable--;
    } else if (strcmp(label, "START") == 0) {
        // Place your code here to handle the start action
        printf("Start button pressed\n");
    } else if (strcmp(label, "STOP") == 0) {
        // Place your code here to handle the stop action
        printf("Stop button pressed\n");
    } else if (strcmp(label, "HOME") == 0) {
        // Place your code here to handle the homing action
        printf("Home button pressed\n");
    }
    
    // Place your code here to handle the variable change by buttons
    printf("Button pressed, new value: %d\n", variable);
}


void create_ui(void)
{
    // Create a slider
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);                            // Set slider width
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, -40);            // Align slider to the center with some offset
    lv_slider_set_range(slider, 0, 100);                      // Set slider range
    lv_slider_set_value(slider, variable, LV_ANIM_OFF);       // Set initial value
    lv_obj_add_event_cb(slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL); // Attach event handler

    // Create label to display slider value
    label_variable = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label_variable, "Value: %d", variable); // Set initial label text
    lv_obj_align(label_variable, LV_ALIGN_CENTER, 0, -80);        // Align label above the slider

    // Create STEP+ button
    lv_obj_t * btn_step_plus = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_step_plus, LV_ALIGN_CENTER, -60, 40);    // Align button to the center with some offset
    lv_obj_add_event_cb(btn_step_plus, button_event_handler, LV_EVENT_CLICKED, NULL); // Attach event handler
    lv_obj_t * label_plus = lv_label_create(btn_step_plus);
    lv_label_set_text(label_plus, "STEP+");

    // Create STEP- button
    lv_obj_t * btn_step_minus = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_step_minus, LV_ALIGN_CENTER, 60, 40);    // Align button to the center with some offset
    lv_obj_add_event_cb(btn_step_minus, button_event_handler, LV_EVENT_CLICKED, NULL); // Attach event handler
    lv_obj_t * label_minus = lv_label_create(btn_step_minus);
    lv_label_set_text(label_minus, "STEP-");

    // Create START button
    lv_obj_t * btn_start = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_start, LV_ALIGN_CENTER, -120, 100);     // Align button with some offset
    lv_obj_add_event_cb(btn_start, button_event_handler, LV_EVENT_CLICKED, NULL); // Attach event handler
    lv_obj_t * label_start = lv_label_create(btn_start);
    lv_label_set_text(label_start, "START");

    // Create STOP button
    lv_obj_t * btn_stop = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_stop, LV_ALIGN_CENTER, 0, 100);         // Align button with some offset
    lv_obj_add_event_cb(btn_stop, button_event_handler, LV_EVENT_CLICKED, NULL); // Attach event handler
    lv_obj_t * label_stop = lv_label_create(btn_stop);
    lv_label_set_text(label_stop, "STOP");

    // Create HOME button
    lv_obj_t * btn_home = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_home, LV_ALIGN_CENTER, 120, 100);       // Align button with some offset
    lv_obj_add_event_cb(btn_home, button_event_handler, LV_EVENT_CLICKED, NULL); // Attach event handler
    lv_obj_t * label_home = lv_label_create(btn_home);
    lv_label_set_text(label_home, "HOME");
}

// Flexy stepper limit switch example https://github.com/pkerspe/ESP-FlexyStepper/blob/master/examples/Example4_Limit_and_latching_EmergencySwitch/Example4_Limit_and_latching_EmergencySwitch.ino


void limitSwitchHandler()
{
  limitSwitchState = digitalRead(LIMIT_SWITCH_PIN);
  lastDebounceTime = millis();
}

/**
 * Set the rotation degree:
 *      - 0: 0 degree
 *      - 90: 90 degree
 *      - 180: 180 degree
 *      - 270: 270 degree
 *
 */
#define LVGL_PORT_ROTATION_DEGREE               (90)

/**
/* To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 * You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 */
//#include <demos/lv_demos.h>
//#include <examples/lv_examples.h>

void setup()
{



//Stepper init
  stepper.setEnablePin(MOTOR_EN_PIN,2); //Active Low=2   for Enable Pin
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP); //define limit switch

  //attach an interrupt to the IO pin of the limit switch and specify the handler function
  attachInterrupt(digitalPinToInterrupt(LIMIT_SWITCH_PIN), limitSwitchHandler, CHANGE);

    // tell the ESP_flexystepper in which direction to move when homing is required (only works with a homing / limit switch connected)
  stepper.setDirectionToHome(-1);

  // set the speed and acceleration rates for the stepper motor
  stepper.setSpeedInStepsPerSecond(SPEED_IN_STEPS_PER_SECOND);
  stepper.setAccelerationInStepsPerSecondPerSecond(ACCELERATION_IN_STEPS_PER_SECOND);
  stepper.setDecelerationInStepsPerSecondPerSecond(DECELERATION_IN_STEPS_PER_SECOND);

  // connect and configure the stepper motor to its IO pins
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  

  stepper.setStepsPerMillimeter(200); //TODO 


    String title = "LVGL porting example";

    Serial.begin(115200);
    Serial.println(title + " start");

    Serial.println("Initialize panel device");
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 180
        .rotate = LV_DISP_ROT_180,
#elif LVGL_PORT_ROTATION_DEGREE == 0
        .rotate = LV_DISP_ROT_NONE,
#endif
    };

    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    Serial.println("Create UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    bsp_display_lock(0);
    // Create the user interface
    create_ui();
    /* Release the mutex */
    bsp_display_unlock();

    Serial.println(title + " end");
}

void loop()
{

if (limitSwitchState != oldConfirmedLimitSwitchState && (millis() - lastDebounceTime) > debounceDelay)
  {
    oldConfirmedLimitSwitchState = limitSwitchState;
    Serial.printf("Limit switch change detected. New state is %i\n", limitSwitchState);
    //active high switch configuration (NC connection with internal pull up)
    if (limitSwitchState == HIGH)
    {
      stepper.setLimitSwitchActive(stepper.LIMIT_SWITCH_COMBINED_BEGIN_AND_END); // this will cause to stop any motion that is currently going on and block further movement in the same direction as long as the switch is agtive
    }
    else
    {
      stepper.clearLimitSwitchActive(); // clear the limit switch flag to allow movement in both directions again
    }
  }


    lv_task_handler(); // Update LVGL tasks, handles rendering and UI events
    stepper.processMovement();
    //Serial.println("IDLE loop");
    delay(5);
}
